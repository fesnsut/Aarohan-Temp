#!/usr/bin/env python3
"""
Test script for the Trading Engine
Run various test scenarios to verify system functionality
"""

import requests
import json
import time
import sys

API_URL = "http://localhost:8080"

def print_header(text):
    print("\n" + "="*60)
    print(f"  {text}")
    print("="*60)

def print_success(text):
    print(f"✓ {text}")

def print_error(text):
    print(f"✗ {text}")

def test_health_check():
    """Test API server health"""
    print_header("Test 1: Health Check")
    try:
        response = requests.get(f"{API_URL}/health", timeout=5)
        data = response.json()
        
        if response.status_code == 200 and data.get('status') == 'healthy':
            print_success(f"API server is healthy")
            print_success(f"Redis status: {data.get('redis')}")
            return True
        else:
            print_error("API server health check failed")
            return False
    except Exception as e:
        print_error(f"Failed to connect to API: {e}")
        return False

def test_place_limit_order():
    """Test placing a limit order"""
    print_header("Test 2: Place Limit Order")
    
    order = {
        "userId": 1,
        "symbol": "AAPL",
        "side": "BUY",
        "type": "LIMIT",
        "price": 150.00,
        "quantity": 10,
        "timeInForce": "GFD"
    }
    
    try:
        response = requests.post(f"{API_URL}/order/place", json=order, timeout=5)
        data = response.json()
        
        if response.status_code == 200 and data.get('success'):
            print_success("Limit order placed successfully")
            print(f"   Order details: {json.dumps(data['data'], indent=2)}")
            return True
        else:
            print_error(f"Failed to place order: {data.get('error', 'Unknown error')}")
            return False
    except Exception as e:
        print_error(f"Error placing order: {e}")
        return False

def test_place_market_order():
    """Test placing a market order"""
    print_header("Test 3: Place Market Order")
    
    order = {
        "userId": 2,
        "symbol": "GOOGL",
        "side": "BUY",
        "type": "MARKET",
        "quantity": 5
    }
    
    try:
        response = requests.post(f"{API_URL}/order/place", json=order, timeout=5)
        data = response.json()
        
        if response.status_code == 200 and data.get('success'):
            print_success("Market order placed successfully")
            print(f"   Order details: {json.dumps(data['data'], indent=2)}")
            return True
        else:
            print_error(f"Failed to place order: {data.get('error', 'Unknown error')}")
            return False
    except Exception as e:
        print_error(f"Error placing order: {e}")
        return False

def test_matching_orders():
    """Test order matching by placing complementary orders"""
    print_header("Test 4: Order Matching")
    
    # Place BUY order
    buy_order = {
        "userId": 3,
        "symbol": "MSFT",
        "side": "BUY",
        "type": "LIMIT",
        "price": 300.00,
        "quantity": 20,
        "timeInForce": "GFD"
    }
    
    # Place SELL order at same price
    sell_order = {
        "userId": 4,
        "symbol": "MSFT",
        "side": "SELL",
        "type": "LIMIT",
        "price": 300.00,
        "quantity": 20,
        "timeInForce": "GFD"
    }
    
    try:
        # Place buy order
        response1 = requests.post(f"{API_URL}/order/place", json=buy_order, timeout=5)
        data1 = response1.json()
        
        if not (response1.status_code == 200 and data1.get('success')):
            print_error("Failed to place BUY order")
            return False
        
        print_success("BUY order placed")
        time.sleep(0.5)  # Brief delay
        
        # Place sell order
        response2 = requests.post(f"{API_URL}/order/place", json=sell_order, timeout=5)
        data2 = response2.json()
        
        if not (response2.status_code == 200 and data2.get('success')):
            print_error("Failed to place SELL order")
            return False
        
        print_success("SELL order placed")
        print_success("Orders should match automatically in the trading engine")
        return True
        
    except Exception as e:
        print_error(f"Error during matching test: {e}")
        return False

