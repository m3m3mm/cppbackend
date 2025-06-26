#pragma once

#include <pqxx/connection>
#include <pqxx/transaction>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "../../game_server/app/player_properties.h"
#include "../app/unit_of_work.h"
#include "../util/tagged_uuid.h"

namespace postgres {
struct DatabaseConfig {
    size_t pool_size = 1;
    std::string url;
};

class RetiredPlayersRepositoryImpl : public player::RetiredPlayersRepository {
public:
    explicit RetiredPlayersRepositoryImpl(pqxx::work& work);

    void Save(const std::vector<player::PlayerRecord>& player_records) override;
    std::vector<player::PlayerRecord> GetPlayersRecordList(size_t offset, size_t limit) const override;
private:
    pqxx::work& work_;
};

class ConnectionPool {
    using PoolType = ConnectionPool;
    using ConnectionPtr = std::shared_ptr<pqxx::connection>;
public:
    class ConnectionWrapper {
    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
            : conn_{std::move(conn)}
            , pool_{&pool} {
        }

        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        ConnectionWrapper(ConnectionWrapper&&) = default;
        ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

        pqxx::connection& operator*() const& noexcept {
            return *conn_;
        }
        pqxx::connection& operator*() const&& = delete;

        pqxx::connection* operator->() const& noexcept {
            return conn_.get();
        }

        ~ConnectionWrapper() {
            if (conn_) {
                pool_->ReturnConnection(std::move(conn_));
            }
        }
    private:
        std::shared_ptr<pqxx::connection> conn_;
        PoolType* pool_;
    };

    // ConnectionFactory is a functional object returning std::shared_ptr<pqxx::connection>
    template <typename ConnectionFactory>
    ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
        pool_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            pool_.emplace_back(connection_factory());
        }
    }

    ConnectionWrapper GetConnection();

private:
    void ReturnConnection(ConnectionPtr&& conn);
    
    std::mutex mutex_;
    std::condition_variable cond_var_;
    std::vector<ConnectionPtr> pool_;
    size_t used_connections_ = 0;
};

class UnitOfWorkImpl : public app_database::UnitOfWork {
public:
    explicit UnitOfWorkImpl(ConnectionPool::ConnectionWrapper&& connection);

    void Commit() override;

    player::RetiredPlayersRepository& GetPlayersRepository() override;

private:
    pqxx::work work_;
    RetiredPlayersRepositoryImpl retired_player_;
};

class UnitOfWorkFactoryImpl : public app_database::UnitOfWorkFactory {
public:
    explicit UnitOfWorkFactoryImpl(ConnectionPool& connection_pool);
  
    app_database::UnitOfWorkHolder CreateUnitOfWork() override;
  
private:
    ConnectionPool& connection_pool_;
};

class Database {
public:
    explicit Database(DatabaseConfig&& config);
  
    app_database::UnitOfWorkFactory& GetUnitOfWorkFactory();
private:
    ConnectionPool connection_pool_;
    UnitOfWorkFactoryImpl unit_factory_{connection_pool_};
};

} //namespace postgres