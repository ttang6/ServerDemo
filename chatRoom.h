#pragma once

#include <string>
#include <unordered_map>
#include <set>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>
#include "lruCache.h"
#include "lfuCache.h"

// Message structure
struct Message {
    std::string id;
    std::string content;
    std::string senderNickname;
    int64_t timestamp;
};

// Chat room structure
struct Room {
    std::string id;
    std::string name;
    std::set<std::string> users;  // Set of user IDs
    std::vector<Message> messages;
};

class ChatRoom {
public:
    ChatRoom();
    ~ChatRoom();

    // Room management
    std::string createRoom(const std::string& name);
    bool joinRoom(const std::string& roomId, const std::string& userId);
    bool leaveRoom(const std::string& roomId, const std::string& userId);
    std::vector<std::string> getRoomList();

    // Message handling
    void broadcastMessage(const std::string& roomId, 
                         const std::string& userId,
                         const std::string& content);
    std::vector<Message> getRoomMessages(const std::string& roomId, 
                                       int limit = 50);

    // User management
    std::string generateUserId();
    std::string generateNickname();

    // Set message callback
    void setMessageCallback(std::function<void(const std::string&, 
                                             const std::string&)> callback);

private:
    // Cache management
    void initCaches();
    void cacheRoom(const Room& room);
    void cacheMessage(const std::string& roomId, const Message& message);

private:
    // LFU cache frequently accessed rooms
    std::unique_ptr<CacheImpl::LfuCache<std::string, Room>> roomCache_;
    
    // LRU cache recent messages
    std::unique_ptr<CacheImpl::LruCache<std::string, std::vector<Message>>> messageCache_;
    
    // Online user
    std::unordered_map<std::string, std::string> userRooms_;  // userId -> roomId
    
    // Message callback
    std::function<void(const std::string&, const std::string&)> messageCallback_{};
    
    std::mutex mutex_;
    
    static constexpr int MAX_ROOM_CACHE_SIZE = 1000;
    static constexpr int MAX_MESSAGE_CACHE_SIZE = 10000;
    static constexpr int MAX_MESSAGES_PER_ROOM = 1000;
};