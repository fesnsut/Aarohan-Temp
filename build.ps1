# Build script for Windows

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Building Trading Engine System" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Check if Docker is installed
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    Write-Host "✗ Docker is not installed. Please install Docker Desktop first." -ForegroundColor Red
    exit 1
}

if (-not (Get-Command docker-compose -ErrorAction SilentlyContinue)) {
    Write-Host "✗ Docker Compose is not installed. Please install Docker Compose first." -ForegroundColor Red
    exit 1
}

Write-Host "✓ Docker and Docker Compose are installed" -ForegroundColor Green
Write-Host ""

# Build all services
Write-Host "→ Building all services..." -ForegroundColor Yellow
Write-Host ""

docker-compose build

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ All services built successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Failed to build services" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "✓ Build completed successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "To start the system, run:" -ForegroundColor Cyan
Write-Host "  .\start.ps1" -ForegroundColor White
Write-Host ""
Write-Host "Or manually:" -ForegroundColor Cyan
Write-Host "  docker-compose up -d" -ForegroundColor White
