#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
#include <SDL2/SDL_ttf.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <locale.h>
#include <pty.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "font.h"
#include "keyboard.h"
#include "vt100.h"

#define Font Font_

#define USAGE "Simple Terminal\nusage: simple-terminal [-h] [-scale 2.0] [-font font.ttf] [-fontsize 14] [-fontshade 0|1] [-o file] [-q] [-r command ...]\n"

/* Arbitrary sizes */
#define DRAW_BUF_SIZ 20 * 1024
#define XK_NO_MOD UINT_MAX
#define XK_ANY_MOD 0

#define REDRAW_TIMEOUT (80 * 1000) /* 80 ms */

/* macros */
#define CLEANMASK(mask) (mask & (KMOD_SHIFT | KMOD_CTRL | KMOD_ALT))
#define SERRNO strerror(errno)
#define TIMEDIFF(t1, t2) ((t1.tv_sec - t2.tv_sec) * 1000 + (t1.tv_usec - t2.tv_usec) / 1000)

enum window_state { WIN_VISIBLE = 1, WIN_REDRAW = 2, WIN_FOCUSED = 4 };

/* Purely graphic info */
typedef struct {
    // Colormap cmap;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Surface *win;
    int scr;
    int tw, th; /* tty width and height */
    int w;      /* window width */
    int h;      /* window height */
    int ch;     /* char height */
    int cw;     /* char width  */
    char state; /* focus, redraw, visible */
} XWindow;

typedef struct {
    SDL_Keycode k;
    Uint16 mask;
    char s[ESC_BUF_SIZ];
} Key;

typedef union {
    int i;
    unsigned int ui;
    float f;
    const void *v;
} Arg;

/* Config.h for applying patches and the configuration. */
#include "config.h"

SDL_Surface *screen;
SDL_Surface *screen2;

/* Drawing Context */
typedef struct {
    SDL_Color colors[LEN(colormap) < 256 ? 256 : LEN(colormap)];
    // TTF_Font *font, *ifont, *bfont, *ibfont;
} DC;

static void draw(void);
static void drawregion(int, int, int, int);

static void run(void);
int ttythread(void *unused);

static void xdraws(char *, Glyph, int, int, int, int);
static void xclear(int, int, int, int);
static void xdrawcursor(void);
static void sdlinit(void);
static void initcolormap(void);
static void sdltermclear(int, int, int, int);
static void xresize(int, int);
static void scale_to_size(int, int);

static void expose(SDL_Event *);
static char *kmap(SDL_Keycode, Uint16);
static void kpress(SDL_Event *);
static void textinput(SDL_Event *);
static void window_event_handler(SDL_Event *);

static void update_render(void);

static void (*event_handler[SDL_LASTEVENT])(SDL_Event *) = {[SDL_KEYDOWN] = kpress, [SDL_TEXTINPUT] = textinput, [SDL_WINDOWEVENT] = window_event_handler};

/* Globals */
static DC dc;
static XWindow xw;
int iofd = -1;
char **opt_cmd = NULL;
int opt_cmd_size = 0;
char *opt_io = NULL;
static float opt_scale = 2.0;
static char *opt_font = NULL;
static int opt_fontsize = 12;
static int opt_fontshade = 0;

static int initial_width = 320;
static int initial_height = 240;
static volatile int thread_should_exit = 0;
SDL_Joystick *joystick;

size_t xwrite(int fd, char *s, size_t len) {
    size_t aux = len;

    while (len > 0) {
        ssize_t r = write(fd, s, len);
        if (r < 0) return r;
        len -= r;
        s += r;
    }
    return aux;
}

void *xmalloc(size_t len) {
    void *p = malloc(len);

    if (!p) die("Out of memory\n");

    return p;
}

void *xrealloc(void *p, size_t len) {
    if ((p = realloc(p, len)) == NULL) die("Out of memory\n");

    return p;
}

void *xcalloc(size_t nmemb, size_t size) {
    void *p = calloc(nmemb, size);

    if (!p) die("Out of memory\n");

    return p;
}

SDL_Thread *thread = NULL;

