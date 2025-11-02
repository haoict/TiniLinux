/*
 * Virtual keyboard implementation
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vkeyboard.h"

// Keyboard layout definition
static VKey qwerty_layout[MAX_KEYBOARD_ROWS][MAX_KEYS_PER_ROW] = {
    // Row 0: Numbers and symbols
    {
        {"1", "!", SDLK_1, 1}, {"2", "@", SDLK_2, 1}, {"3", "#", SDLK_3, 1}, 
        {"4", "$", SDLK_4, 1}, {"5", "%", SDLK_5, 1}, {"6", "^", SDLK_6, 1},
        {"7", "&", SDLK_7, 1}, {"8", "*", SDLK_8, 1}, {"9", "(", SDLK_9, 1},
        {"0", ")", SDLK_0, 1}, {"-", "_", SDLK_MINUS, 1}, {"=", "+", SDLK_EQUALS, 1},
        {"Bksp", "Bksp", SDLK_BACKSPACE, 2}, {NULL}
    },
    
    // Row 1: QWERTY top row
    {
        {"Tab", "Tab", SDLK_TAB, 2}, {"q", "Q", SDLK_q, 1}, {"w", "W", SDLK_w, 1},
        {"e", "E", SDLK_e, 1}, {"r", "R", SDLK_r, 1}, {"t", "T", SDLK_t, 1},
        {"y", "Y", SDLK_y, 1}, {"u", "U", SDLK_u, 1}, {"i", "I", SDLK_i, 1},
        {"o", "O", SDLK_o, 1}, {"p", "P", SDLK_p, 1}, {"[", "{", SDLK_LEFTBRACKET, 1},
        {"]", "}", SDLK_RIGHTBRACKET, 1}, {NULL}
    },
    
    // Row 2: ASDF middle row
    {
        {"Ctrl", "Ctrl", SDLK_LCTRL, 2}, {"a", "A", SDLK_a, 1}, {"s", "S", SDLK_s, 1},
        {"d", "D", SDLK_d, 1}, {"f", "F", SDLK_f, 1}, {"g", "G", SDLK_g, 1},
        {"h", "H", SDLK_h, 1}, {"j", "J", SDLK_j, 1}, {"k", "K", SDLK_k, 1},
        {"l", "L", SDLK_l, 1}, {";", ":", SDLK_SEMICOLON, 1}, {"'", "\"", SDLK_QUOTE, 1},
        {"Enter", "Enter", SDLK_RETURN, 2}, {NULL}
    },
    
    // Row 3: ZXCV bottom row
    {
        {"Shift", "Shift", SDLK_LSHIFT, 2}, {"z", "Z", SDLK_z, 1}, {"x", "X", SDLK_x, 1},
        {"c", "C", SDLK_c, 1}, {"v", "V", SDLK_v, 1}, {"b", "B", SDLK_b, 1},
        {"n", "N", SDLK_n, 1}, {"m", "M", SDLK_m, 1}, {",", "<", SDLK_COMMA, 1},
        {".", ">", SDLK_PERIOD, 1}, {"/", "?", SDLK_SLASH, 1}, 
        {"\\", "|", SDLK_BACKSLASH, 1}, {"`", "~", SDLK_BACKQUOTE, 1}, {NULL}
    },
    
    // Row 4: Space bar and arrows
    {
        {"Esc", "Esc", SDLK_ESCAPE, 1}, {"Space", "Space", SDLK_SPACE, 6}, 
        {"↑", "↑", SDLK_UP, 1}, {"↓", "↓", SDLK_DOWN, 1}, 
        {"←", "←", SDLK_LEFT, 1}, {"→", "→", SDLK_RIGHT, 1}, {NULL}
    }
};

// Initialize virtual keyboard
int vkeyboard_init(VKeyboard *kb, int width, int height, int x, int y) {
    memset(kb, 0, sizeof(VKeyboard));
    
    kb->x = x;
    kb->y = y;
    kb->width = width;
    kb->height = height;
    
    // Calculate key dimensions
    kb->key_width = width / 14;  // Based on longest row
    kb->key_height = height / MAX_KEYBOARD_ROWS;
    
    kb->selected_row = 0;
    kb->selected_col = 0;
    kb->shift_pressed = 0;
    kb->ctrl_pressed = 0;
    
    // Set colors
    kb->bg_color = (SDL_Color){32, 32, 32, 255};      // Dark gray
    kb->key_color = (SDL_Color){64, 64, 64, 255};     // Medium gray
    kb->selected_color = (SDL_Color){128, 128, 255, 255}; // Light blue
    kb->text_color = (SDL_Color){255, 255, 255, 255}; // White
    
    // Copy layout
    for (int row = 0; row < MAX_KEYBOARD_ROWS; row++) {
        kb->rows[row].key_count = 0;
        
        for (int col = 0; col < MAX_KEYS_PER_ROW && qwerty_layout[row][col].label; col++) {
            VKey *src = &qwerty_layout[row][col];
            VKey *dst = &kb->rows[row].keys[col];
            
            // Allocate and copy strings
            if (src->label) {
                dst->label = strdup(src->label);
            }
            if (src->shift_label) {
                dst->shift_label = strdup(src->shift_label);
            }
            
            dst->keycode = src->keycode;
            dst->width = src->width;
            
            kb->rows[row].key_count++;
        }
    }
    
    kb->row_count = MAX_KEYBOARD_ROWS;
    
    printf("Virtual keyboard initialized: %dx%d at (%d,%d)\n", width, height, x, y);
    printf("Key size: %dx%d\n", kb->key_width, kb->key_height);
    
    return 0;
}

// Cleanup virtual keyboard
void vkeyboard_cleanup(VKeyboard *kb) {
    for (int row = 0; row < kb->row_count; row++) {
        for (int col = 0; col < kb->rows[row].key_count; col++) {
            VKey *key = &kb->rows[row].keys[col];
            if (key->label) {
                free(key->label);
                key->label = NULL;
            }
            if (key->shift_label) {
                free(key->shift_label);
                key->shift_label = NULL;
            }
        }
    }
}

// Get key position and size
static void get_key_rect(VKeyboard *kb, int row, int col, SDL_Rect *rect) {
    VKey *key = &kb->rows[row].keys[col];
    
    // Calculate x position based on keys to the left
    int x_offset = 0;
    for (int i = 0; i < col; i++) {
        x_offset += kb->rows[row].keys[i].width * kb->key_width;
    }
    
    rect->x = kb->x + x_offset + col * 2;  // Small gap between keys
    rect->y = kb->y + row * kb->key_height + row * 2;  // Small gap between rows
    rect->w = key->width * kb->key_width - 2;  // Subtract gap
    rect->h = kb->key_height - 2;  // Subtract gap
}

// Render virtual keyboard
void vkeyboard_render(VKeyboard *kb, SDL_Renderer *renderer, TTF_Font *font) {
    if (!font) return;
    
    // Draw background
    SDL_Rect bg_rect = {kb->x, kb->y, kb->width, kb->height};
    SDL_SetRenderDrawColor(renderer, kb->bg_color.r, kb->bg_color.g, kb->bg_color.b, kb->bg_color.a);
    SDL_RenderFillRect(renderer, &bg_rect);
    
    // Draw keys
    for (int row = 0; row < kb->row_count; row++) {
        for (int col = 0; col < kb->rows[row].key_count; col++) {
            VKey *key = &kb->rows[row].keys[col];
            SDL_Rect key_rect;
            
            get_key_rect(kb, row, col, &key_rect);
            
            // Choose color based on selection and special keys
            SDL_Color *bg_color = &kb->key_color;
            if (row == kb->selected_row && col == kb->selected_col) {
                bg_color = &kb->selected_color;
            } else if (key->keycode == SDLK_LSHIFT && kb->shift_pressed) {
                bg_color = &kb->selected_color;
            } else if (key->keycode == SDLK_LCTRL && kb->ctrl_pressed) {
                bg_color = &kb->selected_color;
            }
            
            // Draw key background
            SDL_SetRenderDrawColor(renderer, bg_color->r, bg_color->g, bg_color->b, bg_color->a);
            SDL_RenderFillRect(renderer, &key_rect);
            
            // Draw key border
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            SDL_RenderDrawRect(renderer, &key_rect);
            
            // Draw key label
            const char *label = (kb->shift_pressed && key->shift_label) ? key->shift_label : key->label;
            if (label) {
                SDL_Surface *text_surface = TTF_RenderText_Solid(font, label, kb->text_color);
                if (text_surface) {
                    SDL_Texture *text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
                    if (text_texture) {
                        // Center text in key
                        SDL_Rect text_rect;
                        text_rect.w = text_surface->w;
                        text_rect.h = text_surface->h;
                        text_rect.x = key_rect.x + (key_rect.w - text_rect.w) / 2;
                        text_rect.y = key_rect.y + (key_rect.h - text_rect.h) / 2;
                        
                        SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
                        SDL_DestroyTexture(text_texture);
                    }
                    SDL_FreeSurface(text_surface);
                }
            }
        }
    }
}

// Handle keyboard navigation
void vkeyboard_handle_key(VKeyboard *kb, SDL_KeyboardEvent *key) {
    if (key->state != SDL_PRESSED) return;
    
    switch (key->keysym.sym) {
        case SDLK_UP:
            if (kb->selected_row > 0) {
                kb->selected_row--;
                // Adjust column if current row has fewer keys
                if (kb->selected_col >= kb->rows[kb->selected_row].key_count) {
                    kb->selected_col = kb->rows[kb->selected_row].key_count - 1;
                }
            }
            break;
            
        case SDLK_DOWN:
            if (kb->selected_row < kb->row_count - 1) {
                kb->selected_row++;
                // Adjust column if current row has fewer keys
                if (kb->selected_col >= kb->rows[kb->selected_row].key_count) {
                    kb->selected_col = kb->rows[kb->selected_row].key_count - 1;
                }
            }
            break;
            
        case SDLK_LEFT:
            if (kb->selected_col > 0) {
                kb->selected_col--;
            }
            break;
            
        case SDLK_RIGHT:
            if (kb->selected_col < kb->rows[kb->selected_row].key_count - 1) {
                kb->selected_col++;
            }
            break;
            
        case SDLK_RETURN:
        case SDLK_SPACE:
            // Activate selected key
            {
                VKey *selected_key = &kb->rows[kb->selected_row].keys[kb->selected_col];
                
                if (selected_key->keycode == SDLK_LSHIFT) {
                    kb->shift_pressed = !kb->shift_pressed;
                } else if (selected_key->keycode == SDLK_LCTRL) {
                    kb->ctrl_pressed = !kb->ctrl_pressed;
                }
                // TODO: Send key to terminal
            }
            break;
    }
}

// Handle mouse input
void vkeyboard_handle_mouse(VKeyboard *kb, int x, int y) {
    // Convert mouse coordinates to keyboard coordinates
    if (x < kb->x || y < kb->y || x >= kb->x + kb->width || y >= kb->y + kb->height) {
        return;  // Outside keyboard area
    }
    
    // Find which key was clicked
    for (int row = 0; row < kb->row_count; row++) {
        for (int col = 0; col < kb->rows[row].key_count; col++) {
            SDL_Rect key_rect;
            get_key_rect(kb, row, col, &key_rect);
            
            if (x >= key_rect.x && x < key_rect.x + key_rect.w &&
                y >= key_rect.y && y < key_rect.y + key_rect.h) {
                
                // Select this key
                kb->selected_row = row;
                kb->selected_col = col;
                
                // Activate the key
                VKey *selected_key = &kb->rows[row].keys[col];
                
                if (selected_key->keycode == SDLK_LSHIFT) {
                    kb->shift_pressed = !kb->shift_pressed;
                } else if (selected_key->keycode == SDLK_LCTRL) {
                    kb->ctrl_pressed = !kb->ctrl_pressed;
                }
                // TODO: Send key to terminal
                
                return;
            }
        }
    }
}

// Get currently selected key label
char *vkeyboard_get_selected_key(VKeyboard *kb) {
    if (kb->selected_row >= kb->row_count || 
        kb->selected_col >= kb->rows[kb->selected_row].key_count) {
        return NULL;
    }
    
    VKey *key = &kb->rows[kb->selected_row].keys[kb->selected_col];
    return (kb->shift_pressed && key->shift_label) ? key->shift_label : key->label;
}