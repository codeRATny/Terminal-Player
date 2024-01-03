#ifndef _COLORS_H_
#define _COLORS_H_

#include <inttypes.h>
#include <string.h>

static const uint8_t terminal_escapes[] = {232, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 231};
static const uint8_t terminal_colors[]  = {  0,   8,  18,  28,  38,  48,  58,  68,  78,  88,  98, 108, 118, 128, 138, 148, 158, 168, 178, 188, 198, 208, 218, 228, 238, 255};

size_t find_closest_color(uint8_t color_value);

#endif