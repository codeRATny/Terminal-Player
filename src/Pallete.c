#include "Pallete.h"
#include <stdlib.h>

struct kdtree *kd = NULL;

void pallete_init()
{
    kd = kd_create(3);

    for (int i = 0; i < sizeof(colors) / sizeof(RGB); i++)
    {
        int *value = malloc(sizeof(int));
        *value = colors[i].num;
        kd_insert3f(kd, colors[i].r, colors[i].g, colors[i].b, value);
    }
}

void pallete_deinit()
{
    kd_free(kd);
}

int search_nearest_color(float r, float g, float b)
{
    struct kdres *set = NULL;
    int* data = NULL;
    int result;
    double range = 5.0;

    while (1)
    {
        set = kd_nearest_range3f(kd, r, g, b, range);
        data = kd_res_item_data(set);

        if (data)
        {
            result = *data;
            kd_res_free(set);
            break;
        }

        kd_res_free(set);
        range += 5.0;
    }
    
    // set = kd_nearest_range3(kd, r, g, b, 50);

    // int* data = kd_res_item_data(set);
    return result;
}