#include "chatRoom.h"

ChatRoom::ChatRoom() {
    roomCache_ = std::make_unique<CacheImpl::LfuCache<std::string, Room>>(1000);
    messageCache_ = std::make_unique<CacheImpl::LruCache<std::string, std::vector<Message>>>(10000);
}

std::string ChatRoom::createRoom(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    Room room;
    room.id = generateUserId();  // Reuse ID generation function
    room.name = name;
    roomCache_->put(room.id, room);
    return room.id;
}

bool ChatRoom::joinRoom(const std::string& roomId, const std::string& userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    Room room;
    if (roomCache_->get(roomId, room)) {
        room.users.insert(userId);
        userRooms_[userId] = roomId;
        roomCache_->put(roomId, room);
        return true;
    }
    return false;
}

void ChatRoom::broadcastMessage(const std::string& roomId, 
                              const std::string& userId,
                              const std::string& content) {
    if (messageCallback_) {
        messageCallback_(roomId, content);
    }
}