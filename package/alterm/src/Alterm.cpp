#include "../include/Alterm.hpp"
#include "../include/VirtualKeyboard.hpp"
#include <string>
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>




//Initialize SDL and Create window&renderer 

bool alterm::initialize_window(){

    bool InitializeVar = true;

    if(SDL_Init(SDL_INIT_VIDEO) != 0 ){
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        InitializeVar = false;
    }

    if(TTF_Init() != 0){
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        InitializeVar = false;
    }

    this->pWindow = SDL_CreateWindow("alterm", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 900, 700, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if(this->pWindow == nullptr){
        std::cerr<< "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        InitializeVar = false;
    }

    this->pRenderer = SDL_CreateRenderer(pWindow, -1, SDL_RENDERER_ACCELERATED);
    if(this->pRenderer == nullptr){
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(pWindow);
        SDL_Quit();
        InitializeVar = false;
    }

    return InitializeVar;

}



void alterm::render_background(Uint8 r, Uint8 g, Uint8 b, Uint8 a){
    SDL_SetRenderDrawBlendMode(this->pRenderer, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(this->pRenderer, 0, 0, 0, 255);
    SDL_RenderClear(this->pRenderer);


    SDL_SetRenderDrawBlendMode(this->pRenderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(this->pRenderer, r, g, b, a);

    int w,h;
    SDL_GetWindowSize(this->pWindow, &w, &h);
    SDL_Rect bgRect = {0, 0, w, h};
    SDL_RenderFillRect(this->pRenderer, &bgRect);

    SDL_SetRenderDrawBlendMode(this->pRenderer, SDL_BLENDMODE_NONE);

}

//This function as simple as it, it reduces the amount of code needed to draw and the consuming of memory, it take a char and make a texture for it,
//and save it in a map of char to reuse it when it is needed.
 
SDL_Texture* alterm::get_cached_texture(char c){
    // Check if we need to clear cache due to color change
    SDL_Color current_color = {0, 0, 0, 255};
    if (use_bitmap_font) {
        current_color = cached_font_color;
    } else if (FontManagarInstance) {
        current_color = FontManagarInstance->FontColor;
    }
    
    // Clear cache if color changed from what was used to create cached textures
    static SDL_Color last_used_color = {0, 0, 0, 0}; // Track actual color used for cache
    if (last_used_color.r != current_color.r || last_used_color.g != current_color.g || 
        last_used_color.b != current_color.b || last_used_color.a != current_color.a) {
        clear_char_texture_cache();
        last_used_color = current_color;
    }
    
    if(CharTextureChache.count(c)){
        return CharTextureChache[c];
    }

    SDL_Texture* Texture = nullptr;
    
    if (use_bitmap_font && bitmap_font) {
        // Use bitmap font - ensure correct color is set for terminal text
        bitmap_font->set_color(current_color.r, current_color.g, current_color.b);
        Texture = bitmap_font->create_char_texture(c);
    } else if (Font && FontManagarInstance) {
        // Use TTF font
        std::string s(1,c);
        SDL_Surface* Surface = TTF_RenderUTF8_Blended(Font, s.c_str(), current_color);
        if(Surface) {
            Texture = SDL_CreateTextureFromSurface(this->pRenderer, Surface);
            SDL_FreeSurface(Surface);
        }
    }

    if(Texture){
        CharTextureChache[c] = Texture;
    }
    return Texture;
}

void alterm::clear_char_texture_cache() {
    // Destroy all cached textures
    for (auto& pair : CharTextureChache) {
        if (pair.second) {
            SDL_DestroyTexture(pair.second);
        }
    }
    CharTextureChache.clear();
}


//This function is used to setup the font for the terminal
//It takes 6 parameters:
//r, g, b - the color of the text
//path - the path to the font
//size - the size of the font
//text - the text to be displayed
//and set those values to the TTF_Font* pointer (Font).

void alterm::setup_font(const Uint8 r, const Uint8 g, const Uint8 b, const char* path, const int size,const char* text) {

    // Check if we should use embedded bitmap font
    if (std::string(path) == "embedded_font") {
        use_bitmap_font = true;
        bitmap_font = new BitmapFont();
        bitmap_font->initialize(pRenderer, size); // Use size as scale factor
        bitmap_font->set_color(r, g, b);
        Font = nullptr; // No TTF font needed
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


//All the have work here
//This function render the response from the bash through pty and store it in a vector of strings.
//It takes 1 parameter:
//InputBuffer - the user input string.
//This function marge the tow response in one string and print every character individualy.
//And also make sure that the render don't exceed the screen size.

void alterm::renderer_screen(std::string& InputBuffer, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool ShowCursor, bool should_present){

   render_background(r, g, b, a);


    int WindowWidth = 0;
    int WindowHeight = 0;
    SDL_GetWindowSize(this->pWindow, &WindowWidth, &WindowHeight); 

    
    std::vector<int>  LineHeights; //This is the height of the font.
    int TotalHeight = 0;



    for (const std::string& line : lines){     //Here we calculate for each line, by seperating every lines to characters and calculate the height.                 
        int x = 0;
        int LineHeight = 0;
        int MaxCharHeight = 0;


        for (char c : line){                

                SDL_Texture* ppTexture = get_cached_texture(c);
                if(!ppTexture) continue;

                int texWW = 0, texHH = 0;
                SDL_QueryTexture(ppTexture, NULL, NULL, &texWW, &texHH);

                if(x + texWW > WindowWidth) { //If the character exceed the screen width, we need to start a new line.
                    x = 0;
                    LineHeight += MaxCharHeight + 2;
                    MaxCharHeight = 0;
                } 

                x += texWW;                //else start from the same line.

                if(texHH > MaxCharHeight) MaxCharHeight = texHH;

        }

        LineHeight += MaxCharHeight + 2;
        LineHeights.push_back(LineHeight);
        TotalHeight += LineHeight;     //Sum of the total height
    }


    int TotalLines = lines.size();
    int accumulated = 0;
    int startIndex = 0;


    for(int i = TotalLines - 1; i >= 0; --i){  //We start from the last line and go up, 
        accumulated += LineHeights[i];         //and calculate the their accumulated height.
        if(accumulated > WindowHeight){        //If the accumulated height is bigger than the window height
            startIndex = i + 1;                //we set the the line after [i] line is the first line to render.
            break;
        }
    }
    int VisableLines = TotalLines - startIndex; //The number of lines that are visable to the user.
    startIndex = std::max(0, startIndex - ScrollOffSet); //The line that we start to render from.

    int MaxScroll = std::max(0, TotalLines - VisableLines ); //The max number of lines that can be rendered.
    ScrollOffSet = std::min(ScrollOffSet, MaxScroll); //We set the ScrollOffSet to the max number of lines that can be rendered.     


    
    int CurrentRenderY = 0;
    std::string FullLine;
    for(int i = startIndex; i < TotalLines; ++i){
        FullLine = (i == TotalLines - 1 && !InputBuffer.empty()) ? lines[i] + InputBuffer : lines[i];
    

        int xPos = 0;
        int yPos = CurrentRenderY;
        int MaxLineHeight = 0;


        for(char c : FullLine ){       //Using for loop to render every line of the std::vector<std::string> lines + inputBuffer.
                                        //We iterate through every character of every line of the FullLine.
                  
            SDL_Texture* pTexture = get_cached_texture(c);
            if(!pTexture) continue;

            int texW = 0, texH = 0;
            SDL_QueryTexture(pTexture, NULL, NULL, &texW, &texH);


            if(xPos + texW > WindowWidth){          //If the sum of the x position of the current character and the width of the current character is bigger than the window width, we need to start a new line.
                xPos = 0;
                yPos += MaxLineHeight;                      //Update the y position of the next line.
                MaxLineHeight = 0;
            }



            SDL_Rect desRect = {xPos, yPos, texW, texH};
            SDL_RenderCopy(this->pRenderer, pTexture, NULL, &desRect);
            
            
            xPos += texW;                            //Update the x position of the next character.            
            if(texH > MaxLineHeight)                 //If the height of the current character is bigger than the max height, update the max height.    
                MaxLineHeight = texH;
            
    }
    
    if (i == TotalLines - 1){
        CursorX = xPos;
        CursorH = MaxLineHeight;
        this->y = yPos;
    }
        
    CurrentRenderY = yPos + MaxLineHeight + 2;

    
}


    
        
    if(ShowCursor){

        SDL_Rect cursor_Rect = {CursorX, this->y , 10,CursorH};
        SDL_SetRenderDrawColor(pRenderer, 255, 255, 255, 255);
        SDL_RenderFillRect(pRenderer, &cursor_Rect);
    }

    if(should_present) {
        SDL_RenderPresent(this->pRenderer);
    }

    
}




void alterm::reset_display(){
    this->y = 0;
    lines.clear();
    SDL_SetRenderDrawColor(this->pRenderer, 0, 0, 0, 255);
    SDL_RenderClear(this->pRenderer);
    SDL_RenderPresent(this->pRenderer);
}

void alterm::shutdown_sdl(){

    // Clean up render textures
    if(terminal_texture) {
        SDL_DestroyTexture(terminal_texture);
        terminal_texture = nullptr;
    }
    if(keyboard_texture) {
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

    for (auto& pair : CharTextureChache){
        SDL_DestroyTexture(pair.second);
    }
    CharTextureChache.clear();
    SDL_Quit();
}

bool alterm::create_render_textures() {
    SDL_GetWindowSize(pWindow, &window_width, &window_height);
    
    // Create terminal texture
    terminal_texture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, 
                                       SDL_TEXTUREACCESS_TARGET, window_width, window_height);
    if(!terminal_texture) {
        std::cerr << "Failed to create terminal texture: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create keyboard texture with alpha blending
    keyboard_texture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA8888, 
                                       SDL_TEXTUREACCESS_TARGET, window_width, window_height);
    if(!keyboard_texture) {
        std::cerr << "Failed to create keyboard texture: " << SDL_GetError() << std::endl;
        return false;
    }
    
    SDL_SetTextureBlendMode(keyboard_texture, SDL_BLENDMODE_BLEND);
    return true;
}

void alterm::render_terminal_to_texture(std::string& input_buffer, Uint8 r, Uint8 g, Uint8 b, Uint8 a, bool ShowCursor) {
    if(!terminal_texture) return;
    
    // Set render target to terminal texture
    SDL_SetRenderTarget(pRenderer, terminal_texture);
    
    // Render terminal content (use existing renderer_screen logic, but don't present)
    renderer_screen(input_buffer, r, g, b, a, ShowCursor, false);
    
    // Reset render target
    SDL_SetRenderTarget(pRenderer, nullptr);
}

void alterm::render_keyboard_to_texture(void* keyboard_ptr) {
    if(!keyboard_texture || !keyboard_ptr) return;
    
    VirtualKeyboard* keyboard = static_cast<VirtualKeyboard*>(keyboard_ptr);
    
    // Set render target to keyboard texture
    SDL_SetRenderTarget(pRenderer, keyboard_texture);
    
    // Clear with transparent background
    SDL_SetRenderDrawColor(pRenderer, 0, 0, 0, 0);
    SDL_RenderClear(pRenderer);
    
    // Draw keyboard if active
    if(keyboard->is_active() || keyboard->is_help_shown()) {
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
    if(terminal_texture) {
        SDL_RenderCopy(pRenderer, terminal_texture, nullptr, nullptr);
    }
    
    // Draw keyboard texture on top
    if(keyboard_texture) {
        SDL_RenderCopy(pRenderer, keyboard_texture, nullptr, nullptr);
    }
    
    // Present final result
    SDL_RenderPresent(pRenderer);
}
