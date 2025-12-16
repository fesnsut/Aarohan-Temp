# Getting Started with the Mock Trading Engine

## Overview

This is a complete, production-grade mock trading engine system designed for stock market simulation events. It includes:

- **High-Performance C++ Trading Engine** - Ultra-fast order matching with price-time priority
- **REST API Server** - RESTful endpoints for order management and market data
- **WebSocket Server** - Real-time streaming of trades and market updates
- **Market Data Generator** - Realistic price movements and market simulation
- **Redis** - Low-latency caching and pub/sub messaging
- **PostgreSQL** - Persistent storage for trades and orders
- **Kafka** - Event streaming backbone (optional)
- **Monitoring Tools** - Redis Insight and Kafka UI

## System Requirements

### For Docker Deployment (Recommended)
- Docker Desktop 4.0+ (with Docker Compose)
- 8GB RAM minimum
- 20GB disk space
- Windows 10/11, macOS, or Linux

### For Native Build (Advanced)
- C++17 compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.15+
- Redis 6+
- Python 3.11+
- Required libraries: redis++, hiredis, nlohmann/json, cpp-httplib

## Quick Start (5 Minutes)

### Step 1: Clone/Extract the Project

```bash
cd c:\Users\laksh\Desktop\Codes\Aarohan
```

### Step 2: Start All Services

**On Windows (PowerShell):**
```powershell
.\build.ps1    # Build all Docker images
.\start.ps1    # Start all services
```

**On Linux/Mac:**
```bash
chmod +x build.sh start.sh stop.sh
./build.sh     # Build all Docker images
./start.sh     # Start all services
```

### Step 3: Verify Services Are Running

Open your browser and check:
- **API Server Health**: http://localhost:8080/health
- **Redis Insight**: http://localhost:8001
- **Kafka UI**: http://localhost:8090

### Step 4: Open the Demo Client

Open `demo-client/index.html` in your browser to see the live trading interface.

### Step 5: Place Your First Order

Using the demo client or cURL:

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

## Architecture

```
┌─────────────┐
│   Browser   │
└──────┬──────┘
       │
       ├─────────────────┐
       │                 │
       ▼                 ▼
┌─────────────┐   ┌─────────────┐
│ API Server  │   │  WebSocket  │
│  (REST)     │   │   Server    │
└──────┬──────┘   └──────┬──────┘
       │                 │
       ▼                 ▼
┌─────────────────────────────┐
│      Redis Pub/Sub          │
│   (Queues & Channels)       │
└──────────┬──────────────────┘
           │
           ▼
┌──────────────────────────────┐
│   C++ Trading Engine         │
│  ┌────────────────────────┐  │
│  │ Order Service          │  │
│  │ Balance Service        │  │
│  │ Matching Engine        │  │
│  │ OrderBook Service      │  │
│  │ Market Data Service    │  │
│  │ Snapshot Service       │  │
│  └────────────────────────┘  │
└──────────┬───────────────────┘
           │
           ▼
┌─────────────────────────────┐
│      PostgreSQL Database    │
│   (Persistent Storage)      │
└─────────────────────────────┘
```

## Component Details

### 1. Trading Engine (C++)
- **Location**: `trading-engine/`
- **Purpose**: Core matching engine with microsecond-level latency
- **Features**:
  - Price-time priority matching
  - Limit and Market orders
  - Time-in-force: GFD, IOC, FOK
  - Multi-threaded order processing
  - Automatic snapshots for recovery

### 2. API Server (C++)
- **Location**: `api-server/`
- **Port**: 8080
- **Purpose**: REST API for order submission and queries
- **Endpoints**: See `API_DOCUMENTATION.md`

### 3. WebSocket Server (Python)
- **Location**: `websocket-server/`
- **Port**: 8765
- **Purpose**: Real-time streaming of market data and order updates
- **Channels**:
  - `/ws/marketdata` - Price updates
  - `/ws/orderupdates` - Order status changes
  - `/ws/trades` - Trade confirmations
  - `/ws/all` - All updates

### 4. Data Server (Python)
- **Location**: `data-server/`
- **Purpose**: Generate realistic market data
- **Features**:
  - Random walk price movements
  - Configurable volatility
  - Multiple symbols
  - Realistic bid-ask spreads

## Configuration

All configuration files are in the `config/` directory:

- `engine.json` - Trading engine settings
- `api.json` - API server settings
- `websocket.json` - WebSocket server settings
- `data_server.json` - Market data generator settings

### Example: Changing Redis Connection

Edit `config/engine.json`:
```json
{
  "redis": {
    "host": "your-redis-host",
    "port": 6379,
    "password": "your-password"
  }
}
```

## Testing the System

### Test 1: Health Check
```bash
curl http://localhost:8080/health
```

Expected output:
```json
{
  "status": "healthy",
  "redis": "connected",
  "timestamp": 1702123456789
}
```

