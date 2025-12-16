#!/bin/bash

# Start script for Trading Engine System

echo "=========================================="
echo "  Starting Trading Engine System"
echo "=========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}→ $1${NC}"
}

# Start all services
print_info "Starting all services..."
echo ""

docker-compose up -d

if [ $? -eq 0 ]; then
    print_success "All services started successfully"
else
    print_error "Failed to start services"
    exit 1
fi

echo ""
print_info "Waiting for services to be ready..."
sleep 10

echo ""
print_success "Trading Engine System is running!"
echo ""
echo "Services:"
echo "  → Trading Engine: Running (no exposed port)"
echo "  → API Server: http://localhost:8080"
echo "  → WebSocket Server: ws://localhost:8765"
echo "  → Redis: localhost:6379"
echo "  → PostgreSQL: localhost:5432"
echo "  → Kafka: localhost:9093"
echo ""
echo "Monitoring Tools:"
echo "  → Redis Insight: http://localhost:8001"
echo "  → Kafka UI: http://localhost:8090"
echo ""
echo "To view logs:"
echo "  docker-compose logs -f [service-name]"
echo ""
echo "To stop the system:"
echo "  ./stop.sh"
echo ""
echo "Available test endpoints:"
echo "  → Health Check: curl http://localhost:8080/health"
echo "  → Place Order: curl -X POST http://localhost:8080/order/place \\"
echo "                 -H 'Content-Type: application/json' \\"
echo "                 -d '{\"userId\":1,\"symbol\":\"AAPL\",\"side\":\"BUY\",\"type\":\"LIMIT\",\"price\":150.00,\"quantity\":10}'"
