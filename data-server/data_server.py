import asyncio
import json
import random
import redis.asyncio as redis
from datetime import datetime
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class MarketDataGenerator:
    def __init__(self, config):
        self.config = config
        self.redis_client = None
        self.symbols = config.get('symbols', ['AAPL', 'GOOGL', 'MSFT', 'AMZN', 'TSLA'])
        
        # Initialize prices for each symbol
        self.prices = {
            symbol: random.uniform(50.0, 500.0) for symbol in self.symbols
        }
        
        # Price volatility (standard deviation as percentage)
        self.volatility = config.get('volatility', 0.02)
        
        # Update interval in seconds
        self.update_interval = config.get('updateInterval', 1.0)
    
    async def connect_redis(self):
        """Connect to Redis"""
        self.redis_client = await redis.Redis(
            host=self.config['redis']['host'],
            port=self.config['redis']['port'],
            password=self.config['redis'].get('password', None),
            decode_responses=True
        )
        logger.info("Connected to Redis")
    
    def generate_price_movement(self, current_price):
        """Generate realistic price movement using random walk"""
        # Random percentage change
        change_pct = random.gauss(0, self.volatility)
        
        # Apply change
        new_price = current_price * (1 + change_pct)
        
        # Ensure price stays positive and reasonable
        new_price = max(1.0, new_price)
        new_price = min(10000.0, new_price)
        
        return round(new_price, 2)
    
    def generate_market_tick(self, symbol, price):
        """Generate a market data tick"""
        # Generate bid/ask spread (0.1% to 0.5% of price)
        spread = price * random.uniform(0.001, 0.005)
        
        bid_price = round(price - spread / 2, 2)
        ask_price = round(price + spread / 2, 2)
        
        # Generate random volumes
        bid_quantity = random.randint(100, 10000)
        ask_quantity = random.randint(100, 10000)
        last_quantity = random.randint(10, 1000)
        total_volume = random.randint(100000, 1000000)
        
        return {
            'type': 'tick',
            'symbol': symbol,
            'lastTradePrice': price,
            'lastTradeQuantity': last_quantity,
            'bidPrice': bid_price,
            'bidQuantity': bid_quantity,
            'askPrice': ask_price,
            'askQuantity': ask_quantity,
            'totalVolume': total_volume,
            'timestamp': int(datetime.utcnow().timestamp() * 1000)
        }
    
    def generate_trade(self, symbol, price):
        """Generate a simulated trade"""
        quantity = random.randint(10, 1000)
        
        return {
            'type': 'trade',
            'tradeId': random.randint(1000000, 9999999),
            'symbol': symbol,
            'price': price,
            'quantity': quantity,
            'timestamp': int(datetime.utcnow().timestamp() * 1000)
        }
    
    async def publish_market_data(self):
        """Continuously generate and publish market data"""
        channel = self.config['channels']['marketData']
        trade_channel = self.config['channels']['trade']
        
        while True:
            try:
                for symbol in self.symbols:
                    # Update price
                    current_price = self.prices[symbol]
                    new_price = self.generate_price_movement(current_price)
                    self.prices[symbol] = new_price
                    
                    # Generate and publish market tick
                    tick = self.generate_market_tick(symbol, new_price)
                    await self.redis_client.publish(channel, json.dumps(tick))
                    
                    # Occasionally generate a trade (10% chance per update)
                    if random.random() < 0.1:
                        trade = self.generate_trade(symbol, new_price)
                        await self.redis_client.publish(trade_channel, json.dumps(trade))
                    
                    # Cache current snapshot in Redis
                    snapshot_key = f"orderbook:{symbol}"
                    snapshot = {
                        'symbol': symbol,
                        'lastTradePrice': new_price,
                        'bidPrice': tick['bidPrice'],
                        'bidQuantity': tick['bidQuantity'],
                        'askPrice': tick['askPrice'],
                        'askQuantity': tick['askQuantity'],
                        'totalVolume': tick['totalVolume'],
                        'timestamp': tick['timestamp']
                    }
                    await self.redis_client.set(snapshot_key, json.dumps(snapshot), ex=60)
                
                # Log current prices
                price_str = ", ".join([f"{sym}: ${price:.2f}" for sym, price in self.prices.items()])
                logger.info(f"Market Update - {price_str}")
                
                # Wait before next update
                await asyncio.sleep(self.update_interval)
                
            except Exception as e:
                logger.error(f"Error generating market data: {e}")
                await asyncio.sleep(1)
    
    async def start(self):
        """Start the market data generator"""
        await self.connect_redis()
        logger.info(f"Starting market data generation for symbols: {', '.join(self.symbols)}")
        await self.publish_market_data()

def load_config(config_file='config/data_server.json'):
    """Load configuration from file"""
    try:
        with open(config_file, 'r') as f:
            return json.load(f)
    except Exception as e:
        logger.error(f"Error loading config: {e}")
        # Return default config
        return {
            'redis': {
                'host': 'localhost',
                'port': 6379,
                'password': None
            },
            'channels': {
                'marketData': 'market_data',
                'trade': 'trades'
            },
            'symbols': ['AAPL', 'GOOGL', 'MSFT', 'AMZN', 'TSLA', 
                       'FB', 'NFLX', 'NVDA', 'AMD', 'INTC'],
            'volatility': 0.02,
            'updateInterval': 1.0
        }

async def main():
    print("=" * 50)
    print("  Market Data Generator v1.0")
    print("=" * 50)
    print()
    
    config = load_config()
    
    print(f"Configuration:")
    print(f"  Redis: {config['redis']['host']}:{config['redis']['port']}")
    print(f"  Symbols: {len(config['symbols'])} symbols")
    print(f"  Volatility: {config['volatility']*100:.2f}%")
    print(f"  Update Interval: {config['updateInterval']}s")
    print()
    
    generator = MarketDataGenerator(config)
    await generator.start()

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nShutting down market data generator...")
