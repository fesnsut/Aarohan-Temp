#include "trading_engine.hpp"
#include <nlohmann/json.hpp>
#include <iostream>

namespace TradingEngine {

using json = nlohmann::json;

TradingEngine::TradingEngine(const EngineConfig& config)
    : config_(config), running_(false) {
    
    // Initialize Redis client
    redisClient_ = std::make_shared<RedisClient>(
        config_.redisHost,
        config_.redisPort,
        config_.redisPassword
    );
    
    // Initialize services
    balanceService_ = std::make_shared<BalanceService>();
    orderService_ = std::make_shared<OrderService>(balanceService_);
    matchingEngine_ = std::make_shared<MatchingEngine>(orderService_, balanceService_);
    marketDataService_ = std::make_shared<MarketDataService>();
    snapshotService_ = std::make_shared<SnapshotService>(redisClient_);
    errorService_ = std::make_shared<ErrorService>();
    
    // Set up callbacks
    matchingEngine_->setTradeCallback([this](const Trade& trade) {
        onTrade(trade);
    });
    
    matchingEngine_->setOrderUpdateCallback([this](const Order& order) {
        onOrderUpdate(order);
    });
    
    errorService_->setErrorCallback([this](ErrorCode code, const std::string& msg, const std::string& ctx) {
        onError(code, msg, ctx);
    });
}

TradingEngine::~TradingEngine() {
    stop();
}

bool TradingEngine::start() {
    if (running_) {
        return false;
    }
    
    // Connect to Redis
    if (!redisClient_->connect()) {
        std::cerr << "Failed to connect to Redis" << std::endl;
        return false;
    }
    
    running_ = true;
    
    // Start worker threads for order processing
    for (int i = 0; i < config_.workerThreads; i++) {
        workerThreads_.emplace_back([this]() {
            processOrderQueue();
        });
    }
    
    // Start snapshot thread if enabled
    if (config_.enableSnapshot) {
        snapshotThread_ = std::thread([this]() {
            periodicSnapshot();
        });
    }
    
    std::cout << "Trading Engine started successfully" << std::endl;
    return true;
}

void TradingEngine::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Wait for worker threads to finish
    for (auto& thread : workerThreads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Wait for snapshot thread
    if (snapshotThread_.joinable()) {
        snapshotThread_.join();
    }
    
    // Disconnect from Redis
    redisClient_->disconnect();
    
    std::cout << "Trading Engine stopped" << std::endl;
}

std::pair<std::shared_ptr<Order>, ErrorCode> TradingEngine::submitOrder(
    UserId userId,
    const std::string& symbol,
    Side side,
    OrderType type,
    TimeInForce timeInForce,
    Price price,
    Quantity quantity
) {
    // Create order
    auto [order, errorCode] = orderService_->createOrder(
        userId, symbol, side, type, timeInForce, price, quantity
    );
    
    if (errorCode != ErrorCode::SUCCESS) {
        errorService_->reportError(errorCode, "Failed to create order", 
            "User: " + std::to_string(userId) + ", Symbol: " + symbol);
        return {order, errorCode};
    }
    
    // Process order through matching engine
    auto trades = matchingEngine_->processOrder(order);
    
    // Save order state
    snapshotService_->saveOrderState(*order);
    
    return {order, ErrorCode::SUCCESS};
}

ErrorCode TradingEngine::cancelOrder(OrderId orderId) {
    auto order = orderService_->getOrder(orderId);
    if (!order) {
        return ErrorCode::ORDER_NOT_FOUND;
    }
    
    ErrorCode result = orderService_->cancelOrder(orderId);
    
    if (result == ErrorCode::SUCCESS) {
        // Remove from order book
        auto book = matchingEngine_->getOrderBook(order->symbol);
        book->removeOrder(orderId, order->side);
        
        // Publish update
        publishOrderUpdate(*order);
        
        // Save state
        snapshotService_->saveOrderState(*order);
    }
    
    return result;
}

std::shared_ptr<Order> TradingEngine::getOrderStatus(OrderId orderId) {
    return orderService_->getOrder(orderId);
}

MarketSnapshot TradingEngine::getMarketSnapshot(const std::string& symbol) {
    return matchingEngine_->getMarketSnapshot(symbol);
}

json TradingEngine::getOrderBookDepth(const std::string& symbol, size_t levels) {
    auto book = matchingEngine_->getOrderBook(symbol);
    auto bids = book->getBidDepth(levels);
    auto asks = book->getAskDepth(levels);
    
    return MarketDataService::orderBookToJson(symbol, bids, asks);
}

void TradingEngine::initializeUserBalance(UserId userId, int64_t initialBalance) {
    balanceService_->initializeBalance(userId, initialBalance);
    
    UserBalance balance = balanceService_->getBalance(userId);
    snapshotService_->saveUserBalance(balance);
}

UserBalance TradingEngine::getUserBalance(UserId userId) {
    return balanceService_->getBalance(userId);
}

void TradingEngine::processOrderQueue() {
    while (running_) {
        try {
            // Pop order from Redis queue (blocking with timeout)
            std::string orderJson = redisClient_->popFromQueue(config_.orderInputQueue, 1);
            
            if (!orderJson.empty()) {
                handleOrder(orderJson);
            }
        } catch (const std::exception& e) {
            errorService_->reportError(ErrorCode::SYSTEM_ERROR, 
                "Error processing order queue", e.what());
        }
    }
}

void TradingEngine::handleOrder(const std::string& orderJson) {
    try {
        json j = json::parse(orderJson);
        
        std::string action = j["action"];
        
        if (action == "place") {
            UserId userId = j["userId"];
            std::string symbol = j["symbol"];
            Side side = (j["side"] == "BUY") ? Side::BUY : Side::SELL;
            OrderType type = (j["type"] == "LIMIT") ? OrderType::LIMIT : OrderType::MARKET;
            
            TimeInForce tif = TimeInForce::GFD;
            if (j.contains("timeInForce")) {
                std::string tifStr = j["timeInForce"];
                if (tifStr == "IOC") tif = TimeInForce::IOC;
                else if (tifStr == "FOK") tif = TimeInForce::FOK;
            }
            
            Price price = MarketDataService::doubleToPrice(j["price"]);
            Quantity quantity = j["quantity"];
            
            submitOrder(userId, symbol, side, type, tif, price, quantity);
            
        } else if (action == "cancel") {
            OrderId orderId = j["orderId"];
            cancelOrder(orderId);
        }
        
    } catch (const std::exception& e) {
        errorService_->reportError(ErrorCode::SYSTEM_ERROR, 
            "Error handling order", e.what());
    }
}

void TradingEngine::periodicSnapshot() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(config_.snapshotIntervalSeconds));
        
        if (!running_) break;
        
        try {
            // Create snapshot with timestamp
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()).count();
            
            std::string snapshotId = "snapshot_" + std::to_string(timestamp);
            snapshotService_->createFullSnapshot(snapshotId);
            
        } catch (const std::exception& e) {
            errorService_->reportError(ErrorCode::SYSTEM_ERROR, 
                "Error creating snapshot", e.what());
        }
    }
}

