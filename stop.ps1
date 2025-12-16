# Stop script for Windows

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Stopping Trading Engine System" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

docker-compose down

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ All services stopped successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Failed to stop services" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "To remove all data volumes as well:" -ForegroundColor Yellow
Write-Host "  docker-compose down -v" -ForegroundColor White
