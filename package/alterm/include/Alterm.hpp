#pragma once
#include <cstddef>
#include <map>
#include <unordered_map>
#ifndef __ALTERM_HPP__
#define __ALTERM_HPP__
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>

#include "../include/AnsFilter.hpp"
#include "../include/BitmapFont.hpp"
#include "../include/FontManagar.hpp"
#include "../include/SettingsManager.hpp"
#include "forkpty.hpp"

class alterm : protected FontManagar {
private:
    SDL_Window* pWindow = nullptr;
    SDL_Renderer* pRenderer = nullptr;
    TTF_Font* Font = nullptr;
    FontManagar* FontManagarInstance;
    BitmapFont* bitmap_font = nullptr;
    SDL_Color Color;
    bool use_bitmap_font = false;

    int y;

    int CursorX = 0;
    int CursorH = 0;

    std::unordered_map<char, SDL_Texture*> CharTextureChache;
    SDL_Color cached_font_color;  // Track the color used for cached textures

    // Color-aware cache for bitmap fonts (char + color -> texture)
    std::map<std::pair<char, uint32_t>, SDL_Texture*> ColoredCharCache;

    // Separate textures for layered rendering
    SDL_Texture* terminal_texture = nullptr;
    SDL_Texture* keyboard_texture = nullptr;
    int window_width = 0;
    int window_height = 0;
    int texture_width = 0;
    int texture_height = 0;

public:
    SettingsManager* settings_manager = nullptr;
    int ScrollOffSet = 0;
    std::vector<std::string> lines;

    // Screen buffer for full-screen applications
    std::vector<std::vector<ColoredChar>> screen_buffer;
    int screen_rows = 25;
    int screen_cols = 53;
    int cursor_row = 0;
    int cursor_col = 0;
    bool alternate_screen = false;

    // Scroll region support
    int scroll_top = 0;
    int scroll_bottom = 24;

    // Rendering dirty flag
    bool screen_buffer_dirty = false;

    // Escape sequence processing state
    enum EscapeState { ESC_NORMAL = 0, ESC_START = 1, ESC_CSI = 2, ESC_OSC = 4 };
    EscapeState esc_state = ESC_NORMAL;
    std::string esc_buffer;

    // Current text attributes for screen buffer
    SDL_Color current_fg_color = {255, 255, 255, 255};  // White
    SDL_Color current_bg_color = {0, 0, 0, 0};          // Transparent
    bool reverse_video = false;

    bool initialize_window();
    void update_window_size();
    void reset_y() { this->y = 0; }
    void enter_cursor_reset_x() { this->CursorX = 0; }
    void renderer_screen(std::string& input_buffer, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool ShowCursor, bool should_present = true);
    void setup_font(const Uint8 r, const Uint8 g, const Uint8 b, const char* path, const int size, const char* text);
    void reset_display();
    void render_background(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void shutdown_sdl();
    SDL_Texture* get_cached_texture(char c);
    SDL_Texture* get_colored_cached_texture(char c, SDL_Color color);
    void clear_char_texture_cache();

    // Layered rendering methods
    bool create_render_textures();
    void render_terminal_to_texture(std::string& input_buffer, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool ShowCursor);
    void render_keyboard_to_texture(void* keyboard);  // Use void* to avoid circular dependency
    void composite_and_present();

    // Getters for virtual keyboard
    SDL_Renderer* get_renderer() { return pRenderer; }
    TTF_Font* get_font() { return Font; }
    BitmapFont* get_bitmap_font() { return bitmap_font; }

    // Getters for window dimensions
    int get_window_width() { return window_width; }
    int get_window_height() { return window_height; }
    int get_texture_width() { return texture_width; }
    int get_texture_height() { return texture_height; }

    // Setters
    void set_settings_manager(SettingsManager* settings) { settings_manager = settings; }

    // Screen buffer methods
    void init_screen_buffer();
    void clear_screen_buffer();
    void set_cursor_position(int row, int col);
    void write_to_screen_buffer(const std::string& text);
    void handle_escape_sequence(const std::string& sequence);
    void update_screen_dimensions();
    void process_char(char c);
    void scroll_up(int lines);
    void scroll_down(int lines);
    void scroll_up_region(int origin, int lines);
    void scroll_down_region(int origin, int lines);

    // Dirty flag management
    bool is_screen_buffer_dirty() const { return screen_buffer_dirty; }
    void clear_screen_buffer_dirty() { screen_buffer_dirty = false; }
    void mark_screen_buffer_dirty() { screen_buffer_dirty = true; }

    void trim_lines(size_t maxLines, alterm* term) {
        if (lines.size() > maxLines) {
            size_t to_remove = lines.size() - maxLines;
            lines.erase(lines.begin(), lines.begin() + to_remove);
            term->ScrollOffSet = 0;
        }
    }
};

#endif  //__ALTERM_HPP__