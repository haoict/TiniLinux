#include "../include/AnsFilter.hpp"
#include <regex>


std::string strip_ansi_sequences(const std::string& input) {
    static const std::regex ansi_escape("\x1B\\[[0-9;?]*[a-zA-Z]");
    return std::regex_replace(input, ansi_escape, "");
}

std::string strip_osc_sequences(const std::string& input) {
    std::string output;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\033' && i + 1 < input.size() && input[i + 1] == ']') {
            i += 2;
            while (i < input.size() && input[i] != '\a' && !(input[i] == '\033' && input[i + 1] == '\\')) {
                ++i;
            }
            if (i < input.size() && input[i] == '\a') ++i;
            else if (i + 1 < input.size() && input[i] == '\033' && input[i + 1] == '\\') i += 2;
        } else {
            output += input[i++];
        }
    }
    return output;
}