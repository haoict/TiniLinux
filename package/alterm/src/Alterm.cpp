#include "../include/Alterm.hpp"

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>

#include <algorithm>
#include <string>

#include "../include/AnsFilter.hpp"
#include "../include/VirtualKeyboard.hpp"

// Initialize SDL and Create window&renderer

bool alterm::initialize_window() {
    bool InitializeVar = true;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        InitializeVar = false;
    }

    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        InitializeVar = false;
    }

    this->pWindow = SDL_CreateWindow("alterm", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 900, 700, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (this->pWindow == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        InitializeVar = false;
    }

    this->pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
    if (this->pRenderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(pWindow);
        SDL_Quit();
        InitializeVar = false;
    }

    // Initialize screen buffer
    init_screen_buffer();

    return InitializeVar;
}

void alterm::render_background(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    SDL_SetRenderDrawBlendMode(this->pRenderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(this->pRenderer, 0, 0, 0, 255);
    SDL_RenderClear(this->pRenderer);

    SDL_SetRenderDrawBlendMode(this->pRenderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(this->pRenderer, r, g, b, a);

    int w, h;
    SDL_GetWindowSize(this->pWindow, &w, &h);
    SDL_Rect bgRect = {0, 0, w, h};
    SDL_RenderFillRect(this->pRenderer, &bgRect);

    SDL_SetRenderDrawBlendMode(this->pRenderer, SDL_BLENDMODE_NONE);
}

// This function as simple as it, it reduces the amount of code needed to draw
// and the consuming of memory, it take a char and make a texture for it, and
// save it in a map of char to reuse it when it is needed.

SDL_Texture *alterm::get_cached_texture(char c) {
    // Check if we need to clear cache due to color change
    SDL_Color current_color = {0, 0, 0, 255};
    if (use_bitmap_font) {
        current_color = cached_font_color;
    } else if (FontManagarInstance) {
        current_color = FontManagarInstance->FontColor;
    }

    // Clear cache if color changed from what was used to create cached textures
    static SDL_Color last_used_color = {0, 0, 0, 0};  // Track actual color used for cache
    if (last_used_color.r != current_color.r || last_used_color.g != current_color.g || last_used_color.b != current_color.b || last_used_color.a != current_color.a) {
        clear_char_texture_cache();
        last_used_color = current_color;
    }

    if (CharTextureChache.count(c)) {
        return CharTextureChache[c];
    }

    SDL_Texture *Texture = nullptr;

    if (use_bitmap_font && bitmap_font) {
        // Use bitmap font - ensure correct color is set for terminal text
        bitmap_font->set_color(current_color.r, current_color.g, current_color.b);
        Texture = bitmap_font->create_char_texture(c);
    } else if (Font && FontManagarInstance) {
        // Use TTF font
        std::string s(1, c);
        SDL_Surface *Surface = TTF_RenderUTF8_Solid(Font, s.c_str(), current_color);
        if (Surface) {
            Texture = SDL_CreateTextureFromSurface(this->pRenderer, Surface);
            SDL_FreeSurface(Surface);
        }
    }

    if (Texture) {
        CharTextureChache[c] = Texture;
    }
    return Texture;
}

SDL_Texture *alterm::get_colored_cached_texture(char c, SDL_Color color) {
    // Create a color key by packing RGBA into uint32_t
    uint32_t color_key = (color.r << 24) | (color.g << 16) | (color.b << 8) | color.a;

    // Create cache key
    std::pair<char, uint32_t> cache_key = {c, color_key};

    // Fast lookup using find instead of count (more efficient)
    auto it = ColoredCharCache.find(cache_key);
    if (it != ColoredCharCache.end()) {
        return it->second;
    }

    SDL_Texture *texture = nullptr;

    if (use_bitmap_font && bitmap_font) {
        // Set bitmap font color and create texture
        bitmap_font->set_color(color.r, color.g, color.b);
        texture = bitmap_font->create_char_texture(c);
    } else if (Font && FontManagarInstance) {
        // Use TTF font with specified color
        std::string s(1, c);
        SDL_Surface *Surface = TTF_RenderUTF8_Solid(Font, s.c_str(), color);
        if (Surface) {
            texture = SDL_CreateTextureFromSurface(this->pRenderer, Surface);
            SDL_FreeSurface(Surface);
        }
    }

    if (texture) {
        ColoredCharCache[cache_key] = texture;
    }

    return texture;
}

void alterm::update_window_size() {
    SDL_GetWindowSize(this->pWindow, &window_width, &window_height);
    printf("Window size: %d x %d\n", window_width, window_height);
    SDL_Texture *pTexture = get_colored_cached_texture('A', SDL_Color{255, 255, 255, 255});
    SDL_QueryTexture(pTexture, NULL, NULL, &texture_width, &texture_height);
    printf("Texture width: %d Texture height: %d\n", texture_width, texture_height);
}

void alterm::clear_char_texture_cache() {
    // Destroy all cached textures
    for (auto &pair : CharTextureChache) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    CharTextureChache.clear();

    // Clear colored cache too
    for (auto &pair : ColoredCharCache) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    ColoredCharCache.clear();
}

// This function is used to setup the font for the terminal
// It takes 6 parameters:
// r, g, b - the color of the text
// path - the path to the font
// size - the size of the font
// text - the text to be displayed
// and set those values to the TTF_Font* pointer (Font).

void alterm::setup_font(const Uint8 r, const Uint8 g, const Uint8 b, const char *path, const int size, const char *text) {
    // Check if we should use embedded bitmap font
    if (std::string(path) == "embedded_font") {
        use_bitmap_font = true;
        bitmap_font = new BitmapFont();
        bitmap_font->initialize(pRenderer, size);  // Use size as scale factor
        bitmap_font->set_color(r, g, b);
        Font = nullptr;  // No TTF font needed
        // Initialize cached color for bitmap font
        cached_font_color = {r, g, b, 255};
    } else {
        use_bitmap_font = false;
        FontManagarInstance = new FontManagar();
        FontManagarInstance->Color(r, g, b);
        FontManagarInstance->fontFamily(path, size);
        Font = FontManagarInstance->getFont();
        // Initialize cached color for TTF font
        cached_font_color = {r, g, b, 255};
    }
}

// All the have work here
// This function render the response from the bash through pty and store it in a
// vector of strings. It takes 1 parameter: InputBuffer - the user input string.
// This function marge the tow response in one string and print every character
// individualy. And also make sure that the render don't exceed the screen size.
void alterm::renderer_screen(std::string &InputBuffer, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool ShowCursor, bool should_present) {
    render_background(r, g, b, a);

    // If in alternate screen mode, render screen buffer instead of lines
    if (alternate_screen) {
        // Ensure texture dimensions are cached and screen buffer is sized correctly
        if (texture_width == 0 || texture_height == 0) {
            update_window_size();
        }
        update_screen_dimensions();

        // Render screen buffer
        const SDL_Rect srcRect = {0, 0, texture_width, texture_height};
        const int lineHeight = texture_height + 2;

        for (int row = 0; row < screen_rows && row * lineHeight < window_height && row < screen_buffer.size(); ++row) {
            for (int col = 0; col < screen_cols && col * texture_width < window_width && col < screen_buffer[row].size(); ++col) {
                const ColoredChar &cell = screen_buffer[row][col];

                const SDL_Rect cellRect = {col * texture_width, row * lineHeight, texture_width, texture_height};

                // Render background color if not transparent
                if (cell.bg_color.a > 0) {
                    SDL_SetRenderDrawColor(this->pRenderer, cell.bg_color.r, cell.bg_color.g, cell.bg_color.b, cell.bg_color.a);
                    SDL_RenderFillRect(this->pRenderer, &cellRect);
                }

                // Render character if not space or if it has a background
                if (cell.character != ' ' || cell.bg_color.a > 0) {
                    SDL_Texture *pTexture = get_colored_cached_texture(cell.character, cell.fg_color);
                    if (pTexture) {
                        SDL_RenderCopy(this->pRenderer, pTexture, &srcRect, &cellRect);
                    }
                }
            }
        }

        // Draw cursor in screen buffer mode
        if (ShowCursor && cursor_row >= 0 && cursor_row < screen_rows && cursor_col >= 0 && cursor_col < screen_cols) {
            SDL_SetRenderDrawColor(this->pRenderer, 255, 255, 255, 255);
            SDL_Rect cursor_rect = {cursor_col * texture_width, cursor_row * lineHeight, 2, texture_height};
            SDL_RenderFillRect(this->pRenderer, &cursor_rect);
        }

        if (should_present) {
            SDL_RenderPresent(this->pRenderer);
        }
        return;
    }

    // Ensure texture dimensions are cached (only if not already set)
    if (texture_width == 0 || texture_height == 0) {
        update_window_size();
    }

    // Create visual lines by breaking long logical lines at screen width
    struct VisualLine {
        ColoredLine content;  // Store colored characters
        int logicalLineIndex;
        int height;
    };

    std::vector<VisualLine> visualLines;
    int totalVisualHeight = 0;

    // Process each logical line and break into visual lines
    for (int logicalIndex = 0; logicalIndex < lines.size(); ++logicalIndex) {
        std::string fullLine = lines[logicalIndex];

        // Add input buffer to the last line
        if (logicalIndex == lines.size() - 1 && !InputBuffer.empty()) {
            fullLine += InputBuffer;
        }

        // Break this logical line into visual lines based on screen width
        ColoredLine currentVisualLine;
        int x = 0;

        ColoredLine colored_line = parse_ansi_sequences(fullLine);

        for (const ColoredChar &colored_char : colored_line) {
            // Skip texture creation, just calculate positions using cached dimensions
            if (x + texture_width > window_width && !currentVisualLine.empty()) {
                // This character would exceed screen width, finish current visual line
                VisualLine vLine;
                vLine.content = currentVisualLine;
                vLine.logicalLineIndex = logicalIndex;
                vLine.height = texture_height + 2;
                visualLines.push_back(vLine);
                totalVisualHeight += vLine.height;

                // Start new visual line with this character
                currentVisualLine.clear();
                currentVisualLine.push_back(colored_char);
                x = texture_width;
            } else {
                // Add character to current visual line
                currentVisualLine.push_back(colored_char);
                x += texture_width;
            }
        }

        // Add the remaining content as a visual line
        if (!currentVisualLine.empty() || fullLine.empty()) {
            VisualLine vLine;
            vLine.content = currentVisualLine;
            vLine.logicalLineIndex = logicalIndex;
            vLine.height = texture_height + 2;
            visualLines.push_back(vLine);
            totalVisualHeight += vLine.height;
        }
    }

    int totalVisualLines = visualLines.size();

    // Limit visual lines to prevent memory issues (logical lines are already managed in forkpty.cpp)
    int max_lines = settings_manager ? settings_manager->get_max_lines() : 64;  // Default fallback
    if (totalVisualLines > max_lines) {
        // Calculate how many visual lines to remove from the beginning
        int removeCount = totalVisualLines - max_lines;
        visualLines.erase(visualLines.begin(), visualLines.begin() + removeCount);
        totalVisualLines = visualLines.size();
    }

    // Calculate which visual lines fit in the window
    int accumulated = 0;
    int startVisualIndex = 0;

    for (int i = totalVisualLines - 1; i >= 0; --i) {
        accumulated += visualLines[i].height;
        if (accumulated > window_height) {
            startVisualIndex = i + 1;
            break;
        }
    }

    int visibleVisualLines = totalVisualLines - startVisualIndex;

    // Auto-scroll: if we have input buffer and cursor would be off-screen, adjust scroll
    if (!InputBuffer.empty() && startVisualIndex > 0) {
        ScrollOffSet = 0;
    }

    // Apply scroll offset (now working on visual lines)
    startVisualIndex = std::max(0, startVisualIndex - ScrollOffSet);

    int maxScroll = std::max(0, totalVisualLines - visibleVisualLines);
    ScrollOffSet = std::min(ScrollOffSet, maxScroll);

    // Render visual lines with optimizations
    int CurrentRenderY = 0;
    const int lastLogicalLineIndex = lines.empty() ? -1 : lines.size() - 1;
    const SDL_Rect srcRect = {0, 0, texture_width, texture_height};  // Reuse same source rect
    const int lineHeight = texture_height + 2;

    for (int i = startVisualIndex; i < totalVisualLines; ++i) {
        // Early exit if we've rendered beyond the visible area
        if (CurrentRenderY >= window_height) break;

        const VisualLine &vLine = visualLines[i];
        const ColoredLine &coloredChars = vLine.content;

        int xPos = 0;
        const int yPos = CurrentRenderY;

        // Skip empty lines quickly
        if (!coloredChars.empty()) {
            // Render characters with optimized loop
            for (const ColoredChar &colored_char : coloredChars) {
                // Early exit if character is beyond visible width
                if (xPos >= window_width) break;

                // Only render if character is visible (not space and within bounds)
                if (colored_char.character != ' ') {
                    SDL_Texture *pTexture = get_colored_cached_texture(colored_char.character, colored_char.fg_color);
                    if (pTexture) {
                        const SDL_Rect dstRect = {xPos, yPos, texture_width, texture_height};
                        SDL_RenderCopy(this->pRenderer, pTexture, &srcRect, &dstRect);
                    }
                }
                xPos += texture_width;
            }
        }

        // Update cursor position if this is the last visual line of the last logical line
        if (vLine.logicalLineIndex == lastLogicalLineIndex && i == totalVisualLines - 1) {
            CursorX = xPos;
            CursorH = texture_height;
            this->y = yPos;
        }

        CurrentRenderY += lineHeight;
    }

    // printf("Debug: Lines=%zu, VisualLines=%d, StartIndex=%d, VisibleLines=%d\n", lines.size(), totalVisualLines, startVisualIndex, visibleVisualLines);

    if (ShowCursor && !ScrollOffSet) {
        SDL_Rect cursor_Rect = {CursorX, this->y, 10, CursorH};
        SDL_SetRenderDrawColor(pRenderer, 255, 255, 255, 255);
        SDL_RenderFillRect(pRenderer, &cursor_Rect);
    }

    if (should_present) {
        SDL_RenderPresent(this->pRenderer);
    }
}

void alterm::reset_display() {
    this->y = 0;
    lines.clear();
    SDL_SetRenderDrawColor(this->pRenderer, 0, 0, 0, 255);
    SDL_RenderClear(this->pRenderer);
    SDL_RenderPresent(this->pRenderer);
}

void alterm::shutdown_sdl() {
    // Clean up render textures
    if (terminal_texture) {
        SDL_DestroyTexture(terminal_texture);
        terminal_texture = nullptr;
    }
    if (keyboard_texture) {
        SDL_DestroyTexture(keyboard_texture);
        keyboard_texture = nullptr;
    }

    SDL_DestroyWindow(pWindow);
    SDL_DestroyRenderer(pRenderer);

    if (FontManagarInstance) {
        FontManagarInstance->free();
        delete FontManagarInstance;
    }

    if (bitmap_font) {
        delete bitmap_font;
    }

    for (auto &pair : CharTextureChache) {
        SDL_DestroyTexture(pair.second);
    }
    CharTextureChache.clear();
    SDL_Quit();
}

bool alterm::create_render_textures() {
    SDL_GetWindowSize(pWindow, &window_width, &window_height);

    // Create terminal texture
    terminal_texture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, window_height);
    if (!terminal_texture) {
        std::cerr << "Failed to create terminal texture: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create keyboard texture with alpha blending
    keyboard_texture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, window_height);
    if (!keyboard_texture) {
        std::cerr << "Failed to create keyboard texture: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetTextureBlendMode(keyboard_texture, SDL_BLENDMODE_BLEND);
    return true;
}

void alterm::render_terminal_to_texture(std::string &input_buffer, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool ShowCursor) {
    if (!terminal_texture) return;

    // Set render target to terminal texture
    SDL_SetRenderTarget(pRenderer, terminal_texture);
    renderer_screen(input_buffer, r, g, b, a, ShowCursor, false);

    // Reset render target
    SDL_SetRenderTarget(pRenderer, nullptr);
}

void alterm::render_keyboard_to_texture(void *keyboard_ptr) {
    if (!keyboard_texture || !keyboard_ptr) return;

    VirtualKeyboard *keyboard = static_cast<VirtualKeyboard *>(keyboard_ptr);
    SDL_SetRenderTarget(pRenderer, keyboard_texture);

    // Clear with transparent background
    SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0);
    SDL_RenderClear(pRenderer);

    // Draw keyboard if active
    if (keyboard->is_active() || keyboard->is_help_shown()) {
        keyboard->draw(pRenderer, window_width, window_height);
    }

    // Reset render target
    SDL_SetRenderTarget(pRenderer, nullptr);
}

void alterm::composite_and_present() {
    // Clear main render target
    SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 255);
    SDL_RenderClear(pRenderer);

    // Draw terminal texture
    if (terminal_texture) {
        SDL_RenderCopy(pRenderer, terminal_texture, nullptr, nullptr);
    }

    // Draw keyboard texture on top
    if (keyboard_texture) {
        SDL_RenderCopy(pRenderer, keyboard_texture, nullptr, nullptr);
    }

    // Present final result
    SDL_RenderPresent(pRenderer);
}

