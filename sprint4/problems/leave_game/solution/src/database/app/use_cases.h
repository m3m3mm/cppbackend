#pragma once

#include <pqxx/connection>
#include <pqxx/transaction>

#include <memory>
#include <optional>

#include "../../game_server/app/player_properties.h"
#include "../postgres/postgres.h"

namespace app_database {
class UseCases {
public:
    virtual void AddPlayerRecord(const std::vector<player::PlayerRecord>& player_records) = 0;
    virtual std::vector<player::PlayerRecord> GetPlayersRecordList(size_t offset, size_t limit) const = 0;
protected:
    ~UseCases() = default;
};
}//app_database
