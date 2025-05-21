cmake_minimum_required(VERSION 3.15)
project(HelloLog CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost 1.78.0 REQUIRED)

add_executable(hello_log main.cpp)

set_target_properties(hello_log PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

target_link_libraries(hello_log PRIVATE 
    Boost::boost
    stdc++fs
)
