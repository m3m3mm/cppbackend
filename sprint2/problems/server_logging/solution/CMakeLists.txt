cmake_minimum_required(VERSION 3.10)
project(server_logging)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS log log_setup system filesystem)

add_executable(server_logging
    src/main.cpp
)

target_link_libraries(server_logging
    PRIVATE
    Boost::log
    Boost::log_setup
    Boost::system
    Boost::filesystem
)

target_include_directories(server_logging
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
) 