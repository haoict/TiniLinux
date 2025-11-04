#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>
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
// #include <SDL2/SDL_ttf.h>

#include "font.h"
#include "keyboard.h"

#define Glyph Glyph_
#define Font Font_

#define USAGE "st (c) 2010-2012 st engineers\nusage: st [-v] [-c class] [-f font] [-g geometry] [-o file]\n[-t title] [-e command ...]\n"

/* Arbitrary sizes */
#define ESC_BUF_SIZ 256
#define ESC_ARG_SIZ 16
#define STR_BUF_SIZ 256
#define STR_ARG_SIZ 16
#define DRAW_BUF_SIZ 20 * 1024
#define UTF_SIZ 4
#define XK_NO_MOD UINT_MAX
#define XK_ANY_MOD 0

#define REDRAW_TIMEOUT (80 * 1000) /* 80 ms */

/* macros */
#define CLEANMASK(mask) (mask & (KMOD_SHIFT | KMOD_CTRL | KMOD_ALT))
#define SERRNO strerror(errno)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a) / sizeof(a[0]))
#define DEFAULT(a, b) (a) = (a) ? (a) : (b)
#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define LIMIT(x, a, b) (x) = (x) < (a) ? (a) : (x) > (b) ? (b) : (x)
#define ATTRCMP(a, b) ((a).mode != (b).mode || (a).fg != (b).fg || (a).bg != (b).bg)
#define IS_SET(flag) (term.mode & (flag))
#define TIMEDIFF(t1, t2) ((t1.tv_sec - t2.tv_sec) * 1000 + (t1.tv_usec - t2.tv_usec) / 1000)

#define VT102ID "\033[?6c"

enum glyph_attribute {
    ATTR_NULL = 0,
    ATTR_REVERSE = 1,
    ATTR_UNDERLINE = 2,
    ATTR_BOLD = 4,
    ATTR_GFX = 8,
    ATTR_ITALIC = 16,
    ATTR_BLINK = 32,
};

enum cursor_movement { CURSOR_UP, CURSOR_DOWN, CURSOR_LEFT, CURSOR_RIGHT, CURSOR_SAVE, CURSOR_LOAD };

enum cursor_state { CURSOR_DEFAULT = 0, CURSOR_HIDE = 1, CURSOR_WRAPNEXT = 2 };

enum glyph_state { GLYPH_SET = 1, GLYPH_DIRTY = 2 };

enum term_mode { MODE_WRAP = 1, MODE_INSERT = 2, MODE_APPKEYPAD = 4, MODE_ALTSCREEN = 8, MODE_CRLF = 16, MODE_MOUSEBTN = 32, MODE_MOUSEMOTION = 64, MODE_MOUSE = 32 | 64, MODE_REVERSE = 128, MODE_KBDLOCK = 256 };

enum escape_state {
    ESC_START = 1,
    ESC_CSI = 2,
    ESC_STR = 4, /* DSC, OSC, PM, APC */
    ESC_ALTCHARSET = 8,
    ESC_STR_END = 16, /* a final string was encountered */
    ESC_TEST = 32,    /* Enter in test mode */
};

enum window_state { WIN_VISIBLE = 1, WIN_REDRAW = 2, WIN_FOCUSED = 4 };

/* bit macro */
#undef B0
enum { B0 = 1, B1 = 2, B2 = 4, B3 = 8, B4 = 16, B5 = 32, B6 = 64, B7 = 128 };

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned short ushort;

typedef struct {
    char c[UTF_SIZ]; /* character code */
    uchar mode;      /* attribute flags */
    ushort fg;       /* foreground  */
    ushort bg;       /* background  */
    uchar state;     /* state flags    */
} Glyph;

typedef Glyph *Line;

typedef struct {
    Glyph attr; /* current char attributes */
    int x;
    int y;
    char state;
} TCursor;

/* CSI Escape sequence structs */
/* ESC '[' [[ [<priv>] <arg> [;]] <mode>] */
typedef struct {
    char buf[ESC_BUF_SIZ]; /* raw string */
    int len;               /* raw string length */
    char priv;
    int arg[ESC_ARG_SIZ];
    int narg; /* nb of args */
    char mode;
} CSIEscape;

/* STR Escape sequence structs */
/* ESC type [[ [<priv>] <arg> [;]] <mode>] ESC '\' */
typedef struct {
    char type;             /* ESC type ... */
    char buf[STR_BUF_SIZ]; /* raw string */
    int len;               /* raw string length */
    char *args[STR_ARG_SIZ];
    int narg; /* nb of args */
} STREscape;

/* Internal representation of the screen */
typedef struct {
    int row;     /* nb row */
    int col;     /* nb col */
    Line *line;  /* screen */
    Line *alt;   /* alternate screen */
    bool *dirty; /* dirtyness of lines */
    TCursor c;   /* cursor */
    int top;     /* top    scroll limit */
    int bot;     /* bottom scroll limit */
    int mode;    /* terminal mode flags */
    int esc;     /* escape state flags */
    bool *tabs;
} Term;

