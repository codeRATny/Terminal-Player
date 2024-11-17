#include "Colors.h"

int get_closest(uint8_t val1, uint8_t val2, uint8_t color_value, size_t val1_idx, size_t val2_idx)
{
    if (color_value - val1 >= val2 - color_value)
        return val2_idx;
    else
        return val1_idx;
}

int n = sizeof(terminal_colors) / sizeof(terminal_colors[0]);

size_t find_closest_color(uint8_t color_value)
{
    if (color_value <= terminal_colors[0])
        return 0;

    if (color_value >= terminal_colors[n - 1])
        return n - 1;

    int i = 0, j = n, mid = 0;

    while (i < j)
    {
        mid = (i + j) / 2;

        if (terminal_colors[mid] == color_value)
            return mid;

        if (color_value < terminal_colors[mid])
        {

            if (mid > 0 && color_value > terminal_colors[mid - 1])
                return get_closest(terminal_colors[mid - 1], terminal_colors[mid], color_value, mid - 1, mid);

            j = mid;
        }

        else
        {
            if (mid < n - 1 && color_value < terminal_colors[mid + 1])
                return get_closest(terminal_colors[mid], terminal_colors[mid + 1], color_value, mid, mid + 1);
            i = mid + 1;
        }
    }

    return mid;
}