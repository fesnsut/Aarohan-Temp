#include "error_service.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace TradingEngine {

void ErrorService::reportError(ErrorCode code, const std::string& message, const std::string& context) {
    logError(code, message, context);
    
    if (errorCallback_) {
        errorCallback_(code, message, context);
    }
}

std::string ErrorService::getErrorMessage(ErrorCode code) {
    switch (code) {
        case ErrorCode::SUCCESS:
            return "Success";
        case ErrorCode::INVALID_SYMBOL:
            return "Invalid symbol";
        case ErrorCode::INVALID_QUANTITY:
            return "Invalid quantity";
        case ErrorCode::INVALID_PRICE:
            return "Invalid price";
        case ErrorCode::INSUFFICIENT_BALANCE:
            return "Insufficient balance";
        case ErrorCode::ORDER_NOT_FOUND:
            return "Order not found";
        case ErrorCode::DUPLICATE_ORDER:
            return "Duplicate order";
        case ErrorCode::SYSTEM_ERROR:
            return "System error";
        default:
            return "Unknown error";
    }
}

bool ErrorService::isCriticalError(ErrorCode code) {
    return code == ErrorCode::SYSTEM_ERROR;
}

void ErrorService::logError(ErrorCode code, const std::string& message, const std::string& context) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "] ";
    ss << "ERROR [" << static_cast<int>(code) << "] ";
    ss << getErrorMessage(code) << ": " << message;
    
    if (!context.empty()) {
        ss << " (Context: " << context << ")";
    }
    
    std::cerr << ss.str() << std::endl;
}

} // namespace TradingEngine
