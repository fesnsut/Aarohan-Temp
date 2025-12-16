#include "balance_service.hpp"

namespace TradingEngine {

void BalanceService::initializeBalance(UserId userId, int64_t initialBalance) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    UserBalance balance;
    balance.userId = userId;
    balance.availableBalance = initialBalance;
    balance.lockedBalance = 0;
    
    balances_[userId] = balance;
}

UserBalance BalanceService::getBalance(UserId userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    return getOrCreateBalance(userId);
}

ErrorCode BalanceService::lockFunds(UserId userId, int64_t amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& balance = getOrCreateBalance(userId);
    
    if (balance.availableBalance < amount) {
        return ErrorCode::INSUFFICIENT_BALANCE;
    }
    
    balance.availableBalance -= amount;
    balance.lockedBalance += amount;
    
    return ErrorCode::SUCCESS;
}

ErrorCode BalanceService::unlockFunds(UserId userId, int64_t amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& balance = getOrCreateBalance(userId);
    
    if (balance.lockedBalance < amount) {
        return ErrorCode::SYSTEM_ERROR;
    }
    
    balance.lockedBalance -= amount;
    balance.availableBalance += amount;
    
    return ErrorCode::SUCCESS;
}

ErrorCode BalanceService::transferFunds(UserId fromUserId, UserId toUserId, int64_t amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& fromBalance = getOrCreateBalance(fromUserId);
    auto& toBalance = getOrCreateBalance(toUserId);
    
    if (fromBalance.availableBalance < amount) {
        return ErrorCode::INSUFFICIENT_BALANCE;
    }
    
    fromBalance.availableBalance -= amount;
    toBalance.availableBalance += amount;
    
    return ErrorCode::SUCCESS;
}

bool BalanceService::hasSufficientBalance(UserId userId, int64_t amount) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& balance = getOrCreateBalance(userId);
    return balance.availableBalance >= amount;
}

int64_t BalanceService::calculateRequiredFunds(const Order& order) {
    if (order.side == Side::BUY) {
        // For buy orders, need funds = price * quantity
        return order.price * order.quantity;
    } else {
        // For sell orders, we'd need to lock the shares (not implemented in this mock)
        // In a real system, this would check portfolio holdings
        return 0;
    }
}

ErrorCode BalanceService::completeTrade(UserId userId, int64_t lockedAmount, int64_t actualAmount) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& balance = getOrCreateBalance(userId);
    
    if (balance.lockedBalance < lockedAmount) {
        return ErrorCode::SYSTEM_ERROR;
    }
    
    // Release locked funds
    balance.lockedBalance -= lockedAmount;
    
    // Return unused funds (if partially filled)
    int64_t refund = lockedAmount - actualAmount;
    if (refund > 0) {
        balance.availableBalance += refund;
    }
    
    return ErrorCode::SUCCESS;
}

UserBalance& BalanceService::getOrCreateBalance(UserId userId) {
    auto it = balances_.find(userId);
    if (it == balances_.end()) {
        UserBalance balance;
        balance.userId = userId;
        balance.availableBalance = 0;
        balance.lockedBalance = 0;
        balances_[userId] = balance;
        return balances_[userId];
    }
    return it->second;
}

} // namespace TradingEngine
