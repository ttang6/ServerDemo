#pragma once

#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <thread>
#include <cmath>

#include "cachePolicy.h"

namespace CacheImpl
{
    template <typename Key, typename Value> class LfuCache;

    template <typename Key, typename Value>
    class FreqList
    {
        private:
            struct Node
            {
                int freq;
                Key key;
                Value value;
                std::weak_ptr<Node> prev;
                std::shared_ptr<Node> next;

                Node()
                    : freq(1)
                    , next(nullptr)
                {}
            };

            using NodePtr = std::shared_ptr<Node>;
            int freq_;
            NodePtr head_;
            NodePtr tail_;
        
        public:
            explicit FreqList(int n)
                : freq_(n)
            {
                head_ = std::make_shared<Node>();
                tail_ = std::make_shared<Node>();
                head_->next = tail_;
                tail_->prev = head_;
            }

            bool isEmpty() const
            {
                return head_->next == tail_;
            }

            void addNode(NodePtr node)
            {
                if (!node || !head_ || !tail_)
                    return ;
                
                node->prev = tail_->prev;
                node->next = tail_;
                tail_->prev.lock()->next = node;
                tail_->prev = node;
            }

            void removeNode(NodePtr node)
            {
                if (!node || !head_ || !tail_)
                    return ;
                if (node->prev.expired() || !node->next)
                    return ;
                
                auto prev = node->prev.lock();
                prev->next = node->next;
                node->next->prev = prev;
                node->next = nullptr;
            }

            NodePtr getFirstNode() const {return head_->next;}
            friend class LfuCache<Key, Value>;
    };

    template <typename Key, typename Value>
    class LfuCache : public CachePolicy<Key, Value>
    {
        public:
            using Node = typename FreqList<Key, Value>::Node;
            using NodePtr = std::shared_ptr<Node>;
            using NodeMap = std::unordered_map<Key, NodePtr>;

            LfuCache(int capacity, int maxAverageNum = 1000000)
                : capacity_(capacity)
                , minFreq_(1)
                , maxAverageNum_(maxAverageNum)
                , curAverageNum_(0)
                , curTotalNum_(0)
            {
                // 预分配最小频率的 FreqList
                freqMap_[1] = std::make_unique<FreqList<Key, Value>>(1);
            }

            ~LfuCache() override = default;

            void put(Key key, Value value) override
            {
                if (capacity_ == 0)
                    return ;
                
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = nodeMap_.find(key);
                if (it != nodeMap_.end())
                {
                    it->second->value = value;
                    getKV(it->second, value);
                    return ;
                }

                addKV(key, value);
            }

            bool get(Key key, Value& value) override
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = nodeMap_.find(key);
                if (it != nodeMap_.end())
                {
                    getKV(it->second, value);
                    return true;
                }
                return false;
            }

            Value get(Key key) override
            {
                Value value;
                get(key, value);
                return value;
            }

            void purge()
            {
                nodeMap_.clear();
                freqMap_.clear();
                // 重新创建最小频率的 FreqList
                freqMap_[1] = std::make_unique<FreqList<Key, Value>>(1);
                minFreq_ = 1;
                curAverageNum_ = 0;
                curTotalNum_ = 0;
            }

        private:
            void getKV(NodePtr node, Value& value)
            {
                value = node->value;
                removeFromFreqList(node);
                node->freq++;
                addToFreqList(node);

                if (node->freq - 1 == minFreq_ && freqMap_[node->freq - 1]->isEmpty())
                {
                    updateMinFreq();
                }

                increaseFreqNum();
            }

            void addKV(Key key, Value value)
            {
                if (nodeMap_.size() >= capacity_)
                    kickOut();
                
                NodePtr node = std::make_shared<Node>();
                node->key = key;
                node->value = value;
                nodeMap_[key] = node;
                addToFreqList(node);
                increaseFreqNum();
            }

            void kickOut()
            {
                if (freqMap_.empty() || !freqMap_[minFreq_])
                    return;

                NodePtr node = freqMap_[minFreq_]->getFirstNode();
                if (!node)
                    return;

                removeFromFreqList(node);
                nodeMap_.erase(node->key);
                decreaseFreqNum(node->freq);
            }

            void removeFromFreqList(NodePtr node)
            {
                if (!node)
                    return ;

                auto freq = node->freq;
                if (freqMap_.find(freq) != freqMap_.end() && freqMap_[freq])
                {
                    freqMap_[freq]->removeNode(node);
                }
            }

            void addToFreqList(NodePtr node)
            {
                if (!node)
                    return ;

                auto freq = node->freq;
                if (freqMap_.find(freq) == freqMap_.end())
                {
                    freqMap_[freq] = std::make_unique<FreqList<Key, Value>>(freq);
                }

                freqMap_[freq]->addNode(node);
            }

            void increaseFreqNum()
            {
                curTotalNum_++;
                if (nodeMap_.empty())
                    curAverageNum_ = 0;
                else
                    curAverageNum_ = curTotalNum_ / nodeMap_.size();
                
                if (curAverageNum_ > maxAverageNum_)
                {
                    handleOverMaxAverageNum();
                }
            }

            void decreaseFreqNum(int num)
            {
                curTotalNum_ -= num;
                if (nodeMap_.empty())
                    curAverageNum_ = 0;
                else
                    curAverageNum_ = curTotalNum_ / nodeMap_.size();
            }

            void handleOverMaxAverageNum()
            {
                if (nodeMap_.empty())
                    return ;
                
                for (auto it = nodeMap_.begin(); it != nodeMap_.end(); ++it)
                {
                    if (!it->second)
                        continue;
                    
                    NodePtr node = it->second;
                    removeFromFreqList(node);
                    node->freq = std::max(1, node->freq - maxAverageNum_ / 2);
                    addToFreqList(node);
                }
                updateMinFreq();
            }

            void updateMinFreq()
            {
                minFreq_ = INT8_MAX;
                for (const auto& pair : freqMap_)
                {
                    if (pair.second && !pair.second->isEmpty())
                    {
                        minFreq_ = std::min(minFreq_, pair.first);
                    }
                }
                if (minFreq_ == INT8_MAX)
                    minFreq_ = 1;
            }

        private:
            int capacity_;
            int minFreq_;
            int maxAverageNum_;
            int curAverageNum_;
            int curTotalNum_;
            std::mutex mutex_;
            NodeMap nodeMap_;
            std::unordered_map<int, std::unique_ptr<FreqList<Key, Value>>> freqMap_;
    };

    template <typename Key, typename Value>
    class HashLfuCache
    {
        public:
            HashLfuCache(size_t capacity, int sliceNum, int maxAverageNum = 10)
                : capacity_(capacity)
                , sliceNum_(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency())
            {
                size_t sliceSize = std::ceil(capacity_ / static_cast<double>(sliceNum_));
                for (int i = 0; i < sliceNum_; ++i)
                {
                    lfuSliceCaches_.emplace_back(new LfuCache<Key, Value>(sliceSize, maxAverageNum));
                }
            }

            void put(Key key, Value value)
            {
                size_t sliceIndex = Hash(key) % sliceNum_;
                lfuSliceCaches_[sliceIndex]->put(key, value);
            }

            bool get(Key key, Value& value)
            {
                size_t sliceIndex = Hash(key) % sliceNum_;
                return lfuSliceCaches_[sliceIndex]->get(key, value);
            }

            Value get(Key key)
            {
                Value value{};
                get(key, value);
                return value;
            }

            void purge()
            {
                for (auto& cache : lfuSliceCaches_)
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
            std::vector<std::unique_ptr<LfuCache<Key, Value>>> lfuSliceCaches_;
    };
}