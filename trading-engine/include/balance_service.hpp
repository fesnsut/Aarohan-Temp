#pragma once

#include "types.hpp"
#include <map>
#include <mutex>
#include <memory>

namespace TradingEngine {

class BalanceService {
public:
    BalanceService() = default;
    
    // Initialize user balance
    void initializeBalance(UserId userId, int64_t initialBalance);
    
    // Get user balance
    UserBalance getBalance(UserId userId);
    
    // Lock funds for an order
    ErrorCode lockFunds(UserId userId, int64_t amount);
    
    // Unlock funds (e.g., order cancelled)
    ErrorCode unlockFunds(UserId userId, int64_t amount);
    
    // Transfer funds on trade execution
    ErrorCode transferFunds(UserId fromUserId, UserId toUserId, int64_t amount);
    
    // Check if user has sufficient balance
    bool hasSufficientBalance(UserId userId, int64_t amount);
    
    // Calculate required funds for an order
    int64_t calculateRequiredFunds(const Order& order);
    
    // Release locked funds and update available balance
    ErrorCode completeTrade(UserId userId, int64_t lockedAmount, int64_t actualAmount);
    
private:
    std::map<UserId, UserBalance> balances_;
    mutable std::mutex mutex_;
    
    // Helper to get or create balance entry
    UserBalance& getOrCreateBalance(UserId userId);
};

} // namespace TradingEngine
