#include "command_parser.h"
#include <string>
#include <vector>
#include <sstream>

namespace {

    std::vector<std::string> parse(const std::string& input) {
        std::vector<std::string> tokens {};
        std::istringstream iss(input);
        std::string token {};

        while(iss >> token) {
            tokens.push_back(token);
        }

        return tokens;
    }
}
