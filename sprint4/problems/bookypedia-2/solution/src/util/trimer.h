#pragma once 

#include <string>

namespace util {


    std::string Trim(const std::string& str) {
        const auto start = str.find_first_not_of(' ');
        if (start == str.npos) {
            return {};
        }

        auto trimming_str = str.substr(start, str.find_last_not_of(' ') + 1 - start);
        size_t space = trimming_str.find(' ');

        if(space == trimming_str.npos) {
            return trimming_str;
        } 

        size_t not_space  = trimming_str.find_first_not_of(' ', space);
        if(not_space - space > 1) {
            std::string buffer = "";
            buffer += trimming_str.substr(0,space);
            buffer += " ";
            buffer += trimming_str.substr(not_space);

            return buffer;
        }

        return trimming_str;
    }
}//namespace util