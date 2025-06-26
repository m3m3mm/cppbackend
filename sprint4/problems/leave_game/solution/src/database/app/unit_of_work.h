#pragma once

#include <memory>

#include "../../game_server/app/player_properties.h"

namespace app_database {
struct UnitOfWork {
    virtual void Commit() = 0;
    virtual player::RetiredPlayersRepository& GetPlayersRepository() = 0;
    virtual ~UnitOfWork() = default;
};

using UnitOfWorkHolder = std::unique_ptr<UnitOfWork>;

class UnitOfWorkFactory {
public:
    virtual UnitOfWorkHolder CreateUnitOfWork() = 0;
protected:
    virtual ~UnitOfWorkFactory() = default;
};
}