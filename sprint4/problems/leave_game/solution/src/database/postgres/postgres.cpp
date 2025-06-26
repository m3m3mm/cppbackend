#include <pqxx/result>

#include "../domain.h"
#include "postgres.h"

namespace postgres {
using namespace std::literals;

//__________RetiredPlayersRepositoryImpl__________
RetiredPlayersRepositoryImpl::RetiredPlayersRepositoryImpl(pqxx::work& work) : work_(work) {
}

void RetiredPlayersRepositoryImpl::Save(const std::vector<player::PlayerRecord>& player_records) {
    for(const auto& record : player_records) {
        work_.exec_params(R"(INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4);)",
                          domain::PlayerId::New().ToString(),
                          std::move(record.name),
                          std::move(record.score),
                          std::move(record.total_time.count()));
    }
}

std::vector<player::PlayerRecord> RetiredPlayersRepositoryImpl::GetPlayersRecordList(size_t offset, size_t limit) const {
    const auto query_text = "SELECT name, score, play_time_ms FROM retired_players ORDER BY score DESC, play_time_ms, name"s
                            + " LIMIT "s + std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
    
    std::vector<player::PlayerRecord> records;

    for(const auto& [name, score, play_time] :  work_.query<std::string, size_t, int64_t>(query_text)) {
        records.push_back(player::PlayerRecord{name, std::chrono::milliseconds(play_time), score});
    }

    return records;
}

//__________ConnectionPool__________
ConnectionPool::ConnectionWrapper ConnectionPool::GetConnection() {
    std::unique_lock lock{mutex_};
    // Блокируем текущий поток и ждём, пока cond_var_ не получит уведомление и не освободится
    // хотя бы одно соединение
    cond_var_.wait(lock, [this] {
        return used_connections_ < pool_.size();
    });
    // После выхода из цикла ожидания мьютекс остаётся захваченным

    return {std::move(pool_[used_connections_++]), *this};
}

void ConnectionPool::ReturnConnection(ConnectionPtr&& conn) {
    // Возвращаем соединение обратно в пул
    {
        std::lock_guard lock{mutex_};
        assert(used_connections_ != 0);
        pool_[--used_connections_] = std::move(conn);
    }
    // Уведомляем один из ожидающих потоков об изменении состояния пула
    cond_var_.notify_one();
}

//__________UnitOfWorkImpl__________
UnitOfWorkImpl::UnitOfWorkImpl(ConnectionPool::ConnectionWrapper&& connection)
    : work_(*connection)
    , retired_player_(work_) {
}

void UnitOfWorkImpl::Commit() {
    work_.commit();
}

player::RetiredPlayersRepository &UnitOfWorkImpl::GetPlayersRepository() {
    return retired_player_;
}

//__________UnitOfWorkFactoryImpl__________
UnitOfWorkFactoryImpl::UnitOfWorkFactoryImpl(ConnectionPool& connection_pool) :
    connection_pool_(connection_pool) {
}

app_database::UnitOfWorkHolder UnitOfWorkFactoryImpl::CreateUnitOfWork() {
    return std::make_unique<UnitOfWorkImpl>(connection_pool_.GetConnection());
}


//__________Database__________
Database::Database(DatabaseConfig&& config) 
    : connection_pool_(config.pool_size, [url = config.url]() {
        return std::make_shared<pqxx::connection>(url);
    }) {
    auto conn = connection_pool_.GetConnection();
    pqxx::work work{*conn};

    work.exec(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id UUID CONSTRAINT players_id_constraint PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            score INTEGER NOT NULL CONSTRAINT score_non_negative CHECK (score >= 0),
            play_time_ms INTEGER NOT NULL CONSTRAINT play_time_non_negative CHECK (play_time_ms >= 0)
        );
        CREATE INDEX IF NOT EXISTS retired_players_index ON retired_players (score DESC, play_time_ms, name);
    )");
    work.commit();
}

app_database::UnitOfWorkFactory &Database::GetUnitOfWorkFactory()  {
    return unit_factory_;
}

} // namespace postgres