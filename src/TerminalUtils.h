#ifndef _TERMINAL_UTILS_H_
#define _TERMINAL_UTILS_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "Colors.h"

typedef struct TerminalResolution
{
    int x,y;

} TerminalResolution;

typedef struct TerminalCanvas
{
    char *data;
    size_t pos;
    size_t size;

} TerminalCanvas;

void terminal_cursor_off();
void terminal_cursor_on();
void terminal_clear();
TerminalResolution terminal_resolution();

TerminalCanvas *terminal_canvas_alloc();
TerminalCanvas *terminal_canvas_alloc2(TerminalResolution resol);
void terminal_canvas_add_pixel(TerminalCanvas *canv, size_t pixel_idx);
void terminal_canvas_fini(TerminalCanvas *canv);
void terminal_canvas_reset(TerminalCanvas *canv);
void terminal_canvas_free(TerminalCanvas **canv);

#endif