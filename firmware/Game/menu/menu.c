#include "menu.h"

void print_menu(joy_mv_t select)
{
    for (uint8_t row = 0; row < MENU_ROWS; row++)
    {
        if(row == CHOICE_LINE_1)
        {
            int i = 0;
            char cara = menu[row][i];
            while(i < MENU_COLS && cara != 'S' && select.y == -1){
                display_draw_char(i * FONT_W, row * FONT_H, cara, COLOR_WHITE, COLOR_BLACK);
                i++;
                cara = menu[row][i];
            }
            while(i < MENU_COLS && cara != 'P' && select.y == 1){
                display_draw_char(i * FONT_W, row * FONT_H, cara, COLOR_WHITE, COLOR_BLACK);
                i++;
                cara = menu[row][i];
            }
            while(i < MENU_COLS && cara != ' ' && select.x == 1){
                display_draw_char(i * FONT_W, row * FONT_H, cara, COLOR_BLACK, COLOR_WHITE);
                i++;
                cara = menu[row][i];
            }
            while(i < MENU_COLS){
                display_draw_char(i * FONT_W, row * FONT_H, cara, COLOR_WHITE, COLOR_BLACK);
                i++;
                cara = menu[row][i];
            }
            continue;

        }
        if(row == CHOICE_LINE_2)
        {
            int i = 0;
            char cara = menu[row][i];
            while(i < MENU_COLS && cara != '2' && select.y == -1){
                display_draw_char(i * FONT_W, row * FONT_H, cara, COLOR_WHITE, COLOR_BLACK);
                i++;
                cara = menu[row][i];
            }
            while(i < MENU_COLS && cara != 'P' && select.y == 1){
                display_draw_char(i * FONT_W, row * FONT_H, cara, COLOR_WHITE, COLOR_BLACK);
                i++;
                cara = menu[row][i];
            }
            while(i < MENU_COLS && cara != ' ' && select.x == -1){
                display_draw_char(i * FONT_W, row * FONT_H, cara, COLOR_BLACK, COLOR_WHITE);
                i++;
                cara = menu[row][i];
            }
            while(i < MENU_COLS){
                display_draw_char(i * FONT_W, row * FONT_H, cara, COLOR_WHITE, COLOR_BLACK);
                i++;
                cara = menu[row][i];
            }
            continue;
        }
        display_draw_string(0, row * FONT_H, menu[row], COLOR_WHITE, COLOR_BLACK);
    }
    display_swap();
}

int select_game(int joy_mv)
{
    (void)joy_mv;
    return 0; /* TODO */
}
