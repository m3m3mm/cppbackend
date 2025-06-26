#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>

#include "book.h"

namespace domain {
std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << " by " << book.author_name <<  ", " << book.publication_year;
    return out;
}

std::ostream &operator<<(std::ostream& out, const Book& book) {
    out << book.GetTitle() << ", " << book.GetPublicationYear();
    return out;
}

}  // namespace domain