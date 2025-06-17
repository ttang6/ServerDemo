cmake_minimum_required(VERSION 3.10)
project(DeepSeekChat)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

# Add Crow
include(FetchContent)
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.0+5
)
FetchContent_MakeAvailable(crow)

# Set muduo paths
set(MUDUO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../muduo)
set(MUDUO_INCLUDE_DIR ${MUDUO_ROOT_DIR})
set(MUDUO_LIBRARY_DIR ${MUDUO_ROOT_DIR}/build/release/lib)

# Add muduo include directories
include_directories(${MUDUO_INCLUDE_DIR})

# Add muduo library directories
link_directories(${MUDUO_LIBRARY_DIR})

# Add cpp-httplib
include(FetchContent)
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.1
)
FetchContent_MakeAvailable(httplib)

# Add executables
add_executable(ds_chat
    ds_main.cpp
    deepseek.cpp
)

add_executable(http_server
    http_server.cpp
    deepseek.cpp
)

add_executable(proxy_server
    proxy_server.cpp
)

# Link libraries
target_link_libraries(ds_chat
    PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
    Crow::Crow
)

target_link_libraries(http_server
    PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
    httplib::httplib
)

target_link_libraries(proxy_server
    PRIVATE
    nlohmann_json::nlohmann_json
    httplib::httplib
)

# Include directories
target_include_directories(ds_chat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(http_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(proxy_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 复制静态文件到构建目录
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/static DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/..)