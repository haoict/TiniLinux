#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include <SDL/SDL.h>


#ifdef RG35XXPLUS

#define RAW_UP 103
#define RAW_DOWN 108
#define RAW_LEFT 105
#define RAW_RIGHT 106
#define RAW_A 304
#define RAW_B 305
#define RAW_X 307
#define RAW_Y 306
#define RAW_START 311
#define RAW_SELECT 310
#define RAW_MENU 312
#define RAW_L1 308
#define RAW_L2 314
#define RAW_L3 313
#define RAW_R1 309
#define RAW_R2 315
#define RAW_R3 316
#define RAW_PLUS 115
#define RAW_MINUS 114
#define RAW_POWER 116
#define RAW_YAXIS 17
#define RAW_XAXIS 16

#define RAW_MENU1 RAW_L3
#define RAW_MENU2 RAW_R3

//	RG35xx
#define KEY_UP RAW_UP
#define KEY_DOWN RAW_DOWN
#define KEY_LEFT RAW_LEFT
#define KEY_RIGHT RAW_RIGHT
#define KEY_ENTER RAW_A
#define KEY_TOGGLE RAW_R1
#define KEY_BACKSPACE RAW_B
#define KEY_SHIFT RAW_L1
#define KEY_LOCATION RAW_Y
#define KEY_ACTIVATE RAW_X
#define KEY_QUIT RAW_MENU
#define KEY_TAB RAW_SELECT
#define KEY_RETURN RAW_START
#define KEY_ARROW_LEFT RAW_L2
#define KEY_ARROW_RIGHT RAW_R2
#define KEY_ARROW_UP RAW_PLUS
#define KEY_ARROW_DOWN RAW_MINUS

#elif R36S_SDL12COMPAT

#define RAW_UP 544
#define RAW_DOWN 545
#define RAW_LEFT 546
#define RAW_RIGHT 547
#define RAW_A 305
#define RAW_B 304
#define RAW_X 307
#define RAW_Y 308
#define RAW_START 705
#define RAW_SELECT 704
#define RAW_MENU 708
#define RAW_L1 310
#define RAW_L2 312
#define RAW_L3 706
#define RAW_R1 311
#define RAW_R2 313
#define RAW_R3 707
#define RAW_PLUS 115
#define RAW_MINUS 114
#define RAW_POWER 116

#define KEY_UP RAW_UP
#define KEY_DOWN RAW_DOWN
#define KEY_LEFT RAW_LEFT
#define KEY_RIGHT RAW_RIGHT
#define KEY_ENTER RAW_A
#define KEY_TOGGLE RAW_R1
#define KEY_BACKSPACE RAW_B
#define KEY_SHIFT RAW_L1
#define KEY_LOCATION RAW_Y
#define KEY_ACTIVATE RAW_X
#define KEY_QUIT RAW_MENU
#define KEY_TAB RAW_SELECT
#define KEY_RETURN RAW_START
#define KEY_ARROW_LEFT RAW_L2
#define KEY_ARROW_RIGHT RAW_R2
#define KEY_ARROW_UP RAW_PLUS
#define KEY_ARROW_DOWN RAW_MINUS

#elif RGB30_SDL12COMPAT

#define RAW_UP 544
#define RAW_DOWN 545
#define RAW_LEFT 546
#define RAW_RIGHT 547
#define RAW_A 305
#define RAW_B 304
#define RAW_X 307
#define RAW_Y 308
#define RAW_START 315
#define RAW_SELECT 314
#define RAW_MENU 708
#define RAW_L1 310
#define RAW_L2 312
#define RAW_L3 706
#define RAW_R1 311
#define RAW_R2 313
#define RAW_R3 707
#define RAW_PLUS 115
#define RAW_MINUS 114
#define RAW_POWER 116

#define KEY_UP RAW_UP
#define KEY_DOWN RAW_DOWN
#define KEY_LEFT RAW_LEFT
#define KEY_RIGHT RAW_RIGHT
#define KEY_ENTER RAW_A
#define KEY_TOGGLE RAW_R1
#define KEY_BACKSPACE RAW_B
#define KEY_SHIFT RAW_L1
#define KEY_LOCATION RAW_Y
#define KEY_ACTIVATE RAW_X
#define KEY_QUIT RAW_MENU
#define KEY_TAB RAW_SELECT
#define KEY_RETURN RAW_START
#define KEY_ARROW_LEFT RAW_L2
#define KEY_ARROW_RIGHT RAW_R2
#define KEY_ARROW_UP RAW_PLUS
#define KEY_ARROW_DOWN RAW_MINUS

#elif H700_SDL12COMPAT

#define RAW_UP 544
#define RAW_DOWN 545
#define RAW_LEFT 546
#define RAW_RIGHT 547
#define RAW_A 305
#define RAW_B 304
#define RAW_X 307
#define RAW_Y 308
#define RAW_START 315
#define RAW_SELECT 314
#define RAW_MENU 316
#define RAW_L1 310
#define RAW_L2 312
#define RAW_L3 706
#define RAW_R1 311
#define RAW_R2 313
#define RAW_R3 707
#define RAW_PLUS 115
#define RAW_MINUS 114
#define RAW_POWER 116

