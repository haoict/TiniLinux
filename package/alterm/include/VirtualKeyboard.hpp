#pragma once
#ifndef __VIRTUAL_KEYBOARD_HPP__
#define __VIRTUAL_KEYBOARD_HPP__

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <map>

#define NUM_ROWS 6
#define NUM_KEYS 18

// Special key definitions
#define KEY_QUIT 1000
#define KEY_ACTIVATE 1001

class VirtualKeyboard {
private:
    // Keyboard layout data
    static int row_length[NUM_ROWS];
    static SDL_Keycode keys[2][NUM_ROWS][NUM_KEYS];
    static const char* syms[2][NUM_ROWS][NUM_KEYS];
    
    // State variables
    bool active;
    bool show_help;
    bool shifted;
    int selected_i, selected_j;
    int visual_offset;
    int location; // 0 = bottom, 1 = top
    bool toggled[NUM_ROWS][NUM_KEYS];
    int char_width, char_height;
    bool ignore_next_key; // Flag to prevent recursive key handling
    
    // Font and rendering
    TTF_Font* font;
    class BitmapFont* bitmap_font;  // For bitmap font support
    SDL_Color text_color;
    SDL_Color bg_color;
    SDL_Color key_color;
    SDL_Color sel_color;
    SDL_Color toggled_color;
    SDL_Color sel_toggled_color;
    
    // Texture cache for bitmap font characters
    std::map<char, SDL_Texture*> char_texture_cache;
    SDL_Color cached_color;  // Track the color used for cached textures
    
    // Texture cache for TTF font characters
    std::map<char, SDL_Texture*> ttf_texture_cache;
    SDL_Color cached_ttf_color;  // Track the color used for TTF cached textures
    
    // Helper functions
    void init_keyboard();
    void simulate_key_event(SDL_Keycode key, bool pressed);
    void simulate_text_input(const char* text);
    int compute_visual_offset(int col, int row);
    int compute_new_col(int visual_offset, int old_row, int new_row);
    void draw_string(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color);
    void draw_string_bitmap(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color);
    void update_shift_state();
    SDL_Texture* get_cached_bitmap_texture(char c, SDL_Color color);
    SDL_Texture* get_cached_ttf_texture(char c, SDL_Color color, SDL_Renderer* renderer);
    void clear_texture_cache();
    void clear_ttf_texture_cache();

public:
    VirtualKeyboard();
    ~VirtualKeyboard();
    
    bool initialize(TTF_Font* font_ptr);
    bool initialize_bitmap(class BitmapFont* bitmap_font_ptr);
    void cleanup();
    
    // Main functions
    void draw(SDL_Renderer* renderer, int screen_width, int screen_height);
    bool handle_event(SDL_Event* event);
    
    // State management
    bool is_active() const { return active; }
    void set_active(bool state) { active = state; }
    void toggle_active() { active = !active; }
    
    bool is_help_shown() const { return show_help; }
    void show_help_screen() { show_help = true; }
    void hide_help_screen() { show_help = false; }
    
    void set_location(int loc) { location = loc; }
    int get_location() const { return location; }
};

#endif // __VIRTUAL_KEYBOARD_HPP__