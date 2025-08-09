#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "command_parser.h"
#include "cache_builder.h"
#include "cache.h"

/*
 * Core runtime REPL loop for the engine.
 */
int main() {
    streamcache::Cache cache {};

    while(true) {
        cache.evictExpired();
        std::cout << "> " << std::flush;

        std::string input {};
        std::getline(std::cin, input);

        auto tokens {command_parser::parse(input)};
        if (tokens.empty()) {
            continue;
        }       

        if (tokens[0] == "EXIT") {
            break;
        }

        if (tokens[0] == "SET") {
            auto entry {cache_builder::buildCacheEntry(tokens)};
            if (!entry) {
                std::cout << "Usage: SET <key> <value> <metadata>\n";
                continue;
            }

            cache.set(tokens[1], *entry);

        } else if (tokens[0] == "GET") {
            if (tokens.size() != 2) {
                std::cout << "Usage: GET <key>\n";
                continue;
            }

            auto value {cache.get(tokens[1])};
            if (!value) {
                std::cout << "Key not found.\n";
            } else {
                std::cout << "Value: " << *value << "\n";
            }

        } else if (tokens[0] == "REPLAY") {
            if (tokens.size() != 2) {
                std::cout << "Usaged: REPLAY <key>\n";
                continue;
            }

            cache.replay(tokens[1]);

        } else {
            std::cout << "Invalid command: " << tokens[0] << "\n";
        }
    }

    return 0;
}