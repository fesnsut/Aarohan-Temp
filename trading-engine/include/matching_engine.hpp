#pragma once

#include "types.hpp"
#include "orderbook.hpp"
#include "order_service.hpp"
#include "balance_service.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace TradingEngine {

// Callback for trade execution
using TradeCallback = std::function<void(const Trade&)>;

// Callback for order updates
using OrderUpdateCallback = std::function<void(const Order&)>;

class MatchingEngine {
public:
    MatchingEngine(
        std::shared_ptr<OrderService> orderService,
        std::shared_ptr<BalanceService> balanceService
    );
    
    // Process a new order and attempt matching
    std::vector<Trade> processOrder(std::shared_ptr<Order> order);
    
    // Get or create order book for symbol
    std::shared_ptr<OrderBook> getOrderBook(const std::string& symbol);
    
    // Register callbacks
    void setTradeCallback(TradeCallback callback) { tradeCallback_ = callback; }
    void setOrderUpdateCallback(OrderUpdateCallback callback) { orderUpdateCallback_ = callback; }
    
    // Get market snapshot for a symbol
    MarketSnapshot getMarketSnapshot(const std::string& symbol);
    
private:
    std::shared_ptr<OrderService> orderService_;
    std::shared_ptr<BalanceService> balanceService_;
    std::map<std::string, std::shared_ptr<OrderBook>> orderBooks_;
    std::atomic<uint64_t> nextTradeId_;
    mutable std::mutex mutex_;
    
    TradeCallback tradeCallback_;
    OrderUpdateCallback orderUpdateCallback_;
    
    // Matching logic
    std::vector<Trade> matchLimitOrder(std::shared_ptr<Order> order, std::shared_ptr<OrderBook> book);
    std::vector<Trade> matchMarketOrder(std::shared_ptr<Order> order, std::shared_ptr<OrderBook> book);
    
    // Execute a trade between two orders
    Trade executeTrade(
        std::shared_ptr<Order> buyOrder,
        std::shared_ptr<Order> sellOrder,
        Price tradePrice,
        Quantity tradeQuantity
    );
    
    // Handle IOC and FOK orders
    bool handleIOC(std::shared_ptr<Order> order, const std::vector<Trade>& trades);
    bool handleFOK(std::shared_ptr<Order> order, std::shared_ptr<OrderBook> book);
    
    // Check if order can be filled completely
    bool canFillCompletely(std::shared_ptr<Order> order, std::shared_ptr<OrderBook> book);
    
    // Generate unique trade ID
    uint64_t generateTradeId();
    
    // Notify callbacks
    void notifyTrade(const Trade& trade);
    void notifyOrderUpdate(const Order& order);
};

} // namespace TradingEngine
