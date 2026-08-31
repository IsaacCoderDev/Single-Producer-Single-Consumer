#include "shared_memory.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <system_error>

SharedMemory::SharedMemory(const std::string& name, size_t size, Mode mode)
    : name_(name), size_(size), mode_(mode), fd_(-1), mapped_addr_(MAP_FAILED) 
{
    int oflag = O_RDWR;
    
    if (mode == Mode::Create) {
        oflag |= O_CREAT | O_EXCL;
    }

    fd_ = shm_open(name_.c_str(), oflag, 0666);
    
    if (fd_ == -1) {
        throw std::system_error(errno, std::generic_category(), "shm_open failed");
    }

    if (mode == Mode::Create) {
        if (ftruncate(fd_, size_) == -1) {
            close(fd_);
            shm_unlink(name_.c_str());
            throw std::system_error(errno, std::generic_category(), "ftruncate failed");
        }
    }

    mapped_addr_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    
    if (mapped_addr_ == MAP_FAILED) {
        close(fd_);
        
        if (mode == Mode::Create) {
            shm_unlink(name_.c_str());
        }
        
        throw std::system_error(errno, std::generic_category(), "mmap failed");
    }
}

SharedMemory::~SharedMemory() {
    cleanup();
}

void SharedMemory::cleanup() noexcept {
    if (mapped_addr_ != MAP_FAILED) {
        munmap(mapped_addr_, size_);
        mapped_addr_ = MAP_FAILED;
    }
    
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }

    if (mode_ == Mode::Create) {
        shm_unlink(name_.c_str());
    }
}