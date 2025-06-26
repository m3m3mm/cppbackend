#include <pqxx/pqxx>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "model.h"
#include "json.h"

class BooksTable {
public:
    BooksTable(std::string_view connection);

    std::optional<std::string> HanldeRequest(std::string_view request);
private:
    pqxx::connection conn_;

    void PrepareAddBookCommand(const pqxx::zview& tag);
    std::string TryAddBook(const Book& book);
    std::string TryGetBooks();
};