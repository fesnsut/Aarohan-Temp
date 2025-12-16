cmake_minimum_required(VERSION 3.15)
project(APIServer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../third_party)

set(SOURCES
    src/api_server.cpp
)

add_executable(api_server src/main.cpp ${SOURCES})

target_link_libraries(api_server
    redis++
    hiredis
    pthread
)

install(TARGETS api_server DESTINATION bin)
