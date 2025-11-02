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
#include "FontManagar.hpp"
#include "forkpty.hpp"



class alterm : protected FontManagar{

    private:

    SDL_Window* pWindow = nullptr;
    SDL_Renderer* pRenderer = nullptr;
    TTF_Font* Font = nullptr;
    FontManagar* FontManagarInstance;
    SDL_Color Color;

    int y;

    int CursorX = 0;
    int CursorH = 0;
    
    std::unordered_map<char, SDL_Texture*> CharTextureChache;


    public:

    int ScrollOffSet = 0;
    std::vector<std::string> lines;



    bool initialize_window();
    void reset_y(){this->y = 0;}
    void enter_cursor_reset_x(){this->CursorX = 0;}
    void renderer_screen(std::string& input_buffer, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool ShowCursor);
    void setup_font(const Uint8 r, const Uint8 g, const Uint8 b, const char* path, const int size,const char* text);
    void reset_display();
    void render_background(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void shutdown_sdl();
    SDL_Texture* get_cached_texture(char c);
   

    void trim_lines(size_t MaxLines, alterm* term){
        if(lines.size() > MaxLines){
            size_t to_remove = lines.size() - MaxLines;
            lines.erase(lines.begin(), lines.begin() + to_remove);
            term->ScrollOffSet = 0;
        }
    }

    

};


#endif //__ALTERM_HPP__