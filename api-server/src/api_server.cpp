#include "api_server.hpp"
#include <iostream>
#include <sw/redis++/redis++.h>

namespace APIServer {

class RedisClient {
public:
    RedisClient(const std::string& host, int port, const std::string& password = "")
        : host_(host), port_(port), password_(password), connected_(false) {}
    
    bool connect() {
        try {
            sw::redis::ConnectionOptions opts;
            opts.host = host_;
            opts.port = port_;
            if (!password_.empty()) {
                opts.password = password_;
            }
            opts.socket_timeout = std::chrono::milliseconds(100);
            
            redis_ = std::make_unique<sw::redis::Redis>(opts);
            redis_->ping();
            connected_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Redis connection error: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool pushToQueue(const std::string& queue, const std::string& message) {
        try {
            redis_->rpush(queue, message);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Redis push error: " << e.what() << std::endl;
            return false;
        }
    }
    
    std::string get(const std::string& key) {
        try {
            auto result = redis_->get(key);
            if (result) {
                return *result;
            }
            return "";
        } catch (const std::exception& e) {
            return "";
        }
    }
    
    bool isConnected() const { return connected_; }

private:
    std::string host_;
    int port_;
    std::string password_;
    bool connected_;
    std::unique_ptr<sw::redis::Redis> redis_;
};

APIServerImpl::APIServerImpl(const ServerConfig& config)
    : config_(config) {
    server_ = std::make_unique<httplib::Server>();
    redisClient_ = std::make_shared<RedisClient>(
        config_.redisHost,
        config_.redisPort,
        config_.redisPassword
    );
}

APIServerImpl::~APIServerImpl() {
    stop();
}

bool APIServerImpl::start() {
    if (!redisClient_->connect()) {
        std::cerr << "Failed to connect to Redis" << std::endl;
        return false;
    }
    
    setupRoutes();
    
    std::cout << "API Server starting on " << config_.host << ":" << config_.port << std::endl;
    
    // Start server (this blocks)
    server_->listen(config_.host.c_str(), config_.port);
    
    return true;
}

void APIServerImpl::stop() {
    if (server_) {
        server_->stop();
    }
}

void APIServerImpl::setupRoutes() {
    // Health check
    server_->Get("/health", [this](const httplib::Request& req, httplib::Response& res) {
        handleHealthCheck(req, res);
    });
    
    // Order management
    server_->Post("/order/place", [this](const httplib::Request& req, httplib::Response& res) {
        handlePlaceOrder(req, res);
    });
    
    server_->Post("/order/cancel", [this](const httplib::Request& req, httplib::Response& res) {
        handleCancelOrder(req, res);
    });
    
    server_->Get("/order/status/:orderId", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetOrderStatus(req, res);
    });
    
    server_->Get("/order/user/:userId", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetUserOrders(req, res);
    });
    
    // Market data
    server_->Get("/market/quote/:symbol", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetMarketQuote(req, res);
    });
    
    server_->Get("/market/orderbook/:symbol", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetOrderBook(req, res);
    });
    
    // CORS preflight
    server_->Options(".*", [this](const httplib::Request& req, httplib::Response& res) {
        setCORSHeaders(res);
        res.set_content("", "text/plain");
    });
}

void APIServerImpl::handlePlaceOrder(const httplib::Request& req, httplib::Response& res) {
    setCORSHeaders(res);
    
    try {
        json requestData = json::parse(req.body);
        
        std::string error;
        if (!validateOrderRequest(requestData, error)) {
            res.status = 400;
            res.set_content(createErrorResponse(error).dump(), "application/json");
            return;
        }
        
        // Create order message for engine
        json orderMessage = {
            {"action", "place"},
            {"userId", requestData["userId"]},
            {"symbol", requestData["symbol"]},
            {"side", requestData["side"]},
            {"type", requestData["type"]},
            {"price", requestData["price"]},
            {"quantity", requestData["quantity"]},
            {"timeInForce", requestData.value("timeInForce", "GFD")}
        };
        
        // Push to Redis queue
        if (!redisClient_->pushToQueue(config_.orderInputQueue, orderMessage.dump())) {
            res.status = 500;
            res.set_content(createErrorResponse("Failed to submit order").dump(), "application/json");
            return;
        }
        
        json response = {
            {"success", true},
            {"message", "Order submitted successfully"},
            {"data", {
                {"userId", requestData["userId"]},
                {"symbol", requestData["symbol"]},
                {"side", requestData["side"]},
                {"type", requestData["type"]},
                {"price", requestData["price"]},
                {"quantity", requestData["quantity"]}
            }}
        };
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse(std::string("Invalid request: ") + e.what()).dump(), 
                       "application/json");
    }
}

void APIServerImpl::handleCancelOrder(const httplib::Request& req, httplib::Response& res) {
    setCORSHeaders(res);
    
    try {
        json requestData = json::parse(req.body);
        
        if (!requestData.contains("orderId")) {
            res.status = 400;
            res.set_content(createErrorResponse("Missing orderId").dump(), "application/json");
            return;
        }
        
        json cancelMessage = {
            {"action", "cancel"},
            {"orderId", requestData["orderId"]}
        };
        
        if (!redisClient_->pushToQueue(config_.orderInputQueue, cancelMessage.dump())) {
            res.status = 500;
            res.set_content(createErrorResponse("Failed to cancel order").dump(), "application/json");
            return;
        }
        
        json response = {
            {"success", true},
            {"message", "Order cancellation requested"},
            {"data", {
                {"orderId", requestData["orderId"]}
            }}
        };
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse(std::string("Invalid request: ") + e.what()).dump(), 
                       "application/json");
    }
}

