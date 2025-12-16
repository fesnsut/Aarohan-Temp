# API Documentation

## Base URL
```
http://localhost:8080
```

## Endpoints

### Health Check
Check if the API server is running.

**Endpoint:** `GET /health`

**Response:**
```json
{
  "status": "healthy",
  "redis": "connected",
  "timestamp": 1702123456789
}
```

---

### Place Order
Submit a new order to the trading engine.

**Endpoint:** `POST /order/place`

**Request Body:**
```json
{
  "userId": 1,
  "symbol": "AAPL",
  "side": "BUY",
  "type": "LIMIT",
  "timeInForce": "GFD",
  "price": 150.50,
  "quantity": 10
}
```

**Parameters:**
- `userId` (integer, required): User ID (1-10)
- `symbol` (string, required): Stock symbol (e.g., "AAPL", "GOOGL")
- `side` (string, required): "BUY" or "SELL"
- `type` (string, required): "LIMIT" or "MARKET"
- `timeInForce` (string, optional): "GFD", "IOC", or "FOK" (default: "GFD")
- `price` (number, required for LIMIT): Order price
- `quantity` (integer, required): Number of shares

**Response:**
```json
{
  "success": true,
  "message": "Order submitted successfully",
  "data": {
    "userId": 1,
    "symbol": "AAPL",
    "side": "BUY",
    "type": "LIMIT",
    "price": 150.50,
    "quantity": 10
  }
}
```

---

### Cancel Order
Cancel an existing order.

**Endpoint:** `POST /order/cancel`

**Request Body:**
```json
{
  "orderId": 12345
}
```

**Response:**
```json
{
  "success": true,
  "message": "Order cancellation requested",
  "data": {
    "orderId": 12345
  }
}
```

---

### Get Order Status
Retrieve the current status of an order.

**Endpoint:** `GET /order/status/:orderId`

**Response:**
```json
{
  "success": true,
  "data": {
    "type": "order",
    "orderId": 12345,
    "userId": 1,
    "symbol": "AAPL",
    "side": "BUY",
    "orderType": "LIMIT",
    "timeInForce": "GFD",
    "price": 150.50,
    "quantity": 10,
    "filledQuantity": 5,
    "status": "PARTIALLY_FILLED",
    "timestamp": 1702123456789
  }
}
```

---

### Get Market Quote
Get the latest market data for a symbol.

**Endpoint:** `GET /market/quote/:symbol`

**Response:**
```json
{
  "success": true,
  "data": {
    "type": "snapshot",
    "symbol": "AAPL",
    "lastTradePrice": 150.25,
    "lastTradeQuantity": 100,
    "bidPrice": 150.20,
    "bidQuantity": 500,
    "askPrice": 150.30,
    "askQuantity": 300,
    "totalVolume": 125000,
    "timestamp": 1702123456789
  }
}
```

---

### Get Order Book
Retrieve the full order book for a symbol.

**Endpoint:** `GET /market/orderbook/:symbol`

**Response:**
```json
{
  "success": true,
  "data": {
    "type": "orderbook",
    "symbol": "AAPL",
    "bids": [
      {"price": 150.20, "quantity": 500},
      {"price": 150.15, "quantity": 300},
      {"price": 150.10, "quantity": 200}
    ],
    "asks": [
      {"price": 150.30, "quantity": 300},
      {"price": 150.35, "quantity": 400},
      {"price": 150.40, "quantity": 250}
    ],
    "timestamp": 1702123456789
  }
}
```

---

### Get User Orders
Retrieve all orders for a specific user.

**Endpoint:** `GET /order/user/:userId`

**Response:**
```json
{
  "success": true,
  "data": []
}
```

---

## WebSocket API

### Connection
Connect to the WebSocket server for real-time updates.

**URL:** `ws://localhost:8765/ws/all`

**Available Channels:**
- `/ws/marketdata` - Live price and depth updates
- `/ws/orderupdates` - Order status changes
- `/ws/trades` - Trade confirmations
- `/ws/all` - All channels combined

### Message Types

#### Connection Confirmation
```json
{
  "type": "connection",
  "status": "connected",
  "message": "Connected to WebSocket server on /ws/all",
  "timestamp": 1702123456789
}
```

#### Market Tick
```json
{
  "type": "tick",
  "symbol": "AAPL",
  "lastTradePrice": 150.25,
  "lastTradeQuantity": 100,
  "bidPrice": 150.20,
  "bidQuantity": 500,
  "askPrice": 150.30,
  "askQuantity": 300,
  "totalVolume": 125000,
  "timestamp": 1702123456789
}
```

#### Trade Executed
```json
{
  "type": "trade",
  "tradeId": 98765,
  "buyOrderId": 12345,
  "sellOrderId": 54321,
  "buyUserId": 1,
  "sellUserId": 2,
  "symbol": "AAPL",
  "price": 150.25,
  "quantity": 100,
  "timestamp": 1702123456789
}
```

#### Order Update
```json
{
  "type": "order",
  "orderId": 12345,
  "userId": 1,
  "symbol": "AAPL",
  "side": "BUY",
  "orderType": "LIMIT",
  "timeInForce": "GFD",
  "price": 150.50,
  "quantity": 10,
  "filledQuantity": 10,
  "status": "FILLED",
  "timestamp": 1702123456789
}
```

### WebSocket Commands

#### Subscribe to Channel
```json
{
  "action": "subscribe",
  "channel": "marketdata"
}
```

#### Unsubscribe from Channel
```json
{
  "action": "unsubscribe",
  "channel": "marketdata"
}
```

#### Ping
```json
{
  "action": "ping"
}
```

**Response:**
```json
{
  "type": "pong",
  "timestamp": 1702123456789
}
```

---

## Error Responses

All errors follow this format:

```json
{
  "success": false,
  "error": "Error message description",
  "code": 400
}
```

**Common Error Codes:**
- `400` - Bad Request (invalid parameters)
- `404` - Not Found (order/symbol not found)
- `500` - Internal Server Error

---

## Example Usage

### cURL Examples

**Place a BUY order:**
```bash
curl -X POST http://localhost:8080/order/place \
  -H "Content-Type: application/json" \
  -d '{
    "userId": 1,
    "symbol": "AAPL",
    "side": "BUY",
    "type": "LIMIT",
    "price": 150.00,
    "quantity": 10
  }'
```

**Get market quote:**
```bash
curl http://localhost:8080/market/quote/AAPL
```

### Python Example

```python
import requests
import websockets
import asyncio
import json

# Place an order
order = {
    "userId": 1,
    "symbol": "AAPL",
    "side": "BUY",
    "type": "LIMIT",
    "price": 150.00,
    "quantity": 10
}

response = requests.post('http://localhost:8080/order/place', json=order)
print(response.json())

# WebSocket connection
async def listen_market_data():
    async with websockets.connect('ws://localhost:8765/ws/marketdata') as ws:
        async for message in ws:
            data = json.loads(message)
            print(f"Market update: {data}")

asyncio.run(listen_market_data())
```

### JavaScript Example

```javascript
// Place an order
async function placeOrder() {
    const order = {
        userId: 1,
        symbol: "AAPL",
        side: "BUY",
        type: "LIMIT",
        price: 150.00,
        quantity: 10
    };
    
    const response = await fetch('http://localhost:8080/order/place', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(order)
    });
    
    const result = await response.json();
    console.log(result);
}

// WebSocket connection
const ws = new WebSocket('ws://localhost:8765/ws/all');

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Market update:', data);
};

placeOrder();
```
