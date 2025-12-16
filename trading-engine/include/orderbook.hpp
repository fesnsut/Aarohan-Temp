#pragma once

#include "types.hpp"
#include <map>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

namespace TradingEngine {

// Price level containing orders at the same price
class PriceLevel {
public:
    PriceLevel(Price price) : price_(price), totalQuantity_(0) {}
    
    void addOrder(std::shared_ptr<Order> order);
    void removeOrder(OrderId orderId);
    std::shared_ptr<Order> getFirstOrder();
    
    Price getPrice() const { return price_; }
    Quantity getTotalQuantity() const { return totalQuantity_; }
    size_t getOrderCount() const { return orders_.size(); }
    bool isEmpty() const { return orders_.empty(); }
    
    const std::list<std::shared_ptr<Order>>& getOrders() const { return orders_; }

private:
    Price price_;
    Quantity totalQuantity_;
    std::list<std::shared_ptr<Order>> orders_;
    std::map<OrderId, std::list<std::shared_ptr<Order>>::iterator> orderMap_;
};

// Order book for a single symbol
class OrderBook {
public:
    OrderBook(const std::string& symbol) : symbol_(symbol), lastTradePrice_(0), totalVolume_(0) {}
    
    // Add order to the book
    void addOrder(std::shared_ptr<Order> order);
    
    // Remove order from the book
    bool removeOrder(OrderId orderId, Side side);
    
    // Get best bid/ask prices
    Price getBestBid() const;
    Price getBestAsk() const;
    
    // Get market depth
    std::vector<std::pair<Price, Quantity>> getBidDepth(size_t levels = 10) const;
    std::vector<std::pair<Price, Quantity>> getAskDepth(size_t levels = 10) const;
    
    // Get order
    std::shared_ptr<Order> getOrder(OrderId orderId);
    
    // Market snapshot
    MarketSnapshot getSnapshot() const;
    
    // Getters
    const std::string& getSymbol() const { return symbol_; }
    Price getLastTradePrice() const { return lastTradePrice_; }
    uint64_t getTotalVolume() const { return totalVolume_; }
    
    // Update last trade
    void updateLastTrade(Price price, Quantity quantity);
    
    // Get best bid/ask orders for matching
    std::shared_ptr<Order> getBestBidOrder();
    std::shared_ptr<Order> getBestAskOrder();
    
    // Thread-safe access
    std::mutex& getMutex() { return mutex_; }

private:
    std::string symbol_;
    
    // Buy side: price descending (higher prices first)
    std::map<Price, std::shared_ptr<PriceLevel>, std::greater<Price>> bidLevels_;
    
    // Sell side: price ascending (lower prices first)
    std::map<Price, std::shared_ptr<PriceLevel>, std::less<Price>> askLevels_;
    
    // Order lookup
    std::map<OrderId, std::shared_ptr<Order>> orderMap_;
    
    // Market data
    Price lastTradePrice_;
    uint64_t totalVolume_;
    
    // Thread safety
    mutable std::mutex mutex_;
};

} // namespace TradingEngine
