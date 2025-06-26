#include "json.h"

namespace json = boost::json;

using namespace std::literals;

using std::string_view;

//reqest headers
const static boost::string_view ACTION = "action";
const static boost::string_view PAYLOAD = "payload";

//reqest type string
const static string_view ADD = "add_book";
const static string_view ALL = "all_books";
const static string_view EXIT = "exit";

//responce headers
const static boost::string_view RESULT = "result";

//book headers
const static boost::string_view ID = "id";
const static boost::string_view TITLE = "title";
const static boost::string_view AUTHOR = "author";
const static boost::string_view YEAR = "year";
const static boost::string_view ISBN = "ISBN";

Book ParseBookData(json::object book_obj) {
    if(book_obj.empty()){
        throw std::invalid_argument("Invalid book information"s);
    }

    Book book{.title = std::string(book_obj.at(TITLE).as_string()),
              .author = std::string(book_obj.at(AUTHOR).as_string()),
              .year = book_obj.at(YEAR).as_int64()};

    if(auto id = book_obj.find(ID); id != book_obj.end()) {
        book.id = id->value().as_int64();
    }

    if(auto isbn = book_obj.find(ISBN); isbn != book_obj.end() 
                                        && isbn->value().is_string()) {
        book.ISBN = isbn->value().as_string();
    }

    return book;
} 

RequestInfo ComputeRequestInfo(string_view request) {
    json::object req_obj = json::parse(boost::string_view(request.data())).as_object();

    string_view req_type_str = req_obj.at(ACTION.data()).as_string();
    RequestType req_type;

    if(req_type_str == ADD) {
        req_type = RequestType::ADD_BOOK;
        return {req_type, ParseBookData(req_obj.at(PAYLOAD.data()).as_object())};
    } else if(req_type_str == ALL) {
        req_type = RequestType::ALL_BOOKS;
        return {req_type, std::nullopt};
    }

    req_type = RequestType::EXIT;
    return {req_type, std::nullopt};
}

std::string MakeResponceJSON(bool is_success) {
    json::object result;
    result[RESULT] = is_success;

    return json::serialize(result);
}

std::string MakeResponceJSON(const std::vector<Book>& books) {
    json::array result;
    json::object book_obj;

    for(const auto& book : books) {
        book_obj[ID] = book.id;
        book_obj[TITLE] = book.title;
        book_obj[AUTHOR] = book.author;
        book_obj[YEAR] = book.year;
        if(book.ISBN) {
            book_obj[ISBN] = *book.ISBN;
        } else {
            book_obj[ISBN] = nullptr;
        }
        
        result.push_back(book_obj);
    }

    return json::serialize(result);
}
