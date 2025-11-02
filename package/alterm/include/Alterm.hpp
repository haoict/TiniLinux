#pragma once
#include <cstddef>
#include <unordered_map>
#ifndef __ALTERM_HPP__
#define __ALTERM_HPP__
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_ttf.h>
#include "../include/FontManagar.hpp"
#include "../include/BitmapFont.hpp"
#include "forkpty.hpp"



class alterm : protected FontManagar{

    private:

    SDL_Window* pWindow = nullptr;
    SDL_Renderer* pRenderer = nullptr;
    TTF_Font* Font = nullptr;
    FontManagar* FontManagarInstance;
    BitmapFont* bitmap_font = nullptr;
    SDL_Color Color;
    bool use_bitmap_font = false;

    int y;

    int CursorX = 0;
    int CursorH = 0;
    
    std::unordered_map<char, SDL_Texture*> CharTextureChache;
    SDL_Color cached_font_color; // Track the color used for cached textures
    
    // Separate textures for layered rendering
    SDL_Texture* terminal_texture = nullptr;
    SDL_Texture* keyboard_texture = nullptr;
    int window_width = 0;
    int window_height = 0;


    public:

    int ScrollOffSet = 0;
    std::vector<std::string> lines;



    bool initialize_window();
    void reset_y(){this->y = 0;}
    void enter_cursor_reset_x(){this->CursorX = 0;}
    void renderer_screen(std::string& input_buffer, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool ShowCursor, bool should_present = true);
    void setup_font(const Uint8 r, const Uint8 g, const Uint8 b, const char* path, const int size,const char* text);
    void reset_display();
    void render_background(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void shutdown_sdl();
    SDL_Texture* get_cached_texture(char c);
    void clear_char_texture_cache();
    
    // Layered rendering methods
    bool create_render_textures();
    void render_terminal_to_texture(std::string& input_buffer, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool ShowCursor);
    void render_keyboard_to_texture(void* keyboard);  // Use void* to avoid circular dependency
    void composite_and_present();
    
    // Getters for virtual keyboard
    SDL_Renderer* get_renderer() { return pRenderer; }
    TTF_Font* get_font() { return Font; }
    BitmapFont* get_bitmap_font() { return bitmap_font; }
   

    void trim_lines(size_t MaxLines, alterm* term){
        if(lines.size() > MaxLines){
            size_t to_remove = lines.size() - MaxLines;
            lines.erase(lines.begin(), lines.begin() + to_remove);
            term->ScrollOffSet = 0;
        }
    }

    

};


#endif //__ALTERM_HPP__