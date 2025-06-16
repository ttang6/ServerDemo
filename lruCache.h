#pragma once

#include <cstring>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <cmath>

#include "cachePolicy.h"

namespace CacheImpl
{
    template <typename Key, typename Value> class LruCache;
    template <typename Key, typename Value> class TraversableLruCache;
    
    template <typename Key, typename Value>
    class LruNode
    {
        protected:
            std::shared_ptr<LruNode<Key, Value>> next_;

        private:
            Key key_;
            Value value_;
            size_t accessCount_;
            std::weak_ptr<LruNode<Key, Value>> prev_;

        public:
        LruNode(Key key, Value value)
            : key_(key)
            , value_(value)
            , accessCount_(1)
        {}

        Key getKey() const { return key_; }
        Value getValue() const { return value_; }
        void setValue(const Value& value) { value_ = value; }
        size_t getAccessCount() const { return accessCount_; }
        void incrementAccessCount() { ++accessCount_; }

        friend class LruCache<Key, Value>;
        friend class TraversableLruCache<Key, Value>;
    };

    template <typename Key, typename Value>
    class LruCache : public CachePolicy<Key, Value>
    {
        public:
            using LruNodeType = LruNode<Key, Value>;
            using NodePtr = std::shared_ptr<LruNodeType>;
            using NodeMap = std::unordered_map<Key, NodePtr>;

            // 声明 TraversableLruCache 为友元类
            friend class TraversableLruCache<Key, Value>;

            LruCache(int capacity)
                : capacity_(capacity)
            {
                initializedList();
            }

            ~LruCache() override = default;

            void put(Key key, Value value) override
            {
                if (capacity_ <= 0)
                    return ;
                
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = nodeMap_.find(key);
                if (it != nodeMap_.end())
                {
                    updateNode(it->second, value);
                    return ;
                }

                addNode(key, value);
            }

            bool get(Key key, Value& value) override
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = nodeMap_.find(key);
                if (it != nodeMap_.end())
                {
                    moveToFront(it->second);
                    value = it->second->getValue();
                    return true;
                }
                return false;
            }

            Value get(Key key) override
            {
                Value value{};
                get(key, value);
                return value;
            }

            void remove(Key key)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = nodeMap_.find(key);
                if (it != nodeMap_.end())
                {
                    removeNode(it->second);
                    nodeMap_.erase(it);
                }
            }

            void purge()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                nodeMap_.clear();
                initializedList();
            }

        protected:
            void initializedList()
            {
                dummyHead_ = std::make_shared<LruNodeType>(Key(), Value());
                dummyTail_ = std::make_shared<LruNodeType>(Key(), Value());
                dummyHead_->next_ = dummyTail_;
                dummyTail_->prev_ = dummyHead_;
            }

            void updateNode(NodePtr node, const Value& value)
            {
                node->setValue(value);
                moveToFront(node);
            }

            void addNode(const Key& key, const Value& value)
            {
                if (nodeMap_.size() >= static_cast<size_t>(capacity_))
                {
                    removeLeastUsed();
                }

                NodePtr newNode = std::make_shared<LruNodeType>(key, value);
                insertNode(newNode);
                nodeMap_[key] = newNode;
            }

            void moveToFront(NodePtr node)
            {
                removeNode(node);
                insertNode(node);
            }

            void removeNode(NodePtr node)
            {
                if (!node->prev_.expired() && node->next_)
                {
                    auto prev = node->prev_.lock();
                    if (prev)
                    {
                        prev->next_ = node->next_;
                        node->next_->prev_ = prev;
                        node->next_ = nullptr;
                        node->prev_.reset();
                    }
                }
            }

            void insertNode(NodePtr node)
            {
                if (!dummyTail_->prev_.expired())
                {
                    auto prev = dummyTail_->prev_.lock();
                    if (prev)
                    {
                        node->next_ = dummyTail_;
                        node->prev_ = prev;
                        prev->next_ = node;
                        dummyTail_->prev_ = node;
                    }
                }
            }

            void removeLeastUsed()
            {
                if (dummyHead_->next_ != dummyTail_)
                {
                    NodePtr leastUsed = dummyHead_->next_;
                    removeNode(leastUsed);
                    nodeMap_.erase(leastUsed->getKey());
                }
            }

            int capacity_;
            NodeMap nodeMap_;
            std::mutex mutex_;
            NodePtr dummyHead_;
            NodePtr dummyTail_;
    };

    template <typename Key, typename Value>
    class LruKCache : public LruCache<Key, Value>
    {
        public:
            LruKCache(int capacity, int historyCapacity, int k)
                : LruCache<Key, Value>(capacity)
                , historyList_(std::make_unique<LruCache<Key, size_t>>(historyCapacity))
                , k_(k)
            {}

            Value get(Key key)
            {
                Value value{};
                bool inMainCache = LruCache<Key, Value>::get(key, value);

                size_t historyCount = historyList_->get(key);
                historyCount++;
                historyList_->put(key, historyCount);

                if (inMainCache)
                {
                    return value;
                }

                if (historyCount >= k_)
                {
                    auto it = historyValueMap_.find(key);
                    if (it != historyValueMap_.end())
                    {
                        Value cacheValue = it->second;

                        historyList_->remove(key);
                        historyValueMap_.erase(it);

                        LruCache<Key, Value>::put(key, cacheValue);

                        return cacheValue;
                    }
                }

                return value;
            }

            void put(Key key, Value value)
            {
                Value existingValue{};
                bool inMainCache = LruCache<Key, Value>::get(key, existingValue);

                if (inMainCache)
                {
                    LruCache<Key, Value>::put(key, value);
                    return ;
                }

                size_t historyCount = historyList_->get(key);
                historyCount++;
                historyList_->put(key, historyCount);

                historyValueMap_[key] = value;

                if (historyCount >= k_)
                {
                    historyList_->remove(key);
                    historyValueMap_.erase(key);
                    LruCache<Key, Value>::put(key, value);
                }
            }
        
        private:
            int k_;
            std::unique_ptr<LruCache<Key, size_t>> historyList_;
            std::unordered_map<Key, Value> historyValueMap_;
    };

    template <typename Key, typename Value>
    class HashLruCache
    {
        public:
            HashLruCache(size_t capacity, int sliceNum)
                : capacity_(capacity)
                , sliceNum_(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency())
            {
                size_t sliceSize = std::ceil(capacity_ / static_cast<double>(sliceNum_));
                for (int i = 0; i < sliceNum_; ++i)
                {
                    lruSliceCaches_.emplace_back(new LruCache<Key, Value>(sliceSize));
                }
            }

            void put(Key key, Value value)
            {
                size_t sliceIndex = Hash(key) % sliceNum_;
                lruSliceCaches_[sliceIndex]->put(key, value);
            }

            bool get(Key key, Value& value)
            {
                size_t sliceIndex = Hash(key) % sliceNum_;
                return lruSliceCaches_[sliceIndex]->get(key, value);
            }

            Value get(Key key)
            {
                Value value{};
                get(key, value);
                return value;
            }

            void remove(Key key)
            {
                size_t sliceIndex = Hash(key) % sliceNum_;
                lruSliceCaches_[sliceIndex]->remove(key);
            }

            void purge()
            {
                for (auto& cache : lruSliceCaches_)
                {
                    cache->purge();
                }
            }
        
        private:
            size_t Hash(Key key)
            {
                std::hash<Key> hashFunc;
                return hashFunc(key);
            }
        
        private:
            size_t capacity_;
            int sliceNum_;
            std::vector<std::unique_ptr<LruCache<Key, Value>>> lruSliceCaches_;
    };
}

