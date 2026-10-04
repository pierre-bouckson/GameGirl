#include <stdlib.h>
#include "input.h"

static joy_mv_t last_selection = {0};

joy_mv_t get_joy()
{
    joy_mv_t joy = {joy_adc[0], joy_adc[1]};
    return joy;
}

joy_mv_t selection_joy()
{
    joy_mv_t joy = {joy_adc[0], joy_adc[1]};
    int value_x = joy.x - JOY_CENTER;
    int value_y = joy.y - JOY_CENTER;
    joy_mv_t commande = {0};

    if(abs(value_x) < JOY_DEADZONE && abs(value_y) < JOY_DEADZONE) return commande;

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

bool new_selection()
{
    bool new_select = false;
    /* Compare la direction (-1/0/1) et non les valeurs brutes : le bruit de
     * l'ADC reste dans la zone morte et ne change pas la direction */
    joy_mv_t joy = selection_joy();
    if (last_selection.y != joy.y && joy.y != 0) new_select = true;
    if (last_selection.x != joy.x && joy.x != 0) new_select = true;
    last_selection = joy;
    return new_select;
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