def test_get_market_quote():
    """Test getting market quote"""
    print_header("Test 5: Get Market Quote")
    
    try:
        response = requests.get(f"{API_URL}/market/quote/AAPL", timeout=5)
        data = response.json()
        
        if response.status_code == 200 and data.get('success'):
            quote = data['data']
            print_success("Market quote retrieved successfully")
            print(f"   Symbol: {quote.get('symbol')}")
            print(f"   Last Price: ${quote.get('lastTradePrice', 0):.2f}")
            print(f"   Bid: ${quote.get('bidPrice', 0):.2f} x {quote.get('bidQuantity', 0)}")
            print(f"   Ask: ${quote.get('askPrice', 0):.2f} x {quote.get('askQuantity', 0)}")
            return True
        else:
            print_error(f"Failed to get quote: {data.get('error', 'Unknown error')}")
            return False
    except Exception as e:
        print_error(f"Error getting quote: {e}")
        return False

def test_ioc_order():
    """Test IOC (Immediate or Cancel) order"""
    print_header("Test 6: IOC Order")
    
    order = {
        "userId": 5,
        "symbol": "TSLA",
        "side": "BUY",
        "type": "LIMIT",
        "price": 200.00,
        "quantity": 15,
        "timeInForce": "IOC"
    }
    
    try:
        response = requests.post(f"{API_URL}/order/place", json=order, timeout=5)
        data = response.json()
        
        if response.status_code == 200 and data.get('success'):
            print_success("IOC order placed successfully")
            print("   IOC orders fill immediately or cancel unfilled portion")
            return True
        else:
            print_error(f"Failed to place IOC order: {data.get('error', 'Unknown error')}")
            return False
    except Exception as e:
        print_error(f"Error placing IOC order: {e}")
        return False

def test_validation():
    """Test order validation"""
    print_header("Test 7: Order Validation")
    
    # Invalid order (missing symbol)
    invalid_order = {
        "userId": 1,
        "side": "BUY",
        "type": "LIMIT",
        "price": 150.00,
        "quantity": 10
    }
    
    try:
        response = requests.post(f"{API_URL}/order/place", json=invalid_order, timeout=5)
        data = response.json()
        
        if response.status_code == 400 or not data.get('success'):
            print_success("Order validation working correctly")
            print(f"   Rejected invalid order: {data.get('error', 'Validation failed')}")
            return True
        else:
            print_error("Order validation failed - accepted invalid order")
            return False
    except Exception as e:
        print_error(f"Error testing validation: {e}")
        return False

def run_all_tests():
    """Run all test scenarios"""
    print("\n" + "="*60)
    print("  Trading Engine Test Suite")
    print("="*60)
    
    tests = [
        ("Health Check", test_health_check),
        ("Place Limit Order", test_place_limit_order),
        ("Place Market Order", test_place_market_order),
        ("Order Matching", test_matching_orders),
        ("Get Market Quote", test_get_market_quote),
        ("IOC Order", test_ioc_order),
        ("Order Validation", test_validation),
    ]
    
    results = []
    
    for name, test_func in tests:
        try:
            result = test_func()
            results.append((name, result))
            time.sleep(1)  # Brief pause between tests
        except Exception as e:
            print_error(f"Test '{name}' crashed: {e}")
            results.append((name, False))
    
    # Print summary
    print_header("Test Summary")
    passed = sum(1 for _, result in results if result)
    total = len(results)
    
    for name, result in results:
        status = "PASS" if result else "FAIL"
        symbol = "✓" if result else "✗"
        print(f"{symbol} {name}: {status}")
    
    print(f"\nResults: {passed}/{total} tests passed")
    
    if passed == total:
        print_success("All tests passed! 🎉")
        return 0
    else:
        print_error(f"{total - passed} test(s) failed")
        return 1

if __name__ == "__main__":
    try:
        exit_code = run_all_tests()
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print("\n\nTests interrupted by user")
        sys.exit(1)
