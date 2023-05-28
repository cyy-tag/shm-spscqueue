cmake_minimum_required(VERSION 3.6)
project(shmqueue)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++20")

set(SOURCE_FILES send_read.cpp
        ../shm_spsc.cpp)
add_executable(send_read ${SOURCE_FILES})
target_link_libraries(send_read rt)


set(STATIC_FILES static_rate.cpp
        ../shm_spsc.cpp)
add_executable(static_rate ${STATIC_FILES})
target_link_libraries(static_rate rt)
