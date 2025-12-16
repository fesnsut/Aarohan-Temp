#include "orderbook.hpp"
#include <algorithm>

namespace TradingEngine {

// PriceLevel implementation
void PriceLevel::addOrder(std::shared_ptr<Order> order) {
    orders_.push_back(order);
    orderMap_[order->orderId] = std::prev(orders_.end());
    totalQuantity_ += (order->quantity - order->filledQuantity);
}

void PriceLevel::removeOrder(OrderId orderId) {
    auto it = orderMap_.find(orderId);
    if (it != orderMap_.end()) {
        auto orderIt = it->second;
        totalQuantity_ -= ((*orderIt)->quantity - (*orderIt)->filledQuantity);
        orders_.erase(orderIt);
        orderMap_.erase(it);
    }
}

std::shared_ptr<Order> PriceLevel::getFirstOrder() {
    if (orders_.empty()) {
        return nullptr;
    }
    return orders_.front();
}

// OrderBook implementation
void OrderBook::addOrder(std::shared_ptr<Order> order) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    orderMap_[order->orderId] = order;
    
    auto& levels = (order->side == Side::BUY) ? bidLevels_ : askLevels_;
    
    auto levelIt = levels.find(order->price);
    if (levelIt == levels.end()) {
        auto newLevel = std::make_shared<PriceLevel>(order->price);
        newLevel->addOrder(order);
        levels[order->price] = newLevel;
    } else {
        levelIt->second->addOrder(order);
    }
}

bool OrderBook::removeOrder(OrderId orderId, Side side) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto orderIt = orderMap_.find(orderId);
    if (orderIt == orderMap_.end()) {
        return false;
    }
    
    auto order = orderIt->second;
    auto& levels = (side == Side::BUY) ? bidLevels_ : askLevels_;
    
    auto levelIt = levels.find(order->price);
    if (levelIt != levels.end()) {
        levelIt->second->removeOrder(orderId);
        if (levelIt->second->isEmpty()) {
            levels.erase(levelIt);
        }
    }
    
    orderMap_.erase(orderIt);
    return true;
}

Price OrderBook::getBestBid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (bidLevels_.empty()) {
        return 0;
    }
    return bidLevels_.begin()->first;
}

Price OrderBook::getBestAsk() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (askLevels_.empty()) {
        return 0;
    }
    return askLevels_.begin()->first;
}

std::vector<std::pair<Price, Quantity>> OrderBook::getBidDepth(size_t levels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<Price, Quantity>> depth;
    
    size_t count = 0;
    for (const auto& [price, level] : bidLevels_) {
        if (count >= levels) break;
        depth.emplace_back(price, level->getTotalQuantity());
        count++;
    }
    
    return depth;
}

std::vector<std::pair<Price, Quantity>> OrderBook::getAskDepth(size_t levels) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<Price, Quantity>> depth;
    
    size_t count = 0;
    for (const auto& [price, level] : askLevels_) {
        if (count >= levels) break;
        depth.emplace_back(price, level->getTotalQuantity());
        count++;
    }
    
    return depth;
}

std::shared_ptr<Order> OrderBook::getOrder(OrderId orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = orderMap_.find(orderId);
    if (it != orderMap_.end()) {
        return it->second;
    }
    return nullptr;
}

MarketSnapshot OrderBook::getSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    MarketSnapshot snapshot;
    snapshot.symbol = symbol_;
    snapshot.lastTradePrice = lastTradePrice_;
    snapshot.totalVolume = totalVolume_;
    
    if (!bidLevels_.empty()) {
        auto bestBid = bidLevels_.begin();
        snapshot.bidPrice = bestBid->first;
        snapshot.bidQuantity = bestBid->second->getTotalQuantity();
    }
    
    if (!askLevels_.empty()) {
        auto bestAsk = askLevels_.begin();
        snapshot.askPrice = bestAsk->first;
        snapshot.askQuantity = bestAsk->second->getTotalQuantity();
    }
    
    snapshot.timestamp = std::chrono::high_resolution_clock::now();
    return snapshot;
}

void OrderBook::updateLastTrade(Price price, Quantity quantity) {
    std::lock_guard<std::mutex> lock(mutex_);
    lastTradePrice_ = price;
    totalVolume_ += quantity;
}

std::shared_ptr<Order> OrderBook::getBestBidOrder() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (bidLevels_.empty()) {
        return nullptr;
    }
    return bidLevels_.begin()->second->getFirstOrder();
}

std::shared_ptr<Order> OrderBook::getBestAskOrder() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (askLevels_.empty()) {
        return nullptr;
    }
    return askLevels_.begin()->second->getFirstOrder();
}

} // namespace TradingEngine
