#include "../include/AnsFilter.hpp"

#include <algorithm>
#include <cctype>
#include <regex>

std::string strip_ansi_sequences(const std::string& input) {
    static const std::regex ansi_escape("\x1B\\[[0-9;?]*[a-zA-Z]");
    return std::regex_replace(input, ansi_escape, "");
}

std::string strip_osc_sequences(const std::string& input) {
    std::string output;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\033' && i + 1 < input.size()) {
            if (input[i + 1] == ']') {
                // OSC sequence \033]...
                i += 2;
                while (i < input.size() && input[i] != '\a' && !(input[i] == '\033' && input[i + 1] == '\\')) {
                    ++i;
                }
                if (i < input.size() && input[i] == '\a')
                    ++i;
                else if (i + 1 < input.size() && input[i] == '\033' && input[i + 1] == '\\')
                    i += 2;
            } else if (input[i + 1] == '(') {
                // Charset selection sequence \033(B, \033(0, etc.
                i += 2;
                if (i < input.size()) {
                    i++;  // Skip the charset character
                }
            } else if (input[i + 1] == '[') {
                // CSI sequence \033[...
                i += 2;
                std::string sequence;
                // Collect the sequence parameters
                while (i < input.size() && ((input[i] >= '0' && input[i] <= '9') || input[i] == ';' || input[i] == '?' || input[i] == '!' || input[i] == '=' || input[i] == '>' || input[i] == '<' || input[i] == ' ')) {
                    sequence += input[i];
                    ++i;
                }
                // Get the command character
                if (i < input.size() && ((input[i] >= 'A' && input[i] <= 'Z') || (input[i] >= 'a' && input[i] <= 'z'))) {
                    char command = input[i];
                    i++;

                    // Convert some positioning commands to newlines to preserve structure
                    if (command == 'd' || command == 'H') {
                        // Cursor positioning - add newline to simulate line break
                        if (!output.empty() && output.back() != '\n') {
                            output += '\n';
                        }
                    }
                    // For other commands (colors, etc.), just strip them
                }
            } else {
                // Not a sequence we handle, keep the character
                output += input[i++];
            }
        } else {
            output += input[i++];
        }
    }
    return output;
}

// Convert ANSI color codes to SDL_Color
SDL_Color ansi_to_color(int color_code, bool bright = false) {
    SDL_Color colors[8] = {
        {0, 0, 0, 255},       // 0: Black
        {128, 0, 0, 255},     // 1: Red
        {0, 128, 0, 255},     // 2: Green
        {128, 128, 0, 255},   // 3: Yellow
        {0, 0, 128, 255},     // 4: Blue
        {128, 0, 128, 255},   // 5: Magenta
        {0, 128, 128, 255},   // 6: Cyan
        {192, 192, 192, 255}  // 7: White
    };

    SDL_Color bright_colors[8] = {
        {64, 64, 64, 255},    // 0: Bright Black (Gray)
        {255, 0, 0, 255},     // 1: Bright Red
        {0, 255, 0, 255},     // 2: Bright Green
        {255, 255, 0, 255},   // 3: Bright Yellow
        {0, 0, 255, 255},     // 4: Bright Blue
        {255, 0, 255, 255},   // 5: Bright Magenta
        {0, 255, 255, 255},   // 6: Bright Cyan
        {255, 255, 255, 255}  // 7: Bright White
    };

    if (color_code >= 0 && color_code <= 7) {
        return bright ? bright_colors[color_code] : colors[color_code];
    }

    return {255, 255, 255, 255};  // Default white
}

ColoredLine parse_ansi_sequences(const std::string& input) {
    ColoredLine result;
    SDL_Color current_fg = {255, 255, 255, 255};  // Default white
    SDL_Color current_bg = {0, 0, 0, 0};          // Default transparent
    bool bold = false, italic = false, underline = false;

    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '\033' && i + 1 < input.size() && input[i + 1] == '[') {
            // Parse ANSI escape sequence
            i += 2;  // Skip '\033['
            std::string sequence;

            // Read until we hit the command character
            // Allow more characters in sequences: digits, semicolons, question marks, etc.
            while (i < input.size() && (std::isdigit(input[i]) || input[i] == ';' || input[i] == '?' || input[i] == '=' || input[i] == '>' || input[i] == '<')) {
                sequence += input[i++];
            }

            if (i < input.size()) {
                char command = input[i++];

                if (command == 'm') {  // Color/style command
                    if (sequence.empty() || sequence == "0") {
                        // Reset to defaults
                        current_fg = {255, 255, 255, 255};
                        current_bg = {0, 0, 0, 0};
                        bold = italic = underline = false;
                    } else {
                        // Parse color codes
                        size_t pos = 0;
                        while (pos < sequence.size()) {
                            size_t next_pos = sequence.find(';', pos);
                            if (next_pos == std::string::npos) next_pos = sequence.size();

                            std::string code_str = sequence.substr(pos, next_pos - pos);
                            if (!code_str.empty()) {
                                int code = std::stoi(code_str);

                                if (code == 1)
                                    bold = true;
                                else if (code == 3)
                                    italic = true;
                                else if (code == 4)
                                    underline = true;
                                else if (code == 22)
                                    bold = false;
                                else if (code == 23)
                                    italic = false;
                                else if (code == 24)
                                    underline = false;
                                else if (code >= 30 && code <= 37) {
                                    current_fg = ansi_to_color(code - 30, bold);
                                } else if (code >= 90 && code <= 97) {
                                    current_fg = ansi_to_color(code - 90, true);
                                } else if (code >= 40 && code <= 47) {
                                    current_bg = ansi_to_color(code - 40, false);
                                } else if (code >= 100 && code <= 107) {
                                    current_bg = ansi_to_color(code - 100, true);
                                }
                            }

                            pos = next_pos + 1;
                        }
                    }
                } else {
                    // Handle other terminal control sequences by consuming them
                    // (don't add them as visible text)
                    // Common sequences:
                    // A,B,C,D (cursor movement), H,f (cursor position),
                    // J,K (erase), h,l (mode setting), etc.
                    // Just consume the sequence without processing it for display
                }
            }
        } else {
            // Regular character - add with current formatting
            ColoredChar colored_char(input[i]);
            colored_char.fg_color = current_fg;
            colored_char.bg_color = current_bg;
            colored_char.bold = bold;
            colored_char.italic = italic;
            colored_char.underline = underline;
            result.push_back(colored_char);
            i++;
        }
    }

    return result;
}