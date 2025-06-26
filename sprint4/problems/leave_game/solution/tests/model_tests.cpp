#include <boost/json/array.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <list>
#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>
#include <vector>

#include "../src/game_server/app/application.h"
#include "../src/game_server/app/player_properties.h"
#include "../src/game_server/handlers/target_storage.h"
#include "../src/game_server/json/json_constructor.h"
#include "../src/game_server/json/json_loader.h"
#include "../src/game_server/json/json_tags.h"
#include "../src/game_server/model/detail/loot_generator.h"
#include "../src/game_server/model/dynamic_object_properties.h"
#include "../src/game_server/model/game_properties.h"
#include "../src/game_server/model/static_object_prorerties.h"
#include "../src/game_server/server/extra_data.h"
#include "../src/game_server/sdk.h"
#include "../src/game_server/tagged.h"

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

    model::Map::Id map1_id = model::Map::Id("map1");
    model::Map::Id town_id = model::Map::Id("town");

    GIVEN("Model and correct loot configuration info") {
        WHEN("correct config file") {
            THEN("successful loading game") {
                CHECK_NOTHROW(json_loader::LoadGame("../tests/test_data/config.json", loot_types, true));
            }

            model::Game game = json_loader::LoadGame("../tests/test_data/config.json", loot_types, true);
            
            AND_THEN("correct count loot types in model map") {
                CHECK(game.GetMaps().size() == 1);
                CHECK(game.FindMap(map1_id)->GetLootTypesCount() == 2);
            }

            AND_THEN("correct array of types in class LootTypes") {
                //Взято из  ../tests/test_data/config.json
                boost::json::array types = boost::json::parse("[{\"name\": \"key\","
                                                              "\"file\": \"assets/key.obj\","
                                                              "\"type\": \"obj\","
                                                              "\"rotation\": 90,"
                                                              "\"color\" : \"#338844\","
                                                              "\"scale\": 0.03,"
                                                              "\"value\": 10"
                                                              "},"
                                                              "{"
                                                              "\"name\": \"wallet\","
                                                              "\"file\": \"assets/wallet.obj\","
                                                              "\"type\": \"obj\","
                                                              "\"rotation\": 0,"
                                                              "\"color\" : \"#883344\","
                                                              "\"scale\": 0.01,"
                                                              "\"value\": 30"
                                                              "}]"s).as_array();
                                                              
                CHECK(loot_types.GetTypes(map1_id)->size() == 2);
                CHECK(*loot_types.GetTypes(map1_id) == types);
            }

            std::vector<std::string> names = {"Bob", "John", "Nick", "Tom", "Poll"};
            for(const auto& name : names) {
                game.PrepareUnitParameters(map1_id, name);
            }

            auto session = game.GetSessions()[0];
            session->GenerateLoot(10000ms);

            AND_THEN("type-randomizer get correts loot types") {
                for(const auto& loot : session->GetLoot()) {
                    //Тип может иметь индекс от 0 до количества возможных типов на карте - 1
                    CHECK(loot.GetType() < game.FindMap(map1_id)->GetLootTypesCount());
                }
            }

            AND_THEN("the amount of loot on the map will not exceed the number of players") {
                CHECK(session->GetLoot().size() <= session->GetDogs().size());
            }
        }
    }

    GIVEN("model and bag capacity") {
        WHEN("no info for bag capacity") {
            model::Game game = json_loader::LoadGame("../tests/test_data/config.json", loot_types, true);

            THEN("bag capacity per all of maps equal 3") {
                for(const auto& map : game.GetMaps()){
                    CHECK(map.GetBagCapacity() == 3);
                }
            }
        }

        model::Game game = json_loader::LoadGame("../tests/test_data/config _with_capacity.json", loot_types, false);
        AND_WHEN("config with bag capacity info") {
             
             THEN("bag capacity euqal capacity info in config for map") {
                CHECK(game.FindMap(map1_id)->GetBagCapacity() == 5);
                CHECK(game.FindMap(town_id)->GetBagCapacity() == 4);
             }
        }

        auto parameters_map1 = game.PrepareUnitParameters(map1_id, "Bob");
        auto parameters_town = game.PrepareUnitParameters(town_id, "Paul");

        AND_WHEN("added dogs have a bag"){

            THEN("dog bag capacity equal capacity per map bag capacity info"){
                CHECK(parameters_map1.dog->GetBagCapacity() == 5);
                CHECK(parameters_town.dog->GetBagCapacity() == 4);
            }

            AND_THEN("bag can be filled loot") {
                size_t next_id = 0;
                std::vector<model::Loot> loot {{next_id++, 1, 50, {0.0, 0.0}},
                                               {next_id++, 1, 50, {1.1, 1.1}}};

                for(const auto& obj : loot) {
                    parameters_map1.dog->TryPickUpLoot(obj);
                }
                CHECK(parameters_map1.dog->GetBag().size() == 2);
                
                THEN("loot can be taken out of the bag and score grows by cost of loot"){
                    parameters_map1.dog->LayOutLoot();
                    CHECK(parameters_map1.dog->GetBag().size() == 0);
                    CHECK(parameters_map1.dog->GetScore() == 100);
                }
            }
        }

        AND_WHEN("bag is full") {
            size_t next_id = 0;
            std::vector<model::Loot> loot {{next_id++, 1, 50, {0.0, 0.0}},
                                           {next_id++, 1, 50, {1.1, 1.1}},
                                           {next_id++, 1, 50, {2.2, 2.2}},
                                           {next_id++, 1, 50, {3.3, 3.3}},
                                           {next_id++, 1, 50, {4.4, 4.4}},
                                           {next_id++, 1, 50, {5.5, 5.5}}};

            for(size_t i = 0; i < parameters_map1.dog->GetBagCapacity(); ++i) {
                parameters_map1.dog->TryPickUpLoot(loot[i]);
            }

            THEN("loot is not picked up") {
                parameters_map1.dog->TryPickUpLoot(loot.back());
                CHECK(parameters_map1.dog->GetBag().size() == parameters_map1.dog->GetBagCapacity());
            }
        }

        AND_WHEN("loot picked up") {
            size_t next_id = 0;
            std::vector<model::Loot> loot {{next_id++, 1, 50, {0.0, 0.0}}};

            THEN("it is marked as picked up") {
                loot.front().MarkPikedUp();
                CHECK(loot.front().IsPickedUp());
            }

            AND_THEN("it can't be picked up by another players") {
                //Имитируем взятие лута другим игроком
                loot.front().MarkPikedUp();
                parameters_map1.dog->TryPickUpLoot(loot.front());
                CHECK(parameters_map1.dog->GetBag().size() == 0);
            }
        }           
    }
}