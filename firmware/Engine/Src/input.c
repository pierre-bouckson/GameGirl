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
    int value_x = x - JOY_CENTER;
    int value_y = y - JOY_CENTER;
    joy_mv_t commande = {0};

    if(abs(value_x) < 500 && abs(value_y) < 500) return commande;

    if(abs(value_x) > abs(value_y) * 2)
    {
        commande = (joy_mv_t){abs(value_x) / value_x, 0};
        return commande;
    }
    if(abs(value_y) > abs(value_x) * 2)
    {
        commande = (joy_mv_t){0, abs(value_y) / value_y};
        return commande;
    }
    return commande;
}

uint16_t joy_expo(uint16_t raw)
{
    /* Demi-course de chaque côté du centre : 2048 en dessous, 2047 au-dessus */
    float half = (raw < JOY_CENTER) ? JOY_CENTER : (JOY_MAX - JOY_CENTER);
    float n = ((float)raw - JOY_CENTER) / half;   /* -1..1 */

    if (n > 1.0f)
        n = 1.0f;

    /* Mélange linéaire + cube : pente (1 - JOY_EXPO) au centre, 1 aux extrêmes */
    n = (1.0f - JOY_EXPO) * n + JOY_EXPO * n * n * n;

    return (uint16_t)(JOY_CENTER + n * half + 0.5f);
}