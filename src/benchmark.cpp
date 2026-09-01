#include <benchmark/benchmark.h>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include "ring_buffer.hpp"

// ========================================================================
// THE BASELINE: Naive Mutex-Locked Queue
// ========================================================================
class MutexQueue {
public:
    void push(const MarketTick& item) {
        std::lock_guard<std::mutex> lock(mtx_);
        
        q_.push(item);
    }
    bool pop(MarketTick& out_item) {
        std::lock_guard<std::mutex> lock(mtx_);
        
        if (q_.empty()) return false;
        
        out_item = q_.front();
        q_.pop();
        
        return true;
    }
private:
    std::mutex mtx_;
    std::queue<MarketTick> q_;
};

// ========================================================================
// BENCHMARK 1: std::mutex Contention
// ========================================================================
static void BM_MutexQueue(benchmark::State& state) {
    MutexQueue queue;
    std::atomic<bool> running{true};
    
    std::thread producer([&]() {
        MarketTick tick{0, 50000.0, 1.0, 1, 0};
        
        while (running.load(std::memory_order_relaxed)) {
            queue.push(tick);
        }
    });

    MarketTick out_tick;
    
    for (auto _ : state) {
        while (!queue.pop(out_tick)) {
        
        }
    }

    running.store(false, std::memory_order_relaxed);
    producer.join();
}

BENCHMARK(BM_MutexQueue)->UseRealTime();

// ========================================================================
// BENCHMARK 2: Lock-Free std::atomic Memory Orderings
// ========================================================================
static void BM_LockFreeQueue(benchmark::State& state) {
    SpscRingBuffer<4096> queue;
    std::atomic<bool> running{true};
    
    std::thread producer([&]() {
        MarketTick tick{0, 50000.0, 1.0, 1, 0};
        
        while (running.load(std::memory_order_relaxed)) {
            
            while (!queue.push(tick) && running.load(std::memory_order_relaxed)) {}
        }
    });

    MarketTick out_tick;
    
    for (auto _ : state) {
        
        while (!queue.pop(out_tick)) {}
    }

    running.store(false, std::memory_order_relaxed);
    producer.join();
}

BENCHMARK(BM_LockFreeQueue)->UseRealTime();

BENCHMARK_MAIN();