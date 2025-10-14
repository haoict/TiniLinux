#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/limits.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define MAX_BUFFER_SIZE 512
#define MAX_COMMAND_LENGTH MAX_BUFFER_SIZE
#define MAX_NAME_LENGTH 128

#define VERSION "1.0.0"


#if defined(RG35XXP)
// RG35XXP: A:0, B:1, X:2, Y:3, L1:4, R1:5, L2:9, R2:10, Select:6, Start:7, Fn:8/11, Up Down Left Right: SDL_JOYHATMOTION
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define ITEMS_PER_PAGE 8
#define BTN_A 0
#define BTN_B 1
#define BTN_UP 4 // use L1 because UP is SDL_JOYHATMOTION
#define BTN_DOWN 5 // use R1 because DOWN is SDL_JOYHATMOTION
#define BTN_LEFT 9
#define BTN_RIGHT 10
#define BATTERY_CAPACITY_FILE "/sys/class/power_supply/axp2202-battery/capacity"
#define BRIGHTNESS_FILE "/sys/devices/platform/backlight/backlight/backlight/brightness" // TODO: find correct path
#define VOLUME_COMMAND "amixer get 'digital volume' | awk -F'[][]' '/Mono:/ { print $2 }'"
#define CREDIT "Simple Launcher " VERSION " (RG35) | "

#elif defined(TRIMUISP)
// TRIMUISP: A:0, B:1, X:2, Y:3, L1:4, R1:5, L2:9, R2:10, Select:6, Start:7, Fn:8/11, Up Down Left Right: SDL_JOYHATMOTION
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define ITEMS_PER_PAGE 8
#define BTN_A 1
#define BTN_B 0
#define BTN_UP 4 // use L1 because UP is SDL_JOYHATMOTION
#define BTN_DOWN 5 // use R1 because DOWN is SDL_JOYHATMOTION
#define BTN_LEFT 9
#define BTN_RIGHT 10
#define BATTERY_CAPACITY_FILE "/sys/class/power_supply/axp2202-battery/capacity"
#define BRIGHTNESS_FILE "/sys/devices/platform/backlight/backlight/backlight/brightness" // TODO: find correct path
#define VOLUME_COMMAND "amixer get 'digital volume' | awk -F'[][]' '/Mono:/ { print $2 }'"
#define CREDIT "Simple Launcher " VERSION " (TRIMUISP) | "

#elif defined(R36S)
// R36S:  Up:8, Down:9, Left:10, Right:11, A:1, B:0, X:2, Y:3, L1:4, R1:5, L2:6, R2:7, L3:14, R3:15, Select:12, Start:13, Fn:16
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define ITEMS_PER_PAGE 8
#define BTN_A 1
#define BTN_B 0
#define BTN_UP 8
#define BTN_DOWN 9
#define BTN_LEFT 10
#define BTN_RIGHT 11
#define BATTERY_CAPACITY_FILE "/sys/class/power_supply/battery/capacity"
#define BRIGHTNESS_FILE "/sys/devices/platform/backlight/backlight/backlight/brightness"
#define VOLUME_COMMAND "amixer get Playback | awk -F'[][]' '/Left:/ { print $2 }'"
#define CREDIT "Simple Launcher " VERSION " (R36S) | "

#elif defined(BR2)
// Up:544, Down:545, Left:546, Right:547, A:305, B:304, X:307, Y:308, L1:310, R1:311, L2:312, R2:313, Select:314, Start:315, L3:317, R3:318
#define BTN_A 1
#define BTN_B 0
#define BTN_UP 13
#define BTN_DOWN 14
#define BTN_LEFT 15
#define BTN_RIGHT 16
#endif


#if defined(RGB30)
#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 720
#define ITEMS_PER_PAGE 15
#define BATTERY_CAPACITY_FILE "/sys/class/power_supply/rk817-battery/capacity"
#define BRIGHTNESS_FILE "/sys/devices/platform/backlight/backlight/backlight/brightness"
#define VOLUME_COMMAND "amixer get -c 1 Master | awk -F'[][]' '/Left:/ { print $2 }'"
#define CREDIT "Simple Launcher " VERSION " (RGB30) | "

