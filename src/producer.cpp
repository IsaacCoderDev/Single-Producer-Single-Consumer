#include "shared_memory.hpp"
#include "ring_buffer.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

std::atomic<bool> keep_running{true};

void signal_handler(int) {
    keep_running.store(false, std::memory_order_release);
}

int main() {

    pin_thread_to_core(2);
    
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        
        constexpr size_t CAPACITY = 4096;
        size_t shm_size = sizeof(SpscRingBuffer<CAPACITY>);
        
        std::cout << "Starting Producer. Allocating " << shm_size << " bytes in shared memory..." << std::endl;
        
        SharedMemory shm("/quant_ipc_buffer", shm_size, SharedMemory::Mode::Create);

        // This maps our C++ struct directly onto the raw void* pointer provided by the OS.
        // It initializes the atomic indices to 0 inside the shared memory block.
        auto* ring_buffer = new (shm.data()) SpscRingBuffer<CAPACITY>();

        std::cout << "Feed Handler active. Pumping market data ticks..." << std::endl;

        uint64_t mock_timestamp = 1000000;
        uint32_t simulated_price = 50000;

        while (keep_running.load(std::memory_order_acquire)) {
            
            MarketTick tick{
                mock_timestamp++,
                static_cast<double>(simulated_price),
                1.5,
                1,
                0
            };

            // Tries to push. If the buffer is full, spin.
            while (!ring_buffer->push(tick)) {
                
                if (!keep_running.load(std::memory_order_relaxed)) {
                    break;
                }

                // _mm_pause() instructs the CPU that this is a spin-loop. 
                // It prevents memory-order violations and reduces power consumption 
                // without yielding the thread to the Linux kernel scheduler.
#if defined(__x86_64__) || defined(_M_X64)
                _mm_pause();
#else
                // Fallback for ARM architectures (e.g., Apple Silicon)
                asm volatile("yield" ::: "memory");
#endif
            }

            simulated_price += (mock_timestamp % 3 == 0) ? 1 : -1;

            // Sleep briefly just to prevent this mock loop from spamming stdout too fast.
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        std::cout << "\nProducer shutting down gracefully." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        
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