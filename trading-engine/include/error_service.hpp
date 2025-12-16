#pragma once

#include "types.hpp"
#include <string>
#include <memory>
#include <functional>

namespace TradingEngine {

using ErrorCallback = std::function<void(ErrorCode, const std::string&, const std::string&)>;

class ErrorService {
public:
    ErrorService() = default;
    
    // Register error callback
    void setErrorCallback(ErrorCallback callback) { errorCallback_ = callback; }
    
    // Report error
    void reportError(ErrorCode code, const std::string& message, const std::string& context = "");
    
    // Get error message for code
    static std::string getErrorMessage(ErrorCode code);
    
    // Check if error is critical (requires system shutdown)
    static bool isCriticalError(ErrorCode code);
    
    // Log error
    void logError(ErrorCode code, const std::string& message, const std::string& context = "");

private:
    ErrorCallback errorCallback_;
};

} // namespace TradingEngine