### Test 2: Place a Market Order
```bash
curl -X POST http://localhost:8080/order/place \
  -H "Content-Type: application/json" \
  -d '{
    "userId": 1,
    "symbol": "AAPL",
    "side": "BUY",
    "type": "MARKET",
    "quantity": 5
  }'
```

### Test 3: Get Market Quote
```bash
curl http://localhost:8080/market/quote/AAPL
```

### Test 4: WebSocket Connection (JavaScript)
```javascript
const ws = new WebSocket('ws://localhost:8765/ws/all');

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Received:', data);
};
```

## Order Types Explained

### Limit Orders
- Execute at specified price or better
- Remain in order book if not immediately matched
- Example: BUY 10 shares of AAPL at $150.00

### Market Orders
- Execute immediately at best available price
- No price guarantee
- Example: BUY 10 shares of AAPL at market price

### Time In Force

**GFD (Good For Day)**
- Remains active until filled or market close
- Most common for limit orders

**IOC (Immediate Or Cancel)**
- Fill immediately or cancel unfilled portion
- No resting in order book

**FOK (Fill Or Kill)**
- Fill entire order immediately or cancel completely
- All-or-nothing execution

## Monitoring and Debugging

### View Service Logs
```bash
# All services
docker-compose logs -f

# Specific service
docker-compose logs -f trading-engine
docker-compose logs -f api-server
docker-compose logs -f websocket-server
```

### Check Redis Data
1. Open Redis Insight: http://localhost:8001
2. Connect to `localhost:6379`
3. Browse keys:
   - `order:*` - Order states
   - `orderbook:*` - Market snapshots
   - `balance:*` - User balances

### Monitor Kafka (Optional)
1. Open Kafka UI: http://localhost:8090
2. View topics, messages, and consumer groups

### Database Queries
```bash
# Connect to PostgreSQL
docker exec -it trading-postgres psql -U trading_user -d trading_engine

# View recent trades
SELECT * FROM trades ORDER BY created_at DESC LIMIT 10;

# View orders
SELECT * FROM orders WHERE status = 'FILLED' LIMIT 10;
```

## Performance Tuning

### Increase Worker Threads
Edit `config/engine.json`:
```json
{
  "engine": {
    "workerThreads": 8
  }
}
```

### Adjust Snapshot Frequency
```json
{
  "engine": {
    "snapshotInterval": 30
  }
}
```

### Scale API Server
```bash
# Run multiple API server instances
docker-compose up -d --scale api-server=3
```

## Troubleshooting

### Problem: Services won't start
**Solution**:
1. Check Docker is running: `docker ps`
2. Check port availability: `netstat -ano | findstr "8080"`
3. View logs: `docker-compose logs`

### Problem: Orders not executing
**Solution**:
1. Check trading engine logs: `docker-compose logs trading-engine`
2. Verify Redis connection: `docker-compose logs redis`
3. Check order validation in API server logs

### Problem: No market data
**Solution**:
1. Verify data server is running: `docker-compose ps data-server`
2. Check data server logs: `docker-compose logs data-server`
3. Verify Redis pub/sub: Use Redis Insight to monitor channels

### Problem: WebSocket disconnects
**Solution**:
1. Check WebSocket server logs: `docker-compose logs websocket-server`
2. Verify Redis connection
3. Check client-side network/firewall

## Stopping the System

**Windows:**
```powershell
.\stop.ps1
```

**Linux/Mac:**
```bash
./stop.sh
```

**Remove all data:**
```bash
docker-compose down -v
```

## Development

### Build Locally (Without Docker)

**Trading Engine:**
```bash
cd trading-engine
mkdir build && cd build
cmake ..
make
./trading_engine
```

**API Server:**
```bash
cd api-server
mkdir build && cd build
cmake ..
make
./api_server
```

**Python Services:**
```bash
cd websocket-server
pip install -r requirements.txt
python websocket_server.py
```

## Production Considerations

### Security
- [ ] Add authentication/authorization
- [ ] Use HTTPS/WSS in production
- [ ] Secure Redis with password
- [ ] Use environment variables for secrets
- [ ] Implement rate limiting

### Scalability
- [ ] Use Kubernetes for orchestration
- [ ] Add load balancer for API servers
- [ ] Implement Redis Cluster for high availability
- [ ] Use Kafka for event sourcing
- [ ] Add database replication

### Monitoring
- [ ] Set up Prometheus + Grafana
- [ ] Add application metrics
- [ ] Implement distributed tracing
- [ ] Set up alerts for errors
- [ ] Monitor latency and throughput

## Support and Resources

- **API Documentation**: `API_DOCUMENTATION.md`
- **Demo Client**: `demo-client/index.html`
- **Database Schema**: `database/init.sql`
- **Configuration**: `config/` directory

## License

MIT License - Feel free to use for educational and commercial purposes.

---

**Happy Trading! 🚀📈**
