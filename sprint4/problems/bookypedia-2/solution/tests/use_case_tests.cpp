#include <catch2/catch_test_macros.hpp>

#include "../src/app/unit_of_work.h"
#include "../src/app/use_cases_impl.h"
#include "../src/domain/author.h"

namespace {

struct MockAuthorRepository : domain::AuthorRepository {
    domain::Authors saved_authors;

    void Save(const domain::Author& author) override {
        saved_authors.emplace_back(author);
    }

    domain::Authors GetAuthors() const override {
        return saved_authors;
    }

    std::optional<std::string> FindAuthorByName(const std::string& name) override {
        return std::nullopt;
    }

    std::optional<std::string> FindAuthorById(const std::string& name) override {
        return std::nullopt;
    }

    void DeleteAuthor(const std::string& id) override {
        return;
    }
};

struct MockBookRepository : domain::BookRepository {
    domain::Books saved_books;

    void Save(const domain::Book& book) override {
        saved_books.emplace_back(book);
    }

    domain::BooksInfo FindBooksByTitle(const std::string& title) override {
        return {};
    }

    domain::Book FindBook(const std::string& id) override {
        return {domain::BookId::New(), domain::AuthorId::New(), "title", 2025, {}};
    }

    std::set<std::string> GetTags(const std::string& id) const override {
        return {};
    }

    domain::BooksInfo GetBooks() const override {
        return {};
    }

    domain::BooksInfo GetBooksInfoByAuthorId(const std::string& id) const override {
        return {};
    }

    domain::Books GetBooksByAuthorId(const std::string& id) const override {
        assert("TODO: implement");
        return {};
    }

    void DeleteBook(const std::string& book_id) override {
        return;
    }

    void DeleteBookTags(const std::string& book_id) override {
        return;
    }
};

struct Fixture {
    MockAuthorRepository authors;
    MockBookRepository books;
};

}  // namespace

/*SCENARIO_METHOD(Fixture, "Book Adding") {
    GIVEN("Use cases") {
        app::UseCasesImpl use_cases{authors, books};

        WHEN("Adding an author") {
            const auto author_name = "Joanne Rowling";
            use_cases.AddAuthor(author_name);

            THEN("author with the specified name is saved to repository") {
                REQUIRE(authors.saved_authors.size() == 1);
                CHECK(authors.saved_authors.at(0).GetName() == author_name);
                CHECK(authors.saved_authors.at(0).GetId() != domain::AuthorId{});
            }
        }
    }
}*/