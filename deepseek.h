#pragma once

#include <string>
#include <vector>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

class DeepSeekChat {
public:
    struct Message {
        std::string role;    // "system", "user" or "assistant"
        std::string content;
    };

    DeepSeekChat(const std::string& apiKey);
    ~DeepSeekChat();

    // Send message and get response
    Message sendMessage(const std::string& content);
    
    // Get token usage of last request
    struct TokenUsage {
        int prompt_tokens;
        int completion_tokens;
        int total_tokens;
    };
    TokenUsage getLastTokenUsage() const { return lastTokenUsage_; }

private:
    // CURL callback function
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    
    // Send HTTP request
    std::string sendRequest(const std::string& url, const std::string& data);
    
    // Parse JSON response
    Message parseMessageResponse(const std::string& response);

private:
    std::string apiKey_;
    CURL* curl_;
    const std::string baseUrl_ = "https://api.deepseek.com/chat/completions";
    TokenUsage lastTokenUsage_;
};