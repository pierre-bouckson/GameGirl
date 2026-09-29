#include "menu.h"

void print_menu(void)
{
    for (uint8_t row = 0; row < MENU_ROWS; row++)
    {
        display_draw_string(0, row * FONT_H, menu[row], COLOR_WHITE, COLOR_BLACK);
    }
    display_swap();
}

int select_game(int joy_mv)
{
    
}
