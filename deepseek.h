#pragma once

#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "lfuCache.h"

class DeepSeekChat {
public:
    struct Message {
        std::string role;    // "system", "user" 或 "assistant"
        std::string content;
    };

    struct CacheEntry {
        Message response;
        int token_count;
    };

    DeepSeekChat(const std::string& apiKey);
    ~DeepSeekChat();

    // 发送消息并获取回复
    Message sendMessage(const std::string& content);
    
    // 计算消息的token数量
    int calculateTokens(const std::string& content);
    
    // 获取最后一次请求的token使用情况
    struct TokenUsage {
        int prompt_tokens;
        int completion_tokens;
        int total_tokens;
    };
    TokenUsage getLastTokenUsage() const { return lastTokenUsage_; }

    // 检查缓存中是否存在相同的查询
    bool checkCache(const std::string& query, Message& cached_response);
    
    // 将查询和响应添加到缓存
    void addToCache(const std::string& query, const Message& response, int token_count);

private:
    // CURL 回调函数
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    
    // 发送 HTTP 请求
    std::string sendRequest(const std::string& url, const std::string& data);
    
    // 解析 JSON 响应
    Message parseMessageResponse(const std::string& response);

private:
    std::string apiKey_;
    CURL* curl_;
    const std::string baseUrl_ = "https://api.deepseek.com/chat/completions";
    TokenUsage lastTokenUsage_;
    
    // 使用LFU缓存，容量设置为1000，分片数为4
    CacheImpl::HashLfuCache<std::string, Message> response_cache_{1000, 4};
    const int MAX_CACHE_TOKEN = 64;  // 最大缓存token数
};