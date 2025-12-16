# Start script for Windows

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Starting Trading Engine System" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Start all services
Write-Host "→ Starting all services..." -ForegroundColor Yellow
Write-Host ""

docker-compose up -d

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ All services started successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Failed to start services" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "→ Waiting for services to be ready..." -ForegroundColor Yellow
Start-Sleep -Seconds 10

Write-Host ""
Write-Host "✓ Trading Engine System is running!" -ForegroundColor Green
Write-Host ""
Write-Host "Services:" -ForegroundColor Cyan
Write-Host "  → Trading Engine: Running (no exposed port)"
Write-Host "  → API Server: http://localhost:8080"
Write-Host "  → WebSocket Server: ws://localhost:8765"
Write-Host "  → Redis: localhost:6379"
Write-Host "  → PostgreSQL: localhost:5432"
Write-Host "  → Kafka: localhost:9093"
Write-Host ""
Write-Host "Monitoring Tools:" -ForegroundColor Cyan
Write-Host "  → Redis Insight: http://localhost:8001"
Write-Host "  → Kafka UI: http://localhost:8090"
Write-Host ""
Write-Host "To view logs:" -ForegroundColor Cyan
Write-Host "  docker-compose logs -f [service-name]"
Write-Host ""
Write-Host "To stop the system:" -ForegroundColor Cyan
Write-Host "  .\stop.ps1"
Write-Host ""
Write-Host "Available test endpoints:" -ForegroundColor Cyan
Write-Host "  → Health Check:" -ForegroundColor Yellow
Write-Host "    Invoke-WebRequest http://localhost:8080/health"
Write-Host ""
Write-Host "  → Place Order:" -ForegroundColor Yellow
Write-Host '    Invoke-WebRequest -Method POST http://localhost:8080/order/place `'
Write-Host '      -ContentType "application/json" `'
Write-Host '      -Body ''{"userId":1,"symbol":"AAPL","side":"BUY","type":"LIMIT","price":150.00,"quantity":10}'''
