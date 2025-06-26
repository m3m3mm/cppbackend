#pragma once

#include <pqxx/connection>
#include <pqxx/transaction>

#include <optional>
#include <string>

#include "../app/unit_of_work.h"
#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::work& work)
        : work_{work} {
    }

    void Save(const domain::Author& author) override;

    std::optional<std::string> FindAuthorByName(const std::string& name) override;
    std::optional<std::string> FindAuthorById(const std::string& id) override;

    domain::Authors GetAuthors() const override;

    void DeleteAuthor(const std::string& id) override;

private:
    pqxx::work& work_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::work& work) :
        work_(work) {
    }
  
    void Save(const domain::Book& book) override;

    domain::BooksInfo FindBooksByTitle(const std::string& title) override;
    domain::Book FindBook(const std::string& id) override;

    std::set<std::string> GetTags(const std::string& id) const override;
    domain::BooksInfo GetBooks() const override;
    domain::BooksInfo GetBooksInfoByAuthorId(const std::string& id) const override;
    domain::Books GetBooksByAuthorId(const std::string& id) const override;

    void DeleteBook(const std::string& id) override;
    void DeleteBookTags(const std::string& book_id) override;
private:
    pqxx::work& work_;
};

class UnitOfWorkImpl : public app::UnitOfWork {
public:
    explicit UnitOfWorkImpl(pqxx::connection& connection) 
    : work_(connection)
    , authors_(work_)
    , books_(work_) {
    }
  
    void Commit() override {
        work_.commit();
    }
  
    domain::AuthorRepository& Authors() override {
      return authors_;
    }
  
    domain::BookRepository& Books() override {
      return books_;
    }
  
private:
    pqxx::work work_;
    AuthorRepositoryImpl authors_;
    BookRepositoryImpl books_;
};
  
class UnitOfWorkFactoryImpl : public app::UnitOfWorkFactory {
public:
    explicit UnitOfWorkFactoryImpl(pqxx::connection& connection) :
        connection_(connection) {}
  
    app::UnitOfWorkHolder CreateUnitOfWork() override {
        return std::make_unique<UnitOfWorkImpl>(connection_);
    }
  
private:
    pqxx::connection& connection_;
};  


class Database {
public:
    explicit Database(pqxx::connection connection);

    app::UnitOfWorkFactory& GetUnitOfWorkFactory() {
        return unit_factory_;
    }
private:
    pqxx::connection connection_;
    UnitOfWorkFactoryImpl unit_factory_ {connection_};
};

}  // namespace postgres