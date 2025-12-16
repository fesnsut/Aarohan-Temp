import asyncio
import websockets
import json
import redis.asyncio as redis
from typing import Set
import logging
from datetime import datetime

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class WebSocketServer:
    def __init__(self, config):
        self.config = config
        self.redis_client = None
        self.pubsub = None
        
        # Connected clients for each channel
        self.market_data_clients: Set[websockets.WebSocketServerProtocol] = set()
        self.order_update_clients: Set[websockets.WebSocketServerProtocol] = set()
        self.trade_clients: Set[websockets.WebSocketServerProtocol] = set()
        self.all_clients: Set[websockets.WebSocketServerProtocol] = set()
        
    async def connect_redis(self):
        """Connect to Redis and subscribe to channels"""
        self.redis_client = await redis.Redis(
            host=self.config['redis']['host'],
            port=self.config['redis']['port'],
            password=self.config['redis'].get('password', None),
            decode_responses=True
        )
        
        self.pubsub = self.redis_client.pubsub()
        
        # Subscribe to all channels
        await self.pubsub.subscribe(
            self.config['channels']['marketData'],
            self.config['channels']['orderUpdate'],
            self.config['channels']['trade'],
            self.config['channels']['error']
        )
        
        logger.info("Connected to Redis and subscribed to channels")
    
    async def redis_listener(self):
        """Listen to Redis pub/sub and broadcast to WebSocket clients"""
        try:
            async for message in self.pubsub.listen():
                if message['type'] == 'message':
                    channel = message['channel']
                    data = message['data']
                    
                    try:
                        # Parse the message
                        msg_data = json.loads(data) if isinstance(data, str) else data
                        
                        # Determine which clients to send to
                        if channel == self.config['channels']['marketData']:
                            await self.broadcast(self.market_data_clients, msg_data)
                        elif channel == self.config['channels']['orderUpdate']:
                            await self.broadcast(self.order_update_clients, msg_data)
                        elif channel == self.config['channels']['trade']:
                            await self.broadcast(self.trade_clients, msg_data)
                        
                        # Always send to clients subscribed to all channels
                        await self.broadcast(self.all_clients, msg_data)
                        
                    except json.JSONDecodeError as e:
                        logger.error(f"Error parsing message: {e}")
                        
        except Exception as e:
            logger.error(f"Error in Redis listener: {e}")
    
    async def broadcast(self, clients: Set, message: dict):
        """Broadcast message to all connected clients in the set"""
        if not clients:
            return
        
        # Add timestamp if not present
        if 'timestamp' not in message:
            message['timestamp'] = int(datetime.utcnow().timestamp() * 1000)
        
        message_str = json.dumps(message)
        
        # Send to all clients, remove disconnected ones
        disconnected = set()
        for client in clients:
            try:
                await client.send(message_str)
            except websockets.exceptions.ConnectionClosed:
                disconnected.add(client)
            except Exception as e:
                logger.error(f"Error sending to client: {e}")
                disconnected.add(client)
        
        # Remove disconnected clients
        clients.difference_update(disconnected)
    
    async def handle_client(self, websocket, path):
        """Handle individual WebSocket client connections"""
        client_id = f"{websocket.remote_address[0]}:{websocket.remote_address[1]}"
        logger.info(f"Client connected: {client_id} on path {path}")
        
        # Register client based on path
        if path == '/ws/marketdata':
            self.market_data_clients.add(websocket)
            logger.info(f"Client subscribed to market data: {client_id}")
        elif path == '/ws/orderupdates':
            self.order_update_clients.add(websocket)
            logger.info(f"Client subscribed to order updates: {client_id}")
        elif path == '/ws/trades':
            self.trade_clients.add(websocket)
            logger.info(f"Client subscribed to trades: {client_id}")
        elif path == '/ws/all':
            self.all_clients.add(websocket)
            logger.info(f"Client subscribed to all channels: {client_id}")
        else:
            # Default to all channels
            self.all_clients.add(websocket)
            logger.info(f"Client subscribed to all channels (default): {client_id}")
        
        # Send welcome message
        welcome_msg = {
            'type': 'connection',
            'status': 'connected',
            'message': f'Connected to WebSocket server on {path}',
            'timestamp': int(datetime.utcnow().timestamp() * 1000)
        }
        await websocket.send(json.dumps(welcome_msg))
        
        try:
            # Keep connection alive and handle incoming messages
            async for message in websocket:
                try:
                    data = json.loads(message)
                    
                    # Handle subscription changes
                    if data.get('action') == 'subscribe':
                        channel = data.get('channel')
                        if channel == 'marketdata':
                            self.market_data_clients.add(websocket)
                        elif channel == 'orderupdates':
                            self.order_update_clients.add(websocket)
                        elif channel == 'trades':
                            self.trade_clients.add(websocket)
                        
                        response = {
                            'type': 'subscription',
                            'status': 'success',
                            'channel': channel,
                            'timestamp': int(datetime.utcnow().timestamp() * 1000)
                        }
                        await websocket.send(json.dumps(response))
                    
                    elif data.get('action') == 'unsubscribe':
                        channel = data.get('channel')
                        if channel == 'marketdata':
                            self.market_data_clients.discard(websocket)
                        elif channel == 'orderupdates':
                            self.order_update_clients.discard(websocket)
                        elif channel == 'trades':
                            self.trade_clients.discard(websocket)
                        
                        response = {
                            'type': 'subscription',
                            'status': 'unsubscribed',
                            'channel': channel,
                            'timestamp': int(datetime.utcnow().timestamp() * 1000)
                        }
                        await websocket.send(json.dumps(response))
                    
                    elif data.get('action') == 'ping':
                        pong = {
                            'type': 'pong',
                            'timestamp': int(datetime.utcnow().timestamp() * 1000)
                        }
                        await websocket.send(json.dumps(pong))
                        
                except json.JSONDecodeError:
                    error_msg = {
                        'type': 'error',
                        'message': 'Invalid JSON',
                        'timestamp': int(datetime.utcnow().timestamp() * 1000)
                    }
                    await websocket.send(json.dumps(error_msg))
                    
        except websockets.exceptions.ConnectionClosed:
            logger.info(f"Client disconnected: {client_id}")
        except Exception as e:
            logger.error(f"Error handling client {client_id}: {e}")
        finally:
            # Cleanup: remove client from all sets
            self.market_data_clients.discard(websocket)
            self.order_update_clients.discard(websocket)
            self.trade_clients.discard(websocket)
            self.all_clients.discard(websocket)
            logger.info(f"Client removed: {client_id}")
    
    async def start(self):
        """Start the WebSocket server"""
        await self.connect_redis()
        
        # Start Redis listener task
        redis_task = asyncio.create_task(self.redis_listener())
        
        # Start WebSocket server
        host = self.config['server']['host']
        port = self.config['server']['port']
        
        logger.info(f"Starting WebSocket server on {host}:{port}")
        
        async with websockets.serve(self.handle_client, host, port):
            logger.info(f"WebSocket server running on ws://{host}:{port}")
            await asyncio.Future()  # Run forever

def load_config(config_file='config/websocket.json'):
    """Load configuration from file"""
    try:
        with open(config_file, 'r') as f:
            return json.load(f)
    except Exception as e:
        logger.error(f"Error loading config: {e}")
        # Return default config
        return {
            'server': {
                'host': '0.0.0.0',
                'port': 8765
            },
            'redis': {
                'host': 'localhost',
                'port': 6379,
                'password': None
            },
            'channels': {
                'marketData': 'market_data',
                'orderUpdate': 'order_updates',
                'trade': 'trades',
                'error': 'errors'
            }
        }

async def main():
    print("=" * 50)
    print("  WebSocket Server v1.0")
    print("=" * 50)
    print()
    
    config = load_config()
    
    print(f"Configuration:")
    print(f"  Server: {config['server']['host']}:{config['server']['port']}")
    print(f"  Redis: {config['redis']['host']}:{config['redis']['port']}")
    print()
    
    server = WebSocketServer(config)
    await server.start()

if __name__ == '__main__':
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nShutting down WebSocket server...")
