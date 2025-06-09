#pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <functional>

class WebSocketServer {
public:
    WebSocketServer(muduo::net::EventLoop* loop,
                   const muduo::net::InetAddress& listenAddr,
                   const std::string& name);

    void start();

private:
    // Handle new connection
    void onConnection(const muduo::net::TcpConnectionPtr& conn);
    
    // Handle message
    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                  muduo::net::Buffer* buf,
                  muduo::Timestamp time);
    
    // Handle WebSocket handshake
    bool handleHandshake(const muduo::net::TcpConnectionPtr& conn,
                        const std::string& request);
    
    // Handle WebSocket message
    void handleWebSocketMessage(const muduo::net::TcpConnectionPtr& conn,
                              const std::string& message);
    
    // Send WebSocket message
    void sendWebSocketMessage(const muduo::net::TcpConnectionPtr& conn,
                            const std::string& message);

private:
    muduo::net::TcpServer server_;
    std::unordered_map<std::string, muduo::net::TcpConnectionPtr> connections_;
    
    // Callback function for passing messages to ChatRoom
    std::function<void(const std::string&, const std::string&)> messageCallback_;
};