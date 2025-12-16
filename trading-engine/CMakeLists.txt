cmake_minimum_required(VERSION 3.15)
project(TradingEngine VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX /O2)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3 -march=native)
endif()

# Find required packages
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party)

# Source files
set(SOURCES
    src/orderbook.cpp
    src/balance_service.cpp
    src/order_service.cpp
    src/matching_engine.cpp
    src/market_data_service.cpp
    src/redis_client.cpp
    src/snapshot_service.cpp
    src/error_service.cpp
    src/trading_engine.cpp
)

# Create library
add_library(trading_engine_lib STATIC ${SOURCES})

# Link libraries
target_link_libraries(trading_engine_lib
    Threads::Threads
    redis++
    hiredis
)

# Create executable
add_executable(trading_engine src/main.cpp)
target_link_libraries(trading_engine trading_engine_lib)

# Installation
install(TARGETS trading_engine DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/trading_engine)

# Tests (optional)
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
