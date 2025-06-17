#include "deepseek.h"
#include <sstream>
#include <iostream>
#include <chrono>
#include <regex>

DeepSeekChat::DeepSeekChat(const std::string& apiKey) : apiKey_(apiKey) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
}

DeepSeekChat::~DeepSeekChat() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
    curl_global_cleanup();
}

size_t DeepSeekChat::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t realsize = size * nmemb;
    userp->append((char*)contents, realsize);
    return realsize;
}

std::string DeepSeekChat::sendRequest(const std::string& url, const std::string& data) {
    if (!curl_) {
        throw std::runtime_error("CURL not initialized");
    }

    std::string response;
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    
    // Set POST data
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.c_str());
    
    // Set request headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "Authorization: Bearer " + apiKey_;
    headers = curl_slist_append(headers, authHeader.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    // Set POST method
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);

    CURLcode res = curl_easy_perform(curl_);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    return response;
}

DeepSeekChat::Message DeepSeekChat::sendMessage(const std::string& content) {
    // Construct request data
    nlohmann::json requestData = {
        {"model", "deepseek-chat"},
        {"messages", {
            {
                {"role", "system"},
                {"content", "You are a helpful assistant."}
            },
            {
                {"role", "user"},
                {"content", content}
            }
        }},
        {"stream", false}
    };

    // Ensure JSON uses UTF-8 encoding
    std::string jsonStr = requestData.dump(-1, 32, true);
    std::string response = sendRequest(baseUrl_, jsonStr);
    
    return parseMessageResponse(response);
}

DeepSeekChat::Message DeepSeekChat::parseMessageResponse(const std::string& response) {
    Message message;
    try {
        // Parse JSON with UTF-8 encoding
        auto json = nlohmann::json::parse(response, nullptr, true, true);
        
        // Parse DeepSeek response
        if (json.contains("choices") && !json["choices"].empty()) {
            const auto& choice = json["choices"][0];
            if (choice.contains("message")) {
                message.role = choice["message"]["role"].get<std::string>();
                message.content = choice["message"]["content"].get<std::string>();
            }
        } else {
            throw std::runtime_error("No choices in response");
        }
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse message response: " + std::string(e.what()));
    }
    return message;
}