#include "order_service.hpp"
#include <algorithm>

namespace TradingEngine {

std::pair<std::shared_ptr<Order>, ErrorCode> OrderService::createOrder(
    UserId userId,
    const std::string& symbol,
    Side side,
    OrderType type,
    TimeInForce timeInForce,
    Price price,
    Quantity quantity
) {
    auto order = std::make_shared<Order>();
    order->orderId = generateOrderId();
    order->userId = userId;
    order->symbol = symbol;
    order->side = side;
    order->type = type;
    order->timeInForce = timeInForce;
    order->price = price;
    order->quantity = quantity;
    order->filledQuantity = 0;
    order->status = OrderStatus::PENDING;
    order->timestamp = std::chrono::high_resolution_clock::now();
    
    // Validate order
    ErrorCode validationResult = validateOrder(*order);
    if (validationResult != ErrorCode::SUCCESS) {
        order->status = OrderStatus::REJECTED;
        return {order, validationResult};
    }
    
    // Check and lock funds for buy orders
    if (side == Side::BUY) {
        int64_t requiredFunds = balanceService_->calculateRequiredFunds(*order);
        ErrorCode lockResult = balanceService_->lockFunds(userId, requiredFunds);
        if (lockResult != ErrorCode::SUCCESS) {
            order->status = OrderStatus::REJECTED;
            return {order, lockResult};
        }
    }
    
    // Store order
    {
        std::lock_guard<std::mutex> lock(mutex_);
        orders_[order->orderId] = order;
        userOrders_[userId].push_back(order->orderId);
    }
    
    return {order, ErrorCode::SUCCESS};
}

ErrorCode OrderService::cancelOrder(OrderId orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        return ErrorCode::ORDER_NOT_FOUND;
    }
    
    auto order = it->second;
    
    // Can only cancel pending or partially filled orders
    if (order->status != OrderStatus::PENDING && 
        order->status != OrderStatus::PARTIALLY_FILLED) {
        return ErrorCode::SYSTEM_ERROR;
    }
    
    // Unlock funds for unfilled portion
    if (order->side == Side::BUY) {
        Quantity unfilledQty = order->quantity - order->filledQuantity;
        int64_t lockedFunds = order->price * unfilledQty;
        balanceService_->unlockFunds(order->userId, lockedFunds);
    }
    
    order->status = OrderStatus::CANCELLED;
    return ErrorCode::SUCCESS;
}

std::shared_ptr<Order> OrderService::getOrder(OrderId orderId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orders_.find(orderId);
    if (it != orders_.end()) {
        return it->second;
    }
    return nullptr;
}

void OrderService::updateOrderStatus(OrderId orderId, OrderStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orders_.find(orderId);
    if (it != orders_.end()) {
        it->second->status = status;
    }
}

void OrderService::updateFilledQuantity(OrderId orderId, Quantity filledQty) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orders_.find(orderId);
    if (it != orders_.end()) {
        it->second->filledQuantity += filledQty;
        
        if (it->second->filledQuantity >= it->second->quantity) {
            it->second->status = OrderStatus::FILLED;
        } else if (it->second->filledQuantity > 0) {
            it->second->status = OrderStatus::PARTIALLY_FILLED;
        }
    }
}

std::vector<std::shared_ptr<Order>> OrderService::getUserOrders(UserId userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Order>> result;
    auto it = userOrders_.find(userId);
    if (it != userOrders_.end()) {
        for (OrderId orderId : it->second) {
            auto orderIt = orders_.find(orderId);
            if (orderIt != orders_.end()) {
                result.push_back(orderIt->second);
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<Order>> OrderService::getActiveOrders(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::shared_ptr<Order>> result;
    for (const auto& [orderId, order] : orders_) {
        if (order->symbol == symbol && 
            (order->status == OrderStatus::PENDING || 
             order->status == OrderStatus::PARTIALLY_FILLED)) {
            result.push_back(order);
        }
    }
    return result;
}

ErrorCode OrderService::validateOrder(const Order& order) {
    // Validate symbol
    if (order.symbol.empty()) {
        return ErrorCode::INVALID_SYMBOL;
    }
    
    // Validate quantity
    if (order.quantity == 0) {
        return ErrorCode::INVALID_QUANTITY;
    }
    
    // Validate price for limit orders
    if (order.type == OrderType::LIMIT && order.price == 0) {
        return ErrorCode::INVALID_PRICE;
    }
    
    return ErrorCode::SUCCESS;
}

OrderId OrderService::generateOrderId() {
    return nextOrderId_.fetch_add(1, std::memory_order_relaxed);
}

} // namespace TradingEngine
