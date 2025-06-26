#pragma once

#include <optional>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"
#include "use_cases.h"
#include "unit_of_work.h"

namespace app {
class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(UnitOfWorkFactory& unit_factory)
        : unit_factory_(unit_factory) {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(const domain::AuthorId& author_id, const std::string& title, int year, std::set<std::string> tags) override;

    void EditAuthor(const std::string& id, const std::string& new_ame) override;
    void EditBook(const domain::Book& book) override;

    std::optional<std::string> FindAuthorByName(const std::string& name) override;
    std::optional<std::string> FindAuthorById(const std::string& id) override;

    domain::BooksInfo FindBooksByTitle(const std::string& title);
    domain::Book FindBook(const std::string& id);

    domain::Authors GetAuthors() const override;
    domain::BooksInfo GetBooks() const override;
    domain::BooksInfo GetBooksInfoByAuthorId(const std::string& id) const override;
    domain::Books GetBooksByAuthorId(const std::string& id) const override;
    std::set<std::string> GetTags(const std::string& id) const override;

    void DeleteAuthor(const std::string& id) override;

    void DeleteBook(const std::string& id) override;
    void DeleteBookTags(const std::string& book_id) override;

private:
    UnitOfWorkFactory& unit_factory_;
};
}  // namespace app
