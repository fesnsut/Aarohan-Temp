#pragma once

#include "types.hpp"
#include "orderbook.hpp"
#include "balance_service.hpp"
#include <map>
#include <memory>
#include <mutex>
#include <atomic>

namespace TradingEngine {

class OrderService {
public:
    OrderService(std::shared_ptr<BalanceService> balanceService)
        : balanceService_(balanceService), nextOrderId_(1) {}
    
    // Create and validate a new order
    std::pair<std::shared_ptr<Order>, ErrorCode> createOrder(
        UserId userId,
        const std::string& symbol,
        Side side,
        OrderType type,
        TimeInForce timeInForce,
        Price price,
        Quantity quantity
    );
    
    // Cancel an existing order
    ErrorCode cancelOrder(OrderId orderId);
    
    // Get order by ID
    std::shared_ptr<Order> getOrder(OrderId orderId);
    
    // Update order status
    void updateOrderStatus(OrderId orderId, OrderStatus status);
    
    // Update filled quantity
    void updateFilledQuantity(OrderId orderId, Quantity filledQty);
    
    // Get all orders for a user
    std::vector<std::shared_ptr<Order>> getUserOrders(UserId userId);
    
    // Get all active orders for a symbol
    std::vector<std::shared_ptr<Order>> getActiveOrders(const std::string& symbol);
    
    // Validate order
    ErrorCode validateOrder(const Order& order);

private:
    std::shared_ptr<BalanceService> balanceService_;
    std::map<OrderId, std::shared_ptr<Order>> orders_;
    std::map<UserId, std::vector<OrderId>> userOrders_;
    std::atomic<OrderId> nextOrderId_;
    mutable std::mutex mutex_;
    
    // Generate unique order ID
    OrderId generateOrderId();
};

} // namespace TradingEngine
