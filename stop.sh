#!/bin/bash

# Stop script for Trading Engine System

echo "=========================================="
echo "  Stopping Trading Engine System"
echo "=========================================="
echo ""

GREEN='\033[0;32m'
NC='\033[0m'

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

docker-compose down

if [ $? -eq 0 ]; then
    print_success "All services stopped successfully"
else
    echo "Failed to stop services"
    exit 1
fi

echo ""
echo "To remove all data volumes as well:"
echo "  docker-compose down -v"
