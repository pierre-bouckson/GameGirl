#include "2048.h"
#include <stdlib.h>
#include "display.h"
#include "font8x8.h"
#include <stdio.h>
#include "main.h"
#include <string.h>
#include <stdbool.h>

static int tab[4][4] = {0};   /* static : main.c a déjà un tab global */

void init_grid()
{
    memset(tab, 0, sizeof tab);
}

void add_grid(int number)
{
    bool empty[4][4] = {{false}};
    int count = 0;
    for(int i = 0; i<4;i++)
    {
        for(int j = 0; j<4; j++)
        {
            if(tab[i][j]== 0) 
            {
                empty[i][j] = true;
                count++;
            }
        }
    }
    if(count == 0) return;   /* grille pleine : rand() % 0 planterait */
    int rand_number = rand() % count;


    int number_of_true = 0;

    for(int i = 0; i<4;i++)
    {
        for(int j = 0; j<4; j++)
        {
            if(empty[i][j]== true)
            {
                if(number_of_true == rand_number)
                {
                    tab[i][j] = number;
                    return;
                }
                number_of_true++;
            }
        }
    }
}

void table_shift(joy_mv_t dir)
{
    for(int i = 0; i <4; i++)
    {

    }
}

void new_number()
{
    int rand_number = (rand() % 2) ? 4 : 2;
    printf("%d", rand_number);
    add_grid(rand_number);
}

void menu_2048()
{
    srand(HAL_GetTick());
    init_grid();
    new_number();
    print_grid();
    HAL_Delay(100);
    button_pressed = 0;   /* oublie l'appui qui a lancé le jeu */
    while(!button_pressed)
    {
        joy_mv_t mov = selection_joy(get_joy());
        if(new_selection(mov))
        {
            table_shift(mov);
            new_number();
            print_grid();
        }
    }

}


/* ---- Affichage ---------------------------------------------------------- */

/* Grille ASCII : 4 cases de 6 x 5 caractères + bordures = 29 x 25 caractères */
#define G_CELL_W   6
#define G_CELL_H   5
#define G_W        (4 * (G_CELL_W + 1) + 1)
#define G_H        (4 * (G_CELL_H + 1) + 1)
#define G_X0       ((LCD_W - G_W * FONT_W) / 2)   /* en pixels, grille centrée */
#define G_Y0       (6 * FONT_H)
#define G_BORDER   COLOR_GRAY
#define G_EMPTY    RGB565(40, 40, 40)

/* Couleur de fond d'une case selon sa valeur */
static uint16_t tile_color(int v)
{
    switch (v)
    {
        case 0:    return G_EMPTY;
        case 2:    return RGB565(238, 228, 218);
        case 4:    return RGB565(237, 224, 200);
        case 8:    return RGB565(242, 177, 121);
        case 16:   return RGB565(245, 149, 99);
        case 32:   return RGB565(246, 124, 95);
        case 64:   return RGB565(246, 94, 59);
        case 128:  return RGB565(237, 207, 114);
        case 256:  return RGB565(237, 204, 97);
        case 512:  return RGB565(237, 200, 80);
        case 1024: return RGB565(237, 197, 63);
        case 2048: return RGB565(237, 194, 46);
        default:   return COLOR_PURPLE;
    }
}

static void put_char_g(int col, int row, char c, uint16_t fg, uint16_t bg)
{
    display_draw_char(G_X0 + col * FONT_W, G_Y0 + row * FONT_H, c, fg, bg);
}

void print_grid(void)
{
    display_clear(COLOR_BLACK);

    /* Titre */
    display_draw_string((LCD_W - 8 * FONT_W) / 2, 2 * FONT_H, "= 2048 =", COLOR_YELLOW, COLOR_BLACK);

    for (int row = 0; row < G_H; row++)
    {
        for (int col = 0; col < G_W; col++)
        {
            int on_hline = (row % (G_CELL_H + 1)) == 0;
            int on_vline = (col % (G_CELL_W + 1)) == 0;

            if (on_hline && on_vline)
                put_char_g(col, row, '+', G_BORDER, COLOR_BLACK);
            else if (on_hline)
                put_char_g(col, row, '-', G_BORDER, COLOR_BLACK);
            else if (on_vline)
                put_char_g(col, row, '|', G_BORDER, COLOR_BLACK);
            else
            {
                int v = tab[row / (G_CELL_H + 1)][col / (G_CELL_W + 1)];
                put_char_g(col, row, ' ', COLOR_BLACK, tile_color(v));
            }
        }
    }

    /* Valeurs centrées dans chaque case */
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            int v = tab[i][j];
            if (v == 0)
                continue;

            char buf[8];
            int len = 0;
            for (int n = v; n > 0 && len < 7; n /= 10)
                buf[len++] = '0' + n % 10;

            uint16_t fg = (v <= 4) ? RGB565(119, 110, 101) : COLOR_WHITE;
            int col = j * (G_CELL_W + 1) + 1 + (G_CELL_W - len + 1) / 2;
            int row = i * (G_CELL_H + 1) + 1 + G_CELL_H / 2;
            for (int k = 0; k < len; k++)
                put_char_g(col + k, row, buf[len - 1 - k], fg, tile_color(v));
        }
    }

    display_swap();
}
