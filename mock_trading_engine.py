
import http.server
import socketserver
import json
import time
import threading
from urllib.parse import urlparse
import sys

# In-memory data structures
# Symbol -> {'bids': [], 'asks': []}
# Bids: Descending Price (High to Low)
# Asks: Ascending Price (Low to High)
# Orders: list of dicts or objects
order_books = {}
orders = {} # orderId -> order
next_order_id = 1
lock = threading.Lock()

class Order:
    def __init__(self, user_id, symbol, side, type, price, quantity, tif):
        global next_order_id
        self.id = next_order_id
        next_order_id += 1
        self.user_id = user_id
        self.symbol = symbol
        self.side = side # "BUY" or "SELL"
        self.type = type # "LIMIT" or "MARKET"
        self.price = float(price) if price is not None else 0.0
        self.initial_quantity = int(quantity)
        self.remaining_quantity = int(quantity)
        self.tif = tif # "GFD", "IOC", "FOK"
        self.timestamp = time.time()
        self.status = "NEW"

    def to_dict(self):
        return {
            "orderId": self.id,
            "userId": self.user_id,
            "symbol": self.symbol,
            "side": self.side,
            "type": self.type,
            "price": self.price,
            "quantity": self.initial_quantity,
            "remainingQuantity": self.remaining_quantity,
            "timeInForce": self.tif,
            "status": self.status
        }

def match_order(order):
    # Get or create order book
    if order.symbol not in order_books:
        order_books[order.symbol] = {'bids': [], 'asks': []}
    
    book = order_books[order.symbol]
    
    # Matching logic
    trades = []
    
    if order.side == "BUY":
        # Match against Asks (Low to High)
        asks = book['asks']
        i = 0
        while i < len(asks) and order.remaining_quantity > 0:
            ask = asks[i]
            
            # Check price for LIMIT orders
            if order.type == "LIMIT" and ask.price > order.price:
                break
                
            # Match
            match_qty = min(order.remaining_quantity, ask.remaining_quantity)
            price = ask.price # Trade at maker price
            
            trades.append({
                "symbol": order.symbol,
                "price": price,
                "quantity": match_qty,
                "buyer": order.id,
                "seller": ask.id
            })
            
            order.remaining_quantity -= match_qty
            ask.remaining_quantity -= match_qty
            
            if ask.remaining_quantity == 0:
                ask.status = "FILLED"
                asks.pop(i) # Remove filled ask
            else:
                ask.status = "PARTIALLY_FILLED"
                i += 1 # Should verify if we pop or increment
                # If we pop, we don't increment i
                # Ah, if we pop, the next item slides into i. So we only increment if we DON'T pop.
                # Re-logic:
                # If popped, do nothing (i points to next).
                # If not popped, that means ask wasn't fully filled (so order must be filled), leads to break.
                # Actually if ask is not filled, order MUST be filled (min logic).
                # So we can break.
                break

    else: # SELL
        # Match against Bids (High to Low)
        bids = book['bids']
        i = 0
        while i < len(bids) and order.remaining_quantity > 0:
            bid = bids[i]
            
            # Check price for LIMIT orders
            if order.type == "LIMIT" and bid.price < order.price:
                break
            
            # Match
            match_qty = min(order.remaining_quantity, bid.remaining_quantity)
            price = bid.price # Trade at maker price
            
            trades.append({
                "symbol": order.symbol,
                "price": price,
                "quantity": match_qty,
                "buyer": bid.id,
                "seller": order.id
            })
            
            order.remaining_quantity -= match_qty
            bid.remaining_quantity -= match_qty
            
            if bid.remaining_quantity == 0:
                bid.status = "FILLED"
                bids.pop(i)
            else:
                bid.status = "PARTIALLY_FILLED"
                break

    # Post-match processing
    if order.remaining_quantity > 0:
        if order.tif == "IOC":
            order.status = "CANCELLED" if order.remaining_quantity == order.initial_quantity else "PARTIALLY_FILLED_CANCELLED"
            # Cancel remaiing
        else:
            # Add to book
            order.status = "PARTIALLY_FILLED" if order.remaining_quantity < order.initial_quantity else "NEW"
            if order.side == "BUY":
                book['bids'].append(order)
                # Sort Bids: High to Low
                book['bids'].sort(key=lambda x: x.price, reverse=True)
            else:
                book['asks'].append(order)
                # Sort Asks: Low to High
                book['asks'].sort(key=lambda x: x.price)

    else:
        order.status = "FILLED"

    return trades

class TradingHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        parsed = urlparse(self.path)
        
        if parsed.path == "/health":
            self.send_response(200)
            self.send_header('Content-type', 'application/json')
            self.end_headers()
            self.wfile.write(json.dumps({"status": "healthy", "redis": "connected (mock)"}).encode())
            return
            
        if parsed.path.startswith("/market/quote/"):
            symbol = parsed.path.split("/")[-1]
            with lock:
                book = order_books.get(symbol, {'bids': [], 'asks': []})
                best_bid = book['bids'][0] if book['bids'] else None
                best_ask = book['asks'][0] if book['asks'] else None
                
                quote = {
                    "symbol": symbol,
                    "lastTradePrice": 150.0, # Dummy
                    "bidPrice": best_bid.price if best_bid else 0.0,
                    "bidQuantity": best_bid.remaining_quantity if best_bid else 0,
                    "askPrice": best_ask.price if best_ask else 0.0,
                    "askQuantity": best_ask.remaining_quantity if best_ask else 0
                }
                
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({"success": True, "data": quote}).encode())
            return

        self.send_error(404)

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        
        if self.path == "/order/place":
            try:
                data = json.loads(post_data)
                
                # Validation
                required = ["userId", "symbol", "side", "type", "quantity"]
                for field in required:
                    if field not in data:
                        raise ValueError(f"Missing field: {field}")
                        
                price = data.get("price")
                if data["type"] == "LIMIT" and price is None:
                     raise ValueError("Price required for LIMIT order")
                     
                with lock:
                    order = Order(
                        data["userId"],
                        data["symbol"],
                        data["side"],
                        data["type"],
                        price,
                        data["quantity"],
                        data.get("timeInForce", "GFD")
                    )
                    
                    orders[order.id] = order
                    match_order(order)
                    
                    # Log for debugging
                    print(f"Order processed: {order.id} {order.side} {order.symbol} {order.remaining_quantity}/{order.initial_quantity} left")
                
                self.send_response(200)
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({"success": True, "data": order.to_dict()}).encode())
                
            except Exception as e:
                self.send_response(400) # Changed to 200 with error? No, typically 400 for bad request
                # But test_system check looks for .json() even on error sometimes?
                # test_system says: if response.status_code == 400 or not data.get('success'): ... print_success
                # So 400 is fine.
                self.send_header('Content-type', 'application/json')
                self.end_headers()
                self.wfile.write(json.dumps({"success": False, "error": str(e)}).encode())
            return
            
        self.send_error(404)

def run_server():
    server_address = ('', 8080)
    httpd = http.server.HTTPServer(server_address, TradingHandler)
    print("Mock Trading Engine running on port 8080...")
    httpd.serve_forever()

if __name__ == "__main__":
    run_server()