void TradingEngine::onTrade(const Trade& trade) {
    publishTrade(trade);
    
    // Save trade for persistence
    snapshotService_->saveTrade(trade);
}

void TradingEngine::onOrderUpdate(const Order& order) {
    publishOrderUpdate(order);
    
    // Save order state
    snapshotService_->saveOrderState(order);
}

void TradingEngine::onError(ErrorCode code, const std::string& message, const std::string& context) {
    publishError(code, message);
}

void TradingEngine::publishMarketData(const Trade& trade, const MarketSnapshot& snapshot) {
    json tickData = MarketDataService::generateTickData(trade, snapshot);
    redisClient_->publish(config_.marketDataChannel, tickData.dump());
}

void TradingEngine::publishOrderUpdate(const Order& order) {
    json orderUpdate = MarketDataService::generateOrderUpdate(order);
    redisClient_->publish(config_.orderUpdateChannel, orderUpdate.dump());
}

void TradingEngine::publishTrade(const Trade& trade) {
    json tradeJson = MarketDataService::tradeToJson(trade);
    redisClient_->publish(config_.tradeChannel, tradeJson.dump());
    
    // Also publish market data update
    auto snapshot = matchingEngine_->getMarketSnapshot(trade.symbol);
    publishMarketData(trade, snapshot);
}

void TradingEngine::publishError(ErrorCode code, const std::string& message) {
    json errorMsg = MarketDataService::generateErrorMessage(code, message);
    redisClient_->publish(config_.errorChannel, errorMsg.dump());
}

} // namespace TradingEngine
