#pragma once

#include "types.hpp"
#include "orderbook.hpp"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace TradingEngine {

using json = nlohmann::json;

class MarketDataService {
public:
    MarketDataService() = default;
    
    // Convert market snapshot to JSON
    static json snapshotToJson(const MarketSnapshot& snapshot);
    
    // Convert trade to JSON
    static json tradeToJson(const Trade& trade);
    
    // Convert order to JSON
    static json orderToJson(const Order& order);
    
    // Convert order book depth to JSON
    static json orderBookToJson(
        const std::string& symbol,
        const std::vector<std::pair<Price, Quantity>>& bids,
        const std::vector<std::pair<Price, Quantity>>& asks
    );
    
    // Generate tick data message
    static json generateTickData(const Trade& trade, const MarketSnapshot& snapshot);
    
    // Generate order update message
    static json generateOrderUpdate(const Order& order);
    
    // Generate error message
    static json generateErrorMessage(ErrorCode code, const std::string& message);
    
    // Price formatting (convert fixed-point to decimal)
    static double priceToDouble(Price price, int decimals = 2);
    static Price doubleToPrice(double price, int decimals = 2);
};

} // namespace TradingEngine