#elif defined(H700)
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 640
#define ITEMS_PER_PAGE 8
#define BATTERY_CAPACITY_FILE "/sys/class/power_supply/battery/capacity"
#define BRIGHTNESS_FILE "/sys/devices/platform/backlight/backlight/backlight/brightness"
#define VOLUME_COMMAND "amixer get -c 0 DAC | awk -F'[][]' '/Mono:/ { print $2 }'"
#define CREDIT "Simple Launcher " VERSION " (H700) | "

#endif



const char *FONT_PATH = "./simple-launcher.ttf";
const char *COMMANDS_FILE = "./simple-launcher-commands.txt";

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Joystick *joystick;

TTF_Font *xsFont;
TTF_Font *sFont;
TTF_Font *mFont;
TTF_Font *lFont;
TTF_Font *xlFont;
TTF_Font *titleFont;

typedef struct Command Command;
struct Command {
	char name[MAX_NAME_LENGTH];
	char command[MAX_COMMAND_LENGTH];
};

Command *commands = NULL;
int numCommands = 0;
char title[MAX_BUFFER_SIZE];
char batteryCapacity[8];
char batteryCapacityDisplayString[20] = "Batt: 0%";
char brightness[8];
char brightnessDisplayString[20] = "Brightness: 0%";
char volume[8];
char volumeDisplayString[20] = "Volume: 0%";
char creditDisplayString[MAX_BUFFER_SIZE];
char pagesDisplayString[MAX_BUFFER_SIZE];

void loadCommands(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		printf("Could not open file %s\n", filename);
		return;
	}

	char line[MAX_BUFFER_SIZE];

	if (fgets(title, sizeof(title), file) == NULL) {
		printf("Could not read title\n");
		return;
	}
	title[strlen(title) - 1] = '\0';

	while (fgets(line, sizeof(line), file)) {
		// Remove trailing newline
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = '\0';
		}
		// Skip empty line
		if (line[0] == '\0') {
			continue;
		}

		// Allocate or reallocate commands array
		commands = realloc(commands, (numCommands + 1) * sizeof(*commands));
		if (commands == NULL) {
			printf("Could not allocate memory for commands\n");
			return;
		}

		// Copy name
		strncpy(commands[numCommands].name, line, MAX_NAME_LENGTH - 1);
		commands[numCommands].name[MAX_NAME_LENGTH - 1] = '\0';

		// Read next line for command
		if (fgets(line, sizeof(line), file) == NULL) {
			printf("Could not read command for %s\n", commands[numCommands].name);
			return;
		}

		// Check line overflow
		if (strchr(line, '\n') == NULL) {
			perror("Line is too long (max 512 chars). Skip this command\n");
			strcpy(commands[numCommands].command,
			       "echo 'Line is too long (max 512 chars). Skip this command' >> /dev/tty1 && sleep 2");
			// If the buffer doesn't contain a newline character, read and discard remaining characters
			int ch;
			while ((ch = fgetc(file)) != '\n' && ch != EOF) {
				// Do nothing, just discard the character
			}
		} else {
			// Remove trailing newline
			len = strlen(line);
			if (len > 0 && line[len - 1] == '\n') {
				line[len - 1] = '\0';
			}

			// Copy command
			strncpy(commands[numCommands].command, line, MAX_COMMAND_LENGTH - 1);
			commands[numCommands].command[MAX_COMMAND_LENGTH - 1] = '\0';
		}

		numCommands++;
	}

	fclose(file);
}

void updateHwInfo()
{
	// Update battery percentage
	FILE *batteryFile = fopen(BATTERY_CAPACITY_FILE, "r");
	if (batteryFile == NULL) {
		printf("Could not open battery file\n");
	} else {
		fgets(batteryCapacity, sizeof(batteryCapacity), batteryFile);
		fclose(batteryFile);
		batteryCapacity[strlen(batteryCapacity) - 1] = '\0';
		strcpy(batteryCapacityDisplayString, "Batt: ");
		strcat(batteryCapacityDisplayString, batteryCapacity);
		strcat(batteryCapacityDisplayString, "%");
	}
	// Read brightness
	FILE *brightnessFile = fopen(BRIGHTNESS_FILE, "r");
	if (brightnessFile == NULL) {
		printf("Could not open brightness file\n");
	} else {
		fgets(brightness, sizeof(brightness), brightnessFile);
		fclose(brightnessFile);
		strcpy(brightnessDisplayString, "Brightness: ");
		brightness[strlen(brightness) - 1] = '\0';
		strcat(brightnessDisplayString, brightness);
	}
	// Read volume
	FILE *pipe = popen(VOLUME_COMMAND, "r");
	if (pipe == NULL) {
		printf("Failed to open pipe\n");
	} else {
		fgets(volume, sizeof(volume), pipe);
		pclose(pipe);
		if (volume == NULL || strlen(volume) == 0) {
			strcpy(volume, "0%\n");
		}
		volume[strlen(volume) - 1] = '\0';
		strcpy(volumeDisplayString, "Volume: ");
		strcat(volumeDisplayString, volume);
	}
	strcpy(creditDisplayString, CREDIT);
	strcat(creditDisplayString, brightnessDisplayString);
	strcat(creditDisplayString, " | ");
	strcat(creditDisplayString, volumeDisplayString);
}

