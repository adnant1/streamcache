#pragma once
#include <string>
#include <vector>

namespace util {
    /**
     * Parses a command string into a vector of tokens.
     *
     * @param input The command string to parse.
     * @return A vector of strings representing the tokens in the command.
     */
    std::vector<std::string> parse(const std::string& input);
}