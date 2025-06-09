#include "webServer.h"

WebSocketServer::WebSocketServer(muduo::net::EventLoop* loop,
                               const muduo::net::InetAddress& listenAddr,
                               const std::string& name)
    : server_(loop, listenAddr, name) {
    server_.setConnectionCallback(
        std::bind(&WebSocketServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
        std::bind(&WebSocketServer::onMessage, this, std::placeholders::_1,
                 std::placeholders::_2, std::placeholders::_3));
}

void WebSocketServer::start() {
    server_.start();
}