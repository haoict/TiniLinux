#pragma once
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#ifndef __SETFONT_HPP__
#define __SETFONT_HPP__
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>
#include <iostream>




class FontManagar{

    public:

    TTF_Font* Font;
    SDL_Color FontColor;



    void Color(Uint8 r, Uint8 g, Uint8 b);
    void fontFamily(const char* path, const int size);
    SDL_Texture* textTexture(const char* text, SDL_Renderer* renderer);
    void free();

    TTF_Font* getFont(){return(Font);}


};


#endif 