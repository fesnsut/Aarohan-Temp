# Test System Script for Windows

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Trading Engine Test Suite" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

$API_URL = "http://localhost:8080"

function Test-HealthCheck {
    Write-Host "`n=========================================="
    Write-Host "  Test 1: Health Check"
    Write-Host "=========================================="
    
    try {
        $response = Invoke-RestMethod -Uri "$API_URL/health" -Method Get -TimeoutSec 5
        
        if ($response.status -eq "healthy") {
            Write-Host "✓ API server is healthy" -ForegroundColor Green
            Write-Host "✓ Redis status: $($response.redis)" -ForegroundColor Green
            return $true
        }
    } catch {
        Write-Host "✗ Failed to connect to API: $_" -ForegroundColor Red
        return $false
    }
}

function Test-PlaceLimitOrder {
    Write-Host "`n=========================================="
    Write-Host "  Test 2: Place Limit Order"
    Write-Host "=========================================="
    
    $order = @{
        userId = 1
        symbol = "AAPL"
        side = "BUY"
        type = "LIMIT"
        price = 150.00
        quantity = 10
        timeInForce = "GFD"
    } | ConvertTo-Json
    
    try {
        $response = Invoke-RestMethod -Uri "$API_URL/order/place" -Method Post `
            -Body $order -ContentType "application/json" -TimeoutSec 5
        
        if ($response.success) {
            Write-Host "✓ Limit order placed successfully" -ForegroundColor Green
            return $true
        } else {
            Write-Host "✗ Failed to place order: $($response.error)" -ForegroundColor Red
            return $false
        }
    } catch {
        Write-Host "✗ Error placing order: $_" -ForegroundColor Red
        return $false
    }
}

function Test-GetMarketQuote {
    Write-Host "`n=========================================="
    Write-Host "  Test 3: Get Market Quote"
    Write-Host "=========================================="
    
    try {
        $response = Invoke-RestMethod -Uri "$API_URL/market/quote/AAPL" -Method Get -TimeoutSec 5
        
        if ($response.success) {
            $quote = $response.data
            Write-Host "✓ Market quote retrieved successfully" -ForegroundColor Green
            Write-Host "   Symbol: $($quote.symbol)"
            Write-Host "   Last Price: `$$($quote.lastTradePrice)"
            return $true
        } else {
            Write-Host "✗ Failed to get quote" -ForegroundColor Red
            return $false
        }
    } catch {
        Write-Host "✗ Error getting quote: $_" -ForegroundColor Red
        return $false
    }
}

# Run tests
Write-Host "Starting tests..." -ForegroundColor Yellow
Write-Host ""

$results = @()

$results += @{ Name = "Health Check"; Result = (Test-HealthCheck) }
Start-Sleep -Seconds 1

$results += @{ Name = "Place Limit Order"; Result = (Test-PlaceLimitOrder) }
Start-Sleep -Seconds 1

$results += @{ Name = "Get Market Quote"; Result = (Test-GetMarketQuote) }

# Summary
Write-Host "`n=========================================="
Write-Host "  Test Summary"
Write-Host "=========================================="

$passed = ($results | Where-Object { $_.Result -eq $true }).Count
$total = $results.Count

foreach ($result in $results) {
    if ($result.Result) {
        Write-Host "✓ $($result.Name): PASS" -ForegroundColor Green
    } else {
        Write-Host "✗ $($result.Name): FAIL" -ForegroundColor Red
    }
}

Write-Host "`nResults: $passed/$total tests passed"

if ($passed -eq $total) {
    Write-Host "`nAll tests passed! 🎉" -ForegroundColor Green
    exit 0
} else {
    Write-Host "`n$($total - $passed) test(s) failed" -ForegroundColor Red
    exit 1
}
