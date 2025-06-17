#include <httplib.h>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>
#include "deepseek.h"
#include "config.h"

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

        // Create DeepSeek client
        DeepSeekChat deepseek(DEEPSEEK_API_KEY);

        // Handle message POST request
        svr.Post("/api/message", [&deepseek](const httplib::Request &req, httplib::Response &res) {
            try {
                auto json = nlohmann::json::parse(req.body);
                std::string message = json["message"];
                // std::string conversationId = json.value("conversationId", "");

                // Send message to DeepSeek
                auto response = deepseek.sendMessage(message);

                // Construct response
                nlohmann::json response_json;
                // response_json["conversationId"] = conversationId;
                response_json["content"] = response.content;
                response_json["role"] = response.role;
                
                res.set_content(response_json.dump(), "application/json");

            } catch (const std::exception& e) {
                std::cerr << "Error processing message: " << e.what() << std::endl;
                nlohmann::json error = {
                    {"error", std::string("Error processing message: ") + e.what()}
                };
                res.status = 500;
                res.set_content(error.dump(), "application/json");
            }
        });

        // Add a simple health check endpoint
        svr.Get("/api/hello", [](const httplib::Request &, httplib::Response &res) {
            res.set_content("Hello from DeepSeek server!", "text/plain");
        });

        std::cout << "Server running on http://0.0.0.0:8888" << std::endl;
        svr.listen("0.0.0.0", 8888);

    } catch (const std::exception& e) {
        std::cerr << "Server initialization error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 