#pragma once

#include "types.hpp"
#include "orderbook.hpp"
#include "redis_client.hpp"
#include <memory>
#include <string>

namespace TradingEngine {

class SnapshotService {
public:
    SnapshotService(std::shared_ptr<RedisClient> redisClient)
        : redisClient_(redisClient) {}
    
    // Save order book snapshot to Redis
    bool saveOrderBookSnapshot(const std::string& symbol, const OrderBook& orderBook);
    
    // Load order book snapshot from Redis
    bool loadOrderBookSnapshot(const std::string& symbol, OrderBook& orderBook);
    
    // Save order state
    bool saveOrderState(const Order& order);
    
    // Load order state
    std::shared_ptr<Order> loadOrderState(OrderId orderId);
    
    // Save user balance
    bool saveUserBalance(const UserBalance& balance);
    
    // Load user balance
    UserBalance loadUserBalance(UserId userId);
    
    // Save trade
    bool saveTrade(const Trade& trade);
    
    // Periodic snapshot (for disaster recovery)
    bool createFullSnapshot(const std::string& snapshotId);
    bool restoreFromSnapshot(const std::string& snapshotId);
    
private:
    std::shared_ptr<RedisClient> redisClient_;
    
    // Key generation helpers
    std::string getOrderBookKey(const std::string& symbol);
    std::string getOrderKey(OrderId orderId);
    std::string getBalanceKey(UserId userId);
    std::string getTradeKey(uint64_t tradeId);
    std::string getSnapshotKey(const std::string& snapshotId);
};

} // namespace TradingEngine