void updateRender(int selectedItem, SDL_Color color, SDL_Color highlightColor)
{
	SDL_SetRenderDrawColor(renderer, 14, 14, 14, 255); // Set the color to gray
	SDL_RenderClear(renderer);

	// menu items
	int selectedPage = selectedItem / ITEMS_PER_PAGE;
	for (int i = 0; i < numCommands; i++) {
		if (i >= selectedPage * ITEMS_PER_PAGE && i < (selectedPage + 1) * ITEMS_PER_PAGE) {
			char itemName[MAX_NAME_LENGTH] = "> ";
			SDL_Surface *surface =
				TTF_RenderText_Blended(mFont,
						       selectedItem == i ? strcat(itemName, commands[i].name) : commands[i].name,
						       selectedItem == i ? highlightColor : color);
			SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_Rect rect = { 20, 80 + (i - (selectedPage * ITEMS_PER_PAGE)) * 40, surface->w, surface->h };
			SDL_RenderCopy(renderer, texture, NULL, &rect);
			SDL_FreeSurface(surface);
			SDL_DestroyTexture(texture);
		}
	}
	// pages
	int totalPages = (numCommands - 1) / ITEMS_PER_PAGE + 1;
	sprintf(pagesDisplayString, "Page %d / %d", selectedPage + 1, totalPages);
	SDL_Surface *pagesSurface = TTF_RenderText_Blended(xsFont, pagesDisplayString, color);
	SDL_Texture *pagesTexture = SDL_CreateTextureFromSurface(renderer, pagesSurface);
	SDL_Rect pagesRect = { 20, SCREEN_HEIGHT - pagesSurface->h - 10, pagesSurface->w, pagesSurface->h };
	SDL_RenderCopy(renderer, pagesTexture, NULL, &pagesRect);
	SDL_FreeSurface(pagesSurface);
	SDL_DestroyTexture(pagesTexture);

	// title
	SDL_Surface *titleSurface = TTF_RenderText_Blended(titleFont, title, color);
	SDL_Texture *titleTexture = SDL_CreateTextureFromSurface(renderer, titleSurface);
	SDL_Rect titleText_rect = { 20, 10, titleSurface->w, titleSurface->h };
	SDL_RenderCopy(renderer, titleTexture, NULL, &titleText_rect);
	SDL_FreeSurface(titleSurface);
	SDL_DestroyTexture(titleTexture);
	// battery
	SDL_Surface *batterySurface = TTF_RenderText_Blended(sFont, batteryCapacityDisplayString, color);
	SDL_Texture *batteryTexture = SDL_CreateTextureFromSurface(renderer, batterySurface);
	SDL_Rect batteryRect = { SCREEN_WIDTH - batterySurface->w - 10, 5, batterySurface->w, batterySurface->h };
	SDL_RenderCopy(renderer, batteryTexture, NULL, &batteryRect);
	SDL_FreeSurface(batterySurface);
	SDL_DestroyTexture(batteryTexture);
	// Credit
	SDL_Surface *creditSurface = TTF_RenderText_Blended(xsFont, creditDisplayString, color);
	SDL_Texture *creditTexture = SDL_CreateTextureFromSurface(renderer, creditSurface);
	SDL_Rect creditRect = { SCREEN_WIDTH - creditSurface->w - 10, SCREEN_HEIGHT - pagesSurface->h - 10, creditSurface->w, creditSurface->h };
	SDL_RenderCopy(renderer, creditTexture, NULL, &creditRect);
	SDL_FreeSurface(creditSurface);
	SDL_DestroyTexture(creditTexture);

	// render all
	SDL_RenderPresent(renderer);
}

