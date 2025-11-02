#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <clocale>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>
#include "../include/Alterm.hpp"
#include "../include/HistoryManager.hpp"
#include "../include/SettingsManager.hpp"
#include "../include/VirtualKeyboard.hpp"


std::vector<std::string> CommandHistory;
const size_t MaxHistorySize = 100;
int CommandHistoryIndex = -1;
const std::string HisotryFile = ".alterm_history";


int main(int argc, char* argv[]){

    setlocale(LC_ALL, "");

    load_history(CommandHistory, HisotryFile);
    SettingsManager* Settings = new SettingsManager();
    Settings->load_from_file(".alterm_settings");
    alterm* term = new alterm();


    int pty_fd;
    int child_pid = start_shell_with_pty(pty_fd);
    fcntl(pty_fd, F_SETFL,O_NONBLOCK);

    if(child_pid < 0){  
        std::cerr << "Failed to start shell."<<std::endl;
        return(1);
    }


    if(term->initialize_window() == false){
        std::cout << "Init_error" <<std::endl;
        return(1);
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
    if(!term->create_render_textures()) {
        std::cerr << "Failed to create render textures" << std::endl;
        return 1;
    }
    
    // Initialize virtual keyboard
    VirtualKeyboard* vkb = new VirtualKeyboard();
    
    if (fontPath == "embedded_font") {
        // Use bitmap font for keyboard too
        vkb->initialize_bitmap(term->get_bitmap_font());
    } else {
        // Use TTF font for keyboard
        TTF_Font* keyboard_font = term->get_font();
        vkb->initialize(keyboard_font);
    }
    
    // Initial render to populate textures
    std::string empty_input = "";
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
    const Uint32 frame_delay = 1000 / 60; // 60 FPS limit


    SDL_StartTextInput();

    while(!quit){
        while(SDL_PollEvent(&event)){
            // Let virtual keyboard handle events first
            if(vkb->handle_event(&event)){
                keyboard_dirty = true;
                // Don't set dirty=true for keyboard navigation to prevent full terminal redraw
                // dirty = true;
                continue;
            }
            
            if(event.type == SDL_QUIT)
                quit = true;

            else if(event.type == SDL_TEXTINPUT){
                // Accept text input from virtual keyboard or when keyboard is not active
                input_buffer += event.text.text;
                dirty = true;
                // Force immediate render to prevent lag
            }
            
            else if(event.type == SDL_KEYDOWN && !vkb->is_active()){
                if(event.key.keysym.sym == SDLK_BACKSPACE && !input_buffer.empty()){
                    input_buffer.pop_back();
                    dirty = true;
                }
            

                else if(event.key.keysym.sym == SDLK_UP){
                    if(!CommandHistory.empty()){
                        if(CommandHistoryIndex == -1)
                            CommandHistoryIndex = CommandHistory.size() - 1;
                        else if(CommandHistoryIndex > 0)
                            CommandHistoryIndex--;

                        input_buffer = CommandHistory[CommandHistoryIndex];
                        dirty = true;
                    }
                }

                else if(event.key.keysym.sym == SDLK_DOWN){
                    if(!CommandHistory.empty() && CommandHistoryIndex != -1){
                        if(CommandHistoryIndex < static_cast<int>(CommandHistory.size() -1))
                            CommandHistoryIndex++;
                        else 
                            CommandHistoryIndex = -1;

                        if(CommandHistoryIndex != -1 )
                            input_buffer = CommandHistory[CommandHistoryIndex];
                        else 
                            input_buffer.clear();
                        dirty = true;
                    }
                }

                else if(event.key.keysym.sym == SDLK_RETURN){
                    std::string command = input_buffer;
                    std::string to_send = command + "\n";
                    term->enter_cursor_reset_x();
                    term->ScrollOffSet = 0;


                    if(in_settings_mode){
                        term->lines.push_back("[settings] " + input_buffer);
                        
                        std::vector<std::string> OutputLine = Settings->apply_command(input_buffer);
                        //render the ruselt to the terminal
                        for(const auto& line : OutputLine){
                            term->lines.push_back(line);
                        }

                        if(input_buffer == "exit"){
                            in_settings_mode = false;
                            term->lines.push_back("Exiting Settings... (restart the terminal to see the changes)");
                        }
                        
                        term->lines.push_back("[settings]:");
                        input_buffer.clear();
                        dirty = true;
                        continue;
                    }

                    if(!command.empty()){
                        if(CommandHistory.empty() || command != CommandHistory.back()){
                            CommandHistory.push_back(input_buffer);
                            append_history(input_buffer, HisotryFile);

                            if(CommandHistory.size() > MaxHistorySize)
                                CommandHistory.erase(CommandHistory.begin());
                        }

                        CommandHistoryIndex = -1;

                    }
                
                    if(input_buffer == "clear") {
                        input_buffer.clear();
                        term->reset_display();
                        term->lines.clear();
                        std::string newline = "\n";
                        write(pty_fd, newline.c_str(), newline.size());
                        dirty = true;
                        continue;
                    }

                    if(input_buffer == "exit") {
                            quit = true;
                            input_buffer.clear();
                            break;
                    }


                    if(input_buffer == "settings"){
                        in_settings_mode = true;
                        input_buffer.clear();
                        term->lines.push_back("======= Settings Mode ========");
                        term->lines.push_back("\nType help for more information");
                        term->lines.push_back("\n");

                        //render all the settings.
                        std::vector<std::string> SettingsLines = Settings->render_all_settings();
                        SettingsLines.push_back("[settings]:");
                        term-> lines.insert(term->lines.end(), SettingsLines.begin(), SettingsLines.end());
                        dirty = true;
                        continue;
                }

                write(pty_fd, to_send.c_str(), to_send.size());
                input_buffer.clear();
                dirty = true;
            }


        }

        else if(event.type == SDL_MOUSEWHEEL){
                if(event.wheel.y > 0){
                    //Scroll up
                    term->ScrollOffSet = std::max(0, term->ScrollOffSet + 1);
                    dirty = true;
                }
                else if(event.wheel.y < 0){
                    //Scroll down
                    term->ScrollOffSet = std::max(0, term->ScrollOffSet - 1);
                    dirty = true;
                }
            }

        else if(event.type == SDL_WINDOWEVENT){
            if(event.window.event == SDL_WINDOWEVENT_RESIZED){
                // Recreate render textures for new window size
                term->create_render_textures();
                dirty = true;
                keyboard_dirty = true;  // Force keyboard redraw too
            }
        }

    } 
        
        if(read_pty(pty_fd, term, term->lines)){
            dirty = true;
        }

        
        // Cursor blinking disabled to prevent flickering with virtual keyboard
        // Uint32 CurrentTime = SDL_GetTicks();
        // if(CurrentTime - LastBlinkTime >= 500){
        //     ShowCursor = !ShowCursor;
        //     LastBlinkTime = CurrentTime;
        //     dirty = true;
        // }

        // Layered texture rendering - update textures only when dirty
        if(dirty) {
            term->render_terminal_to_texture(input_buffer, br, bg, bb, ba, ShowCursor);
            dirty = false;
        }
        
        if(keyboard_dirty) {
            term->render_keyboard_to_texture(vkb);
            keyboard_dirty = false;
        }
        
        // Always composite textures and present (atomic update, no flicker)
        term->composite_and_present();

        SDL_Delay(33);  // 30 FPS
    }

    
    term->shutdown_sdl();
    delete term;
    delete Settings;
    delete vkb;
    
    SDL_StopTextInput();

   
    save_trimmed_history(CommandHistory, HisotryFile, MaxHistorySize);
    return(0);
}


