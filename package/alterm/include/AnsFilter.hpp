#pragma once
#include <string>
#include <vector>
#include <SDL2/SDL.h>

// Structure to represent a colored character
struct ColoredChar {
    char character;
    SDL_Color fg_color;  // foreground color
    SDL_Color bg_color;  // background color
    bool bold;
    bool italic;
    bool underline;
    
    ColoredChar(char c = ' ') : character(c), 
                               fg_color({255, 255, 255, 255}),  // default white
                               bg_color({0, 0, 0, 0}),          // default transparent
                               bold(false), italic(false), underline(false) {}
};

// Structure to represent a line of colored text
using ColoredLine = std::vector<ColoredChar>;

std::string strip_ansi_sequences(const std::string& input);
std::string strip_osc_sequences(const std::string& input);
ColoredLine parse_ansi_sequences(const std::string& input);