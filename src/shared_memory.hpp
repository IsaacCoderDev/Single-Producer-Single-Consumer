#pragma once

#include <cstddef>
#include <string>
#include <stdexcept>

class SharedMemory {
    
public:

    enum class Mode { Create, Attach };
    
    SharedMemory(const std::string& name, size_t size, Mode mode);
    
    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;

    SharedMemory(SharedMemory&& other) noexcept;
    SharedMemory& operator=(SharedMemory&& other) noexcept;

    ~SharedMemory();

    void* data() const noexcept { return mapped_addr_; }
    size_t size() const noexcept { return size_; }

private:

    void cleanup() noexcept;

    std::string name_;
    size_t size_;
    Mode mode_;
    int fd_;
    void* mapped_addr_;
};