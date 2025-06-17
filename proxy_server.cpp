#include <httplib.h>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "lfuCache.h"
#include <atomic>
#include "lruCache.h"

// Cache response structure
struct CachedResponse {
    std::string content;
    std::string role;
};

// Cache statistics
struct CacheStats {
    size_t hits = 0;
    size_t misses = 0;
    size_t total_entries = 0;
    
    double getHitRate() const {
        return (hits + misses) == 0 ? 0.0 : (hits * 100.0 / (hits + misses));
    }
};

// 会话历史结构
struct SessionHistory {
    std::vector<std::string> messages;
    std::string lastResponse;
};

int main() {
    try {
        httplib::Server svr;

        // Set CORS headers
        svr.set_default_headers({
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
            {"Access-Control-Allow-Headers", "Content-Type"}
        });

        // Set static file directory
        std::string exe_path = std::filesystem::current_path().string();
        std::string static_path = exe_path + "/../static";
        std::cout << "Setting static directory: " << static_path << std::endl;
        
        if (!std::filesystem::exists(static_path)) {
            std::cout << "Creating static directory: " << static_path << std::endl;
            std::filesystem::create_directories(static_path);
        }
        
        if (!svr.set_mount_point("/", static_path)) {
            std::cout << "Failed to set static directory" << std::endl;
            return 1;
        }

        // Create main server client
        httplib::Client main_server("172.18.0.10", 8888);
        
        // Configure client
        main_server.set_connection_timeout(5);
        main_server.set_read_timeout(60);
        main_server.set_write_timeout(60);
        main_server.set_keep_alive(true);
        main_server.set_default_headers({
            {"Connection", "keep-alive"},
            {"Keep-Alive", "timeout=60"}
        });

        // Create cache (capacity 1000, 4 slices)
        CacheImpl::HashLfuCache<std::string, CachedResponse> response_cache_{1000, 4};
        const int MAX_CACHE_TOKEN = 64;
        CacheStats cache_stats;

        // 创建会话历史LRU缓存，容量为1000个会话
        CacheImpl::HashLruCache<std::string, SessionHistory> session_cache(1000, 4);

        // Token calculation function
        auto calculateTokens = [](const std::string& content) -> int {
            int tokens = 0;
            std::string current_word;
            
            for (char c : content) {
                if (std::isspace(c)) {
                    if (!current_word.empty()) {
                        if (current_word.length() == 1 && (unsigned char)current_word[0] > 127) {
                            tokens += 1;
                        } else {
                            tokens += (current_word.length() + 3) / 4;
                        }
                        current_word.clear();
                    }
                } else {
                    current_word += c;
                }
            }
            
            if (!current_word.empty()) {
                if (current_word.length() == 1 && (unsigned char)current_word[0] > 127) {
                    tokens += 1;
                } else {
                    tokens += (current_word.length() + 3) / 4;
                }
            }
            
            return tokens;
        };

        // Print cache statistics
        auto printCacheStats = [&cache_stats]() {
            std::cout << "\nCache Statistics:" << std::endl;
            std::cout << "Total Capacity: 1000" << std::endl;
            std::cout << "Current Entries: " << cache_stats.total_entries << std::endl;
            std::cout << "Cache Hits: " << cache_stats.hits << std::endl;
            std::cout << "Cache Misses: " << cache_stats.misses << std::endl;
            std::cout << "Hit Rate: " << cache_stats.getHitRate() << "%" << std::endl;
            std::cout << "------------------------" << std::endl;
        };

        // Try to connect to main server
        std::cout << "Connecting to main server..." << std::endl;
        auto test_res = main_server.Get("/api/hello");
        if (!test_res) {
            std::cerr << "Failed to connect to main server" << std::endl;
            return 1;
        }
        std::cout << "Connected to main server" << std::endl;

        // Handle message POST request
        svr.Post("/api/message", [&main_server, &response_cache_, &calculateTokens, &cache_stats, &printCacheStats, &session_cache](const httplib::Request &req, httplib::Response &res) {
            try {
                auto json = nlohmann::json::parse(req.body);
                std::string message = json["message"];
                std::string conversationId = json.value("conversationId", "");
                
                SessionHistory history;
                if (session_cache.get(conversationId, history)) {
                    history.messages.push_back(message);
                } else {
                    history.messages = {message};
                }

                std::cout << "Received message: " << message << std::endl;
                std::cout << "Conversation ID: " << conversationId << std::endl;

                int input_tokens = calculateTokens(message);
                std::cout << "Input tokens: " << input_tokens << std::endl;

                CachedResponse cached_response;
                if (input_tokens <= MAX_CACHE_TOKEN && response_cache_.get(message, cached_response)) {
                    cache_stats.hits++;
                    std::cout << "[Cache Hit] Retrieved from cache" << std::endl;
                    
                    nlohmann::json response;
                    response["conversationId"] = conversationId;
                    response["content"] = cached_response.content;
                    response["role"] = cached_response.role;
                    
                    res.set_content(response.dump(), "application/json");
                    std::cout << "Response sent from cache" << std::endl;
                    
                    printCacheStats();

                    history.lastResponse = cached_response.content;
                    session_cache.put(conversationId, history);
                    return;
                }

                cache_stats.misses++;
                std::cout << "[Cache Miss] Forwarding to main server" << std::endl;

                httplib::Headers headers = {
                    {"Content-Type", "application/json"},
                    {"Connection", "keep-alive"},
                    {"Keep-Alive", "timeout=60"}
                };

                auto main_res = main_server.Post("/api/message", headers, req.body, "application/json");
                
                if (main_res) {
                    std::cout << "Main server status: " << main_res->status << std::endl;
                    
                    auto response_json = nlohmann::json::parse(main_res->body);
                    
                    std::string assistant_reply;
                    std::string role;
                    if (response_json.contains("content") && !response_json["content"].is_null()) {
                        assistant_reply = response_json["content"];
                        role = response_json.value("role", "assistant");
                    } else if (response_json.contains("response") && !response_json["response"].is_null()) {
                        assistant_reply = response_json["response"];
                        role = "assistant";
                    } else {
                        assistant_reply = "(No valid content from main server)";
                        role = "assistant";
                    }
                    
                    if (input_tokens <= MAX_CACHE_TOKEN) {
                        CachedResponse cache_entry{assistant_reply, role};
                        response_cache_.put(message, cache_entry);
                        std::cout << "[Cache Add] Response added to cache" << std::endl;
                    } else {
                        std::cout << "[Cache Skip] Input tokens exceed limit" << std::endl;
                    }
                    
                    nlohmann::json response;
                    response["conversationId"] = conversationId;
                    response["content"] = assistant_reply;
                    response["role"] = role;
                    
                    res.status = main_res->status;
                    
                    for (const auto& header : main_res->headers) {
                        if (header.first != "Content-Length") {
                            res.set_header(header.first, header.second);
                        }
                    }
                    
                    res.set_content(response.dump(), "application/json");
                    std::cout << "Response forwarded to client" << std::endl;
                    
                    printCacheStats();

                    history.lastResponse = assistant_reply;
                    session_cache.put(conversationId, history);
                } else {
                    std::cout << "No response from main server" << std::endl;
                    nlohmann::json error = {
                        {"error", "No response from main server"}
                    };
                    res.status = 502;
                    res.set_content(error.dump(), "application/json");
                }
            } catch (const std::exception& e) {
                std::cerr << "Error processing message: " << e.what() << std::endl;
                nlohmann::json error = {
                    {"error", std::string("Error processing message: ") + e.what()}
                };
                res.status = 500;
                res.set_content(error.dump(), "application/json");
            }
        });

        svr.Get("/api/session/:id", [&session_cache](const httplib::Request &req, httplib::Response &res) {
            try {
                std::string conversationId = req.path_params.at("id");
                SessionHistory history;
                
                if (session_cache.get(conversationId, history)) {
                    nlohmann::json response;
                    response["messages"] = history.messages;
                    response["lastResponse"] = history.lastResponse;
                    res.set_content(response.dump(), "application/json");
                } else {
                    res.status = 404;
                    nlohmann::json error = {
                        {"error", "Session not found"}
                    };
                    res.set_content(error.dump(), "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 500;
                nlohmann::json error = {
                    {"error", std::string("Error retrieving session: ") + e.what()}
                };
                res.set_content(error.dump(), "application/json");
            }
        });

        svr.Get("/api/session/list", [&session_cache](const httplib::Request &req, httplib::Response &res) {
            try {
                nlohmann::json response;
                nlohmann::json sessions = nlohmann::json::array();
                
                response["sessions"] = sessions;
                res.set_content(response.dump(), "application/json");
            } catch (const std::exception& e) {
                res.status = 500;
                nlohmann::json error = {
                    {"error", std::string("Error retrieving session list: ") + e.what()}
                };
                res.set_content(error.dump(), "application/json");
            }
        });

        std::cout << "Proxy server running on http://0.0.0.0:8889" << std::endl;
        svr.listen("0.0.0.0", 8889); 

    } catch (const std::exception& e) {
        std::cerr << "Proxy server initialization error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 