#define KEY_UP RAW_UP
#define KEY_DOWN RAW_DOWN
#define KEY_LEFT RAW_LEFT
#define KEY_RIGHT RAW_RIGHT
#define KEY_ENTER RAW_A
#define KEY_TOGGLE RAW_R1
#define KEY_BACKSPACE RAW_B
#define KEY_SHIFT RAW_L1
#define KEY_LOCATION RAW_Y
#define KEY_ACTIVATE RAW_X
#define KEY_QUIT RAW_MENU
#define KEY_TAB RAW_SELECT
#define KEY_RETURN RAW_START
#define KEY_ARROW_LEFT RAW_L2
#define KEY_ARROW_RIGHT RAW_R2
#define KEY_ARROW_UP RAW_PLUS
#define KEY_ARROW_DOWN RAW_MINUS

#elif TRIMUISP

#define RAW_UP 103
#define RAW_DOWN 108
#define RAW_LEFT 105
#define RAW_RIGHT 106
#define RAW_A 305
#define RAW_B 304
#define RAW_X 307
#define RAW_Y 308
#define RAW_START 315
#define RAW_SELECT 314
#define RAW_MENU 316
#define RAW_L1 310
#define RAW_L2 2
#define RAW_L3 999 // no L3
#define RAW_R1 311
#define RAW_R2 5
#define RAW_R3 998 // no R3
#define RAW_PLUS 997
#define RAW_MINUS 996
#define RAW_POWER 995

#define KEY_UP RAW_UP
#define KEY_DOWN RAW_DOWN
#define KEY_LEFT RAW_LEFT
#define KEY_RIGHT RAW_RIGHT
#define KEY_ENTER RAW_A
#define KEY_TOGGLE RAW_R1
#define KEY_BACKSPACE RAW_B
#define KEY_SHIFT RAW_L1
#define KEY_LOCATION RAW_Y
#define KEY_ACTIVATE RAW_X
#define KEY_QUIT RAW_MENU
#define KEY_TAB RAW_SELECT
#define KEY_RETURN RAW_START
#define KEY_ARROW_LEFT RAW_L2
#define KEY_ARROW_RIGHT RAW_R2
#define KEY_ARROW_UP RAW_PLUS
#define KEY_ARROW_DOWN RAW_MINUS

#elif MIYOOMINI
//	miyoomini
#define KEY_UP SDLK_UP
#define KEY_DOWN SDLK_DOWN
#define KEY_LEFT SDLK_LEFT
#define KEY_RIGHT SDLK_RIGHT
#define KEY_ENTER SDLK_SPACE		   // A
#define KEY_TOGGLE SDLK_LCTRL		   // B
#define KEY_BACKSPACE SDLK_t		   // R1
#define KEY_SHIFT SDLK_e			   // L1
#define KEY_LOCATION SDLK_LALT		   // Y
#define KEY_ACTIVATE SDLK_LSHIFT	   // X
#define KEY_QUIT SDLK_ESCAPE		   // MENU
// #define KEY_HELP SDLK_RETURN //
#define KEY_TAB SDLK_RCTRL			   // SELECT
#define KEY_RETURN SDLK_RETURN		   // START
#define KEY_ARROW_LEFT SDLK_TAB		   // L2
#define KEY_ARROW_RIGHT SDLK_BACKSPACE // R2
// #define KEY_ARROW_UP	SDLK_KP_DIVIDE //
// #define KEY_ARROW_DOWN	SDLK_KP_PERIOD //

#elif TRIMUISMART
//	TRIMUI smart (same as TRIMUI)
#define KEY_UP SDLK_UP
#define KEY_DOWN SDLK_DOWN
#define KEY_LEFT SDLK_LEFT
#define KEY_RIGHT SDLK_RIGHT
#define KEY_ENTER SDLK_SPACE		 // A
#define KEY_TOGGLE SDLK_LCTRL		 // B
#define KEY_BACKSPACE SDLK_BACKSPACE // R
#define KEY_SHIFT SDLK_TAB			 // L
#define KEY_LOCATION SDLK_LSHIFT	 // Y
#define KEY_ACTIVATE SDLK_LALT		 // X
#define KEY_QUIT SDLK_ESCAPE		 // MENU
#define KEY_TAB SDLK_RCTRL			 // SELECT
#define KEY_RETURN SDLK_RETURN		 // START

#else
// generic Linux PC
#define KEY_UP SDLK_UP
#define KEY_DOWN SDLK_DOWN
#define KEY_LEFT SDLK_LEFT
#define KEY_RIGHT SDLK_RIGHT
#define KEY_ENTER SDLK_RETURN
#define KEY_TOGGLE SDLK_LALT
#define KEY_BACKSPACE SDLK_BACKSPACE
#define KEY_SHIFT SDLK_LSHIFT
#define KEY_LOCATION SDLK_ESCAPE
#define KEY_ACTIVATE SDLK_ESCAPE
#define KEY_QUIT SDLK_HOME
#define KEY_HELP SDLK_F1
#define KEY_TAB SDLK_TAB
#define KEY_RETURN SDLK_LCTRL
#define KEY_ARROW_LEFT SDLK_PAGEUP
#define KEY_ARROW_RIGHT SDLK_PAGEDOWN
#define KEY_ARROW_UP SDLK_KP_DIVIDE
#define KEY_ARROW_DOWN SDLK_KP_PERIOD

#endif



void init_keyboard();
void draw_keyboard(SDL_Surface *surface);
int handle_keyboard_event(SDL_Event *event);
int handle_narrow_keys_held(int sym);
extern int active;
extern int show_help;

#endif
