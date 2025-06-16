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
    
    // 设置 POST 数据
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, data.c_str());
    
    // 设置请求头
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "Authorization: Bearer " + apiKey_;
    headers = curl_slist_append(headers, authHeader.c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);

    // 设置 POST 方法
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);

    CURLcode res = curl_easy_perform(curl_);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    // 打印响应内容，用于调试
    std::cout << "API Response: " << response << std::endl;

    return response;
}

int DeepSeekChat::calculateTokens(const std::string& content) {
    // 简单的token估算方法
    // 1. 按空格分割
    // 2. 对于中文，每个字符算作一个token
    // 3. 对于英文，每4个字符算作一个token
    
    int tokens = 0;
    std::string current_word;
    
    for (char c : content) {
        if (std::isspace(c)) {
            if (!current_word.empty()) {
                // 检查是否是中文字符
                if (current_word.length() == 1 && (unsigned char)current_word[0] > 127) {
                    tokens += 1;  // 中文字符算作1个token
                } else {
                    tokens += (current_word.length() + 3) / 4;  // 英文按4个字符算1个token
                }
                current_word.clear();
            }
        } else {
            current_word += c;
        }
    }
    
    // 处理最后一个词
    if (!current_word.empty()) {
        if (current_word.length() == 1 && (unsigned char)current_word[0] > 127) {
            tokens += 1;
        } else {
            tokens += (current_word.length() + 3) / 4;
        }
    }
    
    return tokens;
}

DeepSeekChat::Message DeepSeekChat::sendMessage(const std::string& content) {
    // 计算输入token
    int input_tokens = calculateTokens(content);
    std::cout << "输入消息token数: " << input_tokens << std::endl;
    
    // 检查缓存
    Message cached_response;
    if (input_tokens <= MAX_CACHE_TOKEN && response_cache_.get(content, cached_response)) {
        std::cout << "【缓存命中】从缓存中获取响应" << std::endl;
        std::cout << "缓存响应内容: " << cached_response.content << std::endl;
        return cached_response;
    }
    
    std::cout << "【缓存未命中】需要调用API" << std::endl;
    
    // 构造请求数据
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

    // 确保 JSON 使用 UTF-8 编码
    std::string jsonStr = requestData.dump(-1, 32, true);
    std::string response = sendRequest(baseUrl_, jsonStr);
    
    Message message = parseMessageResponse(response);
    
    // 计算输出token
    int output_tokens = calculateTokens(message.content);
    int total_tokens = input_tokens + output_tokens;
    std::cout << "输出响应token数: " << output_tokens << std::endl;
    std::cout << "总token数: " << total_tokens << std::endl;
    
    // 如果总token数小于等于64，添加到缓存
    if (total_tokens <= MAX_CACHE_TOKEN) {
        response_cache_.put(content, message);
        std::cout << "【缓存添加】响应已添加到缓存" << std::endl;
    } else {
        std::cout << "【缓存跳过】token数超过限制，不添加到缓存" << std::endl;
    }
    
    return message;
}

DeepSeekChat::Message DeepSeekChat::parseMessageResponse(const std::string& response) {
    Message message;
    try {
        // 使用 UTF-8 编码解析 JSON
        auto json = nlohmann::json::parse(response, nullptr, true, true);
        
        // 打印完整的 JSON 响应，用于调试
        std::cout << "Parsed JSON: " << json.dump(2) << std::endl;
        
        // 解析 DeepSeek 响应
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