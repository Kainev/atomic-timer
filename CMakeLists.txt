cmake_minimum_required(VERSION 3.10)
project(atomic_timer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(ADDITIONAL_LIBS ws2_32)
else()
    set(ADDITIONAL_LIBS pthread)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/source)

add_executable(atomic_timer
    source/NTPClient.cpp
    source/AtomicTimer.cpp
    ../tests/main.cpp
)