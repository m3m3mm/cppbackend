#include "urlencode.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

constexpr static std::string_view reserved_symbols = "!\"#$%&'()*+,/:;=?@[]";

bool AnyOf(char value) {
    return std::any_of(reserved_symbols.begin(), reserved_symbols.end(), [&](char c) {
        return c == value;
    });
}

std::string UrlEncode(std::string_view str) {
    std::stringstream ss;

    for (char chr : str) {
        int16_t symbol = chr;

        if (std::isspace(symbol)) {
            ss << '+';
        } else if (AnyOf(chr) || symbol < 32 || symbol >= 128) {
            ss << '%' << std::setfill('0') << std::setw(2) << std::hex
               << symbol;
        } else {
            ss << chr;
        }
    }

    return ss.str();
}
