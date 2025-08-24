#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include "command_parser.h"
#include "cache_builder.h"
#include "cache.h"
#include "eviction_thread.h"

enum class Command {
    EXIT,
    SET,
    GET,
    REPLAY,
    UNKNOWN
};

Command getCommand(const std::string& cmd) {
    if (cmd == "EXIT") return Command::EXIT;
    if (cmd == "SET") return Command::SET;
    if (cmd == "GET") return Command::GET;
    if (cmd == "REPLAY") return Command::REPLAY;
    return Command::UNKNOWN;
}

/*
 * Core runtime REPL loop for the engine.
 */
int main() {
    streamcache::Cache cache(1);

    while(true) {
        std::cout << "> " << std::flush;

        std::string input {};
        std::getline(std::cin, input);

        auto tokens {util::parse(input)};
        if (tokens.empty()) {
            continue;
        }       

        switch (getCommand(tokens[0])) {
            case Command::EXIT:
                break;

            case Command::SET: {
                auto entry {util::buildCacheEntry(tokens)};
                if (!entry) {
                    std::cout << "Usage: SET <key> <value> <metadata>\n";
                    continue;
                }

                cache.set(tokens[1], *entry);
                break;
            }

            case Command::GET:
                if (tokens.size() != 2) {
                    std::cout << "Usage: GET <key>\n";
                    continue;
                }

                {
                    auto value {cache.get(tokens[1])};
                    if (!value) {
                        std::cout << "Key not found.\n";
                    } else {
                        std::cout << "Value: " << *value << "\n";
                    }
                }
                break;

            case Command::REPLAY:
                if (tokens.size() != 2) {
                    std::cout << "Usaged: REPLAY <key>\n";
                    continue;
                }

                cache.replay(tokens[1]);
                break;

            default:
                std::cout << "Invalid command: " << tokens[0] << "\n";
                break;
        }

        if (getCommand(tokens[0]) == Command::EXIT) {
            break;
        }
    }

    return 0;
}