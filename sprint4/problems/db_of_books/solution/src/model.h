#pragma once

#include <optional>
#include <string>
#include <vector>

enum class RequestType{
    ALL_BOOKS,
    ADD_BOOK,
    EXIT
};

struct Book {
    int id;
    std::string title;
    std::string author;
    int year;
    std::optional<std::string> ISBN;
};

struct RequestInfo {
    RequestType type;
    std::optional<Book> payload;
};