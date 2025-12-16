#pragma once

#include "types.hpp"
#include "order_service.hpp"
#include "balance_service.hpp"
#include "matching_engine.hpp"
#include "market_data_service.hpp"
#include "snapshot_service.hpp"
#include "error_service.hpp"
#include "redis_client.hpp"
#include <memory>
#include <thread>
#include <atomic>

namespace TradingEngine {

struct EngineConfig {
    std::string redisHost = "localhost";
    int redisPort = 6379;
    std::string redisPassword = "";
    std::string orderInputQueue = "order_input_queue";
    std::string marketDataChannel = "market_data";
    std::string orderUpdateChannel = "order_updates";
    std::string tradeChannel = "trades";
    std::string errorChannel = "errors";
    int workerThreads = 4;
    bool enableSnapshot = true;
    int snapshotIntervalSeconds = 60;
};

class TradingEngine {
public:
    explicit TradingEngine(const EngineConfig& config);
    ~TradingEngine();
    
    // Start the engine
    bool start();
    
    // Stop the engine
    void stop();
    
    // Check if running
    bool isRunning() const { return running_; }
    
    // Submit order directly (for testing)
    std::pair<std::shared_ptr<Order>, ErrorCode> submitOrder(
        UserId userId,
        const std::string& symbol,
        Side side,
        OrderType type,
        TimeInForce timeInForce,
        Price price,
        Quantity quantity
    );
    
    // Cancel order
    ErrorCode cancelOrder(OrderId orderId);
    
    // Get order status
    std::shared_ptr<Order> getOrderStatus(OrderId orderId);
    
    // Get market snapshot
    MarketSnapshot getMarketSnapshot(const std::string& symbol);
    
    // Get order book depth
    json getOrderBookDepth(const std::string& symbol, size_t levels = 10);
    
    // Initialize user balance
    void initializeUserBalance(UserId userId, int64_t initialBalance);
    
    // Get user balance
    UserBalance getUserBalance(UserId userId);

private:
    EngineConfig config_;
    std::atomic<bool> running_;
    
    // Core services
    std::shared_ptr<RedisClient> redisClient_;
    std::shared_ptr<BalanceService> balanceService_;
    std::shared_ptr<OrderService> orderService_;
    std::shared_ptr<MatchingEngine> matchingEngine_;
    std::shared_ptr<MarketDataService> marketDataService_;
    std::shared_ptr<SnapshotService> snapshotService_;
    std::shared_ptr<ErrorService> errorService_;
    
    // Worker threads
    std::vector<std::thread> workerThreads_;
    std::thread snapshotThread_;
    
    // Processing
    void processOrderQueue();
    void handleOrder(const std::string& orderJson);
    void periodicSnapshot();
    
    // Callbacks
    void onTrade(const Trade& trade);
    void onOrderUpdate(const Order& order);
    void onError(ErrorCode code, const std::string& message, const std::string& context);
    
    // Publishing
    void publishMarketData(const Trade& trade, const MarketSnapshot& snapshot);
    void publishOrderUpdate(const Order& order);
    void publishTrade(const Trade& trade);
    void publishError(ErrorCode code, const std::string& message);
};

} // namespace TradingEngine
