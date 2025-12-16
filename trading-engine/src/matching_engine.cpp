#include "matching_engine.hpp"
#include <algorithm>

namespace TradingEngine {

MatchingEngine::MatchingEngine(
    std::shared_ptr<OrderService> orderService,
    std::shared_ptr<BalanceService> balanceService
) : orderService_(orderService),
    balanceService_(balanceService),
    nextTradeId_(1) {}

std::vector<Trade> MatchingEngine::processOrder(std::shared_ptr<Order> order) {
    auto book = getOrderBook(order->symbol);
    
    std::vector<Trade> trades;
    
    if (order->type == OrderType::LIMIT) {
        trades = matchLimitOrder(order, book);
    } else {
        trades = matchMarketOrder(order, book);
    }
    
    // Handle IOC and FOK
    if (order->timeInForce == TimeInForce::IOC) {
        handleIOC(order, trades);
    } else if (order->timeInForce == TimeInForce::FOK) {
        if (!handleFOK(order, book)) {
            // FOK failed, cancel order
            order->status = OrderStatus::CANCELLED;
            notifyOrderUpdate(*order);
            return {};
        }
    }
    
    // Add unfilled portion to book if GFD and limit order
    if (order->timeInForce == TimeInForce::GFD && 
        order->type == OrderType::LIMIT &&
        order->filledQuantity < order->quantity &&
        order->status != OrderStatus::CANCELLED) {
        book->addOrder(order);
        notifyOrderUpdate(*order);
    }
    
    return trades;
}

std::shared_ptr<OrderBook> MatchingEngine::getOrderBook(const std::string& symbol) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = orderBooks_.find(symbol);
    if (it == orderBooks_.end()) {
        auto book = std::make_shared<OrderBook>(symbol);
        orderBooks_[symbol] = book;
        return book;
    }
    return it->second;
}

MarketSnapshot MatchingEngine::getMarketSnapshot(const std::string& symbol) {
    auto book = getOrderBook(symbol);
    return book->getSnapshot();
}

std::vector<Trade> MatchingEngine::matchLimitOrder(
    std::shared_ptr<Order> order,
    std::shared_ptr<OrderBook> book
) {
    std::vector<Trade> trades;
    
    while (order->filledQuantity < order->quantity) {
        std::shared_ptr<Order> counterOrder;
        
        if (order->side == Side::BUY) {
            counterOrder = book->getBestAskOrder();
            if (!counterOrder || counterOrder->price > order->price) {
                break; // No matching asks or price not good enough
            }
        } else {
            counterOrder = book->getBestBidOrder();
            if (!counterOrder || counterOrder->price < order->price) {
                break; // No matching bids or price not good enough
            }
        }
        
        // Calculate trade quantity
        Quantity remainingQty = order->quantity - order->filledQuantity;
        Quantity counterRemainingQty = counterOrder->quantity - counterOrder->filledQuantity;
        Quantity tradeQty = std::min(remainingQty, counterRemainingQty);
        
        // Execute trade at the counter order's price (price-time priority)
        Trade trade = executeTrade(
            order->side == Side::BUY ? order : counterOrder,
            order->side == Side::SELL ? order : counterOrder,
            counterOrder->price,
            tradeQty
        );
        
        trades.push_back(trade);
        
        // Update filled quantities
        order->filledQuantity += tradeQty;
        counterOrder->filledQuantity += tradeQty;
        
        // Update order statuses
        if (counterOrder->filledQuantity >= counterOrder->quantity) {
            counterOrder->status = OrderStatus::FILLED;
            book->removeOrder(counterOrder->orderId, counterOrder->side);
        } else {
            counterOrder->status = OrderStatus::PARTIALLY_FILLED;
        }
        
        notifyOrderUpdate(*counterOrder);
        notifyTrade(trade);
        
        // Publish market data
        auto snapshot = book->getSnapshot();
        publishMarketData(trade, snapshot);
    }
    
    // Update incoming order status
    if (order->filledQuantity >= order->quantity) {
        order->status = OrderStatus::FILLED;
    } else if (order->filledQuantity > 0) {
        order->status = OrderStatus::PARTIALLY_FILLED;
    }
    
    return trades;
}

