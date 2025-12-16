#pragma once

#include <string>
#include <memory>
#include <functional>
#include <sw/redis++/redis++.h>

namespace TradingEngine {

using RedisMessageCallback = std::function<void(const std::string& channel, const std::string& message)>;

class RedisClient {
public:
    RedisClient(const std::string& host, int port, const std::string& password = "");
    ~RedisClient();
    
    // Connection management
    bool connect();
    void disconnect();
    bool isConnected() const { return connected_; }
    
    // Queue operations (for order input)
    bool pushToQueue(const std::string& queue, const std::string& message);
    std::string popFromQueue(const std::string& queue, int timeoutSeconds = 0);
    std::vector<std::string> popMultipleFromQueue(const std::string& queue, size_t count);
    
    // Pub/Sub operations
    void publish(const std::string& channel, const std::string& message);
    void subscribe(const std::string& channel, RedisMessageCallback callback);
    void unsubscribe(const std::string& channel);
    
    // Key-value operations (for caching)
    bool set(const std::string& key, const std::string& value, int ttlSeconds = 0);
    std::string get(const std::string& key);
    bool exists(const std::string& key);
    bool del(const std::string& key);
    
    // Hash operations (for order book caching)
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    std::string hget(const std::string& key, const std::string& field);
    std::map<std::string, std::string> hgetall(const std::string& key);
    bool hdel(const std::string& key, const std::string& field);
    
    // Sorted set operations (for price levels)
    bool zadd(const std::string& key, double score, const std::string& member);
    std::vector<std::string> zrange(const std::string& key, long long start, long long stop);
    std::vector<std::string> zrevrange(const std::string& key, long long start, long long stop);
    bool zrem(const std::string& key, const std::string& member);

private:
    std::string host_;
    int port_;
    std::string password_;
    bool connected_;
    
    std::unique_ptr<sw::redis::Redis> redis_;
    std::unique_ptr<sw::redis::Subscriber> subscriber_;
    
    void handleSubscription();
};

} // namespace TradingEngine
