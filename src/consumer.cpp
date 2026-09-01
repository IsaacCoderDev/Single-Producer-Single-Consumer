#include "shared_memory.hpp"
#include "ring_buffer.hpp"

#include <iostream>
#include <thread>
#include <csignal>
#include <atomic>
#include <chrono>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

std::atomic<bool> keep_running{true};

void signal_handler(int) {
    keep_running.store(false, std::memory_order_release);
}

int main() {

    pin_thread_to_core(3);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        constexpr size_t CAPACITY = 4096;
        size_t shm_size = sizeof(SpscRingBuffer<CAPACITY>);
        
        std::cout << "Starting Consumer. Attaching to " << shm_size << " bytes in shared memory..." << std::endl;
        
        // If the producer isn't running, this will throw a system_error.
        SharedMemory shm("/quant_ipc_buffer", shm_size, SharedMemory::Mode::Attach);

        auto* ring_buffer = reinterpret_cast<SpscRingBuffer<CAPACITY>*>(shm.data());

        std::cout << "Execution Engine active. Polling for ticks..." << std::endl;

        MarketTick tick;
        size_t ticks_processed = 0;
        auto start_time = std::chrono::steady_clock::now();

        while (keep_running.load(std::memory_order_acquire)) {
            
            // Attempt to pop. If empty, the producer hasn't written anything yet.
            if (ring_buffer->pop(tick)) {
                ticks_processed++;
                
                if (ticks_processed % 1000 == 0) {
                    
                    std::cout << "[TICK RECV] Time: " << tick.timestamp_ns 
                              << " | Price: " << tick.price 
                              << " | Qty: " << tick.quantity 
                              << std::endl;
                }
            } else {
                
#if defined(__x86_64__) || defined(_M_X64)
                _mm_pause();
#else
                asm volatile("yield" ::: "memory");
#endif
            }
        }

        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = end_time - start_time;

        std::cout << "\nConsumer shutting down. Processed " << ticks_processed 
                  << " ticks in " << elapsed.count() << " seconds." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error (Is the producer running?): " << e.what() << std::endl;
        
        return 1;
    }

    return 0;
}

bool pin_thread_to_core(int core_id) {
    
    cpu_set_t cpuset;
    
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    
    // pthread_setaffinity_np sets the CPU affinity mask
    int result = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
    
    if (result != 0) {
        std::cerr << "Error pinning thread to core " << core_id << std::endl;
        
        return false;
    }
    
    std::cout << "Successfully pinned thread to CPU Core " << core_id << std::endl;
    
    return true;
}