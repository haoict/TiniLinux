/*
 * Terminal interface - handles PTY and terminal emulation
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define MAX_COLS 200
#define MAX_ROWS 100
#define GLYPH_CACHE_SIZE 256

// Glyph cache entry
typedef struct {
    SDL_Texture *texture;
    int width, height;
    int valid;
} GlyphCache;

// Terminal character cell
typedef struct {
    char ch;
    SDL_Color fg_color;
    SDL_Color bg_color;
    int bold;
    int reverse;
} TermCell;

// Terminal state
typedef struct {
    TermCell grid[MAX_ROWS][MAX_COLS];
    int cols, rows;
    int cursor_x, cursor_y;
    int cursor_visible;
    SDL_Color default_fg;
    SDL_Color default_bg;
    
    // Glyph cache for performance
    GlyphCache glyph_cache[GLYPH_CACHE_SIZE];
    
    // PTY file descriptors
    int master_fd;
    int slave_fd;
    pid_t child_pid;
    
    // Input/output buffers
    char input_buffer[4096];
    int input_len;
    char output_buffer[4096];
    int output_len;
    
    // VT100 parser state
    int escape_state;
    char escape_buffer[256];
    int escape_len;
} Terminal;

// Function declarations
int terminal_init(Terminal *term, int cols, int rows);
void terminal_cleanup(Terminal *term);
void terminal_update(Terminal *term);
void terminal_render(Terminal *term, SDL_Renderer *renderer, TTF_Font *font);
void terminal_handle_key(Terminal *term, SDL_KeyboardEvent *key);
void terminal_write(Terminal *term, const char *data, int len);

#endif // TERMINAL_H