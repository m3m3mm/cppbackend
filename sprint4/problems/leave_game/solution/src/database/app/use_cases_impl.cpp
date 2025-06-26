#include "use_cases_impl.h"

namespace app_database {
UseCasesImpl::UseCasesImpl(UnitOfWorkFactory& unit_factory) 
    : unit_factory_(unit_factory) {
}

void UseCasesImpl::AddPlayerRecord(const std::vector<player::PlayerRecord>& player_records) {
    auto unit_of_work = unit_factory_.CreateUnitOfWork();

    unit_of_work->GetPlayersRepository().Save(player_records);
    unit_of_work->Commit();
}

std::vector<player::PlayerRecord> UseCasesImpl::GetPlayersRecordList(size_t offset, size_t limit) const {
    return unit_factory_.CreateUnitOfWork()->GetPlayersRepository().GetPlayersRecordList(offset, limit);
}
} // namespace app_database