void sdlloadfonts() {
    // Try to load TTF font if opt_font is set
    if (opt_font && init_ttf_font(opt_font, opt_fontsize)) {
        xw.cw = get_ttf_char_width();
        xw.ch = get_ttf_char_height();
        fprintf(stderr, "Using TTF font: %s (size: %d, char: %dx%d)\n", opt_font, opt_fontsize, xw.cw, xw.ch);
    } else {
        // Fallback to bitmap font
        xw.cw = 6;
        xw.ch = 8;
        fprintf(stderr, "Using embedded bitmap font (6x8)\n");
    }
}

static int shutdown_called = 0;
void sdlshutdown(void) {
    if (SDL_WasInit(SDL_INIT_EVERYTHING) != 0 && !shutdown_called) {
        shutdown_called = 1;
        fprintf(stderr, "SDL shutting down\n");
        if (thread) {
            printf("Signaling ttythread to exit...\n");
            thread_should_exit = 1;
            // ttywrite n key to answer y/n question if blocked on ttyread
            ttywrite("n", 1);

            ttywrite("\033[?1000l", 7);    // disable mouse tracking to unblock ttyread
            SDL_WaitThread(thread, NULL);  // Wait for thread to exit cleanly
            // SDL_KillThread(thread);
            thread = NULL;
        }

        // Cleanup TTF font
        cleanup_ttf_font();

        if (xw.win) SDL_FreeSurface(xw.win);
        if (screen2) SDL_FreeSurface(screen2);
        xw.win = NULL;
        SDL_JoystickClose(joystick);
        SDL_Quit();
    }
}

void window_event_handler(SDL_Event *event) {
#ifdef BR2
    return;  // no resize for BR2 handheld devices builds because of kms video driver
#endif
    switch (event->window.event) {
        case SDL_WINDOWEVENT_RESIZED:
            scale_to_size(event->window.data1, event->window.data2);
            break;
        default:
            break;
    }
}

