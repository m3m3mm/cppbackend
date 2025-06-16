#include <boost/json/array.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <list>
#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <vector>

#include "../src/application.h"
#include "../src/extra_data.h"
#include "../src/game_properties.h"
#include "../src/json_constructor.h"
#include "../src/json_loader.h"
#include "../src/json_tags.h"
#include "../src/loot_generator.h"
#include "../src/model.h"
#include "../src/player_properties.h"
#include "../src/sdk.h"
#include "../src/tagged.h"
#include "../src/target_storage.h"

SCENARIO("Model", "[Model]") {
    using namespace std::literals;
    extra_data::LootTypes loot_types;

    GIVEN("Model and invalid loot configuration info") {
        

        WHEN("config file without lootGeneratorConfig") {
            THEN("throw exception loading game") {
                CHECK_THROWS_AS(json_loader::LoadGame("../tests/test_data/config_without_loot_generator.json", loot_types, true), 
                                                      std::runtime_error);
            }
        }

        WHEN("map lootingType configuration incorrect") {
            THEN("throw exception loading game with map without lootingType") {
                CHECK_THROWS_AS(json_loader::LoadGame("../tests/test_data/config_no_loot.json", loot_types, true), std::runtime_error);
            }
            
            AND_THEN("throw exeption while loading game with map with empty lootingType") {
                CHECK_THROWS_AS(json_loader::LoadGame("../tests/test_data/config_empty_loot.json", loot_types, true), std::runtime_error);
            }
        }
        
    }

    GIVEN("Model and correct loot configuration info") {
        WHEN("correct config file") {
            THEN("successful loading game") {
                CHECK_NOTHROW(json_loader::LoadGame("../tests/test_data/config_with_loot.json", loot_types, true));
            }

            model::Game game = json_loader::LoadGame("../tests/test_data/config_with_loot.json", loot_types, true);
            model::Map::Id  id = model::Map::Id("map1");
            AND_THEN("correct count loot types in model map") {
                CHECK(game.GetMaps().size() == 1);
                CHECK(game.FindMap(id)->GetLootTypesCount() == 2);
            }

            AND_THEN("correct array of types in class LootTypes") {
                //Взято из  ../tests/test_data/config_with_loot.json
                boost::json::array types = boost::json::parse("[{\"name\": \"key\","
                                                              "\"file\": \"assets/key.obj\","
                                                              "\"type\": \"obj\","
                                                              "\"rotation\": 90,"
                                                              "\"color\" : \"#338844\","
                                                              "\"scale\": 0.03"
                                                              "},"
                                                              "{"
                                                              "\"name\": \"wallet\","
                                                              "\"file\": \"assets/wallet.obj\","
                                                              "\"type\": \"obj\","
                                                              "\"rotation\": 0,"
                                                              "\"color\" : \"#883344\","
                                                              "\"scale\": 0.01"
                                                              "}]"s).as_array();
                                                              
                CHECK(loot_types.GetTypes(id)->size() == 2);
                CHECK(*loot_types.GetTypes(id) == types);
            }

            std::vector<std::string> names = {"Bob", "John", "Nick", "Tom", "Poll"};
            for(const auto& name : names) {
                game.PrepareUnitParameters(id, name);
            }

            auto session = game.GetSessions()[0];
            session->GenerateLoot(10000ms);

            AND_THEN("type-randomizer get correts loot types") {
                for(const auto& loot : session->GetLoot()) {
                    //Тип может иметь индекс от 0 до количества возможных типов на карте - 1
                    CHECK(loot.GetType() < game.FindMap(id)->GetLootTypesCount());
                }
            }

            AND_THEN("the amount of loot on the map will not exceed the number of players") {
                CHECK(session->GetLoot().size() <= session->GetDogs().size());
            }
        }
    }
}