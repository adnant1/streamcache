#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "command_parser.h"
#include "cache.h"

/*
 * Core runtime REPL loop for the engine.
 */
int main() {
    streamcache::Cache cache;

    while(true) {
        std::cout << "> " << std::flush;

        std::string input{};
        std::getline(std::cin, input);

        auto tokens{ command_parser::parse(input) };
        if (tokens.empty()) {
            continue;
        }       

        if (tokens[0] == "EXIT") {
            break;
        }

        if (tokens[0] == "SET") {
            if (tokens.size() != 3) {
                std::cout << "Usage: SET <key> <value>\n";
                continue;
            }
            cache.set(tokens[1], tokens[2]);

        } else if (tokens[0] == "GET") {
            if (tokens.size() != 2) {
                std::cout << "Usage: GET <key>\n";
                continue;
            }
            std::string value = cache.get(tokens[1]);
            if (value.empty()) {
                std::cout << "Key not found.\n";
            } else {
                std::cout << "Value: " << value << "\n";
            }
        } else {
            std::cout << "Invalid command: " << tokens[0] << "\n";
        }
    }

    return 0;
}