void scale_to_size(int width, int height) {
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) return;
    xw.w = width;
    xw.h = height;
    printf("set scale to size: %dx%d\n", xw.w, xw.h);

    // Recreate texture for new size
    if (xw.texture) {
        SDL_DestroyTexture(xw.texture);
    }
    xw.texture = SDL_CreateTexture(xw.renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, xw.w, xw.h);
    if (!xw.texture) {
        fprintf(stderr, "Unable to recreate texture: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // Recreate surfaces
    if (xw.win) SDL_FreeSurface(xw.win);
    xw.win = SDL_CreateRGBSurface(0, xw.w, xw.h, 16, 0xF800, 0x7E0, 0x1F, 0);  // console screen
    if (screen2) SDL_FreeSurface(screen2);
    screen2 = SDL_CreateRGBSurface(0, xw.w, xw.h, 16, 0xF800, 0x7E0, 0x1F, 0);  // for keyboardMix

    // Recreate screen surface for compatibility
    if (screen) SDL_FreeSurface(screen);
    screen = SDL_CreateRGBSurface(0, 640, 480, 16, 0xF800, 0x7E0, 0x1F, 0);

    // resize terminal to fit window
    int col, row;
    col = (xw.w - 2 * borderpx) / xw.cw;
    row = (xw.h - 2 * borderpx) / xw.ch;
    tresize(col, row);
    xresize(col, row);
    ttyresize();
}

void sdlinit(void) {
    // const SDL_VideoInfo *vi;
    fprintf(stderr, "SDL init\n");

    // dc.font = dc.ifont = dc.bfont = dc.ibfont = NULL;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0) {
        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_ShowCursor(0);
    SDL_StartTextInput();

    /*if(TTF_Init() == -1) {
        printf("TTF_Init: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    if(atexit(TTF_Quit)) {
        fprintf(stderr,"Unable to register TTF_Quit atexit\n");
    }*/

    /* font */
    sdlloadfonts();
    fprintf(stderr, "SDL font: %s\n", opt_font == NULL ? "embedded_bitmap_font" : opt_font);

    /* colors */
    initcolormap();

    int displayIndex = 0;  // usually 0 unless you have multiple screens
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(displayIndex, &mode) != 0) {
        printf("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
        xw.w = initial_width;
        xw.h = initial_height;
    } else {
        printf("Detected screen: %dx%d @ %dHz\n", mode.w, mode.h, mode.refresh_rate);
        xw.w = mode.w;
        xw.h = mode.h;
#ifndef BR2
        xw.w = initial_width * 2;
        xw.h = initial_height * 2;
#endif
        printf("Setting resolution to: %dx%d\n", xw.w, xw.h);
    }

    xw.window = SDL_CreateWindow("Simple Terminal", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, xw.w, xw.h, SDL_WINDOW_SHOWN);
    if (!xw.window) {
        fprintf(stderr, "Unable to create window: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    xw.renderer = SDL_CreateRenderer(xw.window, -1, SDL_RENDERER_ACCELERATED);
    if (!xw.renderer) {
        xw.renderer = SDL_CreateRenderer(xw.window, -1, SDL_RENDERER_SOFTWARE);
        if (!xw.renderer) {
            fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
            exit(EXIT_FAILURE);
        }
    }

    // if rgb30
    // SDL_RenderSetLogicalSize(xw.renderer, xw.w, xw.h);

    xw.texture = SDL_CreateTexture(xw.renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, xw.w, xw.h);
    if (!xw.texture) {
        fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
    xw.win = SDL_CreateRGBSurface(0, xw.w, xw.h, 16, 0xF800, 0x7E0, 0x1F, 0);   // console screen
    screen2 = SDL_CreateRGBSurface(0, xw.w, xw.h, 16, 0xF800, 0x7E0, 0x1F, 0);  // for keyboardMix

    // Create a temporary surface for the screen to maintain compatibility
    screen = SDL_CreateRGBSurface(0, xw.w, xw.h, 16, 0xF800, 0x7E0, 0x1F, 0);

    // TODO: might need to use system threads
    if (!(thread = SDL_CreateThread(ttythread, "ttythread", NULL))) {
        fprintf(stderr, "Unable to create thread: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    expose(NULL);

    joystick = SDL_JoystickOpen(0);

    SDL_Event event = {.type = SDL_WINDOWEVENT};
    event.window.event = SDL_WINDOWEVENT_EXPOSED;
    SDL_PushEvent(&event);

    // resize terminal to fit window
    int col = (xw.w - 2 * borderpx) / xw.cw;
    int row = (xw.h - 2 * borderpx) / xw.ch;
    tresize(col, row);
    xresize(col, row);
    ttyresize();

    if (opt_scale != 1.0) {
        scale_to_size((int)(xw.w / opt_scale), (int)(xw.h / opt_scale));
    }
}

void update_render(void) {
    if (xw.win == NULL) return;
    memcpy(screen2->pixels, xw.win->pixels, xw.w * xw.h * 2);
    draw_keyboard(screen2);  // screen2(SW) = console + keyboard
    // Update texture with screen pixels and render
    SDL_UpdateTexture(xw.texture, NULL, screen2->pixels, screen2->pitch);
    SDL_RenderClear(xw.renderer);
    SDL_RenderCopy(xw.renderer, xw.texture, NULL, NULL);
    SDL_RenderPresent(xw.renderer);
}

void die(const char *errstr, ...) {
    va_list ap;

    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    sdlshutdown();
}

void xresize(int col, int row) {
    xw.tw = MAX(1, 2 * borderpx + col * xw.cw);
    xw.th = MAX(1, 2 * borderpx + row * xw.ch);
}

void initcolormap(void) {
    int i, r, g, b;

    // TODO: allow these to override the xterm ones somehow?
    memcpy(dc.colors, colormap, sizeof(dc.colors));

    /* init colors [16-255] ; same colors as xterm */
    for (i = 16, r = 0; r < 6; r++) {
        for (g = 0; g < 6; g++) {
            for (b = 0; b < 6; b++) {
                dc.colors[i].r = r == 0 ? 0 : 0x3737 + 0x2828 * r;
                dc.colors[i].g = g == 0 ? 0 : 0x3737 + 0x2828 * g;
                dc.colors[i].b = b == 0 ? 0 : 0x3737 + 0x2828 * b;
                i++;
            }
        }
    }

    for (r = 0; r < 24; r++, i++) {
        b = 0x0808 + 0x0a0a * r;
        dc.colors[i].r = b;
        dc.colors[i].g = b;
        dc.colors[i].b = b;
    }
}

void sdltermclear(int col1, int row1, int col2, int row2) {
    if (xw.win == NULL) return;
    SDL_Rect r = {borderpx + col1 * xw.cw, borderpx + row1 * xw.ch, (col2 - col1 + 1) * xw.cw, (row2 - row1 + 1) * xw.ch};
    SDL_Color c = dc.colors[IS_SET(MODE_REVERSE) ? defaultfg : defaultbg];
    SDL_FillRect(xw.win, &r, SDL_MapRGB(xw.win->format, c.r, c.g, c.b));
}

/*
 * Absolute coordinates.
 */
void xclear(int x1, int y1, int x2, int y2) {
    if (xw.win == NULL) return;
    SDL_Rect r = {x1, y1, x2 - x1, y2 - y1};
    SDL_Color c = dc.colors[IS_SET(MODE_REVERSE) ? defaultfg : defaultbg];
    SDL_FillRect(xw.win, &r, SDL_MapRGB(xw.win->format, c.r, c.g, c.b));
}

void xdraws(char *s, Glyph base, int x, int y, int charlen, int bytelen) {
    int winx = borderpx + x * xw.cw, winy = borderpx + y * xw.ch, width = charlen * xw.cw;
    // TTF_Font *font = dc.font;
    SDL_Color *fg = &dc.colors[base.fg], *bg = &dc.colors[base.bg], *temp, revfg, revbg;

    s[bytelen] = '\0';

    if (base.mode & ATTR_BOLD) {
        if (BETWEEN(base.fg, 0, 7)) {
            /* basic system colors */
            fg = &dc.colors[base.fg + 8];
        } else if (BETWEEN(base.fg, 16, 195)) {
            /* 256 colors */
            fg = &dc.colors[base.fg + 36];
        } else if (BETWEEN(base.fg, 232, 251)) {
            /* greyscale */
            fg = &dc.colors[base.fg + 4];
        }
        /*
         * Those ranges will not be brightened:
         *	8 - 15 – bright system colors
         *	196 - 231 – highest 256 color cube
         *	252 - 255 – brightest colors in greyscale
         */
        // font = dc.bfont;
    }

    /*if(base.mode & ATTR_ITALIC)
        font = dc.ifont;
    if((base.mode & ATTR_ITALIC) && (base.mode & ATTR_BOLD))
        font = dc.ibfont;*/

    if (IS_SET(MODE_REVERSE)) {
        if (fg == &dc.colors[defaultfg]) {
            fg = &dc.colors[defaultbg];
        } else {
            revfg.r = ~fg->r;
            revfg.g = ~fg->g;
            revfg.b = ~fg->b;
            fg = &revfg;
        }

        if (bg == &dc.colors[defaultbg]) {
            bg = &dc.colors[defaultfg];
        } else {
            revbg.r = ~bg->r;
            revbg.g = ~bg->g;
            revbg.b = ~bg->b;
            bg = &revbg;
        }
    }

    if (base.mode & ATTR_REVERSE) temp = fg, fg = bg, bg = temp;

    /* Intelligent cleaning up of the borders. */
    if (x == 0) {
        xclear(0, (y == 0) ? 0 : winy, borderpx, winy + xw.ch + (y == term.row - 1) ? xw.h : 0);
    }
    if (x + charlen >= term.col - 1) {
        xclear(winx + width, (y == 0) ? 0 : winy, xw.w, (y == term.row - 1) ? xw.h : (winy + xw.ch));
    }
    if (y == 0) xclear(winx, 0, winx + width, borderpx);
    if (y == term.row - 1) xclear(winx, winy + xw.ch, winx + width, xw.h);

    // SDL_Surface *text_surface;
    SDL_Rect r = {winx, winy, width, xw.ch};

    if (xw.win != NULL) SDL_FillRect(xw.win, &r, SDL_MapRGB(xw.win->format, bg->r, bg->g, bg->b));
    // draw_keyboard(xw.win);

    if (xw.win != NULL) {
        // TODO: find a better way to draw cursor box y + 1
        int ys = r.y + 1;
        if (is_ttf_loaded()) {
            // Use TTF rendering
            draw_string_ttf(xw.win, s, winx, winy, *fg, *bg, opt_fontshade);
        } else {
            // Use bitmap rendering
            draw_string(xw.win, s, winx, ys, SDL_MapRGB(xw.win->format, fg->r, fg->g, fg->b));
        }
    }

    if (base.mode & ATTR_UNDERLINE) {
        // r.y += TTF_FontAscent(font) + 1;
        r.y += xw.ch;
        r.h = 1;
        if (xw.win != NULL) SDL_FillRect(xw.win, &r, SDL_MapRGB(xw.win->format, fg->r, fg->g, fg->b));
    }
}

void xdrawcursor(void) {
    static int oldx = 0, oldy = 0;
    int sl;
    Glyph g = {{' '}, ATTR_NULL, defaultbg, defaultcs, 0};

    LIMIT(oldx, 0, term.col - 1);
    LIMIT(oldy, 0, term.row - 1);

    if (term.line[term.c.y][term.c.x].state & GLYPH_SET) memcpy(g.c, term.line[term.c.y][term.c.x].c, UTF_SIZ);

    /* remove the old cursor */
    if (term.line[oldy][oldx].state & GLYPH_SET) {
        sl = utf8size(term.line[oldy][oldx].c);
        xdraws(term.line[oldy][oldx].c, term.line[oldy][oldx], oldx, oldy, 1, sl);
    } else {
        sdltermclear(oldx, oldy, oldx, oldy);
    }

    /* draw the new one */
    if (!(term.c.state & CURSOR_HIDE)) {
        if (!(xw.state & WIN_FOCUSED)) g.bg = defaultucs;

        if (IS_SET(MODE_REVERSE)) g.mode |= ATTR_REVERSE, g.fg = defaultcs, g.bg = defaultfg;

        sl = utf8size(g.c);
        xdraws(g.c, g, term.c.x, term.c.y, 1, sl);
        oldx = term.c.x, oldy = term.c.y;
    }
}

void redraw(void) {
    struct timespec tv = {0, REDRAW_TIMEOUT * 1000};

    tfulldirt();
    draw();
    nanosleep(&tv, NULL);
}

void draw(void) {
    drawregion(0, 0, term.col, term.row);
    update_render();
}

void drawregion(int x1, int y1, int x2, int y2) {
    int ic, ib, x, y, ox, sl;
    Glyph base, new;
    char buf[DRAW_BUF_SIZ];

    if (!(xw.state & WIN_VISIBLE)) return;

    for (y = y1; y < y2; y++) {
        if (!term.dirty[y]) continue;

        sdltermclear(0, y, term.col, y);
        term.dirty[y] = 0;
        base = term.line[y][0];
        ic = ib = ox = 0;
        for (x = x1; x < x2; x++) {
            new = term.line[y][x];
            if (ib > 0 && (!(new.state & GLYPH_SET) || ATTRCMP(base, new) || ib >= DRAW_BUF_SIZ - UTF_SIZ)) {
                xdraws(buf, base, ox, y, ic, ib);
                ic = ib = 0;
            }
            if (new.state & GLYPH_SET) {
                if (ib == 0) {
                    ox = x;
                    base = new;
                }
                sl = utf8size(new.c);
                memcpy(buf + ib, new.c, sl);
                ib += sl;
                ++ic;
            }
        }
        if (ib > 0) xdraws(buf, base, ox, y, ic, ib);
    }
    xdrawcursor();
}

void expose(SDL_Event *ev) {
    (void)ev;
    xw.state |= WIN_VISIBLE | WIN_REDRAW;
}

char *kmap(SDL_Keycode k, Uint16 state) {
    int i;
    SDL_Keymod mask;

    for (i = 0; i < LEN(key); i++) {
        mask = key[i].mask;

        if (key[i].k == k && ((state & mask) == mask || (mask == 0 && !state))) {
            return (char *)key[i].s;
        }
    }
    return NULL;
}

void kpress(SDL_Event *ev) {
    SDL_KeyboardEvent *e = &ev->key;
    char buf[32], *customkey;
    int meta, shift, ctrl, synth;
    SDL_Keycode ksym = e->keysym.sym;

    if (IS_SET(MODE_KBDLOCK)) return;

    meta = e->keysym.mod & KMOD_ALT;
    shift = e->keysym.mod & KMOD_SHIFT;
    ctrl = e->keysym.mod & KMOD_CTRL;
    synth = e->keysym.mod & KMOD_SYNTHETIC;

    // printf("kpress: keysym=%d scancode=%d mod=%d\n", ksym, e->keysym.scancode, e->keysym.mod);
    /* 1. custom keys from config.h */
    if ((customkey = kmap(ksym, e->keysym.mod))) {
        char escaped_seq[16] = {0};
        int idx = 0;
        for (int i = 0; customkey[i] != '\0'; i++) {
            unsigned char ch = customkey[i];
            if (ch == 27) {  // '\033'
                strcpy(&escaped_seq[idx], "\\033");
                idx += 4;
            } else if (ch >= 32 && ch <= 126) {
                escaped_seq[idx++] = ch;
            } else {
                sprintf(&escaped_seq[idx], "\\x%02X", ch);
                idx += 4;
            }
        }
        escaped_seq[idx] = '\0';
        printf("Custom key mapped: %s - ksym=%d, scancode=%d, mod=%d\n", escaped_seq, ksym, e->keysym.scancode, e->keysym.mod);

        ttywrite(customkey, strlen(customkey));
        /* 2. handle ctrl key */
    } else if (ctrl && !meta && !shift) {
        switch (ksym) {
            case SDLK_a:
                ttywrite("\001", 1);
                break;
            case SDLK_b:
                ttywrite("\002", 1);
                break;
            case SDLK_c:
                ttywrite("\003", 1);
                break;
            case SDLK_d:
                ttywrite("\004", 1);
                break;
            case SDLK_e:
                ttywrite("\005", 1);
                break;
            case SDLK_f:
                ttywrite("\006", 1);
                break;
            case SDLK_g:
                ttywrite("\007", 1);
                break;
            case SDLK_h:
                ttywrite("\010", 1);
                break;
            case SDLK_i:
                ttywrite("\011", 1);
                break;
            case SDLK_j:
                ttywrite("\012", 1);
                break;
            case SDLK_k:
                ttywrite("\013", 1);
                break;
            case SDLK_l:
                ttywrite("\014", 1);
                break;
            case SDLK_m:
                ttywrite("\015", 1);
                break;
            case SDLK_n:
                ttywrite("\016", 1);
                break;
            case SDLK_o:
                ttywrite("\017", 1);
                break;
            case SDLK_p:
                ttywrite("\020", 1);
                break;
            case SDLK_q:
                ttywrite("\021", 1);
                break;
            case SDLK_r:
                ttywrite("\022", 1);
                break;
            case SDLK_s:
                ttywrite("\023", 1);
                break;
            case SDLK_t:
                ttywrite("\024", 1);
                break;
            case SDLK_u:
                ttywrite("\025", 1);
                break;
            case SDLK_v:
                ttywrite("\026", 1);
                break;
            case SDLK_w:
                ttywrite("\027", 1);
                break;
            case SDLK_x:
                ttywrite("\030", 1);
                break;
            case SDLK_y:
                ttywrite("\031", 1);
                break;
            case SDLK_z:
                ttywrite("\032", 1);
                break;
            default:
                break;
        }
    }
    /* 3. standard keys */
    else {
        // special volumeup/down/powerkey handling
        if (e->keysym.scancode == 128) {
            printf("Volume Up key pressed\n");
        } else if (e->keysym.scancode == 129) {
            printf("Volume Down key pressed\n");
        } else if (e->keysym.scancode == 102) {
            printf("Power key pressed\n");
        }

        switch (ksym) {
            case SDLK_ESCAPE:
                ttywrite("\033", 1);
                break;
            case SDLK_UP:
            case SDLK_DOWN:
            case SDLK_LEFT:
            case SDLK_RIGHT:
                /* XXX: shift up/down doesn't work */
                sprintf(buf, "\033%c%c", IS_SET(MODE_APPKEYPAD) ? 'O' : '[', (shift ? "cdba" : "CDBA")[ksym - SDLK_RIGHT]);
                ttywrite(buf, 3);
                break;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                ttywrite("\033[6~", 4);
                break;
            case SDLK_TAB:
                if (shift)
                    ttywrite("\033[Z", 3);
                else
                    ttywrite("\t", 1);
                break;
            case SDLK_BACKSPACE:
                ttywrite("\x7f", 1);
                break;
            case SDLK_DELETE:
                ttywrite("\033[3~", 4);
                break;
            case SDLK_RETURN:
                if (meta) ttywrite("\033", 1);

                if (IS_SET(MODE_CRLF)) {
                    ttywrite("\r\n", 2);
                } else {
                    ttywrite("\r", 1);
                }
                break;
                /* 3. For printable characters, we'll rely on SDL_TEXTINPUT events */
            default:
                if (synth) {
                    // printf("Synthetic key event: %s\n", SDL_GetKeyName(e->keysym.sym));
                    if (e->keysym.sym <= 128) {
                        char ch = (char)e->keysym.sym;
                        if (meta) {
                            ttywrite("\033", 1);
                        }
                        ttywrite(&ch, 1);
                    }
                }
                /* For SDL2, we handle text input separately with SDL_TEXTINPUT events */
                break;
        }
    }
}

void textinput(SDL_Event *ev) {
    SDL_TextInputEvent *e = &ev->text;
    ttywrite(e->text, strlen(e->text));
}

int ttythread(void *unused) {
    int i;
    fd_set rfd;
    struct timeval drawtimeout, *tv = NULL;
    SDL_Event event;
    (void)unused;

    event.type = SDL_USEREVENT;
    event.user.code = 0;
    event.user.data1 = NULL;
    event.user.data2 = NULL;

    for (i = 0;; i++) {
        if (thread_should_exit) break;
        FD_ZERO(&rfd);
        FD_SET(cmdfd, &rfd);
        if (select(cmdfd + 1, &rfd, NULL, NULL, tv) < 0) {
            if (errno == EINTR) continue;
            die("select failed: %s\n", SERRNO);
        }

        /*
         * Stop after a certain number of reads so the user does not
         * feel like the system is stuttering.
         */
        if (i < 1000 && FD_ISSET(cmdfd, &rfd)) {
            ttyread();

            /*
             * Just wait a bit so it isn't disturbing the
             * user and the system is able to write something.
             */
            drawtimeout.tv_sec = 0;
            drawtimeout.tv_usec = 5;
            tv = &drawtimeout;
            continue;
        }
        i = 0;
        tv = NULL;

        SDL_PushEvent(&event);
    }

    return 0;
}

void run(void) {
    SDL_Event ev;
    int running = 1;
    const Uint32 SCROLL_DELAY = 150;  // milliseconds between auto-scroll
    int buttonUpHeld = 0, buttonDownHeld = 0, buttonLeftHeld = 0, buttonRightHeld = 0;
    Uint32 lastScrollTime = 0;
    while (running) {
        while (SDL_PollEvent(&ev))
        // while (SDL_WaitEvent(&ev))
        {
            if (ev.type == SDL_QUIT) {
                running = 0;
                break;
            }

            if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
                // printf("Keyboard event received - key: %s, state: %s\n", SDL_GetKeyName(ev.key.keysym.sym), (ev.type == SDL_KEYDOWN) ? "DOWN" : "UP");
                int keyboard_event = handle_keyboard_event(&ev);
                if (keyboard_event == -999) {
                    // SDL_QUIT
                    running = 0;
                    break;
                } else if (keyboard_event == 1) {
                    // printf("On-screen keyboard handled the event.\n");
                    /*SDL_Event expose_event = {
                        .type = SDL_VIDEOEXPOSE
                    };
                    SDL_PushEvent(&expose_event);*/
                } else {
                    if (event_handler[ev.type]) (event_handler[ev.type])(&ev);
                }

                // handle narrow keys held
                int held = (ev.type == SDL_KEYDOWN);
                switch (ev.key.keysym.sym) {
                    case KEY_LEFT:
                        buttonLeftHeld = held;
                        break;
                    case KEY_RIGHT:
                        buttonRightHeld = held;
                        break;
                    case KEY_UP:
                        buttonUpHeld = held;
                        break;
                    case KEY_DOWN:
                        buttonDownHeld = held;
                        break;
                    default:
                        break;
                }
            } else if (ev.type == SDL_JOYBUTTONDOWN || ev.type == SDL_JOYBUTTONUP) {
                // printf("Joystick event received - %d\n", ev.jbutton.button);
                SDL_Event sdl_event = {.key = {.type = (ev.jbutton.state == SDL_PRESSED) ? SDL_KEYDOWN : SDL_KEYUP,
                                               .state = (ev.jbutton.state == SDL_PRESSED) ? SDL_PRESSED : SDL_RELEASED,
                                               .keysym = {
                                                   .scancode = ev.jbutton.button,
                                                   .sym = ev.jbutton.button,
                                                   .mod = 0,
                                               }}};

                SDL_PushEvent(&sdl_event);
            } else {
                if (event_handler[ev.type]) (event_handler[ev.type])(&ev);
            }

            switch (ev.type) {
                // case SDL_VIDEORESIZE:
                // case SDL_VIDEOEXPOSE:
                case SDL_USEREVENT:
                    draw();
            }
        }

        Uint32 now = SDL_GetTicks();

        int key = 0;
        if (buttonDownHeld)
            key = KEY_DOWN;
        else if (buttonUpHeld)
            key = KEY_UP;
        else if (buttonLeftHeld)
            key = KEY_LEFT;
        else if (buttonRightHeld)
            key = KEY_RIGHT;

        if (key && now - lastScrollTime > SCROLL_DELAY) {
            handle_narrow_keys_held(key);
            lastScrollTime = now;
        }

        update_render();  // redraw the screen
        SDL_Delay(33);    // ~30 FPS
    }

    sdlshutdown();
}

int main(int argc, char *argv[]) {
    setenv("SDL_NOMOUSE", "1", 1);

    for (int i = 1; i < argc; i++) {
        // Handle multi-character options first
        if (strcmp(argv[i], "-scale") == 0) {
            if (++i < argc) {
                opt_scale = atof(argv[i]);
                if (opt_scale <= 0) {
                    fprintf(stderr, "Invalid scale: %s (must be positive)\n", argv[i]);
                    opt_scale = 2.0;
                }
            } else {
                fprintf(stderr, "Missing argument for -scale\n");
                die(USAGE);
            }
            continue;
        }
        if (strcmp(argv[i], "-font") == 0) {
            if (++i < argc) {
                opt_font = argv[i];
            } else {
                fprintf(stderr, "Missing argument for -font\n");
                die(USAGE);
            }
            continue;
        }
        if (strcmp(argv[i], "-fontsize") == 0) {
            if (++i < argc) {
                opt_fontsize = atoi(argv[i]);
                if (opt_fontsize <= 0) {
                    fprintf(stderr, "Invalid fontsize: %s (must be positive)\n", argv[i]);
                    opt_fontsize = 0;
                }
            } else {
                fprintf(stderr, "Missing argument for -fontsize\n");
                die(USAGE);
            }
            continue;
        }
        if (strcmp(argv[i], "-fontshade") == 0) {
            if (++i < argc) {
                opt_fontshade = atoi(argv[i]);
            } else {
                fprintf(stderr, "Missing argument for -fontshade\n");
                die(USAGE);
            }
            continue;
        }

        switch (argv[i][0] != '-' || argv[i][2] ? -1 : argv[i][1]) {
            case 'r':  // run commands from arguments, must be at the end of argv
                if (++i < argc) {
                    opt_cmd = &argv[i];
                    opt_cmd_size = argc - i;
                    for (int j = 0; j < opt_cmd_size; j++) {
                        printf("Command to execute: %s\n", opt_cmd[j]);
                    }
                    show_help = 0;
                }
                goto run;
            case 'o':  // save output commands to file
                if (++i < argc) opt_io = argv[i];
                break;
            case 'q':  // quiet mode
                active = show_help = 0;
                break;
            case 'h':  // print help
            default:
                die(USAGE);
        }
    }

    if (atexit(sdlshutdown)) {
        fprintf(stderr, "Unable to register SDL_Quit atexit\n");
    }

run:
    setlocale(LC_CTYPE, "");
    tnew((initial_width - 2) / 6, (initial_height - 2) / 8);
    ttynew();
    sdlinit();
    init_keyboard();
    run();
    return 0;
}
