#include "market_data_service.hpp"

namespace TradingEngine {

json MarketDataService::snapshotToJson(const MarketSnapshot& snapshot) {
    return json{
        {"type", "snapshot"},
        {"symbol", snapshot.symbol},
        {"lastTradePrice", priceToDouble(snapshot.lastTradePrice)},
        {"lastTradeQuantity", snapshot.lastTradeQuantity},
        {"bidPrice", priceToDouble(snapshot.bidPrice)},
        {"bidQuantity", snapshot.bidQuantity},
        {"askPrice", priceToDouble(snapshot.askPrice)},
        {"askQuantity", snapshot.askQuantity},
        {"totalVolume", snapshot.totalVolume},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            snapshot.timestamp.time_since_epoch()).count()}
    };
}

json MarketDataService::tradeToJson(const Trade& trade) {
    return json{
        {"type", "trade"},
        {"tradeId", trade.tradeId},
        {"buyOrderId", trade.buyOrderId},
        {"sellOrderId", trade.sellOrderId},
        {"buyUserId", trade.buyUserId},
        {"sellUserId", trade.sellUserId},
        {"symbol", trade.symbol},
        {"price", priceToDouble(trade.price)},
        {"quantity", trade.quantity},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            trade.timestamp.time_since_epoch()).count()}
    };
}

json MarketDataService::orderToJson(const Order& order) {
    return json{
        {"type", "order"},
        {"orderId", order.orderId},
        {"userId", order.userId},
        {"symbol", order.symbol},
        {"side", sideToString(order.side)},
        {"orderType", orderTypeToString(order.type)},
        {"timeInForce", timeInForceToString(order.timeInForce)},
        {"price", priceToDouble(order.price)},
        {"quantity", order.quantity},
        {"filledQuantity", order.filledQuantity},
        {"status", orderStatusToString(order.status)},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            order.timestamp.time_since_epoch()).count()}
    };
}

json MarketDataService::orderBookToJson(
    const std::string& symbol,
    const std::vector<std::pair<Price, Quantity>>& bids,
    const std::vector<std::pair<Price, Quantity>>& asks
) {
    json bidsJson = json::array();
    for (const auto& [price, qty] : bids) {
        bidsJson.push_back({
            {"price", priceToDouble(price)},
            {"quantity", qty}
        });
    }
    
    json asksJson = json::array();
    for (const auto& [price, qty] : asks) {
        asksJson.push_back({
            {"price", priceToDouble(price)},
            {"quantity", qty}
        });
    }
    
    return json{
        {"type", "orderbook"},
        {"symbol", symbol},
        {"bids", bidsJson},
        {"asks", asksJson},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count()}
    };
}

json MarketDataService::generateTickData(const Trade& trade, const MarketSnapshot& snapshot) {
    return json{
        {"type", "tick"},
        {"symbol", trade.symbol},
        {"lastTradePrice", priceToDouble(trade.price)},
        {"lastTradeQuantity", trade.quantity},
        {"bidPrice", priceToDouble(snapshot.bidPrice)},
        {"bidQuantity", snapshot.bidQuantity},
        {"askPrice", priceToDouble(snapshot.askPrice)},
        {"askQuantity", snapshot.askQuantity},
        {"totalVolume", snapshot.totalVolume},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            trade.timestamp.time_since_epoch()).count()}
    };
}

json MarketDataService::generateOrderUpdate(const Order& order) {
    return orderToJson(order);
}

json MarketDataService::generateErrorMessage(ErrorCode code, const std::string& message) {
    return json{
        {"type", "error"},
        {"code", static_cast<int>(code)},
        {"message", message},
        {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count()}
    };
}

double MarketDataService::priceToDouble(Price price, int decimals) {
    int divisor = 1;
    for (int i = 0; i < decimals; i++) {
        divisor *= 10;
    }
    return static_cast<double>(price) / divisor;
}

Price MarketDataService::doubleToPrice(double price, int decimals) {
    int multiplier = 1;
    for (int i = 0; i < decimals; i++) {
        multiplier *= 10;
    }
    return static_cast<Price>(price * multiplier);
}

} // namespace TradingEngine
