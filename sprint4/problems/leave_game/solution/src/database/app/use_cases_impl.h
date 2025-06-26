#pragma once

#include <vector>

#include "use_cases.h"
#include "unit_of_work.h"

namespace app_database {
class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(UnitOfWorkFactory& unit_factory);

    void AddPlayerRecord(const std::vector<player::PlayerRecord>& player_records) override;
    std::vector<player::PlayerRecord> GetPlayersRecordList(size_t offset, size_t limit) const override;
private:
    UnitOfWorkFactory& unit_factory_;
};
}//namespace app_database