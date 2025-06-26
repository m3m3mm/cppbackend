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
	src/game_server/app/detail/app_serializer.h
	src/game_server/app/detail/app_serializer.cpp
	src/game_server/app/application.h
	src/game_server/app/application.cpp
	src/game_server/app/player_properties.h
	src/game_server/app/player_properties.cpp
	src/game_server/model/detail/collision_detector.h
	src/game_server/model/detail/collision_detector.cpp
	src/game_server/model/detail/geom.h
	src/game_server/model/detail/loot_generator.h
	src/game_server/model/detail/loot_generator.cpp
	src/game_server/model/detail/model_serializer.h
	src/game_server/model/detail/model_serializer.cpp
	src/game_server/model/dynamic_object_properties.h
	src/game_server/model/dynamic_object_properties.cpp
	src/game_server/model/game_properties.h
	src/game_server/model/game_properties.cpp
	src/game_server/model/static_object_prorerties.h
	src/game_server/model/static_object_properties.cpp
	src/game_server/json/json_constructor.h
	src/game_server/json/json_constructor.cpp
	src/game_server/json/json_loader.h
	src/game_server/json/json_loader.cpp
	src/game_server/json/json_tags.h
	src/game_server/server/extra_data.h
	src/game_server/server/extra_data.cpp
	src/game_server/boost_json.cpp
	src/game_server/tagged.h
)

target_include_directories(game_lib PUBLIC CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_link_libraries(game_lib PUBLIC ${Boost_LIBRARIES} Threads::Threads CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_library(postgres_lib STATIC
	src/database/util/tagged_uuid.h
	src/database/util/tagged_uuid.cpp
	src/database/app/use_cases.h
	src/database/app/use_cases_impl.h
	src/database/app/use_cases_impl.cpp
	src/database/app/unit_of_work.h
	src/database/postgres/postgres.h
	src/database/postgres/postgres.cpp
	src/database/domain.h
)

target_include_directories(postgres_lib PUBLIC CONAN_PKG::libpq CONAN_PKG::libpqxx)
target_link_libraries(postgres_lib PUBLIC CONAN_PKG::libpq CONAN_PKG::libpqxx)

add_executable(game_server
	src/game_server/handlers/api_handler.h
	src/game_server/handlers/api_handler.cpp
	src/game_server/handlers/command_handler.h
	src/game_server/handlers/command_handler.cpp
	src/game_server/handlers/request_handler.h
	src/game_server/handlers/request_handler.cpp
	src/game_server/handlers/state_handler.h
	src/game_server/handlers/state_handler.cpp
	src/game_server/handlers/target_storage.h
	src/game_server/server/http_server.h 
	src/game_server/server/http_server.cpp
	src/game_server/server/logger.h
	src/game_server/server/logger.cpp
	src/game_server/main.cpp	
	src/game_server/sdk.h
)

add_executable(game_server_tests
	tests/model_tests.cpp
	tests/loot_generator_tests.cpp
	tests/collision-detector-tests.cpp
	tests/state-serialization-tests.cpp
	tests/main.cpp
)

target_link_libraries(game_server game_lib postgres_lib)
target_link_libraries(game_server_tests game_lib postgres_lib CONAN_PKG::catch2)
catch_discover_tests(game_server_tests) 