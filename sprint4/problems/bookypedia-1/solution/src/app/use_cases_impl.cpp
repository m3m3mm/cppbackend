#include "use_cases_impl.h"

#include "../domain/author.h"

namespace app {
using namespace domain;

void UseCasesImpl::AddAuthor(const std::string& name) {
    authors_.Save({AuthorId::New(), name});
}

void UseCasesImpl::AddBook(const domain::AuthorId &author_id, const std::string &title, int year) {
    books_.Save({BookId::New(), author_id, title, year});
}

domain::Authors UseCasesImpl::GetAuthors() const {
    return authors_.GetAuthors();
}

domain::Books UseCasesImpl::GetBooks() const {
    return books_.GetBooks();
}

domain::Books UseCasesImpl::GetBooksByAuthorId(const AuthorId& id) const {
    return books_.GetBooksByAuthorId(id);
}
}  // namespace app
