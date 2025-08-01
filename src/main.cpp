#include <iostream>
#include <string>
#include <sstream>
#include <vector>

/*
 * Core runtime REPL loop for the engine.
 */
int main() {

    while(true) {
        std::cout << "> " << std::flush;

        std::string input;
        std::getline(std::cin, input);

        std::istringstream iss(input);
        std::string token;
        std::vector<std::string> tokens;

        while (iss >> token) {
            tokens.push_back(token);
        }

        if (input == "EXIT") {
            break;
        }
    }

    return 0;
}