// Screen buffer methods
void alterm::init_screen_buffer() {
    // Ensure we have proper dimensions before initializing
    if (texture_width == 0 || texture_height == 0) {
        update_window_size();
    }
    update_screen_dimensions();

    screen_buffer.resize(screen_rows);
    for (auto &row : screen_buffer) {
        row.resize(screen_cols);
        for (auto &cell : row) {
            cell.character = ' ';
            cell.fg_color = {255, 255, 255, 255};  // White text by default
            cell.bg_color = {0, 0, 0, 0};          // Transparent background by default
        }
    }
    cursor_row = 0;
    cursor_col = 0;

    // Initialize scroll region to full screen
    scroll_top = 0;
    scroll_bottom = screen_rows - 1;
}

void alterm::clear_screen_buffer() {
    for (auto &row : screen_buffer) {
        for (auto &cell : row) {
            cell.character = ' ';
            cell.fg_color = {255, 255, 255, 255};
            cell.bg_color = {0, 0, 0, 0};  // Reset to transparent
        }
    }
    cursor_row = 0;
    cursor_col = 0;

    // Reset text attributes to defaults
    current_fg_color = {255, 255, 255, 255};  // White
    current_bg_color = {0, 0, 0, 0};          // Transparent
    reverse_video = false;
}

