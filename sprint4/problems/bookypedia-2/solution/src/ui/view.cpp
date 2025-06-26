#include "view.h"

#include <boost/algorithm/string/trim.hpp>

#include <cassert>
#include <iostream>
#include <set>

#include "../app/use_cases.h"
#include "../menu/menu.h"
#include "../util/trimer.h"

using namespace std::literals;
using namespace domain;

namespace ph = std::placeholders;

namespace ui {
namespace detail {

inline std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}
}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

void PrintBook(std::ostream& out, const domain::BookInfo& book_info, const std::set<std::string>& tags) {
    out << "Title: " << book_info.title << std::endl
        << "Author: " << book_info.author_name << std::endl
        << "Publication year: " << book_info.publication_year << std::endl;
    
    if(!tags.empty()) {
        out << "Tags: ";

        bool it_first = true;
        for(const auto& tag : tags) {
            if(it_first) {
                out << tag;
                it_first = false;
                continue;
            }

            out << ", " << tag;
        }

        out << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction(  //
        "AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1)
        // [this](auto& cmd_input) { return AddAuthor(cmd_input); }
    );
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowBook"s, {}, "Show book info"s, std::bind(&View::ShowBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));

    menu_.AddAction("EditAuthor"s, "<name>", "Edit author"s, std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("EditBook"s, {}, "Edit book title and/or publishing year"s, std::bind(&View::EditBook, this, ph::_1));

    menu_.AddAction("DeleteAuthor"s, "<name>"s, "Delete author"s, std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, {}, "Delete book", std::bind(&View::DeleteBook, this, ph::_1)); 
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);

        if(name.empty()) {
            throw std::invalid_argument("");
        }
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input)) {

            use_cases_.AddBook(domain::AuthorId::FromString(std::move(params->author_id)),
                               std::move(params->title),
                               params->publication_year,
                               params->tags);
        }
    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }

    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    std::string title = "";
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);

    std::optional<domain::BookInfo> book;
    if(title.empty()) {
        book = SelectBook();
        if(!book) {
            throw std::runtime_error("");
        }
        PrintBook(output_, *book, use_cases_.GetTags(book->id));

        return true;
    }

    book = SelectBook(use_cases_.FindBooksByTitle(title));

    if(!book) {
        throw std::runtime_error("");
    }
    PrintBook(output_, *book, use_cases_.GetTags(book->id));

    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowAuthorBooks() const {
    // TODO: handle error
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, use_cases_.GetBooksByAuthorId(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

std::optional<std::string> View::TryRequestAuthor() const {
    output_ << "Enter author name or empty line to select from list:" << std::endl;

    std::string author_name;
    std::getline(input_, author_name);
    boost::algorithm::trim(author_name);

    if(author_name.empty()) {
        return std::nullopt;
    }

    const auto author_id = use_cases_.FindAuthorByName(author_name);
    if(author_id.has_value()) {
        return author_id;
    }

    output_ << "No author found. Do you want to add " + author_name + " (y/n)?" << std::endl;

    std::string answer;
    getline(input_, answer);

    if(answer != "y" && answer != "Y") {
        throw std::logic_error("");
    }

    use_cases_.AddAuthor(author_name);
    
    return use_cases_.FindAuthorByName(author_name);
}

std::set<std::string> View::TryRequestTags(std::string_view message) const {
    output_ << message << std::endl;

    std::set<std::string> result;

    std::string tags;
    getline(input_, tags);

    if(tags.empty()) {
        return {};
    }

    size_t pos = 0;
    std::string delim = ",";

    while (pos < tags.length()) {
        auto delim_pos = tags.find(delim, pos);

        if (delim_pos == tags.npos) {
            delim_pos = tags.size();
        }

        if (auto trimming_str = util::Trim(tags.substr(pos, delim_pos - pos)); !trimming_str.empty()) {
            result.insert(trimming_str);
        }

        pos = delim_pos + delim.size();
    }

    return result;
}

bool View::EditAuthor(std::istream &cmd_input) const {
    std::string author_name = "";
    std::getline(cmd_input, author_name);
    boost::algorithm::trim(author_name);

    std::optional<std::string> author_id;

    if(author_name.empty()) {

        author_id = SelectAuthor();
        if(!author_id) {
            throw std::invalid_argument("Failed to edit author");
        }
        output_ << "Enter new name:" << std::endl;

        std::string new_name = "";
        std::getline(input_, new_name);
        boost::algorithm::trim(new_name);

        use_cases_.EditAuthor(*author_id, new_name);
        return true;
    }
    
    author_id = use_cases_.FindAuthorByName(author_name);
    if(!author_id) {
        throw std::invalid_argument("Failed to edit author");
    }

    output_ << "Enter new name:" << std::endl;

    std::string new_name = "";
    std::getline(input_, new_name);
    boost::algorithm::trim(new_name);

    use_cases_.EditAuthor(*author_id, new_name);

    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    std::string title = "";
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);

    std::optional<domain::BookInfo> book_info;
    if(title.empty()) {
        book_info = SelectBook();
        if(!book_info) {
            throw std::runtime_error("Book not found");
        }
    } else {
        book_info = SelectBook(use_cases_.FindBooksByTitle(title));
        if(!book_info) {
            throw std::runtime_error("Book not found");
        } 
    }

    auto book = use_cases_.FindBook(book_info->id);

    output_ <<"Enter new title or empty line to use the current one (" + book_info->title + "):" << std::endl;;
    std::string new_title = "";
    std::getline(input_, new_title);
    boost::algorithm::trim(new_title);

    if(!new_title.empty()) {
        book.SetTitle(new_title);
    }

    output_ << "Enter publication year or empty line to use the current one (" 
            + std::to_string(book_info->publication_year) + "):" << std::endl;;
    std::string year = "";
    std::getline(input_, year);
    boost::algorithm::trim(year);

    if(!year.empty()) {
        int new_pub_year = std::stoi(year);
        book.SetPublicationYear(new_pub_year);
    }

    std::string message = "Enter tags (";

    bool it_first = true;
    for(const auto& tag : book.GetTags()) {
        if(it_first) {
            message += tag;
            it_first = false;
            continue;
        }

        message += ", " + tag;
    }

    message += "):";

    book.SetTags(TryRequestTags(message));

    use_cases_.EditBook(book);

    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    std::optional<std::string> author_id = TryRequestAuthor();

    if(!author_id) {
        author_id = SelectAuthor();
    }

    if (!author_id)
        return std::nullopt;
    else {
        params.author_id = author_id.value();
        params.tags = TryRequestTags();
        return params;
    }
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    std::string author_name = "";
    std::getline(cmd_input, author_name);
    boost::algorithm::trim(author_name);

    std::optional<std::string> author_id;

    if(author_name.empty()) {

        author_id = SelectAuthor();
        if(!author_id) {
            throw std::invalid_argument("Failed to delete author");
        }

        use_cases_.DeleteAuthor(*author_id);
        return true;
    } 

    author_id = use_cases_.FindAuthorByName(author_name);
    if(!author_id) {
        throw std::invalid_argument("Failed to delete author");
    }

    use_cases_.DeleteAuthor(*author_id);

    return true;
}

bool View::DeleteBook(std::istream &cmd_input) const {
    std::string title = "";
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);

    std::optional<domain::BookInfo> book;
    if(title.empty()) {
        book = SelectBook();
        if(!book) {
            throw std::runtime_error("");
        }

        use_cases_.DeleteBook(book->id);

        return true;
    }

    book = SelectBook(use_cases_.FindBooksByTitle(title));

    if(!book) {
        throw std::runtime_error("");
    }
    use_cases_.DeleteBook(book->id);

    return true;
}

