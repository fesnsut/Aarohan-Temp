#include "redis_client.hpp"
#include <iostream>
#include <thread>

namespace TradingEngine {

RedisClient::RedisClient(const std::string& host, int port, const std::string& password)
    : host_(host), port_(port), password_(password), connected_(false) {}

RedisClient::~RedisClient() {
    disconnect();
}

bool RedisClient::connect() {
    try {
        sw::redis::ConnectionOptions opts;
        opts.host = host_;
        opts.port = port_;
        if (!password_.empty()) {
            opts.password = password_;
        }
        opts.socket_timeout = std::chrono::milliseconds(100);
        
        redis_ = std::make_unique<sw::redis::Redis>(opts);
        
        // Test connection
        redis_->ping();
        
        connected_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis connection error: " << e.what() << std::endl;
        connected_ = false;
        return false;
    }
}

void RedisClient::disconnect() {
    if (subscriber_) {
        subscriber_.reset();
    }
    if (redis_) {
        redis_.reset();
    }
    connected_ = false;
}

bool RedisClient::pushToQueue(const std::string& queue, const std::string& message) {
    try {
        redis_->rpush(queue, message);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis push error: " << e.what() << std::endl;
        return false;
    }
}

std::string RedisClient::popFromQueue(const std::string& queue, int timeoutSeconds) {
    try {
        if (timeoutSeconds > 0) {
            auto result = redis_->blpop(queue, std::chrono::seconds(timeoutSeconds));
            if (result) {
                return result->second;
            }
            return "";
        } else {
            auto result = redis_->lpop(queue);
            if (result) {
                return *result;
            }
            return "";
        }
    } catch (const std::exception& e) {
        std::cerr << "Redis pop error: " << e.what() << std::endl;
        return "";
    }
}

std::vector<std::string> RedisClient::popMultipleFromQueue(const std::string& queue, size_t count) {
    std::vector<std::string> results;
    try {
        for (size_t i = 0; i < count; i++) {
            auto result = redis_->lpop(queue);
            if (result) {
                results.push_back(*result);
            } else {
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Redis pop multiple error: " << e.what() << std::endl;
    }
    return results;
}

void RedisClient::publish(const std::string& channel, const std::string& message) {
    try {
        redis_->publish(channel, message);
    } catch (const std::exception& e) {
        std::cerr << "Redis publish error: " << e.what() << std::endl;
    }
}

void RedisClient::subscribe(const std::string& channel, RedisMessageCallback callback) {
    try {
        if (!subscriber_) {
            sw::redis::ConnectionOptions opts;
            opts.host = host_;
            opts.port = port_;
            if (!password_.empty()) {
                opts.password = password_;
            }
            subscriber_ = std::make_unique<sw::redis::Subscriber>(redis_->subscriber());
        }
        
        subscriber_->on_message([callback](std::string chan, std::string msg) {
            callback(chan, msg);
        });
        
        subscriber_->subscribe(channel);
        
        // Start subscription in a separate thread
        std::thread([this]() {
            try {
                while (connected_) {
                    subscriber_->consume();
                }
            } catch (const std::exception& e) {
                std::cerr << "Redis subscription error: " << e.what() << std::endl;
            }
        }).detach();
        
    } catch (const std::exception& e) {
        std::cerr << "Redis subscribe error: " << e.what() << std::endl;
    }
}

void RedisClient::unsubscribe(const std::string& channel) {
    try {
        if (subscriber_) {
            subscriber_->unsubscribe(channel);
        }
    } catch (const std::exception& e) {
        std::cerr << "Redis unsubscribe error: " << e.what() << std::endl;
    }
}

bool RedisClient::set(const std::string& key, const std::string& value, int ttlSeconds) {
    try {
        if (ttlSeconds > 0) {
            redis_->set(key, value, std::chrono::seconds(ttlSeconds));
        } else {
            redis_->set(key, value);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis set error: " << e.what() << std::endl;
        return false;
    }
}

std::string RedisClient::get(const std::string& key) {
    try {
        auto result = redis_->get(key);
        if (result) {
            return *result;
        }
        return "";
    } catch (const std::exception& e) {
        std::cerr << "Redis get error: " << e.what() << std::endl;
        return "";
    }
}

bool RedisClient::exists(const std::string& key) {
    try {
        return redis_->exists(key) > 0;
    } catch (const std::exception& e) {
        std::cerr << "Redis exists error: " << e.what() << std::endl;
        return false;
    }
}

bool RedisClient::del(const std::string& key) {
    try {
        redis_->del(key);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis del error: " << e.what() << std::endl;
        return false;
    }
}

bool RedisClient::hset(const std::string& key, const std::string& field, const std::string& value) {
    try {
        redis_->hset(key, field, value);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis hset error: " << e.what() << std::endl;
        return false;
    }
}

std::string RedisClient::hget(const std::string& key, const std::string& field) {
    try {
        auto result = redis_->hget(key, field);
        if (result) {
            return *result;
        }
        return "";
    } catch (const std::exception& e) {
        std::cerr << "Redis hget error: " << e.what() << std::endl;
        return "";
    }
}

std::map<std::string, std::string> RedisClient::hgetall(const std::string& key) {
    try {
        std::map<std::string, std::string> result;
        redis_->hgetall(key, std::inserter(result, result.begin()));
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Redis hgetall error: " << e.what() << std::endl;
        return {};
    }
}

bool RedisClient::hdel(const std::string& key, const std::string& field) {
    try {
        redis_->hdel(key, field);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis hdel error: " << e.what() << std::endl;
        return false;
    }
}

bool RedisClient::zadd(const std::string& key, double score, const std::string& member) {
    try {
        redis_->zadd(key, member, score);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis zadd error: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::string> RedisClient::zrange(const std::string& key, long long start, long long stop) {
    try {
        std::vector<std::string> result;
        redis_->zrange(key, start, stop, std::back_inserter(result));
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Redis zrange error: " << e.what() << std::endl;
        return {};
    }
}

std::vector<std::string> RedisClient::zrevrange(const std::string& key, long long start, long long stop) {
    try {
        std::vector<std::string> result;
        redis_->zrevrange(key, start, stop, std::back_inserter(result));
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Redis zrevrange error: " << e.what() << std::endl;
        return {};
    }
}

bool RedisClient::zrem(const std::string& key, const std::string& member) {
    try {
        redis_->zrem(key, member);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Redis zrem error: " << e.what() << std::endl;
        return false;
    }
}

} // namespace TradingEngine