void alterm::set_cursor_position(int row, int col) {
    cursor_row = std::max(0, std::min(row, screen_rows - 1));
    cursor_col = std::max(0, std::min(col, screen_cols - 1));
}

void alterm::write_to_screen_buffer(const std::string &text) {
    for (char c : text) {
        if (c >= 32 && c <= 126) {  // Only printable ASCII characters
            if (cursor_row >= 0 && cursor_row < screen_rows && cursor_col >= 0 && cursor_col < screen_cols) {
                screen_buffer[cursor_row][cursor_col].character = c;

                // Apply current text attributes
                if (reverse_video) {
                    screen_buffer[cursor_row][cursor_col].fg_color = current_bg_color.a > 0 ? current_bg_color : SDL_Color{0, 0, 0, 255};
                    screen_buffer[cursor_row][cursor_col].bg_color = current_fg_color;
                } else {
                    screen_buffer[cursor_row][cursor_col].fg_color = current_fg_color;
                    screen_buffer[cursor_row][cursor_col].bg_color = current_bg_color;
                }

                cursor_col++;
                // Don't auto-wrap - let escape sequences handle positioning
                if (cursor_col >= screen_cols) {
                    cursor_col = screen_cols - 1;  // Stay at edge
                }
            }
        }
        // Note: Don't handle \n, \r here - let control character processing in process_char handle them
    }
}