// Function to execute a shell script
void executeShellScript(const char *script)
{
	// Close SDL window
	SDL_JoystickClose(joystick);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	// Execute shell script
	system("printf \"\\033c\" > /dev/tty1");
	system(script);

	updateHwInfo();

	// Recreate SDL window
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	renderer = SDL_CreateRenderer(window, -1, 0);
	joystick = SDL_JoystickOpen(0);
#if defined(TRIMUISP) || defined(RGB30)
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif
}

int main(int argc, char *argv[])
{
	SDL_Color color = { 255, 255, 255, 80 }; // Low White color
	SDL_Color highlightColor = { 255, 255, 255, 255 }; // White color

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
	TTF_Init();

	window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	if (window == NULL) {
		printf("Could not create window: %s\n", SDL_GetError());
		return 1;
	}

	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("Current working dir: %s\n", cwd);
	} else {
		strcpy(cwd, ".");
	}
	char fontPath[PATH_MAX];
	strcpy(fontPath, cwd);
	strcat(fontPath, "/");
	strcat(fontPath, FONT_PATH);
	char commandFilePath[PATH_MAX];
	strcpy(commandFilePath, cwd);
	strcat(commandFilePath, "/");
	strcat(commandFilePath, COMMANDS_FILE);

	renderer = SDL_CreateRenderer(window, -1, 0);
#if defined(TRIMUISP) || defined(RGB30)
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
#endif
	xsFont = TTF_OpenFont(fontPath, 16);
	sFont = TTF_OpenFont(fontPath, 20);
	mFont = TTF_OpenFont(fontPath, 24);
	lFont = TTF_OpenFont(fontPath, 28);
	xlFont = TTF_OpenFont(fontPath, 32);
	titleFont = TTF_OpenFont(fontPath, 44);
	if (xsFont == NULL || sFont == NULL || mFont == NULL || lFont == NULL || xlFont == NULL || titleFont == NULL) {
		printf("Failed to load font: %s\n", TTF_GetError());
		return 1;
	}

	joystick = SDL_JoystickOpen(0);

	loadCommands(commandFilePath);

	int running = 1;
	int suspend = 0;
	int selectedItem = 0;

	int buttonUpHeld = 0;
	int buttonDownHeld = 0;
	Uint32 lastScrollTime = 0;
	const Uint32 SCROLL_DELAY = 150; // milliseconds between auto-scroll

	Uint32 lastHwUpdate = 0;
	const Uint32 HW_UPDATE_INTERVAL = 2000; // 2000 ms = 2 seconds

	updateHwInfo();

	updateRender(selectedItem, color, highlightColor);

	// main loop
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
		// if (SDL_WaitEvent(&event) != 0) {
			if (event.type == SDL_JOYAXISMOTION || event.type == SDL_MOUSEMOTION || event.type == SDL_WINDOWEVENT) {
				continue;
			}
#if defined(DEBUG)
			printf("Event type: %d\n", event.type);
