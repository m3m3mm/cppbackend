#include "postgres.h"

#include <pqxx/zview.hxx>
#include <pqxx/result>

namespace postgres {
using namespace domain;
using namespace std::literals;
using pqxx::operator"" _zv;

//AuthorRepositoryImpl
void AuthorRepositoryImpl::Save(const domain::Author& author) {
    work_.exec_params(R"(INSERT INTO authors (id, name) VALUES ($1, $2)
                         ON CONFLICT (id) DO UPDATE SET name=$2;
                        )"_zv,
                      author.GetId().ToString(), author.GetName());
}

std::optional<std::string> AuthorRepositoryImpl::FindAuthorByName(const std::string& name) {
    const auto query_text = R"(SELECT id, name FROM authors WHERE name=)" + work_.quote(name) + ";";

    if(const auto& data =  work_.query01<std::string, std::string>(query_text); data.has_value()) {
        const auto& [id, author_name] = *data;
        return id;
    }

    return std::nullopt;
}

std::optional<std::string> AuthorRepositoryImpl::FindAuthorById(const std::string& id) {
    const auto query_text = R"(SELECT id, name FROM authors WHERE id=)" + work_.quote(id) + ";";

    if(const auto& data =  work_.query01<std::string, std::string>(query_text); data.has_value()) {
        const auto& [id, author_name] = *data;
        return author_name;
    }

    return std::nullopt;
}

domain::Authors AuthorRepositoryImpl::GetAuthors() const {
    const auto query_text = "SELECT * FROM authors ORDER BY name;"_zv;
    domain::Authors authors;

    for (const auto& [id, name] : work_.query<std::string, std::string>(query_text)) {
        authors.emplace_back(domain::AuthorId::FromString(id), name);
    }

    return authors;
}

void AuthorRepositoryImpl::DeleteAuthor(const std::string& id) {
    work_.exec_params(R"(DELETE FROM authors WHERE id = $1;)", id);
}

//BookRepositoryImpl
void BookRepositoryImpl::Save(const Book& book) {
    work_.exec_params(R"(INSERT INTO books (id, author_id, title, publication_year) VALUES ($1, $2, $3, $4)
                        ON CONFLICT (id) DO UPDATE SET author_id=$2, title=$3, publication_year=$4;
                        )"_zv,
                      book.GetId().ToString(),
                      book.GetAuthorId().ToString(),
                      book.GetTitle(),
                      book.GetPublicationYear()
    );

    for(const auto& tag : book.GetTags()){
        work_.exec_params(R"(INSERT INTO book_tags (book_id, tag) VALUES ($1, $2);)"_zv,
                          book.GetId().ToString(),
                          tag);
    }
}

domain::BooksInfo BookRepositoryImpl::FindBooksByTitle(const std::string& title) {
    const auto query_text = R"(SELECT b.id, b.title, b.publication_year, a.name AS name
                               FROM books b JOIN authors a ON b.author_id = a.id 
                               WHERE b.title =)" + work_.quote(title) +
                               "ORDER BY title, name, publication_year ASC;";
    
    domain::BooksInfo books;

    for (const auto& [id, title, publication_year, author_name] : work_.query<std::string,

                                                                              std::string, 
                                                                              int,
                                                                              std::string>(query_text)) {
        books.emplace_back(id, title, author_name, publication_year);
    }

    return books;
}

domain::Book BookRepositoryImpl::FindBook(const std::string& id) {
    const auto query_text = R"(SELECT id, author_id, title, publication_year FROM books WHERE id = )" + work_.quote(id) + ";";

    const auto& [book_id_str, author_id, title, publication_year] = work_.query1<std::string,
                                                                                 std::string,
                                                                                 std::string,
                                                                                 int>(query_text);
    auto book_id = domain::BookId::FromString(book_id_str);
    return domain::Book{book_id, AuthorId::FromString(author_id), title, publication_year, GetTags(book_id.ToString())};
}

std::set<std::string> BookRepositoryImpl::GetTags(const std::string& id) const {
    const auto query_text = R"(SELECT tag FROM book_tags WHERE book_id = )" + work_.quote(id) + ";";
    std::set<std::string> result;

    for(const auto& [tag] : work_.query<std::string>(query_text)) {
        result.insert(tag);
    }

    return result;
}

domain::BooksInfo BookRepositoryImpl::GetBooks() const {
    const auto query_text = R"(SELECT b.id, b.title, b.publication_year, a.name AS name 
                               FROM books b JOIN authors a ON b.author_id = a.id 
                               ORDER BY title, name, publication_year ASC;)"_zv;
    domain::BooksInfo books;

    for (const auto& [id, title, publication_year, author_name] : work_.query<std::string,
                                                                              std::string, 
                                                                              int,
                                                                              std::string>(query_text)) {
        books.emplace_back(id, title, author_name, publication_year);
    }

    return books;
}

domain::BooksInfo BookRepositoryImpl::GetBooksInfoByAuthorId(const std::string& id) const {
    const auto query_text = R"(SELECT b.id, b.title, b.publication_year, a.name AS name 
                               FROM books b JOIN authors a ON b.author_id = a.id
                               WHERE b.author_id =)" + work_.quote(id) + ";";

    domain::BooksInfo books;

    for (const auto& [id, title, publication_year, author_name] : work_.query<std::string,
                                                                              std::string, 
                                                                              int,
                                                                              std::string>(query_text)) {
        books.emplace_back(id, title, author_name, publication_year);
    }

    return books;
}

domain::Books BookRepositoryImpl::GetBooksByAuthorId(const std::string& id) const {
    const auto query_text = "SELECT id, author_id, title, publication_year FROM books WHERE author_id = "
                            + work_.quote(id) + " ORDER BY publication_year, title;";
        
    domain::Books books;

    for (const auto& [id, author_id, title, publication_year] : work_.query<std::string, 
                                                                            std::string, 
                                                                            std::string, 
                                                                            int>(query_text)) {
        auto book_id = domain::BookId::FromString(id);
        books.emplace_back(book_id, domain::AuthorId::FromString(author_id), title, publication_year, GetTags(book_id.ToString()));
    }

    return books;
}

void BookRepositoryImpl::DeleteBook(const std::string& id) {
    work_.exec_params(R"(DELETE FROM books WHERE id = $1;)", id);
}

void BookRepositoryImpl::DeleteBookTags(const std::string& book_id) {
    work_.exec_params(R"(DELETE FROM book_tags WHERE book_id = $1;)", book_id);
}

Database::Database(pqxx::connection connection)
    : connection_{std::move(connection)} {
    pqxx::work work{connection_};
    work.exec(R"(CREATE TABLE IF NOT EXISTS authors (
              id UUID CONSTRAINT author_id_constraint PRIMARY KEY,
              name varchar(100) UNIQUE NOT NULL
              );
              )"_zv);
    // ... создать другие таблицы

    work.exec(R"(CREATE TABLE IF NOT EXISTS books (
                id UUID CONSTRAINT book_id_constraint PRIMARY KEY,
                author_id UUID NOT NULL,
                title varchar(100) NOT NULL,
                publication_year INTEGER NOT NULL,
                CONSTRAINT fk_authors
                            FOREIGN KEY(author_id)
                            REFERENCES authors(id)
                );
                )"_zv);
    
    work.exec(R"(CREATE TABLE IF NOT EXISTS book_tags (
                book_id UUID NOT NULL,
                tag varchar(30) NOT NULL
                );
                )"_zv);
    // коммитим изменения
    work.commit();
}

} // namespace postgres