/*
 * Terminal implementation - PTY and character grid management
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <pty.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "terminal.h"
#include "vt100.h"

// Initialize terminal
int terminal_init(Terminal *term, int cols, int rows) {
    memset(term, 0, sizeof(Terminal));
    
    term->cols = cols;
    term->rows = rows;
    term->cursor_x = 0;
    term->cursor_y = 0;
    term->cursor_visible = 1;
    
    // Set default colors
    term->default_fg = (SDL_Color){192, 192, 192, 255};  // Light gray
    term->default_bg = (SDL_Color){0, 0, 0, 255};        // Black
    
    // Initialize grid with default colors
    for (int row = 0; row < MAX_ROWS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            term->grid[row][col].ch = ' ';
            term->grid[row][col].fg_color = term->default_fg;
            term->grid[row][col].bg_color = term->default_bg;
            term->grid[row][col].bold = 0;
            term->grid[row][col].reverse = 0;
        }
    }
    
    // Initialize glyph cache
    for (int i = 0; i < GLYPH_CACHE_SIZE; i++) {
        term->glyph_cache[i].texture = NULL;
        term->glyph_cache[i].valid = 0;
        term->glyph_cache[i].width = 0;
        term->glyph_cache[i].height = 0;
    }
    
    // Create PTY
    struct winsize ws = {
        .ws_row = rows,
        .ws_col = cols,
        .ws_xpixel = 0,
        .ws_ypixel = 0
    };
    
    if (openpty(&term->master_fd, &term->slave_fd, NULL, NULL, &ws) < 0) {
        perror("openpty");
        return -1;
    }
    
    // Make master non-blocking
    int flags = fcntl(term->master_fd, F_GETFL);
    fcntl(term->master_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Fork shell process
    term->child_pid = fork();
    if (term->child_pid == 0) {
        // Child process - become shell
        close(term->master_fd);
        
        // Set up terminal
        setsid();
        ioctl(term->slave_fd, TIOCSCTTY, 0);
        
        // Redirect stdio
        dup2(term->slave_fd, STDIN_FILENO);
        dup2(term->slave_fd, STDOUT_FILENO);
        dup2(term->slave_fd, STDERR_FILENO);
        close(term->slave_fd);
        
        // Set environment
        setenv("TERM", "xterm-256color", 1);
        setenv("SHELL", "/bin/sh", 1);
        
        // Start shell
        execl("/bin/sh", "sh", NULL);
        perror("execl");
        exit(1);
    } else if (term->child_pid < 0) {
        perror("fork");
        close(term->master_fd);
        close(term->slave_fd);
        return -1;
    }
    
    // Parent - close slave fd
    close(term->slave_fd);
    
    printf("Terminal initialized: %dx%d, PTY master fd: %d, child PID: %d\n",
           cols, rows, term->master_fd, term->child_pid);
    
    // Send initial command to get some output
    usleep(100000);  // Wait 100ms for shell to start
    const char *init_cmd = "echo 'Terminal ready'\n";
    write(term->master_fd, init_cmd, strlen(init_cmd));
    
    return 0;
}

// Cleanup terminal
void terminal_cleanup(Terminal *term) {
    // Free glyph cache textures
    for (int i = 0; i < GLYPH_CACHE_SIZE; i++) {
        if (term->glyph_cache[i].texture) {
            SDL_DestroyTexture(term->glyph_cache[i].texture);
            term->glyph_cache[i].texture = NULL;
            term->glyph_cache[i].valid = 0;
        }
    }
    
    if (term->master_fd >= 0) {
        close(term->master_fd);
    }
    
    if (term->child_pid > 0) {
        kill(term->child_pid, SIGTERM);
    }
}

// Scroll terminal up one line
static void scroll_up(Terminal *term) {
    // Move all lines up
    for (int row = 0; row < term->rows - 1; row++) {
        for (int col = 0; col < term->cols; col++) {
            term->grid[row][col] = term->grid[row + 1][col];
        }
    }
    
    // Clear bottom line
    for (int col = 0; col < term->cols; col++) {
        term->grid[term->rows - 1][col].ch = ' ';
        term->grid[term->rows - 1][col].fg_color = term->default_fg;
        term->grid[term->rows - 1][col].bg_color = term->default_bg;
        term->grid[term->rows - 1][col].bold = 0;
        term->grid[term->rows - 1][col].reverse = 0;
    }
}

// Put character at current cursor position
static void put_char(Terminal *term, char ch) {

    
    if (ch == '\r') {
        term->cursor_x = 0;
        return;
    }
    
    if (ch == '\n') {
        term->cursor_x = 0;
        term->cursor_y++;
        if (term->cursor_y >= term->rows) {
            term->cursor_y = term->rows - 1;
            scroll_up(term);
        }
        return;
    }
    
    if (ch == '\b') {
        if (term->cursor_x > 0) {
            term->cursor_x--;
        }
        return;
    }
    
    if (ch == '\t') {
        int next_tab = (term->cursor_x + 8) & ~7;
        term->cursor_x = (next_tab < term->cols) ? next_tab : term->cols - 1;
        return;
    }
    
    // Regular character
    if (term->cursor_x >= 0 && term->cursor_x < term->cols &&
        term->cursor_y >= 0 && term->cursor_y < term->rows) {
        
        term->grid[term->cursor_y][term->cursor_x].ch = ch;
        term->grid[term->cursor_y][term->cursor_x].fg_color = term->default_fg;
        term->grid[term->cursor_y][term->cursor_x].bg_color = term->default_bg;
        term->grid[term->cursor_y][term->cursor_x].bold = 0;
        term->grid[term->cursor_y][term->cursor_x].reverse = 0;
        

        
        term->cursor_x++;
        if (term->cursor_x >= term->cols) {
            term->cursor_x = 0;
            term->cursor_y++;
            if (term->cursor_y >= term->rows) {
                term->cursor_y = term->rows - 1;
                scroll_up(term);
            }
        }
    }
}

// Process output from PTY
static void process_output(Terminal *term, const char *data, int len) {
    for (int i = 0; i < len; i++) {
        char ch = data[i];
        
        if (ch == '\033') {  // ESC - start escape sequence
            term->escape_state = 1;
            term->escape_len = 0;
            term->escape_buffer[term->escape_len++] = ch;
        } else if (term->escape_state) {
            // Handle escape sequence
            term->escape_buffer[term->escape_len++] = ch;
            
            if (vt100_parse_escape(term, term->escape_buffer, term->escape_len)) {
                term->escape_state = 0;
                term->escape_len = 0;
            }
        } else {
            // Regular character
            put_char(term, ch);
        }
    }
}

// Update terminal - read from PTY
void terminal_update(Terminal *term) {
    if (term->master_fd < 0) return;
    
    fd_set readfds;
    struct timeval timeout = {0, 0};  // Non-blocking
    
    FD_ZERO(&readfds);
    FD_SET(term->master_fd, &readfds);
    
    if (select(term->master_fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
        if (FD_ISSET(term->master_fd, &readfds)) {
            char buffer[1024];
            ssize_t bytes_read = read(term->master_fd, buffer, sizeof(buffer) - 1);
            
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                process_output(term, buffer, bytes_read);
            } else if (bytes_read == 0) {
                // EOF - shell exited
                term->master_fd = -1;
            }
        }
    }
}

// Get or create cached glyph
static SDL_Texture* get_cached_glyph(Terminal *term, SDL_Renderer *renderer, TTF_Font *font, char ch, SDL_Color color) {
    unsigned char index = (unsigned char)ch;
    GlyphCache *cache = &term->glyph_cache[index];
    
    // For now, we only cache with default colors to keep it simple
    // A full implementation would hash the character and color combination
    if (!cache->valid || cache->texture == NULL) {
        char text[2] = {ch, '\0'};
        SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
        
        if (surface) {
            cache->texture = SDL_CreateTextureFromSurface(renderer, surface);
            cache->width = surface->w;
            cache->height = surface->h;
            cache->valid = 1;
            SDL_FreeSurface(surface);
        }
    }
    
    return cache->texture;
}

// Render terminal
void terminal_render(Terminal *term, SDL_Renderer *renderer, TTF_Font *font) {
    if (!font) return;
    
    int font_w, font_h;
    TTF_SizeUTF8(font, "M", &font_w, &font_h);
    
    // Render character grid
    for (int row = 0; row < term->rows; row++) {
        for (int col = 0; col < term->cols; col++) {
            TermCell *cell = &term->grid[row][col];
            
            int x = col * font_w;
            int y = row * font_h;
            
            // Draw background
            SDL_Rect bg_rect = {x, y, font_w, font_h};
            SDL_SetRenderDrawColor(renderer, 
                                   cell->bg_color.r, 
                                   cell->bg_color.g, 
                                   cell->bg_color.b, 
                                   cell->bg_color.a);
            SDL_RenderFillRect(renderer, &bg_rect);
            
            // Draw character using cache
            if (cell->ch != ' ' && cell->ch != '\0') {
                SDL_Texture *glyph_texture = get_cached_glyph(term, renderer, font, cell->ch, cell->fg_color);
                
                if (glyph_texture) {
                    GlyphCache *cache = &term->glyph_cache[(unsigned char)cell->ch];
                    SDL_Rect dst_rect = {x, y, cache->width, cache->height};
                    SDL_RenderCopy(renderer, glyph_texture, NULL, &dst_rect);
                }
            }
        }
    }
    
    // Draw cursor
    if (term->cursor_visible) {
        int x = term->cursor_x * font_w;
        int y = term->cursor_y * font_h;
        
        SDL_Rect cursor_rect = {x, y + font_h - 2, font_w, 2};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillRect(renderer, &cursor_rect);
    }
}

// Handle keyboard input
void terminal_handle_key(Terminal *term, SDL_KeyboardEvent *key) {
    if (term->master_fd < 0 || key->state != SDL_PRESSED) return;
    
    char buffer[16] = {0};
    int len = 0;
    
    SDL_Keycode sym = key->keysym.sym;
    SDL_Keymod mod = key->keysym.mod;
    
    if (sym >= SDLK_SPACE && sym <= SDLK_z) {
        // Printable ASCII
        buffer[0] = (char)sym;
        
        // Handle Ctrl combinations
        if (mod & KMOD_CTRL) {
            if (sym >= SDLK_a && sym <= SDLK_z) {
                buffer[0] = sym - SDLK_a + 1;  // Ctrl+A = 1, etc.
            }
        }
        
        len = 1;
    } else {
        // Special keys
        switch (sym) {
            case SDLK_RETURN:
                buffer[0] = '\r';
                len = 1;
                break;
            case SDLK_BACKSPACE:
                buffer[0] = '\b';
                len = 1;
                break;
            case SDLK_TAB:
                buffer[0] = '\t';
                len = 1;
                break;
            case SDLK_UP:
                strcpy(buffer, "\033[A");
                len = 3;
                break;
            case SDLK_DOWN:
                strcpy(buffer, "\033[B");
                len = 3;
                break;
            case SDLK_RIGHT:
                strcpy(buffer, "\033[C");
                len = 3;
                break;
            case SDLK_LEFT:
                strcpy(buffer, "\033[D");
                len = 3;
                break;
            default:
                break;
        }
    }
    
    if (len > 0) {
        write(term->master_fd, buffer, len);
    }
}

// Write data to terminal
void terminal_write(Terminal *term, const char *data, int len) {
    if (term->master_fd >= 0) {
        write(term->master_fd, data, len);
    }
}