void alterm::handle_escape_sequence(const std::string &sequence) {
    if (sequence.length() < 2) {
        return;
    }

    // Handle different types of escape sequences
    if (sequence.find("\033[?") == 0) {
        // Private mode sequences
        if (sequence.find("1049h") != std::string::npos) {
            alternate_screen = true;
            clear_screen_buffer();
        } else if (sequence.find("1049l") != std::string::npos) {
            alternate_screen = false;
            // Reset text attributes when exiting alternate screen
            current_fg_color = {255, 255, 255, 255};  // White
            current_bg_color = {0, 0, 0, 0};          // Transparent
            reverse_video = false;
        } else if (sequence.find("25l") != std::string::npos) {
            // Hide cursor
        } else if (sequence.find("25h") != std::string::npos) {
            // Show cursor
        }
        return;
    }

    if (sequence.find("\033]") == 0) {
        // OSC sequences (window title, etc.) - ignore for now
        return;
    }

    if (sequence.find("\033(") == 0 || sequence.find("\033=") == 0) {
        // Character set or keypad mode - ignore for now
        return;
    }

    if (sequence.find("\033[") != 0) {
        return;  // Not a CSI sequence we handle
    }

    // CSI sequence
    std::string params = sequence.substr(2, sequence.length() - 3);
    char command = sequence.back();

    switch (command) {
        case 'H':  // Cursor position
        case 'f': {
            int row = 1, col = 1;
            size_t semicolon = params.find(';');
            try {
                if (semicolon != std::string::npos) {
                    if (semicolon > 0) {
                        std::string row_str = params.substr(0, semicolon);
                        if (!row_str.empty()) row = std::stoi(row_str);
                    }
                    if (semicolon + 1 < params.length()) {
                        std::string col_str = params.substr(semicolon + 1);
                        if (!col_str.empty()) col = std::stoi(col_str);
                    }
                } else if (!params.empty()) {
                    row = std::stoi(params);
                }
            } catch (const std::exception &) {
                // Invalid number, use defaults
            }

            // Convert to 0-based positioning
            int new_row = row - 1;

            set_cursor_position(new_row, col - 1);
            break;
        }
        case 'J':  // Erase display
            if (params.empty() || params == "0") {
                // Clear from cursor to end of screen
                for (int r = cursor_row; r < screen_rows; r++) {
                    int start_col = (r == cursor_row) ? cursor_col : 0;
                    for (int c = start_col; c < screen_cols; c++) {
                        if (r < screen_buffer.size() && c < screen_buffer[r].size()) {
                            screen_buffer[r][c].character = ' ';
                            screen_buffer[r][c].bg_color = {0, 0, 0, 0};  // Reset background
                        }
                    }
                }
            } else if (params == "1") {
                // Clear from start of screen to cursor
                for (int r = 0; r <= cursor_row && r < screen_rows; r++) {
                    int end_col = (r == cursor_row) ? cursor_col : screen_cols - 1;
                    for (int c = 0; c <= end_col && c < screen_cols; c++) {
                        if (r < screen_buffer.size() && c < screen_buffer[r].size()) {
                            screen_buffer[r][c].character = ' ';
                            screen_buffer[r][c].bg_color = {0, 0, 0, 0};  // Reset background
                        }
                    }
                }
            } else if (params == "2") {
                clear_screen_buffer();
            }
            break;
        case 'K':  // Erase line
            if (cursor_row < screen_buffer.size()) {
                if (params.empty() || params == "0") {
                    // Clear from cursor to end of line
                    for (int c = cursor_col; c < screen_cols && c < screen_buffer[cursor_row].size(); c++) {
                        screen_buffer[cursor_row][c].character = ' ';
                        screen_buffer[cursor_row][c].bg_color = {0, 0, 0, 0};  // Reset background
                    }
                } else if (params == "1") {
                    // Clear from start of line to cursor
                    for (int c = 0; c <= cursor_col && c < screen_buffer[cursor_row].size(); c++) {
                        screen_buffer[cursor_row][c].character = ' ';
                        screen_buffer[cursor_row][c].bg_color = {0, 0, 0, 0};  // Reset background
                    }
                } else if (params == "2") {
                    // Clear entire line
                    for (int c = 0; c < screen_cols && c < screen_buffer[cursor_row].size(); c++) {
                        screen_buffer[cursor_row][c].character = ' ';
                        screen_buffer[cursor_row][c].bg_color = {0, 0, 0, 0};  // Reset background
                    }
                }
            }
            break;
        case 'A':  // Cursor up
            try {
                int n = params.empty() ? 1 : std::stoi(params);
                cursor_row = std::max(0, cursor_row - n);
            } catch (const std::exception &) {
            }
            break;
        case 'B':  // Cursor down
            try {
                int n = params.empty() ? 1 : std::stoi(params);
                cursor_row = std::min(screen_rows - 1, cursor_row + n);
            } catch (const std::exception &) {
            }
            break;
        case 'C':  // Cursor forward
            try {
                int n = params.empty() ? 1 : std::stoi(params);
                cursor_col = std::min(screen_cols - 1, cursor_col + n);
            } catch (const std::exception &) {
            }
            break;
        case 'D':  // Cursor backward
            try {
                int n = params.empty() ? 1 : std::stoi(params);
                cursor_col = std::max(0, cursor_col - n);
            } catch (const std::exception &) {
            }
            break;
        case 'G':  // Cursor horizontal absolute
            try {
                int col = params.empty() ? 1 : std::stoi(params);
                cursor_col = std::max(0, std::min(col - 1, screen_cols - 1));
            } catch (const std::exception &) {
            }
            break;
        case 'd':  // Line position absolute
            try {
                int row = params.empty() ? 1 : std::stoi(params);
                cursor_row = std::max(0, std::min(row - 1, screen_rows - 1));
            } catch (const std::exception &) {
            }
            break;
        case 'X':  // Erase characters
            try {
                int n = params.empty() ? 1 : std::stoi(params);
                for (int i = 0; i < n && cursor_col + i < screen_cols; i++) {
                    if (cursor_row < screen_buffer.size() && cursor_col + i < screen_buffer[cursor_row].size()) {
                        screen_buffer[cursor_row][cursor_col + i].character = ' ';
                        screen_buffer[cursor_row][cursor_col + i].bg_color = {0, 0, 0, 0};  // Reset background
                    }
                }
            } catch (const std::exception &) {
            }
            break;
        case 'm':  // SGR - Select Graphic Rendition (colors/attributes)
        {
            if (params.empty()) {
                // Reset all attributes
                current_fg_color = {255, 255, 255, 255};  // White
                current_bg_color = {0, 0, 0, 0};          // Transparent
                reverse_video = false;
            } else {
                // Parse multiple parameters separated by semicolons
                std::string param_str = params + ";";  // Add trailing semicolon for easier parsing
                size_t pos = 0;
                while (pos < param_str.length()) {
                    size_t next_pos = param_str.find(';', pos);
                    if (next_pos == std::string::npos) break;

                    std::string param = param_str.substr(pos, next_pos - pos);
                    if (!param.empty()) {
                        try {
                            int code = std::stoi(param);
                            switch (code) {
                                case 0:  // Reset
                                    current_fg_color = {255, 255, 255, 255};
                                    current_bg_color = {0, 0, 0, 0};
                                    reverse_video = false;
                                    break;
                                case 7:  // Reverse video
                                    reverse_video = true;
                                    break;
                                case 27:  // Not reverse video
                                    reverse_video = false;
                                    break;
                                case 30:
                                    current_fg_color = {0, 0, 0, 255};
                                    break;  // Black
                                case 31:
                                    current_fg_color = {255, 0, 0, 255};
                                    break;  // Red
                                case 32:
                                    current_fg_color = {0, 255, 0, 255};
                                    break;  // Green
                                case 33:
                                    current_fg_color = {255, 255, 0, 255};
                                    break;  // Yellow
                                case 34:
                                    current_fg_color = {0, 0, 255, 255};
                                    break;  // Blue
                                case 35:
                                    current_fg_color = {255, 0, 255, 255};
                                    break;  // Magenta
                                case 36:
                                    current_fg_color = {0, 255, 255, 255};
                                    break;  // Cyan
                                case 37:
                                    current_fg_color = {255, 255, 255, 255};
                                    break;  // White
                                case 40:
                                    current_bg_color = {0, 0, 0, 255};
                                    break;  // Black background
                                case 41:
                                    current_bg_color = {255, 0, 0, 255};
                                    break;  // Red background
                                case 42:
                                    current_bg_color = {0, 255, 0, 255};
                                    break;  // Green background
                                case 43:
                                    current_bg_color = {255, 255, 0, 255};
                                    break;  // Yellow background
                                case 44:
                                    current_bg_color = {0, 0, 255, 255};
                                    break;  // Blue background
                                case 45:
                                    current_bg_color = {255, 0, 255, 255};
                                    break;  // Magenta background
                                case 46:
                                    current_bg_color = {0, 255, 255, 255};
                                    break;  // Cyan background
                                case 47:
                                    current_bg_color = {255, 255, 255, 255};
                                    break;  // White background
                            }
                        } catch (const std::exception &) {
                            // Invalid number, skip
                        }
                    }
                    pos = next_pos + 1;
                }
            }
        } break;
        case 'r':  // Set scrolling region
        {
            int top = 1, bottom = screen_rows;
            size_t semicolon = params.find(';');
            try {
                if (semicolon != std::string::npos) {
                    if (semicolon > 0) {
                        std::string top_str = params.substr(0, semicolon);
                        if (!top_str.empty()) top = std::stoi(top_str);
                    }
                    if (semicolon + 1 < params.length()) {
                        std::string bottom_str = params.substr(semicolon + 1);
                        if (!bottom_str.empty()) bottom = std::stoi(bottom_str);
                    }
                } else if (!params.empty()) {
                    top = std::stoi(params);
                }

                // Convert to 0-based and validate
                scroll_top = std::max(0, std::min(top - 1, screen_rows - 1));
                scroll_bottom = std::max(scroll_top, std::min(bottom - 1, screen_rows - 1));

                // Move cursor to home position in scroll region
                cursor_row = scroll_top;
                cursor_col = 0;
            } catch (const std::exception &) {
            }
        } break;
        case 'S':  // Scroll up
        {
            try {
                int n = params.empty() ? 1 : std::stoi(params);
                scroll_up_region(scroll_top, n);
            } catch (const std::exception &) {
            }
        } break;
        case 'T':  // Scroll down
        {
            try {
                int n = params.empty() ? 1 : std::stoi(params);
                printf("DEBUG: Scroll DOWN region called, lines=%d, origin=%d, region_bottom=%d\n", n, scroll_top, scroll_bottom);
                scroll_down_region(scroll_top, n);
                printf("DEBUG: Scroll DOWN region completed, lines=%d, origin=%d, region_bottom=%d\n", n, scroll_top, scroll_bottom);
            } catch (const std::exception &) {
            }
        } break;
        case 't':  // Window operations
            // Ignore for now
            break;
        case 'l':  // Reset mode
        case 'h':  // Set mode
            // Various mode settings - ignore for now
            break;
    }
}