std::vector<Trade> MatchingEngine::matchMarketOrder(
    std::shared_ptr<Order> order,
    std::shared_ptr<OrderBook> book
) {
    std::vector<Trade> trades;
    
    while (order->filledQuantity < order->quantity) {
        std::shared_ptr<Order> counterOrder;
        
        if (order->side == Side::BUY) {
            counterOrder = book->getBestAskOrder();
        } else {
            counterOrder = book->getBestBidOrder();
        }
        
        if (!counterOrder) {
            break; // No liquidity
        }
        
        // Calculate trade quantity
        Quantity remainingQty = order->quantity - order->filledQuantity;
        Quantity counterRemainingQty = counterOrder->quantity - counterOrder->filledQuantity;
        Quantity tradeQty = std::min(remainingQty, counterRemainingQty);
        
        // Execute trade at the counter order's price
        Trade trade = executeTrade(
            order->side == Side::BUY ? order : counterOrder,
            order->side == Side::SELL ? order : counterOrder,
            counterOrder->price,
            tradeQty
        );
        
        trades.push_back(trade);
        
        // Update filled quantities
        order->filledQuantity += tradeQty;
        counterOrder->filledQuantity += tradeQty;
        
        // Update counter order status
        if (counterOrder->filledQuantity >= counterOrder->quantity) {
            counterOrder->status = OrderStatus::FILLED;
            book->removeOrder(counterOrder->orderId, counterOrder->side);
        } else {
            counterOrder->status = OrderStatus::PARTIALLY_FILLED;
        }
        
        notifyOrderUpdate(*counterOrder);
        notifyTrade(trade);
        
        // Publish market data
        auto snapshot = book->getSnapshot();
        publishMarketData(trade, snapshot);
    }
    
    // Market orders are always filled or cancelled (no resting)
    if (order->filledQuantity >= order->quantity) {
        order->status = OrderStatus::FILLED;
    } else if (order->filledQuantity > 0) {
        order->status = OrderStatus::PARTIALLY_FILLED;
    } else {
        order->status = OrderStatus::CANCELLED;
    }
    
    return trades;
}

Trade MatchingEngine::executeTrade(
    std::shared_ptr<Order> buyOrder,
    std::shared_ptr<Order> sellOrder,
    Price tradePrice,
    Quantity tradeQuantity
) {
    Trade trade;
    trade.tradeId = generateTradeId();
    trade.buyOrderId = buyOrder->orderId;
    trade.sellOrderId = sellOrder->orderId;
    trade.buyUserId = buyOrder->userId;
    trade.sellUserId = sellOrder->userId;
    trade.symbol = buyOrder->symbol;
    trade.price = tradePrice;
    trade.quantity = tradeQuantity;
    trade.timestamp = std::chrono::high_resolution_clock::now();
    
    // Handle fund transfers
    int64_t tradeValue = tradePrice * tradeQuantity;
    
    // Complete trade for buyer (release locked funds)
    int64_t buyerLockedAmount = buyOrder->price * tradeQuantity;
    balanceService_->completeTrade(buyOrder->userId, buyerLockedAmount, tradeValue);
    
    // Credit seller
    balanceService_->transferFunds(buyOrder->userId, sellOrder->userId, tradeValue);
    
    // Update order book with last trade
    auto book = getOrderBook(trade.symbol);
    book->updateLastTrade(tradePrice, tradeQuantity);
    
    return trade;
}

bool MatchingEngine::handleIOC(std::shared_ptr<Order> order, const std::vector<Trade>& trades) {
    // IOC: Immediate or Cancel - any unfilled portion is cancelled
    if (order->filledQuantity < order->quantity) {
        // Unlock funds for unfilled portion
        if (order->side == Side::BUY) {
            Quantity unfilledQty = order->quantity - order->filledQuantity;
            int64_t lockedFunds = order->price * unfilledQty;
            balanceService_->unlockFunds(order->userId, lockedFunds);
        }
        
        if (order->filledQuantity == 0) {
            order->status = OrderStatus::CANCELLED;
        } else {
            order->status = OrderStatus::PARTIALLY_FILLED;
        }
    }
    return true;
}

bool MatchingEngine::handleFOK(std::shared_ptr<Order> order, std::shared_ptr<OrderBook> book) {
    // FOK: Fill or Kill - must be completely filled immediately or cancelled
    if (!canFillCompletely(order, book)) {
        // Unlock all funds
        if (order->side == Side::BUY) {
            int64_t lockedFunds = order->price * order->quantity;
            balanceService_->unlockFunds(order->userId, lockedFunds);
        }
        return false;
    }
    return true;
}

bool MatchingEngine::canFillCompletely(std::shared_ptr<Order> order, std::shared_ptr<OrderBook> book) {
    Quantity availableQuantity = 0;
    
    if (order->side == Side::BUY) {
        auto askDepth = book->getAskDepth(100);
        for (const auto& [price, qty] : askDepth) {
            if (order->type == OrderType::LIMIT && price > order->price) {
                break;
            }
            availableQuantity += qty;
            if (availableQuantity >= order->quantity) {
                return true;
            }
        }
    } else {
        auto bidDepth = book->getBidDepth(100);
        for (const auto& [price, qty] : bidDepth) {
            if (order->type == OrderType::LIMIT && price < order->price) {
                break;
            }
            availableQuantity += qty;
            if (availableQuantity >= order->quantity) {
                return true;
            }
        }
    }
    
    return false;
}

uint64_t MatchingEngine::generateTradeId() {
    return nextTradeId_.fetch_add(1, std::memory_order_relaxed);
}

void MatchingEngine::notifyTrade(const Trade& trade) {
    if (tradeCallback_) {
        tradeCallback_(trade);
    }
}

void MatchingEngine::notifyOrderUpdate(const Order& order) {
    if (orderUpdateCallback_) {
        orderUpdateCallback_(order);
    }
}

void MatchingEngine::publishMarketData(const Trade& trade, const MarketSnapshot& snapshot) {
    // This will be implemented in the main engine to publish to Redis
}

} // namespace TradingEngine
