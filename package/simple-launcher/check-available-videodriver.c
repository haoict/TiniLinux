#include <SDL2/SDL.h>
#include <stdio.h>

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) { printf("%s\n", SDL_GetError()); return 1; }
    int n = SDL_GetNumVideoDrivers();
    printf("Video drivers: %d\n", n);
    for (int i=0;i<n;i++) printf("%s\n", SDL_GetVideoDriver(i));
    SDL_Quit();
}