void alterm::update_screen_dimensions() {
    if (texture_width > 0 && texture_height > 0) {
        screen_cols = window_width / texture_width;
        screen_rows = window_height / (texture_height + 2);

        // Resize screen buffer if dimensions changed
        if (screen_buffer.size() != screen_rows) {
            screen_buffer.resize(screen_rows);
            for (auto &row : screen_buffer) {
                row.resize(screen_cols);
            }
        } else {
            for (auto &row : screen_buffer) {
                if (row.size() != screen_cols) {
                    row.resize(screen_cols);
                }
            }
        }
    }
}

void alterm::scroll_up(int lines) { scroll_up_region(0, lines); }

void alterm::scroll_down(int lines) { scroll_down_region(0, lines); }

void alterm::scroll_up_region(int origin, int lines) {
    if (lines <= 0 || origin < 0 || origin >= screen_buffer.size()) return;

    int region_bottom = std::min(scroll_bottom, (int)screen_buffer.size() - 1);

    // Limit lines to scroll within the region bounds
    lines = std::min(lines, region_bottom - origin + 1);
    if (lines <= 0) return;

    // Clear the region at top where new content will appear (like reference tclearregion)
    for (int row = origin; row < origin + lines; row++) {
        if (row >= 0 && row <= region_bottom && row < screen_buffer.size()) {
            for (auto &cell : screen_buffer[row]) {
                cell.character = ' ';
                cell.fg_color = {255, 255, 255, 255};
                cell.bg_color = {0, 0, 0, 0};
            }
        }
    }

    // Move lines up: iterate from top to avoid overwriting (like reference loop)
    for (int current_row = origin; current_row <= region_bottom - lines; current_row++) {
        int source_row = current_row + lines;
        if (source_row <= region_bottom && source_row < screen_buffer.size() && current_row >= 0) {
            std::swap(screen_buffer[current_row], screen_buffer[source_row]);
        }
    }

    printf("DEBUG: Scroll UP region completed, lines=%d, origin=%d, region_bottom=%d\n", lines, origin, region_bottom);
    // Mark screen buffer as dirty to trigger re-render
    screen_buffer_dirty = true;
}

