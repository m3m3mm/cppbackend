#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const domain::AuthorId& author_id, const std::string& title, int year, std::set<std::string> tags) = 0;

    virtual void EditAuthor(const std::string& id, const std::string& new_name) = 0;
    virtual void EditBook(const domain::Book& book) = 0;

    virtual std::optional<std::string> FindAuthorByName(const std::string& name) = 0;
    virtual std::optional<std::string> FindAuthorById(const std::string& id) = 0;

    virtual domain::BooksInfo FindBooksByTitle(const std::string& title) = 0;
    virtual domain::Book FindBook(const std::string& id) = 0;

    virtual domain::Authors GetAuthors() const = 0;
    virtual domain::BooksInfo GetBooks() const = 0;
    virtual domain::BooksInfo GetBooksInfoByAuthorId(const std::string& id) const = 0;
    virtual domain::Books GetBooksByAuthorId(const std::string& id) const = 0;
    virtual std::set<std::string> GetTags(const std::string& id) const = 0;

    virtual void DeleteAuthor(const std::string& id) = 0;
    virtual void DeleteBook(const std::string& id) = 0;
    virtual void DeleteBookTags(const std::string& book_id) = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
