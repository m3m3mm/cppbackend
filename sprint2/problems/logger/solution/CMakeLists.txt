cmake_minimum_required(VERSION 3.9)
project(Logger CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(hello_log main.cpp)

target_include_directories(hello_log PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(hello_log ${Boost_LIBRARIES})