void alterm::scroll_down_region(int origin, int lines) {
    if (lines <= 0 || origin < 0 || origin >= screen_buffer.size()) return;

    int region_bottom = std::min(scroll_bottom, (int)screen_buffer.size() - 1);

    // Limit lines to scroll within the region bounds
    lines = std::min(lines, region_bottom - origin + 1);
    if (lines <= 0) return;

    // Clear the region at bottom where new content will appear (like reference tclearregion)
    for (int row = region_bottom - lines + 1; row <= region_bottom; row++) {
        if (row >= 0 && row < screen_buffer.size()) {
            for (auto &cell : screen_buffer[row]) {
                cell.character = ' ';
                cell.fg_color = {255, 255, 255, 255};
                cell.bg_color = {0, 0, 0, 0};
            }
        }
    }

    // Move lines down: iterate from bottom to avoid overwriting (like reference loop)
    for (int current_row = region_bottom; current_row >= origin + lines; current_row--) {
        int source_row = current_row - lines;
        if (source_row >= origin && source_row >= 0 && current_row < screen_buffer.size() && source_row < screen_buffer.size()) {
            std::swap(screen_buffer[current_row], screen_buffer[source_row]);
        }
    }

    // Mark screen buffer as dirty to trigger re-render
    screen_buffer_dirty = true;
}