#endif

			switch (event.type) {
			case SDL_QUIT:
				running = 0;
				break;
			case SDL_KEYDOWN:
#if defined(DEBUG)
				printf("Key pressed: %d | %d\n", event.key.keysym.sym, event.key.keysym.scancode);
#endif
				// PC Keyboard
				switch (event.key.keysym.sym) {
				case SDLK_UP:
					if (selectedItem > 0)
						selectedItem--;
					else
						selectedItem = numCommands - 1;
					break;
				case SDLK_DOWN:
					if (selectedItem < numCommands - 1)
						selectedItem++;
					else
						selectedItem = 0;
					break;
				case SDLK_LEFT:
					selectedItem = MAX(0, selectedItem - ITEMS_PER_PAGE);
					break;
				case SDLK_RIGHT:
					selectedItem = MIN(selectedItem + ITEMS_PER_PAGE, numCommands - 1);
					break;
				case SDLK_RETURN:
					executeShellScript(commands[selectedItem].command);
					break;
				}
				// R36S Special keys
				switch (event.key.keysym.scancode) {
				case 128: // volume up
				case 129: // volume down
					SDL_Delay(200);
					updateHwInfo();
					break;
				case 102: // power
					if (!suspend) {
						suspend = 1;
						system("systemctl suspend");
					} else {
						suspend = 0;
					}
					break;
				}
				break;
				/*
			case SDL_JOYAXISMOTION:
				if (event.jaxis.axis == 1) { // Y axis
					if (event.jaxis.value < -10000) { // Up
						if (selectedItem > 0)
							selectedItem--;
						else
							selectedItem = numCommands - 1;
					} else if (event.jaxis.value > 10000) { // Down
						if (selectedItem < numCommands - 1)
							selectedItem++;
						else
							selectedItem = 0;
					}
				}
				break;
			*/
#if defined(RG35XXP) || defined(TRIMUISP)
			case SDL_JOYHATMOTION:
				if (event.jhat.value == SDL_HAT_UP) {
					if (selectedItem > 0)
						selectedItem--;
					else
						selectedItem = numCommands - 1;
				} else if (event.jhat.value == SDL_HAT_DOWN) {
					if (selectedItem < numCommands - 1)
						selectedItem++;
					else
						selectedItem = 0;
				} else if (event.jhat.value == SDL_HAT_LEFT) {
					selectedItem = MAX(0, selectedItem - ITEMS_PER_PAGE);
				} else if (event.jhat.value == SDL_HAT_RIGHT) {
					selectedItem = MIN(selectedItem + ITEMS_PER_PAGE, numCommands - 1);
				}
				break;
#endif
			case SDL_JOYBUTTONDOWN:
#if defined(DEBUG)
				printf("Key pressed: %d\n", event.jbutton.button);
#endif
				switch (event.jbutton.button) {
				case BTN_A:
					executeShellScript(commands[selectedItem].command);
					break;
				case BTN_B:
					selectedItem = 0;
					break;
				case BTN_UP:
					// if (selectedItem > 0)
					// 	selectedItem--;
					// else
					// 	selectedItem = numCommands - 1;
					buttonUpHeld = SDL_JoystickGetButton(joystick, BTN_UP);
					buttonDownHeld = SDL_JoystickGetButton(joystick, BTN_DOWN);
					break;
				case BTN_DOWN:
					// if (selectedItem < numCommands - 1)
					// 	selectedItem++;
					// else
					// 	selectedItem = 0;
					buttonUpHeld = SDL_JoystickGetButton(joystick, BTN_UP);
					buttonDownHeld = SDL_JoystickGetButton(joystick, BTN_DOWN);
					break;
				case BTN_LEFT:
					selectedItem = MAX(0, selectedItem - ITEMS_PER_PAGE);
					break;
				case BTN_RIGHT:
					selectedItem = MIN(selectedItem + ITEMS_PER_PAGE, numCommands - 1);
					break;
				}

				break;
			case SDL_JOYBUTTONUP:

				buttonUpHeld = SDL_JoystickGetButton(joystick, BTN_UP);
				buttonDownHeld = SDL_JoystickGetButton(joystick, BTN_DOWN);

				break;

			default:
				continue;
			}
		}

		// Handle auto-scroll when button is held
		Uint32 now = SDL_GetTicks();
		if (buttonUpHeld && now - lastScrollTime > SCROLL_DELAY) {
			if (selectedItem > 0)
				selectedItem--;
			else
				selectedItem = numCommands - 1;
			lastScrollTime = now;
		} 
		if (buttonDownHeld && now - lastScrollTime > SCROLL_DELAY) {
			if (selectedItem < numCommands - 1)
				selectedItem++;
			else
				selectedItem = 0;
			lastScrollTime = now;
		}
		
		// Update hardware info every 2 seconds
		if (now - lastHwUpdate >= HW_UPDATE_INTERVAL) {
			updateHwInfo();
			lastHwUpdate = now;
		}
		
		// Update render
		updateRender(selectedItem, color, highlightColor);
		SDL_Delay(16); // ~60 FPS
	}

	free(commands);

	TTF_CloseFont(xsFont);
	TTF_CloseFont(sFont);
	TTF_CloseFont(mFont);
	TTF_CloseFont(lFont);
	TTF_CloseFont(xlFont);
	TTF_CloseFont(titleFont);
	SDL_JoystickClose(joystick);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_Quit();
	SDL_Quit();

	return 0;
}
