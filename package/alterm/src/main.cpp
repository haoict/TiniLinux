#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <clocale>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../include/Alterm.hpp"
#include "../include/HistoryManager.hpp"
#include "../include/SettingsManager.hpp"
#include "../include/VirtualKeyboard.hpp"

// Function to check if we should enter raw mode (pass all input to shell)
bool should_use_raw_mode(const std::string& command) {
    // List of commands that require raw terminal input
    static const std::vector<std::string> raw_mode_commands = {"nano", "vi", "vim", "emacs", "less", "more", "man", "top", "htop", "tmux", "screen", "mc", "lynx", "w3m", "ssh", "telnet", "ftp"};

    // Extract first word (command name)
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    // Check if command needs raw mode
    for (const auto& raw_cmd : raw_mode_commands) {
        if (cmd == raw_cmd) {
            return true;
        }
    }

    return false;
}

std::vector<std::string> CommandHistory;
const size_t MaxHistorySize = 100;
int CommandHistoryIndex = -1;
const std::string HisotryFile = ".alterm_history";

// Function to set terminal window size
void set_terminal_size(int pty_fd, int cols, int rows) {
    struct winsize ws;
    ws.ws_col = cols;
    ws.ws_row = rows;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    if (ioctl(pty_fd, TIOCSWINSZ, &ws) == -1) {
        perror("ioctl TIOCSWINSZ failed");
    }
}

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "");

    load_history(CommandHistory, HisotryFile);
    SettingsManager* Settings = new SettingsManager();
    Settings->load_from_file(".alterm_settings");
    alterm* term = new alterm();
    term->set_settings_manager(Settings);

    int pty_fd;
    int child_pid = start_shell_with_pty(pty_fd);
    fcntl(pty_fd, F_SETFL, O_NONBLOCK);

    if (child_pid < 0) {
        std::cerr << "Failed to start shell." << std::endl;
        return (1);
    }

    // Set initial terminal size (80x25 is a standard default)
    set_terminal_size(pty_fd, 80, 25);

    if (term->initialize_window() == false) {
        std::cout << "Init_error" << std::endl;
        return (1);
    }

    Uint32 LastBlinkTime = SDL_GetTicks();
    bool ShowCursor = true;

    std::string fontSize = Settings->get("font_size");
    std::string fontPath = Settings->get("font_family");
    int FontSize = std::stoi(fontSize);

    Uint8 r = std::stoi(Settings->get("font_color_r"));
    Uint8 g = std::stoi(Settings->get("font_color_g"));
    Uint8 b = std::stoi(Settings->get("font_color_b"));

    Uint8 br = std::stoi(Settings->get("bg_color_r"));
    Uint8 bg = std::stoi(Settings->get("bg_color_g"));
    Uint8 bb = std::stoi(Settings->get("bg_color_b"));
    Uint8 ba = std::stoi(Settings->get("bg_opacity"));

    term->setup_font(r, g, b, fontPath.c_str(), FontSize, "Alterm");

    // Create render textures for layered rendering
    if (!term->create_render_textures()) {
        std::cerr << "Failed to create render textures" << std::endl;
        return 1;
    }

    // Initialize virtual keyboard
    VirtualKeyboard* vkb = new VirtualKeyboard();

    if (fontPath == "embedded_font") {
        // Use bitmap font for keyboard too
        vkb->initialize_bitmap(term->get_bitmap_font());
        // Pre-cache common characters for bitmap font
        vkb->pre_cache_common_chars({0, 0, 0, 255});  // Black text
    } else {
        // Use TTF font for keyboard
        TTF_Font* keyboard_font = term->get_font();
        vkb->initialize(keyboard_font);
        // Pre-cache common characters for TTF font (need renderer)
        vkb->pre_cache_common_chars({0, 0, 0, 255}, term->get_renderer());  // Black text
    }

    // Initial render to populate textures
    std::string empty_input = "";
    term->update_window_size();
    term->update_screen_dimensions();
    term->render_terminal_to_texture(empty_input, br, bg, bb, ba, ShowCursor);
    term->render_keyboard_to_texture(vkb);

    SDL_Event event;
    bool quit = false;
    std::string input_buffer;
    bool dirty = true;
    bool keyboard_dirty = false;  // Start false, only set true when keyboard changes
    bool in_settings_mode = false;

    // Frame limiting variables
    Uint32 frame_start = 0;
    const Uint32 frame_delay = 1000 / 60;  // 60 FPS limit

    SDL_StartTextInput();

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            // Let virtual keyboard handle events first
            if (vkb->handle_event(&event)) {
                keyboard_dirty = true;
                continue;
            }

            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }

            else if (event.type == SDL_TEXTINPUT) {
                char* text = event.text.text;

                // Check if this is a control character (ASCII 1-31)
                if (text[0] >= 1 && text[0] <= 31 && text[1] == '\0') {
                    // Control character - send directly to shell
                    write(pty_fd, text, 1);
                    if (text[0] == 3) {  // Ctrl+C
                        input_buffer.clear();
                    }
                } else {
                    // Regular text - send directly to PTY (like a proper terminal)
                    write(pty_fd, text, strlen(text));
                }
                dirty = true;
            }

            else if (event.type == SDL_KEYDOWN) {
                // Handle ALL keys directly like a proper terminal
                bool key_handled = false;

                // Handle Ctrl combinations first
                if (event.key.keysym.mod & KMOD_CTRL) {
                    char ctrl_char = 0;
                    switch (event.key.keysym.sym) {
                        case SDLK_a:
                            ctrl_char = '\x01';
                            break;  // Ctrl+A
                        case SDLK_b:
                            ctrl_char = '\x02';
                            break;  // Ctrl+B
                        case SDLK_c:
                            ctrl_char = '\x03';
                            break;  // Ctrl+C
                        case SDLK_d:
                            ctrl_char = '\x04';
                            break;  // Ctrl+D
                        case SDLK_e:
                            ctrl_char = '\x05';
                            break;  // Ctrl+E
                        case SDLK_f:
                            ctrl_char = '\x06';
                            break;  // Ctrl+F
                        case SDLK_g:
                            ctrl_char = '\x07';
                            break;  // Ctrl+G
                        case SDLK_h:
                            ctrl_char = '\x08';
                            break;  // Ctrl+H (backspace)
                        case SDLK_i:
                            ctrl_char = '\x09';
                            break;  // Ctrl+I (tab)
                        case SDLK_j:
                            ctrl_char = '\x0A';
                            break;  // Ctrl+J
                        case SDLK_k:
                            ctrl_char = '\x0B';
                            break;  // Ctrl+K
                        case SDLK_l:
                            ctrl_char = '\x0C';
                            break;  // Ctrl+L
                        case SDLK_m:
                            ctrl_char = '\x0D';
                            break;  // Ctrl+M (return)
                        case SDLK_n:
                            ctrl_char = '\x0E';
                            break;  // Ctrl+N
                        case SDLK_o:
                            ctrl_char = '\x0F';
                            break;  // Ctrl+O
                        case SDLK_p:
                            ctrl_char = '\x10';
                            break;  // Ctrl+P
                        case SDLK_q:
                            ctrl_char = '\x11';
                            break;  // Ctrl+Q
                        case SDLK_r:
                            ctrl_char = '\x12';
                            break;  // Ctrl+R
                        case SDLK_s:
                            ctrl_char = '\x13';
                            break;  // Ctrl+S
                        case SDLK_t:
                            ctrl_char = '\x14';
                            break;  // Ctrl+T
                        case SDLK_u:
                            ctrl_char = '\x15';
                            break;  // Ctrl+U
                        case SDLK_v:
                            ctrl_char = '\x16';
                            break;  // Ctrl+V
                        case SDLK_w:
                            ctrl_char = '\x17';
                            break;  // Ctrl+W
                        case SDLK_x:
                            ctrl_char = '\x18';
                            break;  // Ctrl+X
                        case SDLK_y:
                            ctrl_char = '\x19';
                            break;  // Ctrl+Y
                        case SDLK_z:
                            ctrl_char = '\x1A';
                            break;  // Ctrl+Z
                    }
                    if (ctrl_char) {
                        write(pty_fd, &ctrl_char, 1);
                        if (ctrl_char == '\x03') {  // Ctrl+C clears local buffer
                            input_buffer.clear();
                        }
                        key_handled = true;
                    }
                }

                // Handle special keys
                if (!key_handled) {
                    switch (event.key.keysym.sym) {
                        case SDLK_BACKSPACE: {
                            char backspace = '\x7F';  // DEL character (127) - standard for backspace
                            write(pty_fd, &backspace, 1);
                            key_handled = true;
                            break;
                        }
                        case SDLK_RETURN: {
                            char enter = '\r';
                            write(pty_fd, &enter, 1);
                            key_handled = true;
                            break;
                        }
                        case SDLK_TAB: {
                            char tab = '\t';
                            write(pty_fd, &tab, 1);
                            key_handled = true;
                            break;
                        }
                        case SDLK_ESCAPE: {
                            char escape = '\x1B';
                            write(pty_fd, &escape, 1);
                            key_handled = true;
                            break;
                        }
                        case SDLK_UP: {
                            const char up[] = "\x1B[A";
                            write(pty_fd, up, 3);
                            key_handled = true;
                            break;
                        }
                        case SDLK_DOWN: {
                            const char down[] = "\x1B[B";
                            write(pty_fd, down, 3);
                            key_handled = true;
                            break;
                        }
                        case SDLK_LEFT: {
                            const char left[] = "\x1B[D";
                            write(pty_fd, left, 3);
                            key_handled = true;
                            break;
                        }
                        case SDLK_RIGHT: {
                            const char right[] = "\x1B[C";
                            write(pty_fd, right, 3);
                            key_handled = true;
                            break;
                        }
                    }
                }

                if (key_handled) {
                    input_buffer.clear();  // Clear local buffer since we sent to PTY
                    dirty = true;
                    continue;
                }

                // All key handling is now done above - keys are sent directly to PTY
                dirty = true;
            }

            else if (event.type == SDL_MOUSEWHEEL) {
                if (event.wheel.y > 0) {
                    // Scroll up
                    term->ScrollOffSet = std::max(0, term->ScrollOffSet + 3);
                    dirty = true;
                } else if (event.wheel.y < 0) {
                    // Scroll down
                    term->ScrollOffSet = std::max(0, term->ScrollOffSet - 3);
                    dirty = true;
                }
            }

            else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    // Recreate render textures for new window size
                    term->create_render_textures();

                    // Update terminal size in PTY
                    term->update_window_size();
                    if (term->get_texture_width() > 0 && term->get_texture_height() > 0) {
                        int cols = term->get_window_width() / term->get_texture_width();
                        int rows = term->get_window_height() / (term->get_texture_height() + 2);
                        set_terminal_size(pty_fd, cols, rows);
                    }

                    dirty = true;
                    keyboard_dirty = true;  // Force keyboard redraw too
                }
            }
        }

        if (read_pty(pty_fd, term, term->lines)) {
            dirty = true;
        }

        // Check if screen buffer operations (like scrolling) require re-render
        if (term->is_screen_buffer_dirty()) {
            dirty = true;
            term->clear_screen_buffer_dirty();
        }

        // Cursor blinking disabled to prevent flickering with virtual keyboard
        // Uint32 CurrentTime = SDL_GetTicks();
        // if(CurrentTime - LastBlinkTime >= 500){
        //     ShowCursor = !ShowCursor;
        //     LastBlinkTime = CurrentTime;
        //     dirty = true;
        // }

        // Layered texture rendering - update textures only when dirty
        bool need_present = false;

        if (dirty) {
            term->render_terminal_to_texture(input_buffer, br, bg, bb, ba, ShowCursor);
            dirty = false;
            need_present = true;
        }

        if (keyboard_dirty) {
            term->render_keyboard_to_texture(vkb);
            keyboard_dirty = false;
            need_present = true;
        }

        // Only composite and present if something changed
        if (need_present) {
            term->composite_and_present();
        }

        // Use event-driven rendering with a small delay to prevent 100% CPU
        // usage
        if (!need_present) {
            SDL_Delay(10);  // Only delay when nothing to render
        } else {
            SDL_Delay(1);  // Minimal delay when actively rendering
        }
    }

    term->shutdown_sdl();
    delete term;
    delete Settings;
    delete vkb;

    SDL_StopTextInput();

    save_trimmed_history(CommandHistory, HisotryFile, MaxHistorySize);
    return (0);
}