void alterm::process_char(char c) {
    // Handle escape sequence detection globally
    if (c == '\033') {
        // Start collecting escape sequence
        esc_buffer = "\033";
        esc_state = ESC_START;
        return;
    }

    if (esc_state != ESC_NORMAL) {
        // We're in an escape sequence
        esc_buffer += c;

        // Check for alternate screen sequences
        if (esc_buffer.find("\033[?1049h") != std::string::npos) {
            alternate_screen = true;
            clear_screen_buffer();
            esc_state = ESC_NORMAL;
            esc_buffer.clear();
            return;
        } else if (esc_buffer.find("\033[?1049l") != std::string::npos) {
            alternate_screen = false;
            // Reset text attributes when exiting alternate screen
            current_fg_color = {255, 255, 255, 255};  // White
            current_bg_color = {0, 0, 0, 0};          // Transparent
            reverse_video = false;
            esc_state = ESC_NORMAL;
            esc_buffer.clear();
            return;
        }

        // Simple check for end of CSI sequence
        if (esc_buffer.length() > 2 && esc_buffer[1] == '[' && ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '@' && c <= '~'))) {
            // Debug: Log ALL escape sequences to see what nano is actually sending (escaped)
            std::string escaped_seq;
            for (char ch : esc_buffer) {
                if (ch == '\033') {
                    escaped_seq += "\\033";
                } else if (ch >= 32 && ch <= 126) {
                    escaped_seq += ch;
                } else {
                    escaped_seq += "\\x" + std::to_string((unsigned char)ch);
                }
            }
            printf("DEBUG: All escape sequences: %s (alternate_screen=%s)\n", escaped_seq.c_str(), alternate_screen ? "true" : "false");

            // Always handle escape sequences (not just in alternate screen mode)
            // The reference implementation processes all escape sequences
            handle_escape_sequence(esc_buffer);
            esc_state = ESC_NORMAL;
            esc_buffer.clear();
        } else if (esc_buffer.length() > 20) {
            // Prevent buffer overflow - reset on very long sequences
            esc_state = ESC_NORMAL;
            esc_buffer.clear();
        }
        return;
    }

    // Character processing based on current mode
    if (alternate_screen) {
        // Screen buffer mode - handle all characters
        if (c >= 32) {
            // Printable character - write directly to current cursor position
            write_to_screen_buffer(std::string(1, c));
        } else {
            // Control characters in screen buffer mode - minimal handling
            switch (c) {
                case '\r':
                    cursor_col = 0;  // Carriage return
                    break;
                case '\b':
                    if (cursor_col > 0) cursor_col--;  // Backspace
                    break;
                case '\n':
                    // Line feed - move down one line, keep column
                    cursor_row++;
                    if (cursor_row > scroll_bottom) {
                        // Auto-scroll when cursor goes beyond scroll region bottom
                        scroll_up_region(scroll_top, 1);
                        cursor_row = scroll_bottom;
                    }
                    break;
                case '\t':
                    // Tab - move to next tab stop
                    cursor_col = ((cursor_col / 8) + 1) * 8;
                    if (cursor_col >= screen_cols) cursor_col = screen_cols - 1;
                    break;
                    // Ignore other control characters - let escape sequences handle positioning
            }
        }
    }
    // Note: Normal shell mode is handled separately in the existing line processing code
}
