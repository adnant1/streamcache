#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "command_parser.h"

/*
 * Core runtime REPL loop for the engine.
 */
int main() {

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
    }

    return 0;
}