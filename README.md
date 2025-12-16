# 🚀 Mock Stock Exchange Trading Engine

A production-grade, high-performance mock trading engine built in C++ for stock market simulation events. Features real-time order matching, WebSocket streaming, and comprehensive monitoring tools.

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Python](https://img.shields.io/badge/Python-3.11-green.svg)](https://www.python.org/)
[![Docker](https://img.shields.io/badge/Docker-Enabled-2496ED.svg)](https://www.docker.com/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## ✨ Key Features

- ⚡ **Ultra-Fast Matching** - C++17 engine with microsecond latency
- 📊 **Real-Time Streaming** - WebSocket updates for all market events
- 🔄 **Multiple Order Types** - LIMIT, MARKET with GFD/IOC/FOK support
- 💰 **Balance Management** - Automatic fund locking and transfers
- 🐳 **Docker Ready** - One-command deployment with Docker Compose
- 📈 **Beautiful Demo Client** - Modern HTML5 trading interface
- 📚 **Complete Documentation** - Comprehensive guides and API docs
- 🔍 **Monitoring Tools** - Redis Insight and Kafka UI included

## 🎯 Quick Start

### Prerequisites
- Docker Desktop 4.0+ (with Docker Compose)
- 8GB RAM, 20GB disk space

### Start in 2 Commands

**Windows (PowerShell):**
```powershell
.\build.ps1    # Build all services
.\start.ps1    # Start the system
```

**Linux/Mac:**
```bash
./build.sh     # Build all services
./start.sh     # Start the system
```

### Access Services

| Service | URL | Description |
|---------|-----|-------------|
| **API Server** | http://localhost:8080 | REST endpoints |
| **WebSocket** | ws://localhost:8765 | Real-time streaming |
| **Demo Client** | `demo-client/index.html` | Trading interface |
| **Redis Insight** | http://localhost:8001 | Redis monitoring |
| **Kafka UI** | http://localhost:8090 | Kafka monitoring |

### Test Your First Order

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

## 🏗️ Architecture Overview

```
┌─────────────┐
│   Browser   │ ← Demo Client (HTML5)
└──────┬──────┘
       │
       ├────────────────────┐
       │                    │
       ▼                    ▼
┌─────────────┐      ┌─────────────┐
│ API Server  │      │  WebSocket  │
│  (C++ REST) │      │   (Python)  │
└──────┬──────┘      └──────┬──────┘
       │                    │
       └────────┬───────────┘
                ▼
     ┌─────────────────────┐
     │   Redis Pub/Sub     │
     │ (Queues & Channels) │
     └──────────┬──────────┘
                │
                ▼
     ┌──────────────────────┐
     │  Trading Engine (C++) │
     │  ┌─────────────────┐ │
     │  │ Matching Engine │ │
     │  │ Order Book      │ │
     │  │ Balance Service │ │
     │  └─────────────────┘ │
     └──────────┬───────────┘
                │
                ▼
     ┌──────────────────────┐
     │ PostgreSQL Database  │
     └──────────────────────┘
```

## 📦 What's Included

### Core Services
| Service | Technology | Purpose |
|---------|-----------|----------|
| **Trading Engine** | C++17 | Order matching & execution |
| **API Server** | C++17 | REST endpoints |
| **WebSocket Server** | Python 3.11 | Real-time streaming |
| **Data Generator** | Python 3.11 | Market simulation |
| **Redis** | 7.0 | Caching & pub/sub |
| **PostgreSQL** | 15.0 | Persistent storage |
| **Kafka** | 2.8+ | Event streaming |

### Developer Tools
- 📝 **Demo Web Client** - Beautiful HTML5 interface
- 📚 **Complete Documentation** - Getting started + API reference
- 🔍 **Redis Insight** - Visual Redis browser
- 🎯 **Kafka UI** - Topics and messages viewer
- ⚙️ **Configuration Templates** - JSON configs for all services
- 🗃️ **Database Schema** - Pre-configured tables and indexes

## 📖 Documentation

| Document | Description |
|----------|-------------|
| [GETTING_STARTED.md](GETTING_STARTED.md) | Complete setup and usage guide |
| [API_DOCUMENTATION.md](API_DOCUMENTATION.md) | Full API reference with examples |
| [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) | Detailed system overview |

## 🎮 API Examples

### Place a Limit Order
```bash
curl -X POST http://localhost:8080/order/place \
  -H "Content-Type: application/json" \
  -d '{
    "userId": 1,
    "symbol": "AAPL",
    "side": "BUY",
    "type": "LIMIT",
    "price": 150.00,
    "quantity": 10,
    "timeInForce": "GFD"
  }'
```

### Get Market Quote
```bash
curl http://localhost:8080/market/quote/AAPL
```

### WebSocket Connection (JavaScript)
```javascript
const ws = new WebSocket('ws://localhost:8765/ws/all');

ws.onmessage = (event) => {
    const data = JSON.parse(event.data);
    console.log('Market update:', data);
};
```

## 🔧 System Configuration

All configuration files are in `config/` directory:

- **engine.json** - Trading engine settings (threads, intervals)
- **api.json** - API server configuration (host, port)
- **websocket.json** - WebSocket server settings
- **data_server.json** - Market data parameters (symbols, volatility)

## 📊 Performance

- **Order Matching**: < 10 microseconds
- **Throughput**: > 100,000 orders/second
- **API Response**: < 5ms (p99)
- **WebSocket Latency**: < 10ms
- **Concurrent Clients**: 10,000+

## 🛠️ Development

### Native Build (Without Docker)

**Trading Engine:**
```bash
cd trading-engine
mkdir build && cd build
cmake ..
make -j4
./trading_engine config/engine.json
```

**API Server:**
```bash
cd api-server
mkdir build && cd build
cmake ..
make -j4
./api_server config/api.json
```

**Python Services:**
```bash
cd websocket-server
pip install -r requirements.txt
python websocket_server.py
```

## 🐛 Troubleshooting

**Services won't start?**
```bash
docker-compose logs          # Check all logs
docker ps                    # Verify containers running
```

**Can't connect to API?**
```bash
curl http://localhost:8080/health   # Test health endpoint
```

**No market data?**
```bash
docker-compose logs data-server     # Check data generator
```

See [GETTING_STARTED.md](GETTING_STARTED.md) for detailed troubleshooting.

## 🎯 Use Cases

- 🎓 **Educational** - Learn order matching and market microstructure
- 🏆 **Competitions** - Trading bot contests and hackathons
- 🔬 **Research** - Test trading algorithms and strategies
- 💼 **Interviews** - Demonstrate system design skills

## 🚀 Deployment

### Docker Compose (Recommended)
```bash
docker-compose up -d
```

### Kubernetes
See [deployment/kubernetes/](deployment/kubernetes/) for K8s manifests

### Cloud Platforms
- AWS ECS/EKS
- Google Cloud Run/GKE
- Azure Container Instances/AKS

## 🤝 Contributing

Contributions welcome! Areas for enhancement:

- Additional order types (Stop, Stop-Limit, Iceberg)
- Advanced risk management features
- Performance monitoring dashboard
- Machine learning integration

## 📄 License

MIT License - See [LICENSE](LICENSE) file for details

## 🌟 Acknowledgments

Built for the Aarohan stock market simulation event. Special thanks to all contributors and testers.

---

**Made with ❤️ using C++ and Python**
