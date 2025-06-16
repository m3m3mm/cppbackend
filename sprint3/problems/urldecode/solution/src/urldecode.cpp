#include "urldecode.h"

#include <charconv>
#include <stdexcept>

const static int VALUE_SIZE = 2;

std::string UrlDecode(std::string encoded_url) {
    std::string decoded_url = "";
    size_t pos = 0;
    
    while (pos < encoded_url.length()) {
        if(size_t next_pos; (next_pos = encoded_url.find('%', pos)) != encoded_url.npos
                         || (next_pos = encoded_url.find('+', pos)) != encoded_url.npos) {
                            
            if(encoded_url[next_pos] == '%'){
                decoded_url += encoded_url.substr(pos, next_pos - pos);

                std::string value = encoded_url.substr(next_pos + 1, VALUE_SIZE);
                if(value.size() < VALUE_SIZE) {
                    throw std::invalid_argument("Encoded character must be of the form %XY");
                }

                int number = 0;
                const auto [ptr, ec] = std::from_chars(value.begin().base(), value.end().base(), number, 16);
                if(ec != std::errc() || ptr - value.begin().base() != VALUE_SIZE) {
                     throw std::invalid_argument("Encoded character must contain only hexadecimal values");

                }
                decoded_url += char(number);
                pos = next_pos + 3;
            }

            if(encoded_url[next_pos] == '+') {
                decoded_url += encoded_url.substr(pos, next_pos - pos);
                decoded_url += " ";
                pos = next_pos + 1;
            }
        } else {
            decoded_url += encoded_url.substr(pos);
            break;
        }
    }

    return decoded_url;
}
