#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "books_table.h"

int main(int argc, const char* argv[]) {
    try {
        if (argc == 1) {
            std::cout << "Usage: db_example <conn-string>\n";
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n";
            return EXIT_FAILURE;
        }

        BooksTable books_table(argv[1]);

        std::string line = "";
        while(std::getline(std::cin, line)) {
            auto responce = books_table.HanldeRequest(line);
            if(!responce) {
                break;
            }
            std::cout << *responce << std::endl;
            std::cin >> std::ws;
        }
        
        
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}