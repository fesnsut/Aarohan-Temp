#!/bin/bash

# Build script for Trading Engine System

echo "=========================================="
echo "  Building Trading Engine System"
echo "=========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}→ $1${NC}"
}

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    print_error "Docker is not installed. Please install Docker first."
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    print_error "Docker Compose is not installed. Please install Docker Compose first."
    exit 1
fi

print_success "Docker and Docker Compose are installed"
echo ""

# Build all services
print_info "Building all services..."
echo ""

docker-compose build

if [ $? -eq 0 ]; then
    print_success "All services built successfully"
else
    print_error "Failed to build services"
    exit 1
fi

echo ""
print_success "Build completed successfully!"
echo ""
echo "To start the system, run:"
echo "  ./start.sh"
echo ""
echo "Or manually:"
echo "  docker-compose up -d"
