#pragma once

#include "../domain/author.h"
#include "../domain/author_fwd.h"
#include "../domain/book.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {
class UseCasesImpl : public UseCases {
public:
    explicit UseCasesImpl(domain::AuthorRepository& authors,  domain::BookRepository& books)
        : authors_{authors}
        , books_(books) {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(const domain::AuthorId& author_id, const std::string& title, int year) override;

    domain::Authors GetAuthors() const override;
    domain::Books GetBooks() const override;
    domain::Books GetBooksByAuthorId(const domain::AuthorId& id) const override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};
}  // namespace app
