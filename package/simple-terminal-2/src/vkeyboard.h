/*
 * Virtual keyboard interface
 */

#ifndef VKEYBOARD_H
#define VKEYBOARD_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define MAX_KEYS_PER_ROW 15
#define MAX_KEYBOARD_ROWS 5

typedef struct {
    char *label;
    char *shift_label;
    SDL_Keycode keycode;
    int width;  // Width in key units (1 = normal key, 2 = double wide, etc.)
} VKey;

typedef struct {
    VKey keys[MAX_KEYS_PER_ROW];
    int key_count;
} VKeyRow;

typedef struct {
    VKeyRow rows[MAX_KEYBOARD_ROWS];
    int row_count;
    int x, y, width, height;
    int key_width, key_height;
    int selected_row, selected_col;
    int shift_pressed;
    int ctrl_pressed;
    SDL_Color bg_color;
    SDL_Color key_color;
    SDL_Color selected_color;
    SDL_Color text_color;
} VKeyboard;

// Function declarations
int vkeyboard_init(VKeyboard *kb, int width, int height, int x, int y);
void vkeyboard_cleanup(VKeyboard *kb);
void vkeyboard_render(VKeyboard *kb, SDL_Renderer *renderer, TTF_Font *font);
void vkeyboard_handle_key(VKeyboard *kb, SDL_KeyboardEvent *key);
void vkeyboard_handle_mouse(VKeyboard *kb, int x, int y);
char *vkeyboard_get_selected_key(VKeyboard *kb);

#endif // VKEYBOARD_H