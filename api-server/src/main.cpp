#include "api_server.hpp"
#include <iostream>
#include <signal.h>
#include <fstream>

using namespace APIServer;

std::unique_ptr<APIServerImpl> g_server;

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
    if (g_server) {
        g_server->stop();
    }
    exit(signum);
}

ServerConfig loadConfig(const std::string& configFile) {
    ServerConfig config;
    
    try {
        std::ifstream file(configFile);
        if (file.is_open()) {
            json j;
            file >> j;
            
            if (j.contains("server")) {
                config.host = j["server"].value("host", "0.0.0.0");
                config.port = j["server"].value("port", 8080);
            }
            
            if (j.contains("redis")) {
                config.redisHost = j["redis"].value("host", "localhost");
                config.redisPort = j["redis"].value("port", 6379);
                config.redisPassword = j["redis"].value("password", "");
            }
            
            if (j.contains("queues")) {
                config.orderInputQueue = j["queues"].value("orderInput", "order_input_queue");
            }
            
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading config: " << e.what() << std::endl;
        std::cerr << "Using default configuration" << std::endl;
    }
    
    return config;
}

int main(int argc, char* argv[]) {
    std::cout << "==================================" << std::endl;
    std::cout << "  Trading API Server v1.0" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::string configFile = "config/api.json";
    if (argc > 1) {
        configFile = argv[1];
    }
    
    std::cout << "Loading configuration from: " << configFile << std::endl;
    ServerConfig config = loadConfig(configFile);
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Server: " << config.host << ":" << config.port << std::endl;
    std::cout << "  Redis: " << config.redisHost << ":" << config.redisPort << std::endl;
    std::cout << std::endl;
    
    g_server = std::make_unique<APIServerImpl>(config);
    
    if (!g_server->start()) {
        std::cerr << "Failed to start API server" << std::endl;
        return 1;
    }
    
    return 0;
}
