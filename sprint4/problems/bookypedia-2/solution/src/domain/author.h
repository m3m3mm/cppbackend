#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../util/tagged_uuid.h"

namespace domain {

namespace detail {
struct AuthorTag {};
}  // namespace detail

using AuthorId = util::TaggedUUID<detail::AuthorTag>;

class Author {
public:
    Author(AuthorId id, std::string name)
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const AuthorId& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

private:
    AuthorId id_;
    std::string name_;
};

using Authors = std::vector<Author>;

class AuthorRepository {
public:
    virtual void Save(const Author& author) = 0;

    virtual std::optional<std::string> FindAuthorByName(const std::string& name) = 0;
    virtual std::optional<std::string> FindAuthorById(const std::string& id) = 0;

    virtual Authors GetAuthors() const = 0;

    virtual void DeleteAuthor(const std::string& id) = 0;
protected:
    ~AuthorRepository() = default;
};

}  // namespace domain
