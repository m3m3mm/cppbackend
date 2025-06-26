#pragma once

#include <string>
#include <vector>

#include "../domain/author.h"
#include "../domain/book.h"

namespace app {

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const domain::AuthorId& author_id, const std::string& title, int year) = 0;

    virtual domain::Authors GetAuthors() const = 0;
    virtual domain::Books GetBooks() const = 0;
    virtual domain::Books GetBooksByAuthorId(const domain::AuthorId& id) const = 0;
protected:
    ~UseCases() = default;
};

}  // namespace app
