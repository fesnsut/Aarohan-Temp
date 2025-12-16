#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <chrono>

namespace TradingEngine {

// Type aliases for clarity and performance
using OrderId = uint64_t;
using UserId = uint64_t;
using Price = int64_t;  // Fixed-point representation (e.g., cents)
using Quantity = uint64_t;
using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;

// Order side
enum class Side : uint8_t {
    BUY = 0,
    SELL = 1
};

// Order type
enum class OrderType : uint8_t {
    LIMIT = 0,
    MARKET = 1
};

// Time in force
enum class TimeInForce : uint8_t {
    GFD = 0,  // Good For Day
    IOC = 1,  // Immediate Or Cancel
    FOK = 2   // Fill Or Kill
};

// Order status
enum class OrderStatus : uint8_t {
    PENDING = 0,
    PARTIALLY_FILLED = 1,
    FILLED = 2,
    CANCELLED = 3,
    REJECTED = 4
};

// Error codes
enum class ErrorCode : uint8_t {
    SUCCESS = 0,
    INVALID_SYMBOL = 1,
    INVALID_QUANTITY = 2,
    INVALID_PRICE = 3,
    INSUFFICIENT_BALANCE = 4,
    ORDER_NOT_FOUND = 5,
    DUPLICATE_ORDER = 6,
    SYSTEM_ERROR = 7
};

// Order structure
struct Order {
    OrderId orderId;
    UserId userId;
    std::string symbol;
    Side side;
    OrderType type;
    TimeInForce timeInForce;
    Price price;
    Quantity quantity;
    Quantity filledQuantity;
    OrderStatus status;
    Timestamp timestamp;
    
    Order() : orderId(0), userId(0), side(Side::BUY), type(OrderType::LIMIT),
              timeInForce(TimeInForce::GFD), price(0), quantity(0),
              filledQuantity(0), status(OrderStatus::PENDING),
              timestamp(std::chrono::high_resolution_clock::now()) {}
};

// Trade structure
struct Trade {
    uint64_t tradeId;
    OrderId buyOrderId;
    OrderId sellOrderId;
    UserId buyUserId;
    UserId sellUserId;
    std::string symbol;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
    
    Trade() : tradeId(0), buyOrderId(0), sellOrderId(0),
              buyUserId(0), sellUserId(0), price(0), quantity(0),
              timestamp(std::chrono::high_resolution_clock::now()) {}
};

// Market data snapshot
struct MarketSnapshot {
    std::string symbol;
    Price lastTradePrice;
    Quantity lastTradeQuantity;
    Price bidPrice;
    Quantity bidQuantity;
    Price askPrice;
    Quantity askQuantity;
    uint64_t totalVolume;
    Timestamp timestamp;
    
    MarketSnapshot() : lastTradePrice(0), lastTradeQuantity(0),
                       bidPrice(0), bidQuantity(0), askPrice(0),
                       askQuantity(0), totalVolume(0),
                       timestamp(std::chrono::high_resolution_clock::now()) {}
};

// User balance
struct UserBalance {
    UserId userId;
    int64_t availableBalance;
    int64_t lockedBalance;
    
    UserBalance() : userId(0), availableBalance(0), lockedBalance(0) {}
    
    int64_t totalBalance() const {
        return availableBalance + lockedBalance;
    }
};

// Helper functions
inline std::string sideToString(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

inline std::string orderTypeToString(OrderType type) {
    return type == OrderType::LIMIT ? "LIMIT" : "MARKET";
}

inline std::string timeInForceToString(TimeInForce tif) {
    switch (tif) {
        case TimeInForce::GFD: return "GFD";
        case TimeInForce::IOC: return "IOC";
        case TimeInForce::FOK: return "FOK";
        default: return "UNKNOWN";
    }
}

inline std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "PENDING";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REJECTED: return "REJECTED";
        default: return "UNKNOWN";
    }
}

} // namespace TradingEngine
