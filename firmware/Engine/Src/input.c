#include <stdlib.h>
#include "input.h"

joy_mv_t get_joy(uint16_t x, uint16_t y)
{
    (void)x;
    (void)y;
    joy_mv_t commande = {0};
    return commande; /* TODO */
}

joy_mv_t selection_joy(uint16_t x, uint16_t y)
{
    int value_x = x - 2048;
    int value_y = y - 2048;
    joy_mv_t commande = {0};

    if(abs(value_x) < 500 && abs(value_y) < 500) return commande;

    if(abs(value_x) > abs(value_y) * 2)
    {
        commande = (joy_mv_t){abs(value_x) / value_x, 0};
        return commande;
    }
    if(abs(value_y) > abs(value_x) * 3)
    {
        commande = (joy_mv_t){0, abs(value_y) / value_y};
        return commande;
    }
    return commande;
}