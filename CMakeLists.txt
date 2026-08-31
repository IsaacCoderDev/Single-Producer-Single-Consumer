cmake_minimum_required(VERSION 3.20)
project(LockFreeIPC VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -O3 -march=native")

add_library(shm_core src/shared_memory.cpp)

target_link_libraries(shm_core rt pthread)

add_executable(producer src/producer.cpp)
target_link_libraries(producer shm_core)

add_executable(consumer src/consumer.cpp)
target_link_libraries(consumer shm_core)