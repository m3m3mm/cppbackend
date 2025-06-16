#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""s) == ""s);
    BOOST_TEST(UrlDecode("Hello world!"s) == "Hello world!"s);
    BOOST_TEST(UrlDecode("Hello%20world%21"s) == "Hello world!"s);
    BOOST_TEST(
        UrlDecode(
            "%20%21%22%23%24%25%26%27%28%29%2a%2B%2c%2F%3a%3B%3d%3F%40%5b%5D"s
        ) == " !\"#$%&'()*+,/:;=?@[]"s
    );
    BOOST_TEST(UrlDecode("hello+world"s) == "hello world"s);
    BOOST_CHECK_THROW(UrlDecode("%"s), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%1"s), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%1Z"s), std::invalid_argument);
    BOOST_CHECK_THROW(UrlDecode("%ZZ"s), std::invalid_argument);

}