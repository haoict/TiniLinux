/*
 * VT100/ANSI escape sequence implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vt100.h"

// Parse ANSI color codes
static SDL_Color get_ansi_color(int color_code) {
    SDL_Color colors[] = {
        {0, 0, 0, 255},         // 0: Black
        {128, 0, 0, 255},       // 1: Red
        {0, 128, 0, 255},       // 2: Green
        {128, 128, 0, 255},     // 3: Yellow
        {0, 0, 128, 255},       // 4: Blue
        {128, 0, 128, 255},     // 5: Magenta
        {0, 128, 128, 255},     // 6: Cyan
        {192, 192, 192, 255},   // 7: White
        {128, 128, 128, 255},   // 8: Bright Black
        {255, 0, 0, 255},       // 9: Bright Red
        {0, 255, 0, 255},       // 10: Bright Green
        {255, 255, 0, 255},     // 11: Bright Yellow
        {0, 0, 255, 255},       // 12: Bright Blue
        {255, 0, 255, 255},     // 13: Bright Magenta
        {0, 255, 255, 255},     // 14: Bright Cyan
        {255, 255, 255, 255},   // 15: Bright White
    };
    
    if (color_code >= 0 && color_code < 16) {
        return colors[color_code];
    }
    
    return (SDL_Color){192, 192, 192, 255};  // Default white
}

// Parse CSI sequence parameters
static int parse_csi_params(const char *seq, int len, int *params, int max_params) {
    int param_count = 0;
    int current_param = 0;
    int has_param = 0;
    
    for (int i = 2; i < len - 1; i++) {  // Skip ESC[ and final character
        char ch = seq[i];
        
        if (isdigit(ch)) {
            current_param = current_param * 10 + (ch - '0');
            has_param = 1;
        } else if (ch == ';') {
            if (param_count < max_params) {
                params[param_count++] = has_param ? current_param : 0;
            }
            current_param = 0;
            has_param = 0;
        }
    }
    
    // Add final parameter
    if (param_count < max_params) {
        params[param_count++] = has_param ? current_param : 0;
    }
    
    return param_count;
}

// Parse VT100/ANSI escape sequences
int vt100_parse_escape(Terminal *term, const char *escape_seq, int len) {
    if (len < 2) return 0;  // Need more characters
    
    if (escape_seq[0] != '\033') return 1;  // Not an escape sequence
    
    // Single character escapes
    if (len == 2) {
        switch (escape_seq[1]) {
            case 'M':  // Reverse index (scroll down)
                if (term->cursor_y > 0) {
                    term->cursor_y--;
                } else {
                    // Scroll down (move content up)
                    for (int row = term->rows - 1; row > 0; row--) {
                        for (int col = 0; col < term->cols; col++) {
                            term->grid[row][col] = term->grid[row - 1][col];
                        }
                    }
                    // Clear top line
                    for (int col = 0; col < term->cols; col++) {
                        term->grid[0][col].ch = ' ';
                        term->grid[0][col].fg_color = term->default_fg;
                        term->grid[0][col].bg_color = term->default_bg;
                    }
                }
                return 1;
            case 'D':  // Index (scroll up)
                term->cursor_y++;
                if (term->cursor_y >= term->rows) {
                    term->cursor_y = term->rows - 1;
                    // Scroll up logic is handled in put_char
                }
                return 1;
            case 'E':  // Next line
                term->cursor_x = 0;
                term->cursor_y++;
                if (term->cursor_y >= term->rows) {
                    term->cursor_y = term->rows - 1;
                }
                return 1;
            case 'c':  // Reset terminal
                // Reset to initial state
                for (int row = 0; row < MAX_ROWS; row++) {
                    for (int col = 0; col < MAX_COLS; col++) {
                        term->grid[row][col].ch = ' ';
                        term->grid[row][col].fg_color = term->default_fg;
                        term->grid[row][col].bg_color = term->default_bg;
                        term->grid[row][col].bold = 0;
                        term->grid[row][col].reverse = 0;
                    }
                }
                term->cursor_x = 0;
                term->cursor_y = 0;
                return 1;
        }
    }
    
    // OSC sequences (ESC]...)
    if (len >= 3 && escape_seq[1] == ']') {
        // Look for terminator (BEL \x07 or ST \x1b\\)
        for (int i = 2; i < len; i++) {
            if (escape_seq[i] == '\x07' || 
                (i < len - 1 && escape_seq[i] == '\x1b' && escape_seq[i+1] == '\\')) {
                // Found complete OSC sequence, ignore it (just consume)
                return 1;
            }
        }
        return 0;  // Need more characters for complete OSC sequence
    }
    
    // CSI sequences (ESC[...)
    if (len >= 3 && escape_seq[1] == '[') {
        char final_char = escape_seq[len - 1];
        
        // Check if we have the complete sequence
        if (!isalpha(final_char) && final_char != '@' && final_char != '`') {
            return 0;  // Need more characters
        }
        
        int params[16] = {0};
        int param_count = parse_csi_params(escape_seq, len, params, 16);
        
        switch (final_char) {
            case 'A':  // Cursor up
                {
                    int n = (param_count > 0) ? params[0] : 1;
                    if (n == 0) n = 1;
                    term->cursor_y -= n;
                    if (term->cursor_y < 0) term->cursor_y = 0;
                }
                return 1;
                
            case 'B':  // Cursor down
                {
                    int n = (param_count > 0) ? params[0] : 1;
                    if (n == 0) n = 1;
                    term->cursor_y += n;
                    if (term->cursor_y >= term->rows) term->cursor_y = term->rows - 1;
                }
                return 1;
                
            case 'C':  // Cursor right
                {
                    int n = (param_count > 0) ? params[0] : 1;
                    if (n == 0) n = 1;
                    term->cursor_x += n;
                    if (term->cursor_x >= term->cols) term->cursor_x = term->cols - 1;
                }
                return 1;
                
            case 'D':  // Cursor left
                {
                    int n = (param_count > 0) ? params[0] : 1;
                    if (n == 0) n = 1;
                    term->cursor_x -= n;
                    if (term->cursor_x < 0) term->cursor_x = 0;
                }
                return 1;
                
            case 'H':  // Cursor position
            case 'f':
                {
                    int row = (param_count > 0) ? params[0] : 1;
                    int col = (param_count > 1) ? params[1] : 1;
                    
                    if (row == 0) row = 1;
                    if (col == 0) col = 1;
                    
                    term->cursor_y = row - 1;
                    term->cursor_x = col - 1;
                    
                    if (term->cursor_y >= term->rows) term->cursor_y = term->rows - 1;
                    if (term->cursor_x >= term->cols) term->cursor_x = term->cols - 1;
                    if (term->cursor_y < 0) term->cursor_y = 0;
                    if (term->cursor_x < 0) term->cursor_x = 0;
                }
                return 1;
                
            case 'J':  // Erase display
                {
                    int n = (param_count > 0) ? params[0] : 0;
                    
                    if (n == 0) {  // Clear from cursor to end of screen
                        // Clear current line from cursor
                        for (int col = term->cursor_x; col < term->cols; col++) {
                            term->grid[term->cursor_y][col].ch = ' ';
                            term->grid[term->cursor_y][col].fg_color = term->default_fg;
                            term->grid[term->cursor_y][col].bg_color = term->default_bg;
                        }
                        // Clear lines below
                        for (int row = term->cursor_y + 1; row < term->rows; row++) {
                            for (int col = 0; col < term->cols; col++) {
                                term->grid[row][col].ch = ' ';
                                term->grid[row][col].fg_color = term->default_fg;
                                term->grid[row][col].bg_color = term->default_bg;
                            }
                        }
                    } else if (n == 1) {  // Clear from beginning to cursor
                        // Clear lines above
                        for (int row = 0; row < term->cursor_y; row++) {
                            for (int col = 0; col < term->cols; col++) {
                                term->grid[row][col].ch = ' ';
                                term->grid[row][col].fg_color = term->default_fg;
                                term->grid[row][col].bg_color = term->default_bg;
                            }
                        }
                        // Clear current line to cursor
                        for (int col = 0; col <= term->cursor_x; col++) {
                            term->grid[term->cursor_y][col].ch = ' ';
                            term->grid[term->cursor_y][col].fg_color = term->default_fg;
                            term->grid[term->cursor_y][col].bg_color = term->default_bg;
                        }
                    } else if (n == 2) {  // Clear entire screen
                        for (int row = 0; row < term->rows; row++) {
                            for (int col = 0; col < term->cols; col++) {
                                term->grid[row][col].ch = ' ';
                                term->grid[row][col].fg_color = term->default_fg;
                                term->grid[row][col].bg_color = term->default_bg;
                            }
                        }
                    }
                }
                return 1;
                
            case 'K':  // Erase line
                {
                    int n = (param_count > 0) ? params[0] : 0;
                    
                    if (n == 0) {  // Clear from cursor to end of line
                        for (int col = term->cursor_x; col < term->cols; col++) {
                            term->grid[term->cursor_y][col].ch = ' ';
                            term->grid[term->cursor_y][col].fg_color = term->default_fg;
                            term->grid[term->cursor_y][col].bg_color = term->default_bg;
                        }
                    } else if (n == 1) {  // Clear from beginning to cursor
                        for (int col = 0; col <= term->cursor_x; col++) {
                            term->grid[term->cursor_y][col].ch = ' ';
                            term->grid[term->cursor_y][col].fg_color = term->default_fg;
                            term->grid[term->cursor_y][col].bg_color = term->default_bg;
                        }
                    } else if (n == 2) {  // Clear entire line
                        for (int col = 0; col < term->cols; col++) {
                            term->grid[term->cursor_y][col].ch = ' ';
                            term->grid[term->cursor_y][col].fg_color = term->default_fg;
                            term->grid[term->cursor_y][col].bg_color = term->default_bg;
                        }
                    }
                }
                return 1;
                
            case 'm':  // Set graphics mode (colors, etc.)
                if (param_count == 0) {
                    // Reset all attributes
                    term->default_fg = (SDL_Color){192, 192, 192, 255};
                    term->default_bg = (SDL_Color){0, 0, 0, 255};
                } else {
                    for (int i = 0; i < param_count; i++) {
                        int param = params[i];
                        
                        if (param == 0) {  // Reset
                            term->default_fg = (SDL_Color){192, 192, 192, 255};
                            term->default_bg = (SDL_Color){0, 0, 0, 255};
                        } else if (param >= 30 && param <= 37) {  // Foreground color
                            term->default_fg = get_ansi_color(param - 30);
                        } else if (param >= 40 && param <= 47) {  // Background color
                            term->default_bg = get_ansi_color(param - 40);
                        } else if (param >= 90 && param <= 97) {  // Bright foreground
                            term->default_fg = get_ansi_color(param - 90 + 8);
                        } else if (param >= 100 && param <= 107) {  // Bright background
                            term->default_bg = get_ansi_color(param - 100 + 8);
                        }
                    }
                }
                return 1;
                
            default:
                // Unknown sequence, consume it
                return 1;
        }
    }
    
    return 0;  // Need more characters or unknown sequence
}