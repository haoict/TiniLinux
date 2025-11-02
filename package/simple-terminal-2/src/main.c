/*
 * Simple Terminal 2 - Modern SDL2-based terminal emulator
 * Built from scratch for embedded systems
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pty.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "terminal.h"
#include "vkeyboard.h"
#include "vt100.h"

// Global application state
static struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    Terminal terminal;
    VKeyboard keyboard;
    int running;
    int keyboard_visible;
    int font_width, font_height;
} app = {0};

// Screen dimensions based on platform
#ifdef RGB30
    #define SCREEN_WIDTH 720
    #define SCREEN_HEIGHT 720
#elif defined(H700)
    #define SCREEN_WIDTH 640
    #define SCREEN_HEIGHT 480
#elif defined(PC)
    #define SCREEN_WIDTH 1024
    #define SCREEN_HEIGHT 768
#else
    #define SCREEN_WIDTH 800
    #define SCREEN_HEIGHT 600
#endif

#define FONT_SIZE 16

// Initialize SDL2 and create window
static int init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return -1;
    }

    app.window = SDL_CreateWindow("Simple Terminal 2",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                  SCREEN_WIDTH, SCREEN_HEIGHT,
                                  SDL_WINDOW_SHOWN);
    if (!app.window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    app.renderer = SDL_CreateRenderer(app.window, -1, 
                                      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!app.renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(app.window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    return 0;
}

// Load font and calculate dimensions
static int init_font(void) {
    // Try to load a monospace font
    const char *font_paths[] = {
        "/usr/share/fonts/Fiery_Turk.ttf",
        "/usr/share/fonts/SourceCodePro-Regular.ttf",
        "/usr/share/fonts/FreeSans.ttf",
        "/usr/share/fonts/Ubuntu.ttf",
        NULL
    };

    printf("Attempting to load fonts...\n");
    for (int i = 0; font_paths[i]; i++) {
        printf("Trying font: %s\n", font_paths[i]);
        app.font = TTF_OpenFont(font_paths[i], FONT_SIZE);
        if (app.font) {
            printf("Successfully loaded font: %s\n", font_paths[i]);
            break;
        } else {
            printf("Failed to load %s: %s\n", font_paths[i], TTF_GetError());
        }
    }

    if (!app.font) {
        fprintf(stderr, "Failed to load any font. TTF_Error: %s\n", TTF_GetError());
        return -1;
    }

    // Calculate character dimensions
    TTF_SizeUTF8(app.font, "M", &app.font_width, &app.font_height);
    printf("Font dimensions: %dx%d\n", app.font_width, app.font_height);

    return 0;
}

// Handle SDL events
static void handle_events(void) {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                app.running = 0;
                break;
                
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE || 
                    event.key.keysym.sym == SDLK_F1 ||
                    event.key.keysym.sym == SDLK_MENU) {
                    app.keyboard_visible = !app.keyboard_visible;
                    printf("Keyboard visibility toggled to: %s\n", app.keyboard_visible ? "visible" : "hidden");
                } else if (app.keyboard_visible) {
                    vkeyboard_handle_key(&app.keyboard, &event.key);
                } else {
                    terminal_handle_key(&app.terminal, &event.key);
                }
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (app.keyboard_visible) {
                    vkeyboard_handle_mouse(&app.keyboard, event.button.x, event.button.y);
                }
                break;
                
            default:
                break;
        }
    }
}

// Render the application
static void render(void) {
    // Clear screen
    SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
    SDL_RenderClear(app.renderer);
    
    // Render terminal
    terminal_render(&app.terminal, app.renderer, app.font);
    
    // Render virtual keyboard if visible
    if (app.keyboard_visible) {
        vkeyboard_render(&app.keyboard, app.renderer, app.font);
    }
    
    // Present frame
    SDL_RenderPresent(app.renderer);
}

// Cleanup resources
static void cleanup(void) {
    if (app.font) {
        TTF_CloseFont(app.font);
    }
    
    terminal_cleanup(&app.terminal);
    vkeyboard_cleanup(&app.keyboard);
    
    if (app.renderer) {
        SDL_DestroyRenderer(app.renderer);
    }
    
    if (app.window) {
        SDL_DestroyWindow(app.window);
    }
    
    TTF_Quit();
    SDL_Quit();
}

// Signal handler for child process
static void sigchld_handler(int sig) {
    (void)sig;
    int status;
    waitpid(-1, &status, WNOHANG);
    app.running = 0;  // Exit when shell exits
}

int main(void) {
    printf("Starting Simple Terminal 2...\n");
    
    // Initialize SDL
    if (init_sdl() < 0) {
        return 1;
    }
    
    // Initialize font
    if (init_font() < 0) {
        cleanup();
        return 1;
    }
    
    // Calculate terminal dimensions
    int term_cols = SCREEN_WIDTH / app.font_width;
    int term_rows = (SCREEN_HEIGHT * 2 / 3) / app.font_height;  // Leave space for keyboard
    
    printf("Terminal size: %dx%d\n", term_cols, term_rows);
    
    // Initialize terminal
    if (terminal_init(&app.terminal, term_cols, term_rows) < 0) {
        cleanup();
        return 1;
    }
    
    // Initialize virtual keyboard
    if (vkeyboard_init(&app.keyboard, SCREEN_WIDTH, SCREEN_HEIGHT / 3, 
                       0, SCREEN_HEIGHT * 2 / 3) < 0) {
        cleanup();
        return 1;
    }
    
    // Set up signal handler
    signal(SIGCHLD, sigchld_handler);
    
    app.running = 1;
    app.keyboard_visible = 0;
    
    printf("Entering main loop...\n");
    
    // Main loop
    while (app.running) {
        handle_events();
        terminal_update(&app.terminal);
        render();
        
        SDL_Delay(16);  // ~60 FPS
    }
    
    printf("Exited main loop\n");
    
    cleanup();
    return 0;
}