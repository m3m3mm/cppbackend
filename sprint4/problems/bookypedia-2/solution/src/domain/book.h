#pragma once

#include <set>
#include <string>
#include <iosfwd>
#include <iostream>
#include <vector>

#include "../util/tagged_uuid.h"
#include "author.h"

namespace domain {
struct BookInfo {
    std::string id;
    std::string title;
    std::string author_name;
    int publication_year;
};
    
using BooksInfo = std::vector<BookInfo>;
using BookId = util::TaggedUUID<class BookTag>;

class Book {
  public:
    Book(BookId id, AuthorId author_id, std::string title, int publication_year, std::set<std::string> tags) 
        : tags_(std::move(tags))
        , id_(std::move(id))
        , author_id_(std::move(author_id))
        , title_(std::move(title))
        , publication_year_(std::move(publication_year)) {
    }

    void SetTitle(const std::string& title) {
      title_ = title;
    }

    void SetPublicationYear(int year) {
      publication_year_ = year;
    }

    void SetTags(const std::set<std::string> tags) {
      tags_ = tags;
    }

    const std::set<std::string>& GetTags() const {
        return tags_;
    }

    const BookId& GetId() const noexcept {
        return id_;
    }

    const AuthorId& GetAuthorId() const noexcept {
        return author_id_;
    }

    const std::string& GetTitle() const noexcept {
        return title_;
    }

    const int GetPublicationYear() const noexcept {
        return publication_year_;
    }

  private:
    std::set<std::string> tags_;
    BookId id_;
    AuthorId author_id_;
    std::string title_;
    int publication_year_;
};

using Books = std::vector<Book>;

class BookRepository {
  public:
    virtual void Save(const Book& book) = 0;

    virtual BooksInfo FindBooksByTitle(const std::string& title) = 0;
    virtual Book FindBook(const std::string& id) = 0; 
    
    virtual std::set<std::string> GetTags(const std::string& id) const = 0;
    virtual BooksInfo GetBooks() const = 0;
    virtual BooksInfo GetBooksInfoByAuthorId(const std::string& id) const = 0;
    virtual Books GetBooksByAuthorId(const std::string& id) const  = 0;

    virtual void DeleteBook(const std::string& book_id) = 0;
    virtual void DeleteBookTags(const std::string& book_id) = 0;
  protected:
    ~BookRepository() = default;
};


std::ostream& operator<<(std::ostream& out, const BookInfo& book);
std::ostream& operator<<(std::ostream& out, const Book& book);

}  // namespace domain