/* Purely graphic info */
typedef struct {
    // Colormap cmap;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Surface *win;
    int scr;
    bool isfixed;       /* is fixed geometry? */
    int fx, fy, fw, fh; /* fixed geometry */
    int tw, th;         /* tty width and height */
    int w;              /* window width */
    int h;              /* window height */
    int ch;             /* char height */
    int cw;             /* char width  */
    char state;         /* focus, redraw, visible */
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

static void die(const char *, ...);
static void draw(void);
static void redraw(void);
static void drawregion(int, int, int, int);
static void execsh(void);
static void sigchld(int);
static void run(void);
int ttythread(void *unused);

static void csidump(void);
static void csihandle(void);
static void csiparse(void);
static void csireset(void);
static void strreset(void);

static void tclearregion(int, int, int, int);
static void tcursor(int);
static void tdeletechar(int);
static void tdeleteline(int);
static void tinsertblank(int);
static void tinsertblankline(int);
static void tmoveto(int, int);
static void tnew(int, int);
static void tnewline(int);
static void tputtab(bool);
static void tputc(char *, int);
static void treset(void);
static int tresize(int, int);
static void tscrollup(int, int);
static void tscrolldown(int, int);
static void tsetattr(int *, int);
static void tsetchar(char *, Glyph *, int, int);
static void tsetscroll(int, int);
static void tswapscreen(void);
static void tsetdirt(int, int);
static void tsetmode(bool, bool, int *, int);
static void tfulldirt(void);

static void ttynew(void);
static void ttyread(void);
static void ttyresize(void);
static void ttywrite(const char *, size_t);

static void xdraws(char *, Glyph, int, int, int, int);
static void xclear(int, int, int, int);
static void xdrawcursor(void);
static void sdlinit(void);
static void initcolormap(void);
static void sdltermclear(int, int, int, int);
static void xresize(int, int);

static void expose(SDL_Event *);
static char *kmap(SDL_Keycode, Uint16);
static void kpress(SDL_Event *);
static void textinput(SDL_Event *);

static int utf8decode(char *, long *);
static int utf8encode(long *, char *);
static int utf8size(char *);
static int isfullutf8(char *, int);

static ssize_t xwrite(int, char *, size_t);
static void *xmalloc(size_t);
static void *xrealloc(void *, size_t);
static void *xcalloc(size_t nmemb, size_t size);
static void xflip(void);

static void (*handler[SDL_LASTEVENT])(SDL_Event *) = {[SDL_KEYDOWN] = kpress, [SDL_TEXTINPUT] = textinput};

/* Globals */
static DC dc;
static XWindow xw;
static Term term;
static CSIEscape csiescseq;
static STREscape strescseq;
static int cmdfd;
static pid_t pid;
static int iofd = -1;
static char **opt_cmd = NULL;
static int opt_cmd_size = 0;
static char *opt_io = NULL;
static char *opt_title = NULL;
static char *opt_class = NULL;
static char *opt_font = NULL;

static char *usedfont = NULL;
static int usedfontsize = 0;

static int high_res = 0;
static int initial_width = 320;
static int initial_height = 240;
static volatile int thread_should_exit = 0;
SDL_Joystick *joystick;

ssize_t xwrite(int fd, char *s, size_t len) {
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

void sdlloadfonts(char *fontstr, int fontsize) {
    // char *bfontstr;

    usedfont = fontstr;
    usedfontsize = fontsize;

    /* XXX: Strongly assumes the original setting had a : in it! */
    /*if((bfontstr = strchr(fontstr, ':'))) {
        *bfontstr = '\0';
        bfontstr++;
    } else {
        bfontstr = strchr(fontstr, '\0');
        bfontstr++;
    }*/

    /*if(dc.font) TTF_CloseFont(dc.font);
    dc.font = TTF_OpenFont(fontstr, fontsize);
    fprintf(stderr, "%s\n", fontstr);*/
    // TTF_SizeUTF8(dc.font, "O", &xw.cw, &xw.ch);
    xw.cw = 6;
    xw.ch = 8;

    /*if(dc.ifont) TTF_CloseFont(dc.ifont);
    dc.ifont = TTF_OpenFont(fontstr, fontsize);
    fprintf(stderr, "%s\n", fontstr);
    TTF_SetFontStyle(dc.ifont, TTF_STYLE_ITALIC);

    if(dc.bfont) TTF_CloseFont(dc.bfont);
    dc.bfont = TTF_OpenFont(bfontstr, fontsize);
    fprintf(stderr, "%s\n", bfontstr);

    if(dc.ibfont) TTF_CloseFont(dc.ibfont);
    dc.ibfont = TTF_OpenFont(bfontstr, fontsize);
    fprintf(stderr, "%s\n", bfontstr);
    TTF_SetFontStyle(dc.ibfont, TTF_STYLE_ITALIC);*/
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
        if (xw.win) SDL_FreeSurface(xw.win);
        if (screen2) SDL_FreeSurface(screen2);
        xw.win = NULL;
        SDL_JoystickClose(joystick);
        SDL_Quit();
    }
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
    usedfont = (opt_font == NULL) ? font : opt_font;
    sdlloadfonts(usedfont, fontsize);

    fprintf(stderr, "SDL font: %s\n", usedfont);
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
        xw.w = mode.w / 2;
        xw.h = mode.h / 2;

        #ifndef BR2
        xw.w = 320;
        xw.h = 240;
        printf("Generic build, using 320x240\n");
        #endif
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
}

void xflip(void) {
    if (xw.win == NULL) return;
    memcpy(screen2->pixels, xw.win->pixels, xw.w * xw.h * 2);
    draw_keyboard(screen2);  // screen2(SW) = console + keyboard
    // Update texture with screen pixels and render
    SDL_UpdateTexture(xw.texture, NULL, screen2->pixels, screen2->pitch);
    SDL_RenderClear(xw.renderer);
    SDL_RenderCopy(xw.renderer, xw.texture, NULL, NULL);
    SDL_RenderPresent(xw.renderer);
}

int utf8decode(char *s, long *u) {
    uchar c;
    int i, n, rtn;

    rtn = 1;
    c = *s;
    if (~c & B7) { /* 0xxxxxxx */
        *u = c;
        return rtn;
    } else if ((c & (B7 | B6 | B5)) == (B7 | B6)) { /* 110xxxxx */
        *u = c & (B4 | B3 | B2 | B1 | B0);
        n = 1;
    } else if ((c & (B7 | B6 | B5 | B4)) == (B7 | B6 | B5)) { /* 1110xxxx */
        *u = c & (B3 | B2 | B1 | B0);
        n = 2;
    } else if ((c & (B7 | B6 | B5 | B4 | B3)) == (B7 | B6 | B5 | B4)) { /* 11110xxx */
        *u = c & (B2 | B1 | B0);
        n = 3;
    } else {
        goto invalid;
    }

    for (i = n, ++s; i > 0; --i, ++rtn, ++s) {
        c = *s;
        if ((c & (B7 | B6)) != B7) /* 10xxxxxx */
            goto invalid;
        *u <<= 6;
        *u |= c & (B5 | B4 | B3 | B2 | B1 | B0);
    }

    if ((n == 1 && *u < 0x80) || (n == 2 && *u < 0x800) || (n == 3 && *u < 0x10000) || (*u >= 0xD800 && *u <= 0xDFFF)) {
        goto invalid;
    }

    return rtn;
invalid:
    *u = 0xFFFD;

    return rtn;
}

int utf8encode(long *u, char *s) {
    uchar *sp;
    ulong uc;
    int i, n;

    sp = (uchar *)s;
    uc = *u;
    if (uc < 0x80) {
        *sp = uc; /* 0xxxxxxx */
        return 1;
    } else if (*u < 0x800) {
        *sp = (uc >> 6) | (B7 | B6); /* 110xxxxx */
        n = 1;
    } else if (uc < 0x10000) {
        *sp = (uc >> 12) | (B7 | B6 | B5); /* 1110xxxx */
        n = 2;
    } else if (uc <= 0x10FFFF) {
        *sp = (uc >> 18) | (B7 | B6 | B5 | B4); /* 11110xxx */
        n = 3;
    } else {
        goto invalid;
    }

    for (i = n, ++sp; i > 0; --i, ++sp) *sp = ((uc >> 6 * (i - 1)) & (B5 | B4 | B3 | B2 | B1 | B0)) | B7; /* 10xxxxxx */

    return n + 1;
invalid:
    /* U+FFFD */
    *s++ = '\xEF';
    *s++ = '\xBF';
    *s = '\xBD';

    return 3;
}

/* use this if your buffer is less than UTF_SIZ, it returns 1 if you can decode
   UTF-8 otherwise return 0 */
int isfullutf8(char *s, int b) {
    uchar *c1, *c2, *c3;

    c1 = (uchar *)s;
    c2 = (uchar *)++s;
    c3 = (uchar *)++s;
    if (b < 1) {
        return 0;
    } else if ((*c1 & (B7 | B6 | B5)) == (B7 | B6) && b == 1) {
        return 0;
    } else if ((*c1 & (B7 | B6 | B5 | B4)) == (B7 | B6 | B5) && ((b == 1) || ((b == 2) && (*c2 & (B7 | B6)) == B7))) {
        return 0;
    } else if ((*c1 & (B7 | B6 | B5 | B4 | B3)) == (B7 | B6 | B5 | B4) && ((b == 1) || ((b == 2) && (*c2 & (B7 | B6)) == B7) || ((b == 3) && (*c2 & (B7 | B6)) == B7 && (*c3 & (B7 | B6)) == B7))) {
        return 0;
    } else {
        return 1;
    }
}

int utf8size(char *s) {
    uchar c = *s;

    if (~c & B7) {
        return 1;
    } else if ((c & (B7 | B6 | B5)) == (B7 | B6)) {
        return 2;
    } else if ((c & (B7 | B6 | B5 | B4)) == (B7 | B6 | B5)) {
        return 3;
    } else {
        return 4;
    }
}

void die(const char *errstr, ...) {
    va_list ap;

    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    sdlshutdown();
}

void execsh(void) {
    char **args;
    char *envshell = getenv("SHELL");
    const struct passwd *pass = getpwuid(getuid());

    unsetenv("COLUMNS");
    unsetenv("LINES");
    unsetenv("TERMCAP");

    if (pass) {
        setenv("LOGNAME", pass->pw_name, 1);
        setenv("USER", pass->pw_name, 1);
        setenv("SHELL", pass->pw_shell, 0);
        setenv("HOME", pass->pw_dir, 0);
    }
    chdir(getenv("HOME"));

    char *home = get_current_dir_name();
    setenv("HOME", home, 1);
    free(home);

    setenv("PS1", "\\[\\033[32m\\]\\W\\[\\033[00m\\]\\$ ", 1);
    setenv("LANG", "", 1);

    if (show_help != 0) {
        system("uname -a");
        system("echo '\n'");
    }

    signal(SIGCHLD, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGALRM, SIG_DFL);

    DEFAULT(envshell, shell);
    setenv("TERM", termname, 1);
    args = (char *[]){envshell, "-i", NULL};

    // executing opt_cmd
    for (int i = 0; i < opt_cmd_size; i++) {
        char echo_cmd[255];
        sprintf(echo_cmd, "echo '\n$ %s\n'", opt_cmd[i]);
        system(echo_cmd);
        system(opt_cmd[i]);
    }

    execvp(args[0], args);
    exit(EXIT_FAILURE);
}

void sigchld(int a) {
    int stat = 0;
    (void)a;

    if (waitpid(pid, &stat, 0) < 0) die("Waiting for pid %hd failed: %s\n", pid, SERRNO);

    if (WIFEXITED(stat)) {
        exit(WEXITSTATUS(stat));
    } else {
        exit(EXIT_FAILURE);
    }
}

void ttynew(void) {
    int m, s;
    struct winsize w = {term.row, term.col, 0, 0};

    /* seems to work fine on linux, openbsd and freebsd */
    if (openpty(&m, &s, NULL, NULL, &w) < 0) die("openpty failed: %s\n", SERRNO);

    switch (pid = fork()) {
        case -1:
            die("fork failed\n");
            break;
        case 0:
            setsid(); /* create a new process group */
            dup2(s, STDIN_FILENO);
            dup2(s, STDOUT_FILENO);
            dup2(s, STDERR_FILENO);
            if (ioctl(s, TIOCSCTTY, NULL) < 0) die("ioctl TIOCSCTTY failed: %s\n", SERRNO);
            close(s);
            close(m);
            execsh();
            break;
        default:
            close(s);
            cmdfd = m;
            signal(SIGCHLD, sigchld);
            if (opt_io) {
                iofd = (!strcmp(opt_io, "-")) ? STDOUT_FILENO : open(opt_io, O_WRONLY | O_CREAT, 0666);
                if (iofd < 0) {
                    fprintf(stderr, "Error opening %s:%s\n", opt_io, strerror(errno));
                }
            }
    }
}

void dump(char c) {
    static int col;

    fprintf(stderr, " %02x '%c' ", c, isprint(c) ? c : '.');
    if (++col % 10 == 0) fprintf(stderr, "\n");
}

void ttyread(void) {
    static char buf[BUFSIZ];
    static int buflen = 0;
    char *ptr;
    char s[UTF_SIZ];
    int charsize; /* size of utf8 char in bytes */
    long utf8c;
    int ret;

    /* append read bytes to unprocessed bytes */
    if ((ret = read(cmdfd, buf + buflen, LEN(buf) - buflen)) < 0) die("Couldn't read from shell: %s\n", SERRNO);

    /* process every complete utf8 char */
    buflen += ret;
    ptr = buf;
    while (buflen >= UTF_SIZ || isfullutf8(ptr, buflen)) {
        charsize = utf8decode(ptr, &utf8c);
        utf8encode(&utf8c, s);
        tputc(s, charsize);
        ptr += charsize;
        buflen -= charsize;
    }

    /* keep any uncomplete utf8 char for the next call */
    memmove(buf, ptr, buflen);
}

void ttywrite(const char *s, size_t n) {
    if (write(cmdfd, s, n) == -1) die("write error on tty: %s\n", SERRNO);
}

void ttyresize(void) {
    struct winsize w;

    w.ws_row = term.row;
    w.ws_col = term.col;
    w.ws_xpixel = xw.tw;
    w.ws_ypixel = xw.th;
    if (ioctl(cmdfd, TIOCSWINSZ, &w) < 0) fprintf(stderr, "Couldn't set window size: %s\n", SERRNO);
}

void tsetdirt(int top, int bot) {
    int i;

    LIMIT(top, 0, term.row - 1);
    LIMIT(bot, 0, term.row - 1);

    for (i = top; i <= bot; i++) term.dirty[i] = 1;
}

void tfulldirt(void) { tsetdirt(0, term.row - 1); }

void tcursor(int mode) {
    static TCursor c;

    if (mode == CURSOR_SAVE) {
        c = term.c;
    } else if (mode == CURSOR_LOAD) {
        term.c = c;
        tmoveto(c.x, c.y);
    }
}

void treset(void) {
    uint i;

    term.c = (TCursor){{.mode = ATTR_NULL, .fg = defaultfg, .bg = defaultbg}, .x = 0, .y = 0, .state = CURSOR_DEFAULT};

    memset(term.tabs, 0, term.col * sizeof(*term.tabs));
    for (i = tabspaces; i < term.col; i += tabspaces) term.tabs[i] = 1;
    term.top = 0;
    term.bot = term.row - 1;
    term.mode = MODE_WRAP;

    tclearregion(0, 0, term.col - 1, term.row - 1);
}

void tnew(int col, int row) {
    /* set screen size */
    term.row = row;
    term.col = col;
    term.line = xmalloc(term.row * sizeof(Line));
    term.alt = xmalloc(term.row * sizeof(Line));
    term.dirty = xmalloc(term.row * sizeof(*term.dirty));
    term.tabs = xmalloc(term.col * sizeof(*term.tabs));

    for (row = 0; row < term.row; row++) {
        term.line[row] = xmalloc(term.col * sizeof(Glyph));
        term.alt[row] = xmalloc(term.col * sizeof(Glyph));
        term.dirty[row] = 0;
    }
    memset(term.tabs, 0, term.col * sizeof(*term.tabs));
    /* setup screen */
    treset();
}

void tswapscreen(void) {
    Line *tmp = term.line;

    term.line = term.alt;
    term.alt = tmp;
    term.mode ^= MODE_ALTSCREEN;
    tfulldirt();
}

void tscrolldown(int orig, int n) {
    int i;
    Line temp;

    LIMIT(n, 0, term.bot - orig + 1);

    tclearregion(0, term.bot - n + 1, term.col - 1, term.bot);

    for (i = term.bot; i >= orig + n; i--) {
        temp = term.line[i];
        term.line[i] = term.line[i - n];
        term.line[i - n] = temp;

        term.dirty[i] = 1;
        term.dirty[i - n] = 1;
    }
}

void tscrollup(int orig, int n) {
    int i;
    Line temp;
    LIMIT(n, 0, term.bot - orig + 1);

    tclearregion(0, orig, term.col - 1, orig + n - 1);

    for (i = orig; i <= term.bot - n; i++) {
        temp = term.line[i];
        term.line[i] = term.line[i + n];
        term.line[i + n] = temp;

        term.dirty[i] = 1;
        term.dirty[i + n] = 1;
    }
}

void tnewline(int first_col) {
    int y = term.c.y;

    if (y == term.bot) {
        tscrollup(term.top, 1);
    } else {
        y++;
    }
    tmoveto(first_col ? 0 : term.c.x, y);
}

void csiparse(void) {
    /* int noarg = 1; */
    char *p = csiescseq.buf;

    csiescseq.narg = 0;
    if (*p == '?') csiescseq.priv = 1, p++;

    while (p < csiescseq.buf + csiescseq.len) {
        while (isdigit(*p)) {
            csiescseq.arg[csiescseq.narg] *= 10;
            csiescseq.arg[csiescseq.narg] += *p++ - '0' /*, noarg = 0 */;
        }
        if (*p == ';' && csiescseq.narg + 1 < ESC_ARG_SIZ) {
            csiescseq.narg++, p++;
        } else {
            csiescseq.mode = *p;
            csiescseq.narg++;

            return;
        }
    }
}

void tmoveto(int x, int y) {
    LIMIT(x, 0, term.col - 1);
    LIMIT(y, 0, term.row - 1);
    term.c.state &= ~CURSOR_WRAPNEXT;
    term.c.x = x;
    term.c.y = y;
}

void tsetchar(char *c, Glyph *attr, int x, int y) {
    static char *vt100_0[62] = {
        /* 0x41 - 0x7e */
        "↑", "↓", "→", "←", "█", "▚", "☃",      /* A - G */
        0,   0,   0,   0,   0,   0,   0,   0,   /* H - O */
        0,   0,   0,   0,   0,   0,   0,   0,   /* P - W */
        0,   0,   0,   0,   0,   0,   0,   " ", /* X - _ */
        "◆", "▒", "␉", "␌", "␍", "␊", "°", "±", /* ` - g */
        "␤", "␋", "┘", "┐", "┌", "└", "┼", "⎺", /* h - o */
        "⎻", "─", "⎼", "⎽", "├", "┤", "┴", "┬", /* p - w */
        "│", "≤", "≥", "π", "≠", "£", "·",      /* x - ~ */
    };

    /*
     * The table is proudly stolen from rxvt.
     */
    if (attr->mode & ATTR_GFX) {
        if (c[0] >= 0x41 && c[0] <= 0x7e && vt100_0[c[0] - 0x41]) {
            c = vt100_0[c[0] - 0x41];
        }
    }

    term.dirty[y] = 1;
    term.line[y][x] = *attr;
    memcpy(term.line[y][x].c, c, UTF_SIZ);
    term.line[y][x].state |= GLYPH_SET;
}

void tclearregion(int x1, int y1, int x2, int y2) {
    int x, y, temp;

    if (x1 > x2) temp = x1, x1 = x2, x2 = temp;
    if (y1 > y2) temp = y1, y1 = y2, y2 = temp;

    LIMIT(x1, 0, term.col - 1);
    LIMIT(x2, 0, term.col - 1);
    LIMIT(y1, 0, term.row - 1);
    LIMIT(y2, 0, term.row - 1);

    for (y = y1; y <= y2; y++) {
        term.dirty[y] = 1;
        for (x = x1; x <= x2; x++) term.line[y][x].state = 0;
    }
}

void tdeletechar(int n) {
    int src = term.c.x + n;
    int dst = term.c.x;
    int size = term.col - src;

    term.dirty[term.c.y] = 1;

    if (src >= term.col) {
        tclearregion(term.c.x, term.c.y, term.col - 1, term.c.y);
        return;
    }

    memmove(&term.line[term.c.y][dst], &term.line[term.c.y][src], size * sizeof(Glyph));
    tclearregion(term.col - n, term.c.y, term.col - 1, term.c.y);
}

void tinsertblank(int n) {
    int src = term.c.x;
    int dst = src + n;
    int size = term.col - dst;

    term.dirty[term.c.y] = 1;

    if (dst >= term.col) {
        tclearregion(term.c.x, term.c.y, term.col - 1, term.c.y);
        return;
    }

    memmove(&term.line[term.c.y][dst], &term.line[term.c.y][src], size * sizeof(Glyph));
    tclearregion(src, term.c.y, dst - 1, term.c.y);
}

void tinsertblankline(int n) {
    if (term.c.y < term.top || term.c.y > term.bot) return;

    tscrolldown(term.c.y, n);
}

void tdeleteline(int n) {
    if (term.c.y < term.top || term.c.y > term.bot) return;

    tscrollup(term.c.y, n);
}

void tsetattr(int *attr, int l) {
    int i;

    for (i = 0; i < l; i++) {
        switch (attr[i]) {
            case 0:
                term.c.attr.mode &= ~(ATTR_REVERSE | ATTR_UNDERLINE | ATTR_BOLD | ATTR_ITALIC | ATTR_BLINK);
                term.c.attr.fg = defaultfg;
                term.c.attr.bg = defaultbg;
                break;
            case 1:
                term.c.attr.mode |= ATTR_BOLD;
                break;
            case 3: /* enter standout (highlight) */
                term.c.attr.mode |= ATTR_ITALIC;
                break;
            case 4:
                term.c.attr.mode |= ATTR_UNDERLINE;
                break;
            case 5:
                term.c.attr.mode |= ATTR_BLINK;
                break;
            case 7:
                term.c.attr.mode |= ATTR_REVERSE;
                break;
            case 21:
            case 22:
                term.c.attr.mode &= ~ATTR_BOLD;
                break;
            case 23: /* leave standout (highlight) mode */
                term.c.attr.mode &= ~ATTR_ITALIC;
                break;
            case 24:
                term.c.attr.mode &= ~ATTR_UNDERLINE;
                break;
            case 25:
                term.c.attr.mode &= ~ATTR_BLINK;
                break;
            case 27:
                term.c.attr.mode &= ~ATTR_REVERSE;
                break;
            case 38:
                if (i + 2 < l && attr[i + 1] == 5) {
                    i += 2;
                    if (BETWEEN(attr[i], 0, 255)) {
                        term.c.attr.fg = attr[i];
                    } else {
                        fprintf(stderr, "erresc: bad fgcolor %d\n", attr[i]);
                    }
                } else {
                    fprintf(stderr, "erresc(38): gfx attr %d unknown\n", attr[i]);
                }
                break;
            case 39:
                term.c.attr.fg = defaultfg;
                break;
            case 48:
                if (i + 2 < l && attr[i + 1] == 5) {
                    i += 2;
                    if (BETWEEN(attr[i], 0, 255)) {
                        term.c.attr.bg = attr[i];
                    } else {
                        fprintf(stderr, "erresc: bad bgcolor %d\n", attr[i]);
                    }
                } else {
                    fprintf(stderr, "erresc(48): gfx attr %d unknown\n", attr[i]);
                }
                break;
            case 49:
                term.c.attr.bg = defaultbg;
                break;
            default:
                if (BETWEEN(attr[i], 30, 37)) {
                    term.c.attr.fg = attr[i] - 30;
                } else if (BETWEEN(attr[i], 40, 47)) {
                    term.c.attr.bg = attr[i] - 40;
                } else if (BETWEEN(attr[i], 90, 97)) {
                    term.c.attr.fg = attr[i] - 90 + 8;
                } else if (BETWEEN(attr[i], 100, 107)) {
                    term.c.attr.bg = attr[i] - 100 + 8;
                } else {
                    fprintf(stderr, "erresc(default): gfx attr %d unknown\n", attr[i]), csidump();
                }
                break;
        }
    }
}

void tsetscroll(int t, int b) {
    int temp;

    LIMIT(t, 0, term.row - 1);
    LIMIT(b, 0, term.row - 1);
    if (t > b) {
        temp = t;
        t = b;
        b = temp;
    }
    term.top = t;
    term.bot = b;
}

#define MODBIT(x, set, bit) ((set) ? ((x) |= (bit)) : ((x) &= ~(bit)))

void tsetmode(bool priv, bool set, int *args, int narg) {
    int *lim, mode;

    for (lim = args + narg; args < lim; ++args) {
        if (priv) {
            switch (*args) {
                break;
                case 1: /* DECCKM -- Cursor key */
                    MODBIT(term.mode, set, MODE_APPKEYPAD);
                    break;
                case 5: /* DECSCNM -- Reverse video */
                    mode = term.mode;
                    MODBIT(term.mode, set, MODE_REVERSE);
                    if (mode != term.mode) redraw();
                    break;
                case 6: /* XXX: DECOM -- Origin */
                    break;
                case 7: /* DECAWM -- Auto wrap */
                    MODBIT(term.mode, set, MODE_WRAP);
                    break;
                case 8: /* XXX: DECARM -- Auto repeat */
                    break;
                case 0:  /* Error (IGNORED) */
                case 12: /* att610 -- Start blinking cursor (IGNORED) */
                    break;
                case 25:
                    MODBIT(term.c.state, !set, CURSOR_HIDE);
                    break;
                case 1000: /* 1000,1002: enable xterm mouse report */
                    MODBIT(term.mode, set, MODE_MOUSEBTN);
                    break;
                case 1002:
                    MODBIT(term.mode, set, MODE_MOUSEMOTION);
                    break;
                case 1049: /* = 1047 and 1048 */
                case 47:
                case 1047:
                    if (IS_SET(MODE_ALTSCREEN)) tclearregion(0, 0, term.col - 1, term.row - 1);
                    if ((set && !IS_SET(MODE_ALTSCREEN)) || (!set && IS_SET(MODE_ALTSCREEN))) {
                        tswapscreen();
                    }
                    if (*args != 1049) break;
                    /* pass through */
                case 1048:
                    tcursor((set) ? CURSOR_SAVE : CURSOR_LOAD);
                    break;
                case 2004: /* bracketed paste mode */
                    // MODBIT(term.mode, set, MODE_BRACKETPASTE);
                    break;
                default:
                    /* case 2:  DECANM -- ANSI/VT52 (NOT SUPPOURTED) */
                    /* case 3:  DECCOLM -- Column  (NOT SUPPORTED) */
                    /* case 4:  DECSCLM -- Scroll (NOT SUPPORTED) */
                    /* case 18: DECPFF -- Printer feed (NOT SUPPORTED) */
                    /* case 19: DECPEX -- Printer extent (NOT SUPPORTED) */
                    /* case 42: DECNRCM -- National characters (NOT SUPPORTED) */
                    fprintf(stderr, "erresc: unknown private set/reset mode %d\n", *args);
                    break;
            }
        } else {
            switch (*args) {
                case 0: /* Error (IGNORED) */
                    break;
                case 2: /* KAM -- keyboard action */
                    MODBIT(term.mode, set, MODE_KBDLOCK);
                    break;
                case 4: /* IRM -- Insertion-replacement */
                    MODBIT(term.mode, set, MODE_INSERT);
                    break;
                case 12: /* XXX: SRM -- Send/Receive */
                    break;
                case 20: /* LNM -- Linefeed/new line */
                    MODBIT(term.mode, set, MODE_CRLF);
                    break;
                default:
                    fprintf(stderr, "erresc: unknown set/reset mode %d\n", *args);
                    break;
            }
        }
    }
}
#undef MODBIT

void csihandle(void) {
    switch (csiescseq.mode) {
        default:
        unknown:
            fprintf(stderr, "erresc: unknown csi ");
            csidump();
            /* die(""); */
            break;
        case '@': /* ICH -- Insert <n> blank char */
            DEFAULT(csiescseq.arg[0], 1);
            tinsertblank(csiescseq.arg[0]);
            break;
        case 'A': /* CUU -- Cursor <n> Up */
        case 'e':
            DEFAULT(csiescseq.arg[0], 1);
            tmoveto(term.c.x, term.c.y - csiescseq.arg[0]);
            break;
        case 'B': /* CUD -- Cursor <n> Down */
            DEFAULT(csiescseq.arg[0], 1);
            tmoveto(term.c.x, term.c.y + csiescseq.arg[0]);
            break;
        case 'c': /* DA -- Device Attributes */
            if (csiescseq.arg[0] == 0) ttywrite(VT102ID, sizeof(VT102ID) - 1);
            break;
        case 'C': /* CUF -- Cursor <n> Forward */
        case 'a':
            DEFAULT(csiescseq.arg[0], 1);
            tmoveto(term.c.x + csiescseq.arg[0], term.c.y);
            break;
        case 'D': /* CUB -- Cursor <n> Backward */
            DEFAULT(csiescseq.arg[0], 1);
            tmoveto(term.c.x - csiescseq.arg[0], term.c.y);
            break;
        case 'E': /* CNL -- Cursor <n> Down and first col */
            DEFAULT(csiescseq.arg[0], 1);
            tmoveto(0, term.c.y + csiescseq.arg[0]);
            break;
        case 'F': /* CPL -- Cursor <n> Up and first col */
            DEFAULT(csiescseq.arg[0], 1);
            tmoveto(0, term.c.y - csiescseq.arg[0]);
            break;
        case 'g': /* TBC -- Tabulation clear */
            switch (csiescseq.arg[0]) {
                case 0: /* clear current tab stop */
                    term.tabs[term.c.x] = 0;
                    break;
                case 3: /* clear all the tabs */
                    memset(term.tabs, 0, term.col * sizeof(*term.tabs));
                    break;
                default:
                    goto unknown;
            }
            break;
        case 'G': /* CHA -- Move to <col> */
        case '`': /* HPA */
            DEFAULT(csiescseq.arg[0], 1);
            tmoveto(csiescseq.arg[0] - 1, term.c.y);
            break;
        case 'H': /* CUP -- Move to <row> <col> */
        case 'f': /* HVP */
            DEFAULT(csiescseq.arg[0], 1);
            DEFAULT(csiescseq.arg[1], 1);
            tmoveto(csiescseq.arg[1] - 1, csiescseq.arg[0] - 1);
            break;
        case 'I': /* CHT -- Cursor Forward Tabulation <n> tab stops */
            DEFAULT(csiescseq.arg[0], 1);
            while (csiescseq.arg[0]--) tputtab(1);
            break;
        case 'J': /* ED -- Clear screen */
            switch (csiescseq.arg[0]) {
                case 0: /* below */
                    tclearregion(term.c.x, term.c.y, term.col - 1, term.c.y);
                    if (term.c.y < term.row - 1) tclearregion(0, term.c.y + 1, term.col - 1, term.row - 1);
                    break;
                case 1: /* above */
                    if (term.c.y > 1) tclearregion(0, 0, term.col - 1, term.c.y - 1);
                    tclearregion(0, term.c.y, term.c.x, term.c.y);
                    break;
                case 2: /* all */
                    tclearregion(0, 0, term.col - 1, term.row - 1);
                    break;
                default:
                    goto unknown;
            }
            break;
        case 'K': /* EL -- Clear line */
            switch (csiescseq.arg[0]) {
                case 0: /* right */
                    tclearregion(term.c.x, term.c.y, term.col - 1, term.c.y);
                    break;
                case 1: /* left */
                    tclearregion(0, term.c.y, term.c.x, term.c.y);
                    break;
                case 2: /* all */
                    tclearregion(0, term.c.y, term.col - 1, term.c.y);
                    break;
            }
            break;
        case 'S': /* SU -- Scroll <n> line up */
            DEFAULT(csiescseq.arg[0], 1);
            tscrollup(term.top, csiescseq.arg[0]);
            break;
        case 'T': /* SD -- Scroll <n> line down */
            DEFAULT(csiescseq.arg[0], 1);
            tscrolldown(term.top, csiescseq.arg[0]);
            break;
        case 'L': /* IL -- Insert <n> blank lines */
            DEFAULT(csiescseq.arg[0], 1);
            tinsertblankline(csiescseq.arg[0]);
            break;
        case 'l': /* RM -- Reset Mode */
            tsetmode(csiescseq.priv, 0, csiescseq.arg, csiescseq.narg);
            break;
        case 'M': /* DL -- Delete <n> lines */
            DEFAULT(csiescseq.arg[0], 1);
            tdeleteline(csiescseq.arg[0]);
            break;
        case 'X': /* ECH -- Erase <n> char */
            DEFAULT(csiescseq.arg[0], 1);
            tclearregion(term.c.x, term.c.y, term.c.x + csiescseq.arg[0], term.c.y);
            break;
        case 'P': /* DCH -- Delete <n> char */
            DEFAULT(csiescseq.arg[0], 1);
            tdeletechar(csiescseq.arg[0]);
            break;
        case 'Z': /* CBT -- Cursor Backward Tabulation <n> tab stops */
            DEFAULT(csiescseq.arg[0], 1);
            while (csiescseq.arg[0]--) tputtab(0);
            break;
        case 'd': /* VPA -- Move to <row> */
            DEFAULT(csiescseq.arg[0], 1);
            tmoveto(term.c.x, csiescseq.arg[0] - 1);
            break;
        case 'h': /* SM -- Set terminal mode */
            tsetmode(csiescseq.priv, 1, csiescseq.arg, csiescseq.narg);
            break;
        case 'm': /* SGR -- Terminal attribute (color) */
            tsetattr(csiescseq.arg, csiescseq.narg);
            break;
        case 'r': /* DECSTBM -- Set Scrolling Region */
            if (csiescseq.priv) {
                goto unknown;
            } else {
                DEFAULT(csiescseq.arg[0], 1);
                DEFAULT(csiescseq.arg[1], term.row);
                tsetscroll(csiescseq.arg[0] - 1, csiescseq.arg[1] - 1);
                tmoveto(0, 0);
            }
            break;
        case 's': /* DECSC -- Save cursor position (ANSI.SYS) */
            tcursor(CURSOR_SAVE);
            break;
        case 'u': /* DECRC -- Restore cursor position (ANSI.SYS) */
            tcursor(CURSOR_LOAD);
            break;
    }
}

void csidump(void) {
    int i;
    uint c;

    printf("ESC[");
    for (i = 0; i < csiescseq.len; i++) {
        c = csiescseq.buf[i] & 0xff;
        if (isprint(c)) {
            putchar(c);
        } else if (c == '\n') {
            printf("(\\n)");
        } else if (c == '\r') {
            printf("(\\r)");
        } else if (c == 0x1b) {
            printf("(\\e)");
        } else {
            printf("(%02x)", c);
        }
    }
    putchar('\n');
}

void csireset(void) { memset(&csiescseq, 0, sizeof(csiescseq)); }

void strreset(void) { memset(&strescseq, 0, sizeof(strescseq)); }

void tputtab(bool forward) {
    uint x = term.c.x;

    if (forward) {
        if (x == term.col) return;
        for (++x; x < term.col && !term.tabs[x]; ++x) /* nothing */
            ;
    } else {
        if (x == 0) return;
        for (--x; x > 0 && !term.tabs[x]; --x) /* nothing */
            ;
    }
    tmoveto(x, term.c.y);
}

void tputc(char *c, int len) {
    uchar ascii = *c;
    bool control = ascii < '\x20' || ascii == 0177;

    if (iofd != -1) {
        if (xwrite(iofd, c, len) < 0) {
            fprintf(stderr, "Error writting in %s:%s\n", opt_io, strerror(errno));
            close(iofd);
            iofd = -1;
        }
    }
    /*
     * STR sequences must be checked before of anything
     * because it can use some control codes as part of the sequence
     */
    if (term.esc & ESC_STR) {
        switch (ascii) {
            case '\033':
                term.esc = ESC_START | ESC_STR_END;
                break;
            case '\a': /* backwards compatibility to xterm */
                term.esc = 0;
                break;
            default:
                strescseq.buf[strescseq.len++] = ascii;
                if (strescseq.len + 1 >= STR_BUF_SIZ) {
                    term.esc = 0;
                }
        }
        return;
    }
    /*
     * Actions of control codes must be performed as soon they arrive
     * because they can be embedded inside a control sequence, and
     * they must not cause conflicts with sequences.
     */
    if (control) {
        switch (ascii) {
            case '\t': /* HT */
                tputtab(1);
                return;
            case '\b': /* BS */
                tmoveto(term.c.x - 1, term.c.y);
                return;
            case '\r': /* CR */
                tmoveto(0, term.c.y);
                return;
            case '\f': /* LF */
            case '\v': /* VT */
            case '\n': /* LF */
                /* go to first col if the mode is set */
                tnewline(IS_SET(MODE_CRLF));
                return;
            case '\a': /* BEL */
                return;
            case '\033': /* ESC */
                csireset();
                term.esc = ESC_START;
                return;
            case '\016': /* SO */
                term.c.attr.mode |= ATTR_GFX;
                return;
            case '\017': /* SI */
                term.c.attr.mode &= ~ATTR_GFX;
                return;
            case '\032': /* SUB */
            case '\030': /* CAN */
                csireset();
                return;
            case '\005': /* ENQ (IGNORED) */
            case '\000': /* NUL (IGNORED) */
            case '\021': /* XON (IGNORED) */
            case '\023': /* XOFF (IGNORED) */
            case 0177:   /* DEL (IGNORED) */
                return;
        }
    } else if (term.esc & ESC_START) {
        if (term.esc & ESC_CSI) {
            csiescseq.buf[csiescseq.len++] = ascii;
            if (BETWEEN(ascii, 0x40, 0x7E) || csiescseq.len >= ESC_BUF_SIZ) {
                term.esc = 0;
                csiparse(), csihandle();
            }
        } else if (term.esc & ESC_STR_END) {
            term.esc = 0;
        } else if (term.esc & ESC_ALTCHARSET) {
            switch (ascii) {
                case '0': /* Line drawing set */
                    term.c.attr.mode |= ATTR_GFX;
                    break;
                case 'B': /* USASCII */
                    term.c.attr.mode &= ~ATTR_GFX;
                    break;
                case 'A': /* UK (IGNORED) */
                case '<': /* multinational charset (IGNORED) */
                case '5': /* Finnish (IGNORED) */
                case 'C': /* Finnish (IGNORED) */
                case 'K': /* German (IGNORED) */
                    break;
                default:
                    fprintf(stderr, "esc unhandled charset: ESC ( %c\n", ascii);
            }
            term.esc = 0;
        } else if (term.esc & ESC_TEST) {
            if (ascii == '8') { /* DEC screen alignment test. */
                char E[UTF_SIZ] = "E";
                int x, y;

                for (x = 0; x < term.col; ++x) {
                    for (y = 0; y < term.row; ++y) tsetchar(E, &term.c.attr, x, y);
                }
            }
            term.esc = 0;
        } else {
            switch (ascii) {
                case '[':
                    term.esc |= ESC_CSI;
                    break;
                case '#':
                    term.esc |= ESC_TEST;
                    break;
                case 'P': /* DCS -- Device Control String */
                case '_': /* APC -- Application Program Command */
                case '^': /* PM -- Privacy Message */
                case ']': /* OSC -- Operating System Command */
                case 'k': /* old title set compatibility */
                    strreset();
                    strescseq.type = ascii;
                    term.esc |= ESC_STR;
                    break;
                case '(': /* set primary charset G0 */
                    term.esc |= ESC_ALTCHARSET;
                    break;
                case ')': /* set secondary charset G1 (IGNORED) */
                case '*': /* set tertiary charset G2 (IGNORED) */
                case '+': /* set quaternary charset G3 (IGNORED) */
                    term.esc = 0;
                    break;
                case 'D': /* IND -- Linefeed */
                    if (term.c.y == term.bot) {
                        tscrollup(term.top, 1);
                    } else {
                        tmoveto(term.c.x, term.c.y + 1);
                    }
                    term.esc = 0;
                    break;
                case 'E':        /* NEL -- Next line */
                    tnewline(1); /* always go to first col */
                    term.esc = 0;
                    break;
                case 'H': /* HTS -- Horizontal tab stop */
                    term.tabs[term.c.x] = 1;
                    term.esc = 0;
                    break;
                case 'M': /* RI -- Reverse index */
                    if (term.c.y == term.top) {
                        tscrolldown(term.top, 1);
                    } else {
                        tmoveto(term.c.x, term.c.y - 1);
                    }
                    term.esc = 0;
                    break;
                case 'Z': /* DECID -- Identify Terminal */
                    ttywrite(VT102ID, sizeof(VT102ID) - 1);
                    term.esc = 0;
                    break;
                case 'c': /* RIS -- Reset to inital state */
                    treset();
                    term.esc = 0;
                    break;
                case '=': /* DECPAM -- Application keypad */
                    term.mode |= MODE_APPKEYPAD;
                    term.esc = 0;
                    break;
                case '>': /* DECPNM -- Normal keypad */
                    term.mode &= ~MODE_APPKEYPAD;
                    term.esc = 0;
                    break;
                case '7': /* DECSC -- Save Cursor */
                    tcursor(CURSOR_SAVE);
                    term.esc = 0;
                    break;
                case '8': /* DECRC -- Restore Cursor */
                    tcursor(CURSOR_LOAD);
                    term.esc = 0;
                    break;
                case '\\': /* ST -- Stop */
                    term.esc = 0;
                    break;
                default:
                    fprintf(stderr, "erresc: unknown sequence ESC 0x%02X '%c'\n", (uchar)ascii, isprint(ascii) ? ascii : '.');
                    term.esc = 0;
            }
        }
        /*
         * All characters which forms part of a sequence are not
         * printed
         */
        return;
    }
    /*
     * Display control codes only if we are in graphic mode
     */
    if (control && !(term.c.attr.mode & ATTR_GFX)) return;
    if (IS_SET(MODE_WRAP) && term.c.state & CURSOR_WRAPNEXT) tnewline(1); /* always go to first col */
    tsetchar(c, &term.c.attr, term.c.x, term.c.y);
    if (term.c.x + 1 < term.col)
        tmoveto(term.c.x + 1, term.c.y);
    else
        term.c.state |= CURSOR_WRAPNEXT;
}

int tresize(int col, int row) {
    int i, x;
    int minrow = MIN(row, term.row);
    int mincol = MIN(col, term.col);
    int slide = term.c.y - row + 1;
    bool *bp;

    if (col < 1 || row < 1) return 0;

    /* free unneeded rows */
    i = 0;
    if (slide > 0) {
        /* slide screen to keep cursor where we expect it -
         * tscrollup would work here, but we can optimize to
         * memmove because we're freeing the earlier lines */
        for (/* i = 0 */; i < slide; i++) {
            free(term.line[i]);
            free(term.alt[i]);
        }
        memmove(term.line, term.line + slide, row * sizeof(Line));
        memmove(term.alt, term.alt + slide, row * sizeof(Line));
    }
    for (i += row; i < term.row; i++) {
        free(term.line[i]);
        free(term.alt[i]);
    }

    /* resize to new height */
    term.line = xrealloc(term.line, row * sizeof(Line));
    term.alt = xrealloc(term.alt, row * sizeof(Line));
    term.dirty = xrealloc(term.dirty, row * sizeof(*term.dirty));
    term.tabs = xrealloc(term.tabs, col * sizeof(*term.tabs));

    /* resize each row to new width, zero-pad if needed */
    for (i = 0; i < minrow; i++) {
        term.dirty[i] = 1;
        term.line[i] = xrealloc(term.line[i], col * sizeof(Glyph));
        term.alt[i] = xrealloc(term.alt[i], col * sizeof(Glyph));
        for (x = mincol; x < col; x++) {
            term.line[i][x].state = 0;
            term.alt[i][x].state = 0;
        }
    }

    /* allocate any new rows */
    for (/* i == minrow */; i < row; i++) {
        term.dirty[i] = 1;
        term.line[i] = xcalloc(col, sizeof(Glyph));
        term.alt[i] = xcalloc(col, sizeof(Glyph));
    }
    if (col > term.col) {
        bp = term.tabs + term.col;

        memset(bp, 0, sizeof(*term.tabs) * (col - term.col));
        while (--bp > term.tabs && !*bp) /* nothing */
            ;
        for (bp += tabspaces; bp < term.tabs + col; bp += tabspaces) *bp = 1;
    }
    /* update terminal size */
    term.col = col;
    term.row = row;
    /* make use of the LIMIT in tmoveto */
    tmoveto(term.c.x, term.c.y);
    /* reset scrolling region */
    tsetscroll(0, row - 1);

    return (slide > 0);
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

    {
        // SDL_Surface *text_surface;
        SDL_Rect r = {winx, winy, width, xw.ch};

        if (xw.win != NULL) SDL_FillRect(xw.win, &r, SDL_MapRGB(xw.win->format, bg->r, bg->g, bg->b));
        // draw_keyboard(xw.win);

        /*#ifdef USE_ANTIALIASING
                if(!(text_surface=TTF_RenderUTF8_Shaded(font,s,*fg, *bg))) {
        #else
                if(!(text_surface=TTF_RenderUTF8_Solid(font,s,*fg))) {
        #endif
                    printf("Could not TTF_RenderUTF8_Solid: %s\n", TTF_GetError());
                    exit(EXIT_FAILURE);
                } else {
                    SDL_BlitSurface(text_surface,NULL,xw.win,&r);
                    SDL_FreeSurface(text_surface);
                }*/
        int xs = r.x;
        if (xw.win != NULL) {
            // TODO: find a better way to draw cursor box y + 1
            int ys = r.y + 1;
            draw_string(xw.win, s, xs, ys, SDL_MapRGB(xw.win->format, fg->r, fg->g, fg->b));
        }

        /*while(*s) {
            draw_char(xw.win, *s, xs, r.y, SDL_MapRGB(xw.win->format, fg->r, fg->g, fg->b));
            xs += 6;
            s++;
        }*/

        if (base.mode & ATTR_UNDERLINE) {
            // r.y += TTF_FontAscent(font) + 1;
            r.y += xw.ch;
            r.h = 1;
            if (xw.win != NULL) SDL_FillRect(xw.win, &r, SDL_MapRGB(xw.win->format, fg->r, fg->g, fg->b));
        }
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
    xflip();
}

void drawregion(int x1, int y1, int x2, int y2) {
    int ic, ib, x, y, ox, sl;
    Glyph base, new;
    char buf[DRAW_BUF_SIZ];
    bool alt = IS_SET(MODE_ALTSCREEN);

    if (!(xw.state & WIN_VISIBLE)) return;

    for (y = y1; y < y2; y++) {
        if (!term.dirty[y]) continue;

        sdltermclear(0, y, term.col, y);
        term.dirty[y] = 0;
        base = term.line[y][0];
        ic = ib = ox = 0;
        for (x = x1; x < x2; x++) {
            new = term.line[y][x];
            if (ib > 0 && (!(new.state &GLYPH_SET) || ATTRCMP(base, new) || ib >= DRAW_BUF_SIZ - UTF_SIZ)) {
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
    int meta, shift, ctrl, synth, i;
    SDL_Keycode ksym = e->keysym.sym;

    if (IS_SET(MODE_KBDLOCK)) return;

    meta = e->keysym.mod & KMOD_ALT;
    shift = e->keysym.mod & KMOD_SHIFT;
    ctrl = e->keysym.mod & KMOD_CTRL;
    synth = e->keysym.mod & KMOD_SYNTHETIC;

    // printf("kpress: keysym=%d scancode=%d mod=%d\n", ksym, e->keysym.scancode, e->keysym.mod);
    /* 2. custom keys from config.h */
    if ((customkey = kmap(ksym, e->keysym.mod))) {
        printf("Custom key mapped: %s\n", customkey);
        ttywrite(customkey, strlen(customkey));
        /* 2. hardcoded (overrides X lookup) */
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
        // special volumeup/down/ powerkey handling
        if (e->keysym.scancode == 128) {
            printf("Volume Up key pressed\n");
        } else if (e->keysym.scancode == 129) {
            printf("Volume Down key pressed\n");
        } else if (e->keysym.scancode == 102) {
            printf("Power key pressed\n");
        }

        switch (ksym) {
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

        if (SDL_PushEvent(&event)) {
            // printf("Warning: unable to push tty update event. %s\n", SDL_GetError());
        }
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
                    if (handler[ev.type]) (handler[ev.type])(&ev);
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
                if (handler[ev.type]) (handler[ev.type])(&ev);
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

        xflip();        // redraw the screen
        SDL_Delay(33);  // ~30 FPS
    }

    sdlshutdown();
}

int main(int argc, char *argv[]) {
    setenv("SDL_NOMOUSE", "1", 1);

    xw.fw = xw.fh = xw.fx = xw.fy = 0;
    xw.isfixed = false;

    // char *high_res_env = getenv("HIGH_RES");
    // if (high_res_env) {
    //     high_res = 1;
    // }

    for (int i = 1; i < argc; i++) {
        switch (argv[i][0] != '-' || argv[i][2] ? -1 : argv[i][1]) {
            case 'c':
                if (++i < argc) opt_class = argv[i];
                break;
            case 'd':
                /* run commands from arguments with on screen keyboard */
                if (++i < argc) {
                    opt_cmd = &argv[i];
                    opt_cmd_size = argc - 2;
                    for (int j = 0; j < opt_cmd_size; j++) {
                        printf("Command to execute: %s\n", opt_cmd[j]);
                    }
                    show_help = 0;
                    active = 1;
                }
                goto run;
            case 'e':
                /* run commands from arguments without on screen keyboard */
                if (++i < argc) {
                    opt_cmd = &argv[i];
                    opt_cmd_size = argc - 2;
                    for (int j = 0; j < opt_cmd_size; j++) {
                        printf("Command to execute: %s\n", opt_cmd[j]);
                    }
                    show_help = 0;
                    active = 0;
                }
                goto run;
            case 'f':
                if (++i < argc) opt_font = argv[i];
                break;
            case 'o':
                if (++i < argc) opt_io = argv[i];
                break;
            case 't':
                if (++i < argc) opt_title = argv[i];
                break;
            case 'q':
                active = show_help = 0;
                break;
            case 'v':
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
