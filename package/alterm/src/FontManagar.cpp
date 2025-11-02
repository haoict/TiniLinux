#include "../include/FontManagar.hpp"
#include <SDL2/SDL_render.h>


void FontManagar::Color(Uint8 r, Uint8 g, Uint8 b) {
    FontColor = {r, g, b};
}

void FontManagar::fontFamily(const char* path, const int size) {
    Font = TTF_OpenFont(path, size);

    if(!Font){
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        exit(1);
    }

}

void FontManagar::free(){
    TTF_CloseFont(Font);
}