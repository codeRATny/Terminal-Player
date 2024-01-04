#include "TerminalUtils.h"

static const char cursor_off_escape[] = "\e[?25l";
static const char cursor_on_escape[] = "\e[?25h";
static const char clear_escape[] = "\033[2J";
static const char set_x_y_escape[] = "\e[%d;%dH";

void terminal_cursor_off()
{
    write(STDOUT_FILENO, cursor_off_escape, sizeof(cursor_off_escape));
}

void terminal_cursor_on()
{
    write(STDOUT_FILENO, cursor_on_escape, sizeof(cursor_on_escape));
}

void terminal_clear()
{
    write(STDOUT_FILENO, clear_escape, sizeof(clear_escape));
}

void terminal_seek_coord(int x, int y)
{
    char buf[512];

    snprintf(buf, 512, set_x_y_escape, x, y);

    write(STDOUT_FILENO, buf, strlen(buf) + 1);
}

TerminalResolution terminal_resolution()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    TerminalResolution ret;

    ret.x = w.ws_col;
    ret.y = w.ws_row;

    return ret;
}

// 17 - max size per one pixel's escape sequence
TerminalCanvas *terminal_canvas_alloc()
{
    TerminalCanvas *canv = malloc(sizeof(TerminalCanvas));

    TerminalResolution resol = terminal_resolution();

    canv->size = 17 * resol.x * resol.y + 1;
    canv->data = malloc(canv->size);
    canv->pos = 0;

    return canv;
}

TerminalCanvas *terminal_canvas_alloc2(TerminalResolution resol)
{
    TerminalCanvas *canv = malloc(sizeof(TerminalCanvas));

    canv->size = 17 * resol.x * resol.y + 1;
    canv->data = malloc(canv->size);
    canv->pos = 0;

    return canv;
}

void terminal_canvas_reset(TerminalCanvas *canv)
{
    memset(canv->data, 0, canv->size);
    canv->pos = 0;
}

void terminal_canvas_free(TerminalCanvas **canv)
{
    free((*canv)->data);
    free(*canv);
}

void terminal_canvas_add_pixel(TerminalCanvas *canv, size_t pixel_idx)
{
    char buff[64];
    snprintf(buff, 64, "\e[48;5;%dm \e[0m", terminal_escapes[pixel_idx]);
    size_t len = strlen(buff);
    memcpy(canv->data + canv->pos, buff, len);
    canv->pos += len;
}

void terminal_canvas_fini(TerminalCanvas *canv)
{
    canv->data[canv->pos + 1] = '\0';
}