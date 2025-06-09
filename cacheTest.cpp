#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <iomanip>
#include <random>
#include <algorithm>
#include "lruCache.h"
#include "lfuCache.h"

using namespace CacheImpl;

// Timer class
class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsed() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
    
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// Print test results
void printResults(const std::string& cacheType, int totalOps, int hits, double time) {
    double hitRate = (totalOps > 0) ? (static_cast<double>(hits) / totalOps * 100) : 0;
    std::cout << cacheType << " - Hit Rate: " << std::fixed << std::setprecision(2) 
              << hitRate << "% (" << hits << "/" << totalOps << ")\n";
    std::cout << "Time: " << time << " ms\n";
}

// Test LRU cache with hot data access
void testLruHotData(LruCache<int, std::string>& cache) {
    const int CACHE_SIZE = 20;
    const int TOTAL_OPS = 200000;
    const int HOT_KEYS = 10;      // Number of hot keys
    const int COLD_KEYS = 2000;   // Number of cold keys
    int hits = 0;
    int totalGets = 0;
    
    Timer timer;
    
    // Generate random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> hotDist(0, HOT_KEYS - 1);
    std::uniform_int_distribution<> coldDist(HOT_KEYS, HOT_KEYS + COLD_KEYS - 1);
    std::uniform_real_distribution<> probDist(0, 1);
    
    // Execute test
    for (int i = 0; i < TOTAL_OPS; ++i) {
        // 70% get operations, 30% put operations
        if (probDist(gen) < 0.7) {
            totalGets++;
            int key;
            if (probDist(gen) < 0.8) { // 80% probability to access hot data
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string result;
            if (cache.get(key, result)) {
                hits++;
            }
        } else {
            int key;
            if (probDist(gen) < 0.8) { // 80% probability to update hot data
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
            cache.put(key, value);
        }
    }
    
    printResults("LRU", totalGets, hits, timer.elapsed());
}

// Test LRU-K cache with hot data access
void testLruKHotData(LruKCache<int, std::string>& cache) {
    const int CACHE_SIZE = 20;
    const int TOTAL_OPS = 200000;
    const int HOT_KEYS = 10;
    const int COLD_KEYS = 2000;
    int hits = 0;
    int totalGets = 0;
    
    Timer timer;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> hotDist(0, HOT_KEYS - 1);
    std::uniform_int_distribution<> coldDist(HOT_KEYS, HOT_KEYS + COLD_KEYS - 1);
    std::uniform_real_distribution<> probDist(0, 1);
    
    for (int i = 0; i < TOTAL_OPS; ++i) {
        if (probDist(gen) < 0.7) {
            totalGets++;
            int key;
            if (probDist(gen) < 0.8) {
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string result = cache.get(key);
            if (!result.empty()) {
                hits++;
            }
        } else {
            int key;
            if (probDist(gen) < 0.8) {
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
            cache.put(key, value);
        }
    }
    
    printResults("LRU-K", totalGets, hits, timer.elapsed());
}

// Test Hash-LRU cache with hot data access
void testHashLruHotData(HashLruCache<int, std::string>& cache) {
    const int CACHE_SIZE = 20;
    const int TOTAL_OPS = 200000;
    const int HOT_KEYS = 10;
    const int COLD_KEYS = 2000;
    int hits = 0;
    int totalGets = 0;
    
    Timer timer;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> hotDist(0, HOT_KEYS - 1);
    std::uniform_int_distribution<> coldDist(HOT_KEYS, HOT_KEYS + COLD_KEYS - 1);
    std::uniform_real_distribution<> probDist(0, 1);
    
    for (int i = 0; i < TOTAL_OPS; ++i) {
        if (probDist(gen) < 0.7) {
            totalGets++;
            int key;
            if (probDist(gen) < 0.8) {
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string result;
            if (cache.get(key, result)) {
                hits++;
            }
        } else {
            int key;
            if (probDist(gen) < 0.8) {
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
            cache.put(key, value);
        }
    }
    
    printResults("Hash-LRU", totalGets, hits, timer.elapsed());
}

// Test LFU cache with hot data access
void testLfuHotData(LfuCache<int, std::string>& cache) {
    const int CACHE_SIZE = 20;
    const int TOTAL_OPS = 200000;
    const int HOT_KEYS = 10;      // Number of hot keys
    const int COLD_KEYS = 2000;   // Number of cold keys
    int hits = 0;
    int totalGets = 0;
    
    Timer timer;
    
    // Generate random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> hotDist(0, HOT_KEYS - 1);
    std::uniform_int_distribution<> coldDist(HOT_KEYS, HOT_KEYS + COLD_KEYS - 1);
    std::uniform_real_distribution<> probDist(0, 1);
    
    // Execute test
    for (int i = 0; i < TOTAL_OPS; ++i) {
        // 70% get operations, 30% put operations
        if (probDist(gen) < 0.7) {
            totalGets++;
            int key;
            if (probDist(gen) < 0.8) { // 80% probability to access hot data
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string result;
            if (cache.get(key, result)) {
                hits++;
            }
        } else {
            int key;
            if (probDist(gen) < 0.8) { // 80% probability to update hot data
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
            cache.put(key, value);
        }
    }
    
    printResults("LFU", totalGets, hits, timer.elapsed());
}

// Test Hash-LFU cache with hot data access
void testHashLfuHotData(HashLfuCache<int, std::string>& cache) {
    const int CACHE_SIZE = 20;
    const int TOTAL_OPS = 200000;
    const int HOT_KEYS = 10;
    const int COLD_KEYS = 2000;
    int hits = 0;
    int totalGets = 0;
    
    Timer timer;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> hotDist(0, HOT_KEYS - 1);
    std::uniform_int_distribution<> coldDist(HOT_KEYS, HOT_KEYS + COLD_KEYS - 1);
    std::uniform_real_distribution<> probDist(0, 1);
    
    for (int i = 0; i < TOTAL_OPS; ++i) {
        if (probDist(gen) < 0.7) {
            totalGets++;
            int key;
            if (probDist(gen) < 0.8) {
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string result;
            if (cache.get(key, result)) {
                hits++;
            }
        } else {
            int key;
            if (probDist(gen) < 0.8) {
                key = hotDist(gen);
            } else {
                key = coldDist(gen);
            }
            
            std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
            cache.put(key, value);
        }
    }
    
    printResults("Hash-LFU", totalGets, hits, timer.elapsed());
}

// Test hot data access
void testHotDataAccess() {
    std::cout << "\n=== Test 1: Hot Data Access ===\n";
    
    LruCache<int, std::string> lru(20);
    LruKCache<int, std::string> lruk(20, 100, 2);
    HashLruCache<int, std::string> hashLru(20, 4);
    LfuCache<int, std::string> lfu(20);
    HashLfuCache<int, std::string> hashLfu(20, 4);
    
    testLruHotData(lru);
    testLruKHotData(lruk);
    testHashLruHotData(hashLru);
    testLfuHotData(lfu);
    testHashLfuHotData(hashLfu);
}

// Test loop pattern
void testLoopPattern() {
    std::cout << "\n=== Test 2: Loop Scan ===\n";
    
    const int CACHE_SIZE = 20;
    const int TOTAL_OPS = 200000;
    const int LOOP_SIZE = 200;  // Loop range
    
    LruCache<int, std::string> lru(CACHE_SIZE);
    LruKCache<int, std::string> lruk(CACHE_SIZE, 200, 2);
    HashLruCache<int, std::string> hashLru(CACHE_SIZE, 4);
    LfuCache<int, std::string> lfu(CACHE_SIZE);
    HashLfuCache<int, std::string> hashLfu(CACHE_SIZE, 4);
    
    // Test LRU
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        int current_pos = 0;
        for (int i = 0; i < TOTAL_OPS; ++i) {
            if (probDist(gen) < 0.7) {
                totalGets++;
                int key;
                if (probDist(gen) < 0.6) {  // 60% sequential scan
                    key = current_pos;
                    current_pos = (current_pos + 1) % LOOP_SIZE;
                } else if (probDist(gen) < 0.9) {  // 30% random jump
                    key = gen() % LOOP_SIZE;
                } else {  // 10% out of range access
                    key = LOOP_SIZE + (gen() % LOOP_SIZE);
                }
                
                std::string result;
                if (lru.get(key, result)) {
                    hits++;
                }
            } else {
                int key = gen() % (LOOP_SIZE * 2);
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                lru.put(key, value);
            }
        }
        printResults("LRU", totalGets, hits, timer.elapsed());
    }
    
    // Test LRU-K
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        int current_pos = 0;
        for (int i = 0; i < TOTAL_OPS; ++i) {
            if (probDist(gen) < 0.7) {
                totalGets++;
                int key;
                if (probDist(gen) < 0.6) {
                    key = current_pos;
                    current_pos = (current_pos + 1) % LOOP_SIZE;
                } else if (probDist(gen) < 0.9) {
                    key = gen() % LOOP_SIZE;
                } else {
                    key = LOOP_SIZE + (gen() % LOOP_SIZE);
                }
                
                std::string result = lruk.get(key);
                if (!result.empty()) {
                    hits++;
                }
            } else {
                int key = gen() % (LOOP_SIZE * 2);
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                lruk.put(key, value);
            }
        }
        printResults("LRU-K", totalGets, hits, timer.elapsed());
    }
    
    // Test Hash-LRU
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        int current_pos = 0;
        for (int i = 0; i < TOTAL_OPS; ++i) {
            if (probDist(gen) < 0.7) {
                totalGets++;
                int key;
                if (probDist(gen) < 0.6) {
                    key = current_pos;
                    current_pos = (current_pos + 1) % LOOP_SIZE;
                } else if (probDist(gen) < 0.9) {
                    key = gen() % LOOP_SIZE;
                } else {
                    key = LOOP_SIZE + (gen() % LOOP_SIZE);
                }
                
                std::string result;
                if (hashLru.get(key, result)) {
                    hits++;
                }
            } else {
                int key = gen() % (LOOP_SIZE * 2);
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                hashLru.put(key, value);
            }
        }
        printResults("Hash-LRU", totalGets, hits, timer.elapsed());
    }

    // Test LFU
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        int current_pos = 0;
        for (int i = 0; i < TOTAL_OPS; ++i) {
            if (probDist(gen) < 0.7) {
                totalGets++;
                int key;
                if (probDist(gen) < 0.6) {
                    key = current_pos;
                    current_pos = (current_pos + 1) % LOOP_SIZE;
                } else if (probDist(gen) < 0.9) {
                    key = gen() % LOOP_SIZE;
                } else {
                    key = LOOP_SIZE + (gen() % LOOP_SIZE);
                }
                
                std::string result;
                if (lfu.get(key, result)) {
                    hits++;
                }
            } else {
                int key = gen() % (LOOP_SIZE * 2);
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                lfu.put(key, value);
            }
        }
        printResults("LFU", totalGets, hits, timer.elapsed());
    }

    // Test Hash-LFU
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        int current_pos = 0;
        for (int i = 0; i < TOTAL_OPS; ++i) {
            if (probDist(gen) < 0.7) {
                totalGets++;
                int key;
                if (probDist(gen) < 0.6) {
                    key = current_pos;
                    current_pos = (current_pos + 1) % LOOP_SIZE;
                } else if (probDist(gen) < 0.9) {
                    key = gen() % LOOP_SIZE;
                } else {
                    key = LOOP_SIZE + (gen() % LOOP_SIZE);
                }
                
                std::string result;
                if (hashLfu.get(key, result)) {
                    hits++;
                }
            } else {
                int key = gen() % (LOOP_SIZE * 2);
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                hashLfu.put(key, value);
            }
        }
        printResults("Hash-LFU", totalGets, hits, timer.elapsed());
    }
}

// Test workload shift
void testWorkloadShift() {
    std::cout << "\n=== Test 3: Workload Heavily Change ===\n";
    
    const int CACHE_SIZE = 20;
    const int TOTAL_OPS = 100000;
    const int PHASE_LENGTH = TOTAL_OPS / 3;
    
    LruCache<int, std::string> lru(CACHE_SIZE);
    LruKCache<int, std::string> lruk(CACHE_SIZE, 100, 2);
    HashLruCache<int, std::string> hashLru(CACHE_SIZE, 4);
    LfuCache<int, std::string> lfu(CACHE_SIZE);
    HashLfuCache<int, std::string> hashLfu(CACHE_SIZE, 4);
    
    // Test LRU
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        for (int i = 0; i < TOTAL_OPS; ++i) {
            int phase = i / PHASE_LENGTH;
            int key;
            
            if (phase == 0) {
                key = gen() % CACHE_SIZE;  // Phase 1: Access data within cache range
            } else if (phase == 1) {
                key = CACHE_SIZE + (gen() % (CACHE_SIZE * 10));  // Phase 2: Access data outside cache range
            } else {
                key = gen() % CACHE_SIZE;  // Phase 3: Access data within cache range again
            }
            
            if (probDist(gen) < 0.7) {
                totalGets++;
                std::string result;
                if (lru.get(key, result)) {
                    hits++;
                }
            } else {
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                lru.put(key, value);
            }
        }
        printResults("LRU", totalGets, hits, timer.elapsed());
    }
    
    // Test LRU-K
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        for (int i = 0; i < TOTAL_OPS; ++i) {
            int phase = i / PHASE_LENGTH;
            int key;
            
            if (phase == 0) {
                key = gen() % CACHE_SIZE;
            } else if (phase == 1) {
                key = CACHE_SIZE + (gen() % (CACHE_SIZE * 10));
            } else {
                key = gen() % CACHE_SIZE;
            }
            
            if (probDist(gen) < 0.7) {
                totalGets++;
                std::string result = lruk.get(key);
                if (!result.empty()) {
                    hits++;
                }
            } else {
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                lruk.put(key, value);
            }
        }
        printResults("LRU-K", totalGets, hits, timer.elapsed());
    }
    
    // Test Hash-LRU
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        for (int i = 0; i < TOTAL_OPS; ++i) {
            int phase = i / PHASE_LENGTH;
            int key;
            
            if (phase == 0) {
                key = gen() % CACHE_SIZE;
            } else if (phase == 1) {
                key = CACHE_SIZE + (gen() % (CACHE_SIZE * 10));
            } else {
                key = gen() % CACHE_SIZE;
            }
            
            if (probDist(gen) < 0.7) {
                totalGets++;
                std::string result;
                if (hashLru.get(key, result)) {
                    hits++;
                }
            } else {
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                hashLru.put(key, value);
            }
        }
        printResults("Hash-LRU", totalGets, hits, timer.elapsed());
    }

    // Test LFU
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        for (int i = 0; i < TOTAL_OPS; ++i) {
            int phase = i / PHASE_LENGTH;
            int key;
            
            if (phase == 0) {
                key = gen() % CACHE_SIZE;
            } else if (phase == 1) {
                key = CACHE_SIZE + (gen() % (CACHE_SIZE * 10));
            } else {
                key = gen() % CACHE_SIZE;
            }
            
            if (probDist(gen) < 0.7) {
                totalGets++;
                std::string result;
                if (lfu.get(key, result)) {
                    hits++;
                }
            } else {
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                lfu.put(key, value);
            }
        }
        printResults("LFU", totalGets, hits, timer.elapsed());
    }

    // Test Hash-LFU
    {
        int hits = 0;
        int totalGets = 0;
        Timer timer;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> probDist(0, 1);
        
        for (int i = 0; i < TOTAL_OPS; ++i) {
            int phase = i / PHASE_LENGTH;
            int key;
            
            if (phase == 0) {
                key = gen() % CACHE_SIZE;
            } else if (phase == 1) {
                key = CACHE_SIZE + (gen() % (CACHE_SIZE * 10));
            } else {
                key = gen() % CACHE_SIZE;
            }
            
            if (probDist(gen) < 0.7) {
                totalGets++;
                std::string result;
                if (hashLfu.get(key, result)) {
                    hits++;
                }
            } else {
                std::string value = "value_" + std::to_string(key) + "_" + std::to_string(i);
                hashLfu.put(key, value);
            }
        }
        printResults("Hash-LFU", totalGets, hits, timer.elapsed());
    }
}

int main() {
    testHotDataAccess();
    testLoopPattern();
    testWorkloadShift();
    return 0;
}