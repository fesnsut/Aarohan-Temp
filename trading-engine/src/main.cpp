#include "trading_engine.hpp"
#include <iostream>
#include <signal.h>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace TradingEngine;
using json = nlohmann::json;

// Global engine pointer for signal handling
std::shared_ptr<TradingEngine::TradingEngine> g_engine;

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
    if (g_engine) {
        g_engine->stop();
    }
    exit(signum);
}

EngineConfig loadConfig(const std::string& configFile) {
    EngineConfig config;
    
    try {
        std::ifstream file(configFile);
        if (file.is_open()) {
            json j;
            file >> j;
            
            if (j.contains("redis")) {
                config.redisHost = j["redis"].value("host", "localhost");
                config.redisPort = j["redis"].value("port", 6379);
                config.redisPassword = j["redis"].value("password", "");
            }
            
            if (j.contains("queues")) {
                config.orderInputQueue = j["queues"].value("orderInput", "order_input_queue");
            }
            
            if (j.contains("channels")) {
                config.marketDataChannel = j["channels"].value("marketData", "market_data");
                config.orderUpdateChannel = j["channels"].value("orderUpdate", "order_updates");
                config.tradeChannel = j["channels"].value("trade", "trades");
                config.errorChannel = j["channels"].value("error", "errors");
            }
            
            if (j.contains("engine")) {
                config.workerThreads = j["engine"].value("workerThreads", 4);
                config.enableSnapshot = j["engine"].value("enableSnapshot", true);
                config.snapshotIntervalSeconds = j["engine"].value("snapshotInterval", 60);
            }
            
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        std::cerr << "Using default configuration" << std::endl;
    }
    
    return config;
}

void initializeMockUsers(TradingEngine::TradingEngine& engine) {
    // Initialize some test users with balances
    std::cout << "Initializing mock user accounts..." << std::endl;
    
    for (UserId userId = 1; userId <= 10; userId++) {
        engine.initializeUserBalance(userId, 1000000); // $10,000 per user (in cents)
        std::cout << "  User " << userId << ": $10,000" << std::endl;
    }
    
    std::cout << "User initialization complete" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "==================================" << std::endl;
    std::cout << "  Mock Trading Engine v1.0" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;
    
    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Load configuration
    std::string configFile = "config/engine.json";
    if (argc > 1) {
        configFile = argv[1];
    }
    
    std::cout << "Loading configuration from: " << configFile << std::endl;
    EngineConfig config = loadConfig(configFile);
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Redis: " << config.redisHost << ":" << config.redisPort << std::endl;
    std::cout << "  Worker Threads: " << config.workerThreads << std::endl;
    std::cout << "  Snapshot Enabled: " << (config.enableSnapshot ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
    
    // Create engine
    g_engine = std::make_shared<TradingEngine::TradingEngine>(config);
    
    // Initialize mock users
    initializeMockUsers(*g_engine);
    std::cout << std::endl;
    
    // Start engine
    std::cout << "Starting trading engine..." << std::endl;
    if (!g_engine->start()) {
        std::cerr << "Failed to start trading engine" << std::endl;
        return 1;
    }
    
    std::cout << std::endl;
    std::cout << "Trading Engine is running" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;
    
    // Keep running
    while (g_engine->isRunning()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
