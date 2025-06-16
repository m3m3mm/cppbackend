cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake) 

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(game_lib STATIC
	src/model.h
	src/model.cpp
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/json_tags.h
	src/game_properties.h
	src/game_properties.cpp
	src/player_properties.h
	src/player_properties.cpp
	src/extra_data.h
	src/extra_data.cpp
	src/loot_generator.h
	src/loot_generator.cpp
)

target_include_directories(game_lib PUBLIC  CONAN_PKG::boost)
target_link_libraries(game_lib PUBLIC ${Boost_LIBRARIES} Threads::Threads CONAN_PKG::boost)

add_executable(game_server 
	src/main.cpp	
	src/sdk.h
	src/http_server.cpp
	src/http_server.h
	src/request_handler.cpp
	src/request_handler.h
	src/target_storage.h
	src/logger.h
	src/logger.cpp
	src/json_constructor.h
	src/json_constructor.cpp
	src/application.h
	src/application.cpp
	src/command_handler.h
	src/command_handler.cpp
	src/api_handler.h
	src/api_handler.cpp
)

add_executable(game_server_tests
	tests/model_tests.cpp
	tests/loot_generator_tests.cpp
	tests/main.cpp
)

target_link_libraries(game_server game_lib)
target_link_libraries(game_server_tests game_lib CONAN_PKG::catch2)
catch_discover_tests(game_server_tests) 