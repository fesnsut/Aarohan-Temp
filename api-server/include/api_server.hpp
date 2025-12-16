#pragma once

#include <string>
#include <functional>
#include <memory>
#include <httplib.h>
#include <nlohmann/json.hpp>

namespace APIServer {

using json = nlohmann::json;

struct ServerConfig {
    std::string host = "0.0.0.0";
    int port = 8080;
    std::string redisHost = "localhost";
    int redisPort = 6379;
    std::string redisPassword = "";
    std::string orderInputQueue = "order_input_queue";
    std::string marketDataChannel = "market_data";
    std::string orderUpdateChannel = "order_updates";
    std::string tradeChannel = "trades";
    int maxRequestSize = 1024 * 1024; // 1MB
};

class RedisClient;

class APIServerImpl {
public:
    explicit APIServerImpl(const ServerConfig& config);
    ~APIServerImpl();
    
    bool start();
    void stop();
    
private:
    ServerConfig config_;
    std::unique_ptr<httplib::Server> server_;
    std::shared_ptr<RedisClient> redisClient_;
    
    // Route handlers
    void setupRoutes();
    void handlePlaceOrder(const httplib::Request& req, httplib::Response& res);
    void handleCancelOrder(const httplib::Request& req, httplib::Response& res);
    void handleGetOrderStatus(const httplib::Request& req, httplib::Response& res);
    void handleGetMarketQuote(const httplib::Request& req, httplib::Response& res);
    void handleGetOrderBook(const httplib::Request& req, httplib::Response& res);
    void handleGetUserOrders(const httplib::Request& req, httplib::Response& res);
    void handleHealthCheck(const httplib::Request& req, httplib::Response& res);
    
    // Helper methods
    json createErrorResponse(const std::string& error, int code = 400);
    json createSuccessResponse(const json& data);
    bool validateOrderRequest(const json& orderData, std::string& error);
    
    // CORS headers
    void setCORSHeaders(httplib::Response& res);
};

} // namespace APIServer