std::optional<std::string> View::SelectAuthor() const {
    output_ << "Select author:" << std::endl;
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid author num");
    }

    --author_idx;
    if (author_idx < 0 or author_idx >= authors.size()) {
        throw std::runtime_error("Invalid author num");
    }

    return authors[author_idx].id;
}

std::optional<domain::BookInfo> View::SelectBook() const {
    auto books = use_cases_.GetBooks();
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid book num");
    }

    return books[book_idx];
}

std::optional<domain::BookInfo> View::SelectBook(const domain::BooksInfo& books) const {
    if(books.empty()) {
        return std::nullopt;
    }

    if(books.size() == 1) {
        return books.front();
    }
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        throw std::runtime_error("Invalid book num");
    }

    --book_idx;
    if (book_idx < 0 or book_idx >= books.size()) {
        throw std::runtime_error("Invalid book num");
    }

    return books[book_idx];
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_autors;

    for (const auto& author : use_cases_.GetAuthors()) {
        dst_autors.emplace_back(author.GetId().ToString(), author.GetName());
    }

    return dst_autors;
}

domain::BooksInfo View::GetBooks() const {
    return use_cases_.GetBooks();
}

domain::BooksInfo View::GetBooksInfoByAuthorId(const std::string& author_id) const {
    return use_cases_.GetBooksInfoByAuthorId(author_id);
}

}  // namespace ui