void APIServerImpl::handleGetOrderStatus(const httplib::Request& req, httplib::Response& res) {
    setCORSHeaders(res);
    
    try {
        std::string orderId = req.path_params.at("orderId");
        
        // Get order from Redis cache
        std::string orderKey = "order:" + orderId;
        std::string orderData = redisClient_->get(orderKey);
        
        if (orderData.empty()) {
            res.status = 404;
            res.set_content(createErrorResponse("Order not found").dump(), "application/json");
            return;
        }
        
        json orderJson = json::parse(orderData);
        json response = createSuccessResponse(orderJson);
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse(std::string("Error: ") + e.what()).dump(), 
                       "application/json");
    }
}

void APIServerImpl::handleGetMarketQuote(const httplib::Request& req, httplib::Response& res) {
    setCORSHeaders(res);
    
    try {
        std::string symbol = req.path_params.at("symbol");
        
        // Get market snapshot from Redis
        std::string snapshotKey = "orderbook:" + symbol;
        std::string snapshotData = redisClient_->get(snapshotKey);
        
        if (snapshotData.empty()) {
            res.status = 404;
            res.set_content(createErrorResponse("Symbol not found").dump(), "application/json");
            return;
        }
        
        json snapshotJson = json::parse(snapshotData);
        json response = createSuccessResponse(snapshotJson);
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse(std::string("Error: ") + e.what()).dump(), 
                       "application/json");
    }
}

void APIServerImpl::handleGetOrderBook(const httplib::Request& req, httplib::Response& res) {
    setCORSHeaders(res);
    
    try {
        std::string symbol = req.path_params.at("symbol");
        
        // Get order book from Redis
        std::string bookKey = "orderbook:" + symbol;
        std::string bookData = redisClient_->get(bookKey);
        
        if (bookData.empty()) {
            // Return empty order book
            json emptyBook = {
                {"symbol", symbol},
                {"bids", json::array()},
                {"asks", json::array()}
            };
            res.status = 200;
            res.set_content(createSuccessResponse(emptyBook).dump(), "application/json");
            return;
        }
        
        json bookJson = json::parse(bookData);
        json response = createSuccessResponse(bookJson);
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse(std::string("Error: ") + e.what()).dump(), 
                       "application/json");
    }
}

void APIServerImpl::handleGetUserOrders(const httplib::Request& req, httplib::Response& res) {
    setCORSHeaders(res);
    
    try {
        std::string userId = req.path_params.at("userId");
        
        // In a real implementation, would query all user orders from Redis
        json response = createSuccessResponse(json::array());
        
        res.status = 200;
        res.set_content(response.dump(), "application/json");
        
    } catch (const std::exception& e) {
        res.status = 400;
        res.set_content(createErrorResponse(std::string("Error: ") + e.what()).dump(), 
                       "application/json");
    }
}

void APIServerImpl::handleHealthCheck(const httplib::Request& req, httplib::Response& res) {
    setCORSHeaders(res);
    
    json health = {
        {"status", "healthy"},
        {"redis", redisClient_->isConnected() ? "connected" : "disconnected"},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()}
    };
    
    res.status = 200;
    res.set_content(health.dump(), "application/json");
}

json APIServerImpl::createErrorResponse(const std::string& error, int code) {
    return json{
        {"success", false},
        {"error", error},
        {"code", code}
    };
}

json APIServerImpl::createSuccessResponse(const json& data) {
    return json{
        {"success", true},
        {"data", data}
    };
}

bool APIServerImpl::validateOrderRequest(const json& orderData, std::string& error) {
    if (!orderData.contains("userId")) {
        error = "Missing userId";
        return false;
    }
    
    if (!orderData.contains("symbol")) {
        error = "Missing symbol";
        return false;
    }
    
    if (!orderData.contains("side")) {
        error = "Missing side";
        return false;
    }
    
    std::string side = orderData["side"];
    if (side != "BUY" && side != "SELL") {
        error = "Invalid side (must be BUY or SELL)";
        return false;
    }
    
    if (!orderData.contains("type")) {
        error = "Missing type";
        return false;
    }
    
    std::string type = orderData["type"];
    if (type != "LIMIT" && type != "MARKET") {
        error = "Invalid type (must be LIMIT or MARKET)";
        return false;
    }
    
    if (!orderData.contains("quantity")) {
        error = "Missing quantity";
        return false;
    }
    
    if (orderData["quantity"] <= 0) {
        error = "Invalid quantity (must be positive)";
        return false;
    }
    
    if (type == "LIMIT") {
        if (!orderData.contains("price")) {
            error = "Missing price for LIMIT order";
            return false;
        }
        
        if (orderData["price"] <= 0) {
            error = "Invalid price (must be positive)";
            return false;
        }
    }
    
    return true;
}

void APIServerImpl::setCORSHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
}

} // namespace APIServer
