#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>

struct MarketTick {
    uint64_t timestamp_ns;
    double price;
    double quantity;
    uint32_t instrument_id;
    uint32_t flags;
};

constexpr size_t CACHE_LINE_SIZE = 64;

template <size_t Capacity>
struct SpscRingBuffer {
    
    static_assert((Capacity != 0) && ((Capacity & (Capacity - 1)) == 0),
                  "Capacity must be a power of 2");

    // ========================================================================
    // PRODUCER CACHE LINE
    // ========================================================================
    // Aligning the head forces it to start at the beginning of a new cache line.
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head{0};
    
    // Padding to ensure nothing else leaks into the producer's 64-byte territory
    char padding1[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

    // ========================================================================
    // CONSUMER CACHE LINE
    // ========================================================================
    // Aligning the tail forces it onto the next isolated cache line.
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail{0};
    
    // Padding to separate the tail from the raw data array
    char padding2[CACHE_LINE_SIZE - sizeof(std::atomic<size_t>)];

    // ========================================================================
    // DATA PAYLOAD
    // ========================================================================
    // The contiguous block of memory holding the actual ticks
    alignas(CACHE_LINE_SIZE) MarketTick data[Capacity];

    // ========================================================================
    // PRODUCER: Write to the buffer
    // ========================================================================
    bool push(const MarketTick& item) {
        
        size_t current_head = head.load(std::memory_order_relaxed);
        
        // ACQUIRE: Read the consumer's tail. 
        size_t current_tail = tail.load(std::memory_order_acquire);

        if (current_head - current_tail >= Capacity) {
            return false;
        }

        data[current_head & (Capacity - 1)] = item;

        // RELEASE: Publish the new head pointer.
        head.store(current_head + 1, std::memory_order_release);
        
        return true;
    }

    // ========================================================================
    // CONSUMER: Read from the buffer
    // ========================================================================
    bool pop(MarketTick& out_item) {
        
        size_t current_tail = tail.load(std::memory_order_relaxed);

        // ACQUIRE: Read the producer's head.
        size_t current_head = head.load(std::memory_order_acquire);

        if (current_head == current_tail) {
            return false; // Buffer is empty
        }

        out_item = data[current_tail & (Capacity - 1)];

        // RELEASE: Publish the new tail pointer.
        tail.store(current_tail + 1, std::memory_order_release);
        
        return true;
    }
};