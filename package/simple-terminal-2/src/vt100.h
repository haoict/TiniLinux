/*
 * VT100/ANSI escape sequence parser
 */

#ifndef VT100_H
#define VT100_H

#include "terminal.h"

// Parse VT100/ANSI escape sequences
int vt100_parse_escape(Terminal *term, const char *escape_seq, int len);

#endif // VT100_H