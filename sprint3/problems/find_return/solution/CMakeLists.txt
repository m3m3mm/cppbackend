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
	src/app/application.h
	src/app/application.cpp
	src/app/player_properties.h
	src/app/player_properties.cpp
	src/model/detail/collision_detector.h
	src/model/detail/collision_detector.cpp
	src/model/detail/geom.h
	src/model/detail/loot_generator.h
	src/model/detail/loot_generator.cpp
	src/model/dynamic_object_properties.h
	src/model/dynamic_object_properties.cpp
	src/model/game_properties.h
	src/model/game_properties.cpp
	src/model/static_object_prorerties.h
	src/model/static_object_properties.cpp
	src/json/json_constructor.h
	src/json/json_constructor.cpp
	src/json/json_loader.h
	src/json/json_loader.cpp
	src/json/json_tags.h
	src/server/extra_data.h
	src/server/extra_data.cpp
	src/boost_json.cpp
	src/tagged.h
)

target_include_directories(game_lib PUBLIC  CONAN_PKG::boost)
target_link_libraries(game_lib PUBLIC ${Boost_LIBRARIES} Threads::Threads CONAN_PKG::boost)

add_executable(game_server
	src/handlers/api_handler.h
	src/handlers/api_handler.cpp
	src/handlers/command_handler.h
	src/handlers/command_handler.cpp
	src/handlers/request_handler.h
	src/handlers/request_handler.cpp
	src/handlers/target_storage.h
	src/server/http_server.h 
	src/server/http_server.cpp
	src/server/logger.h
	src/server/logger.cpp
	src/main.cpp	
	src/sdk.h
)

add_executable(game_server_tests
	tests/model_tests.cpp
	tests/loot_generator_tests.cpp
	tests/collision-detector-tests.cpp
	tests/main.cpp
)

target_link_libraries(game_server game_lib)
target_link_libraries(game_server_tests game_lib CONAN_PKG::catch2)
catch_discover_tests(game_server_tests) 