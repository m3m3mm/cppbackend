#include "../domain/author.h"
#include "use_cases_impl.h"

namespace app {

void UseCasesImpl::AddAuthor(const std::string& name) {
    auto work = unit_factory_.CreateUnitOfWork(); 
    work->Authors().Save({domain::AuthorId::New(), name});
    work->Commit();
}

void UseCasesImpl::AddBook(const domain::AuthorId& author_id, const std::string &title, int year, std::set<std::string> tags) {
    auto work = unit_factory_.CreateUnitOfWork(); 
    work->Books().Save({domain::BookId::New(), author_id, title, year, tags});
    work->Commit();
}

void UseCasesImpl::EditAuthor(const std::string& id, const std::string& new_name) {
    auto work = unit_factory_.CreateUnitOfWork();

    work->Authors().Save({domain::AuthorId::FromString(id), new_name});
    work->Commit();
}

void UseCasesImpl::EditBook(const domain::Book& book) {
    auto work = unit_factory_.CreateUnitOfWork();
    work->Books().DeleteBookTags(book.GetId().ToString());
    work->Books().Save(book);
    work->Commit();
}

std::optional<std::string> UseCasesImpl::FindAuthorByName(const std::string& name) {
    return unit_factory_.CreateUnitOfWork()->Authors().FindAuthorByName(name);
}

std::optional<std::string> UseCasesImpl::FindAuthorById(const std::string& id) {
    return unit_factory_.CreateUnitOfWork()->Authors().FindAuthorById(id);
}

domain::BooksInfo UseCasesImpl::FindBooksByTitle(const std::string& title) {
    return unit_factory_.CreateUnitOfWork()->Books().FindBooksByTitle(title);
}

domain::Book UseCasesImpl::FindBook(const std::string& id) {
    return unit_factory_.CreateUnitOfWork()->Books().FindBook(id);
}

domain::Authors UseCasesImpl::GetAuthors() const {
    return unit_factory_.CreateUnitOfWork()->Authors().GetAuthors();
}

domain::BooksInfo UseCasesImpl::GetBooks() const {
    return unit_factory_.CreateUnitOfWork()->Books().GetBooks();
}

domain::BooksInfo UseCasesImpl::GetBooksInfoByAuthorId(const std::string& id) const {
    return unit_factory_.CreateUnitOfWork()->Books().GetBooksInfoByAuthorId(id);
}

domain::Books UseCasesImpl::GetBooksByAuthorId(const std::string& id) const {
    return unit_factory_.CreateUnitOfWork()->Books().GetBooksByAuthorId(id);
}

std::set<std::string> UseCasesImpl::GetTags(const std::string& id) const {
    return  unit_factory_.CreateUnitOfWork()->Books().GetTags(id);
}

void UseCasesImpl::DeleteAuthor(const std::string& id) {
    auto work = unit_factory_.CreateUnitOfWork();

   for(const auto& book : work->Books().GetBooksByAuthorId(id)) {
        work->Books().DeleteBookTags(book.GetId().ToString());
        work->Books().DeleteBook(book.GetId().ToString());
    }

    work->Authors().DeleteAuthor(id);
    
    work->Commit();
}

void UseCasesImpl::DeleteBook(const std::string& id) {
    auto work = unit_factory_.CreateUnitOfWork();

    work->Books().DeleteBookTags(id);
    work->Books().DeleteBook(id);

    work->Commit();
}

void UseCasesImpl::DeleteBookTags(const std::string& book_id) {
    auto work = unit_factory_.CreateUnitOfWork();

    work->Books().DeleteBookTags(book_id);
    work->Commit();
}
} // namespace app
