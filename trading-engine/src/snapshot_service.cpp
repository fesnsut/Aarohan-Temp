#include "snapshot_service.hpp"
#include "market_data_service.hpp"
#include <nlohmann/json.hpp>

namespace TradingEngine {

using json = nlohmann::json;

bool SnapshotService::saveOrderBookSnapshot(const std::string& symbol, const OrderBook& orderBook) {
    try {
        auto snapshot = orderBook.getSnapshot();
        json snapshotJson = MarketDataService::snapshotToJson(snapshot);
        
        std::string key = getOrderBookKey(symbol);
        return redisClient_->set(key, snapshotJson.dump());
    } catch (const std::exception& e) {
        return false;
    }
}

bool SnapshotService::loadOrderBookSnapshot(const std::string& symbol, OrderBook& orderBook) {
    try {
        std::string key = getOrderBookKey(symbol);
        std::string data = redisClient_->get(key);
        
        if (data.empty()) {
            return false;
        }
        
        // In a real implementation, would reconstruct the order book from saved data
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool SnapshotService::saveOrderState(const Order& order) {
    try {
        json orderJson = MarketDataService::orderToJson(order);
        std::string key = getOrderKey(order.orderId);
        return redisClient_->set(key, orderJson.dump());
    } catch (const std::exception& e) {
        return false;
    }
}

std::shared_ptr<Order> SnapshotService::loadOrderState(OrderId orderId) {
    try {
        std::string key = getOrderKey(orderId);
        std::string data = redisClient_->get(key);
        
        if (data.empty()) {
            return nullptr;
        }
        
        json orderJson = json::parse(data);
        auto order = std::make_shared<Order>();
        
        order->orderId = orderJson["orderId"];
        order->userId = orderJson["userId"];
        order->symbol = orderJson["symbol"];
        // ... parse other fields
        
        return order;
    } catch (const std::exception& e) {
        return nullptr;
    }
}

bool SnapshotService::saveUserBalance(const UserBalance& balance) {
    try {
        json balanceJson = {
            {"userId", balance.userId},
            {"availableBalance", balance.availableBalance},
            {"lockedBalance", balance.lockedBalance}
        };
        
        std::string key = getBalanceKey(balance.userId);
        return redisClient_->set(key, balanceJson.dump());
    } catch (const std::exception& e) {
        return false;
    }
}

UserBalance SnapshotService::loadUserBalance(UserId userId) {
    UserBalance balance;
    balance.userId = userId;
    
    try {
        std::string key = getBalanceKey(userId);
        std::string data = redisClient_->get(key);
        
        if (data.empty()) {
            return balance;
        }
        
        json balanceJson = json::parse(data);
        balance.availableBalance = balanceJson["availableBalance"];
        balance.lockedBalance = balanceJson["lockedBalance"];
    } catch (const std::exception& e) {
        // Return default balance on error
    }
    
    return balance;
}

bool SnapshotService::saveTrade(const Trade& trade) {
    try {
        json tradeJson = MarketDataService::tradeToJson(trade);
        std::string key = getTradeKey(trade.tradeId);
        
        // Save to Redis queue for database persistence
        redisClient_->pushToQueue("db_write_queue", tradeJson.dump());
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool SnapshotService::createFullSnapshot(const std::string& snapshotId) {
    // In a real implementation, would create a complete snapshot of the engine state
    // including all order books, balances, and pending orders
    return true;
}

bool SnapshotService::restoreFromSnapshot(const std::string& snapshotId) {
    // In a real implementation, would restore engine state from a saved snapshot
    return true;
}

std::string SnapshotService::getOrderBookKey(const std::string& symbol) {
    return "orderbook:" + symbol;
}

std::string SnapshotService::getOrderKey(OrderId orderId) {
    return "order:" + std::to_string(orderId);
}

std::string SnapshotService::getBalanceKey(UserId userId) {
    return "balance:" + std::to_string(userId);
}

std::string SnapshotService::getTradeKey(uint64_t tradeId) {
    return "trade:" + std::to_string(tradeId);
}

std::string SnapshotService::getSnapshotKey(const std::string& snapshotId) {
    return "snapshot:" + snapshotId;
}

} // namespace TradingEngine
