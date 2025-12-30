
# Script to run Mock Trading Engine and Test System

Write-Host "Starting Mock Trading Engine..." -ForegroundColor Cyan
$engineProcess = Start-Process python -ArgumentList "mock_trading_engine.py" -PassThru -WindowStyle Hidden

Write-Host "Waiting for engine to initialize (2 seconds)..." -ForegroundColor Yellow
Start-Sleep -Seconds 2

try {
    Write-Host "Running Test System..." -ForegroundColor Cyan
    python test_system.py
}
finally {
    Write-Host "Stopping Mock Trading Engine..." -ForegroundColor Yellow
    Stop-Process -Id $engineProcess.Id -Force -ErrorAction SilentlyContinue
    Write-Host "Done." -ForegroundColor Green
}
