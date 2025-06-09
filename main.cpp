#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <muduo/base/Timestamp.h>
#include <iostream>

using namespace muduo;
using namespace muduo::net;

void onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        std::cout << "New connection from " << conn->peerAddress().toIpPort() << std::endl;
    } else {
        std::cout << "Connection closed from " << conn->peerAddress().toIpPort() << std::endl;
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
    std::string msg(buf->retrieveAllAsString());
    std::cout << "Received message: " << msg << std::endl;
    
    // echo server
    conn->send(msg);
}

int main() {
    EventLoop loop;
    InetAddress listenAddr(8080);
    
    TcpServer server(&loop, listenAddr, "ChatServer");
    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);
    
    server.start();
    std::cout << "Server started on port 8080" << std::endl;
    
    loop.loop();
    return 0;
}