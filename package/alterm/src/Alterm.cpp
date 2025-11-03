#include "../include/Alterm.hpp"

#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>

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
