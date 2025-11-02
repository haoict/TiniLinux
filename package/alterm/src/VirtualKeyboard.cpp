#include "../include/VirtualKeyboard.hpp"
#include "../include/BitmapFont.hpp"
#include <SDL2/SDL_events.h>
#include <cstring>

// Static keyboard layout data
int VirtualKeyboard::row_length[NUM_ROWS] = {13, 17, 17, 15, 14, 9};

SDL_Keycode VirtualKeyboard::keys[2][NUM_ROWS][NUM_KEYS] = {
    {{SDLK_ESCAPE, SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12},
     {SDLK_BACKQUOTE, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_0, SDLK_MINUS, SDLK_EQUALS, SDLK_BACKSPACE, SDLK_INSERT, SDLK_DELETE, SDLK_UP},
     {SDLK_TAB, SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i, SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET, SDLK_BACKSLASH, SDLK_HOME, SDLK_END, SDLK_DOWN},
     {SDLK_CAPSLOCK, SDLK_a, SDLK_s, SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE, SDLK_RETURN, SDLK_PAGEUP, SDLK_LEFT},
     {SDLK_LSHIFT, SDLK_z, SDLK_x, SDLK_c, SDLK_v, SDLK_b, SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RSHIFT, SDLK_PAGEDOWN, SDLK_RIGHT},
     {SDLK_LCTRL, SDLK_LGUI, SDLK_LALT, SDLK_SPACE, SDLK_RALT, SDLK_RGUI, SDLK_RCTRL, SDLK_PRINTSCREEN, SDLK_PAUSE, KEY_QUIT}},
    {{SDLK_ESCAPE, SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12},
     {'~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', SDLK_BACKSPACE, SDLK_INSERT, SDLK_DELETE, SDLK_UP},
     {SDLK_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '|', SDLK_HOME, SDLK_END, SDLK_DOWN},
     {SDLK_CAPSLOCK, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', SDLK_RETURN, SDLK_PAGEUP, SDLK_LEFT},
     {SDLK_LSHIFT, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', SDLK_RSHIFT, SDLK_PAGEDOWN, SDLK_RIGHT},
     {SDLK_LCTRL, SDLK_LGUI, SDLK_LALT, SDLK_SPACE, SDLK_RALT, SDLK_RGUI, SDLK_RCTRL, SDLK_PRINTSCREEN, KEY_QUIT}}};

const char* VirtualKeyboard::syms[2][NUM_ROWS][NUM_KEYS] = {
    {{"Esc", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", nullptr},
     {"` ", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Bsp", "Ins", "Del", " ^ ", nullptr},
     {"Tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\\", "Home", "End", " \xde ", nullptr},
     {"Caps", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "Enter", "Pg Up", " < ", nullptr},
     {"Shift", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", " Shift", "Pg Dn", " > ", nullptr},
     {"Ctrl", " ", "Alt", "   Space   ", "Alt", " ", "Ctrl", "PrS", "Exit", nullptr}},
    {{"Esc", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", nullptr},
     {"~ ", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", "_", "+", "Bsp", "Ins", "Del", " ^ ", nullptr},
     {"Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "{", "}", "|", "Home", "End", " \xde ", nullptr},
     {"Caps", "A", "S", "D", "F", "G", "H", "J", "K", "L", ":", "\"", "Enter", "Pg Up", " < ", nullptr},
     {"Shift", "Z", "X", "C", "V", "B", "N", "M", "<", ">", "?", " Shift", "Pg Dn", " > ", nullptr},
     {"Ctrl", " ", "Alt", "   Space   ", "Alt", " ", "Ctrl", "PrS", "Exit", nullptr}}};

VirtualKeyboard::VirtualKeyboard() {
    init_keyboard();
    
    // Initialize colors
    text_color = {0, 0, 0, 255};        // Black text
    bg_color = {64, 64, 64, 255};       // Dark gray background
    key_color = {128, 128, 128, 255};   // Gray keys
    sel_color = {128, 255, 128, 255};   // Light green selection
    toggled_color = {192, 192, 0, 255}; // Yellow toggled keys
    sel_toggled_color = {255, 255, 128, 255}; // Light yellow selected toggled
    
    font = nullptr;
    bitmap_font = nullptr;
    
    // Initialize texture caches
    char_texture_cache.clear();
    cached_color = {0, 0, 0, 0};  // Invalid color initially
    ttf_texture_cache.clear();
    cached_ttf_color = {0, 0, 0, 0};  // Invalid color initially
}

VirtualKeyboard::~VirtualKeyboard() {
    cleanup();
}

void VirtualKeyboard::init_keyboard() {
    // Initialize all toggle states to false
    for (int j = 0; j < NUM_ROWS; j++) {
        for (int i = 0; i < NUM_KEYS; i++) {
            toggled[j][i] = false;
        }
    }
    
    selected_i = 0;
    selected_j = 0;
    shifted = false;
    location = 0; // bottom
    visual_offset = 0;
    active = false;
    show_help = false; // Show help initially
}

bool VirtualKeyboard::initialize(TTF_Font* font_ptr) {
    font = font_ptr;
    char_width = TTF_FontHeight(font) - 8;
    char_height = TTF_FontHeight(font) - 2;
    return font != nullptr;
}

bool VirtualKeyboard::initialize_bitmap(BitmapFont* bitmap_font_ptr) {
    bitmap_font = bitmap_font_ptr;
        
    char_width = bitmap_font->get_char_width();   // Get scaled character width
    char_height = bitmap_font->get_char_height(); // Get scaled character height

    return bitmap_font != nullptr;
}

void VirtualKeyboard::cleanup() {
    clear_texture_cache();
    clear_ttf_texture_cache();
    // Nothing else to cleanup as we don't own the font
}

void VirtualKeyboard::simulate_key_event(SDL_Keycode key, bool pressed) {
    SDL_Event event;
    event.type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
    event.key.state = pressed ? SDL_PRESSED : SDL_RELEASED;
    event.key.keysym.sym = key;
    event.key.keysym.scancode = SDL_GetScancodeFromKey(key);
    event.key.keysym.mod = KMOD_NONE;
    event.key.repeat = 0;
    event.key.timestamp = SDL_GetTicks();
    event.key.windowID = 0;
    
    SDL_PushEvent(&event);
}

void VirtualKeyboard::simulate_text_input(const char* text) {
    SDL_Event event;
    event.type = SDL_TEXTINPUT;
    event.text.timestamp = SDL_GetTicks();
    event.text.windowID = 0;
    strncpy(event.text.text, text, SDL_TEXTINPUTEVENT_TEXT_SIZE - 1);
    event.text.text[SDL_TEXTINPUTEVENT_TEXT_SIZE - 1] = '\0';
    
    SDL_PushEvent(&event);
}

void VirtualKeyboard::draw_string(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color) {
    if (!font || !text) return;
    
    int orig_x = x;
    
    while (*text) {
        if (*text == '\n') {
            x = orig_x;
            y += char_height;
        } else {
            // Use cached texture for this character
            SDL_Texture* char_texture = get_cached_ttf_texture(*text, color, renderer);
            if (char_texture) {
                // Query texture size
                int tex_w, tex_h;
                SDL_QueryTexture(char_texture, nullptr, nullptr, &tex_w, &tex_h);
                
                SDL_Rect dst = {x, y, tex_w, tex_h};
                SDL_RenderCopy(renderer, char_texture, nullptr, &dst);
                // Don't destroy texture - it's cached!
                
                x += tex_w;
            } else {
                // Fallback: move by estimated width
                x += char_width;
            }
        }
        text++;
    }
}

void VirtualKeyboard::draw_string_bitmap(SDL_Renderer* renderer, const char* text, int x, int y, SDL_Color color) {
    if (!bitmap_font || !text) return;
    
    int orig_x = x;
    
    while (*text) {
        if (*text == '\n') {
            x = orig_x;
            y += char_height;
        } else {
            // Use cached texture for this character
            SDL_Texture* char_texture = get_cached_bitmap_texture(*text, color);
            if (char_texture) {
                SDL_Rect dst = {x, y, char_width, char_height};
                SDL_RenderCopy(renderer, char_texture, nullptr, &dst);
                // Don't destroy texture - it's cached!
            }
            x += char_width;
        }
        text++;
    }
}

SDL_Texture* VirtualKeyboard::get_cached_bitmap_texture(char c, SDL_Color color) {
    if (!bitmap_font) return nullptr;
    
    // Check if we need to clear cache due to color change
    if (cached_color.r != color.r || cached_color.g != color.g || 
        cached_color.b != color.b || cached_color.a != color.a) {
        clear_texture_cache();
        cached_color = color;
    }
    
    // Check if we already have this character cached
    if (char_texture_cache.count(c)) {
        return char_texture_cache[c];
    }
    
    // Create new texture for this character
    bitmap_font->set_color(color.r, color.g, color.b);
    SDL_Texture* texture = bitmap_font->create_char_texture(c);
    
    if (texture) {
        char_texture_cache[c] = texture;
    }
    
    return texture;
}

void VirtualKeyboard::clear_texture_cache() {
    // Destroy all cached textures
    for (auto& pair : char_texture_cache) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    char_texture_cache.clear();
}

SDL_Texture* VirtualKeyboard::get_cached_ttf_texture(char c, SDL_Color color, SDL_Renderer* renderer) {
    if (!font || !renderer) return nullptr;
    
    // Check if we need to clear cache due to color change
    if (cached_ttf_color.r != color.r || cached_ttf_color.g != color.g || 
        cached_ttf_color.b != color.b || cached_ttf_color.a != color.a) {
        clear_ttf_texture_cache();
        cached_ttf_color = color;
    }
    
    // Check if we already have this character cached
    if (ttf_texture_cache.count(c)) {
        return ttf_texture_cache[c];
    }
    
    // Create new texture for this character
    char text[2] = {c, '\0'};
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) return nullptr;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    
    if (texture) {
        ttf_texture_cache[c] = texture;
    }
    
    return texture;
}

void VirtualKeyboard::clear_ttf_texture_cache() {
    // Destroy all cached TTF textures
    for (auto& pair : ttf_texture_cache) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    ttf_texture_cache.clear();
}

void VirtualKeyboard::draw(SDL_Renderer* renderer, int screen_width, int screen_height) {
    if (!active && !show_help) return;
    
    if (show_help) {
        // Draw help overlay with semi-transparent background
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);  // Semi-transparent black
        SDL_Rect help_bg = {0, 0, screen_width, screen_height};
        SDL_RenderFillRect(renderer, &help_bg);
        
        const char* help_text[] = {
            "Alterm Virtual Keyboard Help",
            "",
            "Navigation:",
            "  Arrow Keys: Move cursor on keyboard",
            "  Enter/Return: Press selected key", 
            "  Escape: Toggle keyboard on/off",
            "  F1: Toggle shift mode",
            "  F2: Toggle keyboard location (top/bottom)",
            "",
            "Terminal Commands:",
            "  clear: Clear screen",
            "  exit: Quit application",
            "  settings: Enter settings mode",
            "",
            "Press any key to continue...",
        };
        
        int y_pos = 50;
        for (const char* line : help_text) {
            if (strlen(line) == 0) {
                y_pos += 20;
                continue;
            }
            // Use bitmap font if available, otherwise TTF font
            if (bitmap_font) {
                draw_string_bitmap(renderer, line, 50, y_pos, {255, 255, 255, 255});
            } else {
                draw_string(renderer, line, 50, y_pos, {255, 255, 255, 255});
            }
            y_pos += 25;
        }
        return;
    }

    
    int key_height = char_height - 2;
    
    // Calculate keyboard position
    int total_width = 0;
    for (int i = 0; i < row_length[0] && syms[0][0][i]; i++) {
        total_width += strlen(syms[0][0][i]) * char_width + 8 + 4;  // Key width + padding
    }
    
    int start_x = (screen_width - total_width) / 2;
    int start_y = location == 0 ? screen_height - (NUM_ROWS * key_height) - 30 : 20;
    
    // Set up blending for keyboard overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    // Draw keyboard background with better opacity
    SDL_Rect bg_rect = {start_x - 4, start_y - 3, total_width + 3, NUM_ROWS * (key_height + 2) + 4};
    SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, 255);  // More opaque
    SDL_RenderFillRect(renderer, &bg_rect);
    
    // Draw keys
    int y = start_y;

    for (int row = 0; row < NUM_ROWS; row++) {
        int x = start_x;
        
        for (int col = 0; col < row_length[row] && syms[shifted][row][col]; col++) {
            const char* key_text = syms[shifted][row][col];
            int key_width = strlen(key_text) * char_width + 8;


            // Determine key color
            SDL_Color current_color = key_color;
            if (toggled[row][col]) {
                current_color = (selected_i == col && selected_j == row) ? sel_toggled_color : toggled_color;
            } else if (selected_i == col && selected_j == row) {
                current_color = sel_color;
            }
            
            // Draw key background
            SDL_Rect key_rect = {x, y, key_width, key_height};
            SDL_SetRenderDrawColor(renderer, current_color.r, current_color.g, current_color.b, current_color.a);
            SDL_RenderFillRect(renderer, &key_rect);
            
            // Draw key text - use bitmap font if available, otherwise TTF font
            if (bitmap_font) {
                draw_string_bitmap(renderer, key_text, x + 4, y + 2, text_color);
            } else {
                draw_string(renderer, key_text, x + 4, y - 2, text_color);
            }
            
            x += key_width + 4;
        }
        y += key_height + 2;
    }
}

int VirtualKeyboard::compute_visual_offset(int col, int row) {
    int sum = 0;
    for (int i = 0; i < col; i++) {
        sum += 1 + strlen(syms[0][row][i]);
    }
    sum += (1 + strlen(syms[0][row][col])) / 2;
    return sum;
}

int VirtualKeyboard::compute_new_col(int visual_offset, int old_row, int new_row) {
    // For the short last row (space bar row), manually adjust mapping
    if (new_row == 5) {
        if (old_row == 0) {
            if (selected_i >= 4 && selected_i <= 7) {
                return 3; // Space key
            }
        } else if (old_row == 4) {
            if (selected_i >= 6 && selected_i <= 9) {
                return 3; // Space key
            }
        }
    }
    
    // Original logic for other cases
    int new_sum = 0;
    int new_col = 0;
    while (new_col < row_length[new_row] - 1 && new_sum + (1 + strlen(syms[0][new_row][new_col])) / 2 < visual_offset) {
        new_sum += 1 + strlen(syms[0][new_row][new_col]);
        new_col++;
    }
    return new_col;
}

void VirtualKeyboard::update_shift_state() {
    // Update shifted state based on shift key toggle
    shifted = toggled[4][0] || toggled[4][11]; // Left or right shift
}

bool VirtualKeyboard::handle_event(SDL_Event* event) {
    if (event->type == SDL_KEYDOWN) {
        SDL_Keycode key = event->key.keysym.sym;
        
        // Global toggle key (Escape)
        if (key == SDLK_ESCAPE) {
            if (show_help) {
                show_help = false;
            } else {
                active = !active;
            }
            return true;
        }
        
        // If help is shown, any key dismisses it
        if (show_help) {
            show_help = false;
            return true;
        }
        
        // If keyboard is not active, don't handle navigation
        if (!active) {
            return false;
        }
        
        // Handle keyboard navigation
        switch (key) {
            case SDLK_LEFT:
                if (selected_i > 0) {
                    selected_i--;
                } else {
                    selected_i = row_length[selected_j] - 1;
                }
                visual_offset = compute_visual_offset(selected_i, selected_j);
                return true;
                
            case SDLK_RIGHT:
                if (selected_i < row_length[selected_j] - 1) {
                    selected_i++;
                } else {
                    selected_i = 0;
                }
                visual_offset = compute_visual_offset(selected_i, selected_j);
                return true;
                
            case SDLK_UP:
                if (selected_j > 0) {
                    selected_i = compute_new_col(visual_offset, selected_j, selected_j - 1);
                    selected_j--;
                } else {
                    selected_i = compute_new_col(visual_offset, selected_j, NUM_ROWS - 1);
                    selected_j = NUM_ROWS - 1;
                }
                if (selected_i >= row_length[selected_j]) {
                    selected_i = row_length[selected_j] - 1;
                }
                return true;
                
            case SDLK_DOWN:
                if (selected_j < NUM_ROWS - 1) {
                    selected_i = compute_new_col(visual_offset, selected_j, selected_j + 1);
                    selected_j++;
                } else {
                    selected_i = compute_new_col(visual_offset, selected_j, 0);
                    selected_j = 0;
                }
                if (selected_i < 0) {
                    selected_i = 0;
                }
                return true;
                
            case SDLK_RETURN:
            case SDLK_SPACE: {
                // Press the selected key
                SDL_Keycode target_key = keys[shifted][selected_j][selected_i];
                
                if (target_key == KEY_QUIT) {
                    // Send quit event
                    SDL_Event quit_event;
                    quit_event.type = SDL_QUIT;
                    SDL_PushEvent(&quit_event);
                } else if (target_key == SDLK_LSHIFT || target_key == SDLK_RSHIFT) {
                    // Toggle shift
                    toggled[selected_j][selected_i] = !toggled[selected_j][selected_i];
                    update_shift_state();
                } else if (target_key == SDLK_LCTRL || target_key == SDLK_RCTRL ||
                          target_key == SDLK_LALT || target_key == SDLK_RALT ||
                          target_key == SDLK_LGUI || target_key == SDLK_RGUI ||
                          target_key == SDLK_CAPSLOCK) {
                    // Toggle modifier keys
                    toggled[selected_j][selected_i] = !toggled[selected_j][selected_i];
                } else {
                    // Handle regular keys
                    if (target_key >= 32 && target_key < 127) {
                        // For printable characters, send as text input
                        char text[2] = {(char)target_key, '\0'};
                        simulate_text_input(text);
                    } else {
                        // For special keys (arrows, function keys, etc.), send as key event
                        simulate_key_event(target_key, true);
                        simulate_key_event(target_key, false);
                    }
                }
                return true;
            }
            
            case SDLK_F1:
                // Toggle shift mode
                shifted = !shifted;
                toggled[4][0] = shifted; // Update left shift toggle
                return true;
                
            case SDLK_F2:
                // Toggle keyboard location
                location = 1 - location;
                return true;
        }
    }
    
    return false;
}