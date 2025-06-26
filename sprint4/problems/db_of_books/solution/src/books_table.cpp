#include "books_table.h"

using namespace std::literals;

// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;
using std::string;

constexpr auto tag_ins_book = "ins_book"_zv;

BooksTable::BooksTable(std::string_view connection) : conn_(connection.data()) {                
    pqxx::work w(conn_);
    w.exec("CREATE TABLE IF NOT EXISTS books (id SERIAL PRIMARY KEY, "_zv
                                             "title varchar(100) NOT NULL, "_zv
                                             "author varchar(100) NOT NULL, "_zv
                                             "year integer NOT NULL, "_zv
                                             "ISBN char(13) UNIQUE);"_zv);
    
    w.commit();

    PrepareAddBookCommand(tag_ins_book);
}

std::optional<string> BooksTable::HanldeRequest(std::string_view request) {    
    auto req_info = ComputeRequestInfo(request);

    switch (req_info.type) {
        case RequestType::ADD_BOOK :
            return TryAddBook(*req_info.payload);

        case RequestType::ALL_BOOKS :
            return TryGetBooks();

        default:
            return std::nullopt;
    }
}

void BooksTable::PrepareAddBookCommand(const pqxx::zview& tag) {
    conn_.prepare(tag, "INSERT INTO books (title, author, year, ISBN) VALUES ($1, $2, $3, $4)"_zv);
}

string BooksTable::TryAddBook(const Book& book) {
    try{
        pqxx::work w(conn_);
        w.exec_prepared(tag_ins_book, book.title, book.author, book.year, book.ISBN);
        w.commit();
    } catch (...) {
        return MakeResponceJSON(false);
    }
    
    return MakeResponceJSON(true);
}

string BooksTable::TryGetBooks() {
    pqxx::read_transaction r(conn_);

    const auto query_text = "SELECT * FROM books ORDER BY year DESC, title, author, ISBN"_zv;
    std::vector<Book> books;

    for (const auto& [id, title, author, year, isbn] :r.query<size_t, string, string, int,
                                                              std::optional<string>>(query_text)) {
        books.emplace_back(id, title, author, year, isbn);
    }

    return MakeResponceJSON(books);   
};
