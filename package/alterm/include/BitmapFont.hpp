#pragma once
#ifndef __BITMAPFONT_HPP__
#define __BITMAPFONT_HPP__

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

class BitmapFont {
private:
    static const unsigned char embedded_font[];
    SDL_Renderer* renderer;
    SDL_Color color;
    int scale;
    
public:
    BitmapFont();
    ~BitmapFont();
    
    void initialize(SDL_Renderer* renderer, int scale_factor = 1);
    void set_color(Uint8 r, Uint8 g, Uint8 b);
    void set_scale(int scale_factor);
    
    // Create a texture for a single character
    SDL_Texture* create_char_texture(char c);
    
    // Get character dimensions (scaled)
    int get_char_width() const { return 6 * scale; }
    int get_char_height() const { return 8 * scale; }
};

#endif // __BITMAPFONT_HPP__