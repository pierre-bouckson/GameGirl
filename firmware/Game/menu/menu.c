#include "menu.h"
#include "puissance4.h"
#include "rocket.h"

#define COLOR_FRAME     RGB565(60, 70, 120)
#define COLOR_TEXT_DIM  RGB565(150, 150, 170)
#define COLOR_TILE_IDLE RGB565(70, 70, 90)

/* Dégradé du titre, une couleur par ligne de lettre */
static const uint16_t title_gradient[TITLE_LETTER_H] = {
    RGB565(255, 80, 160), RGB565(235, 80, 190), RGB565(210, 80, 215),
    RGB565(180, 80, 240), RGB565(150, 80, 255)};

/* Couleur de chaque jeu, dans l'ordre de game_id_t */
static const uint16_t tile_accent[TILE_COUNT] = {
    RGB565(80, 220, 100), RGB565(80, 200, 255),    /* Snake, Pong */
    RGB565(255, 160, 60), RGB565(255, 215, 60),    /* 2048, Puissance4 */
    RGB565(180, 120, 255)};                        /* Rocket */

/* Tuile sélectionnée : rangée 0..2 et colonne 0..1 (la 3e rangée n'a que Rocket) */
static uint8_t sel_row = 0;
static uint8_t sel_col = 0;

typedef struct
{
    char c;
    uint16_t fg;
    uint16_t bg;
} cell_t;

/* Case d'une icône : lettre majuscule = bloc plein, chiffre = case de 2048 */
static cell_t icon_cell(char c)
{
    switch (c)
    {
        case 'G': return (cell_t){' ', COLOR_BLACK, RGB565(80, 220, 100)};
        case 'W': return (cell_t){' ', COLOR_BLACK, COLOR_WHITE};
        case 'R': return (cell_t){' ', COLOR_BLACK, RGB565(230, 60, 60)};
        case 'Y': return (cell_t){' ', COLOR_BLACK, RGB565(255, 215, 60)};
        case 'B': return (cell_t){' ', COLOR_BLACK, RGB565(40, 90, 220)};
        case 'D': return (cell_t){' ', COLOR_BLACK, RGB565(110, 110, 130)};
        case '.': return (cell_t){' ', COLOR_BLACK, COLOR_BLACK};   /* trou de la grille */
        case '2': return (cell_t){c, COLOR_BLACK, RGB565(238, 228, 218)};
        case '4': return (cell_t){c, COLOR_BLACK, RGB565(237, 200, 80)};
        case '8': return (cell_t){c, COLOR_WHITE, RGB565(242, 140, 80)};
        case '1':
        case '6': return (cell_t){c, COLOR_WHITE, RGB565(245, 100, 80)};
        case ':': return (cell_t){c, COLOR_DARKGRAY, COLOR_BLACK};
        case 'o': return (cell_t){c, COLOR_CYAN, COLOR_BLACK};
        default:  return (cell_t){c, COLOR_WHITE, COLOR_BLACK};
    }
}

/* Case (row, col) dans une tuile : style selon bord / icône / nom et sélection */
static cell_t tile_cell(char c, uint8_t tile, uint8_t in_r, uint8_t in_c, uint8_t selected)
{
    uint16_t accent = tile_accent[tile];
    uint8_t border = (in_r == 0 || in_r == TILE_H - 1 || in_c == 0 || in_c == TILE_W - 1);

    if (border)
        return (cell_t){c, selected ? accent : COLOR_TILE_IDLE, COLOR_BLACK};

    if (in_r >= TILE_ICON_FIRST && in_r <= TILE_ICON_LAST)
        return icon_cell(c);

    if (in_r == TILE_NAME_ROW)
    {
        if (selected)
            return (cell_t){c, COLOR_BLACK, accent};   /* barre de sélection */
        return (cell_t){c, COLOR_TEXT_DIM, COLOR_BLACK};
    }

    return (cell_t){c, COLOR_WHITE, COLOR_BLACK};
}

/* Retourne la tuile qui contient (row, col) et la position dedans, ou -1 */
static int8_t find_tile(uint8_t row, uint8_t col, uint8_t *in_r, uint8_t *in_c)
{
    for (uint8_t t = 0; t < TILE_COUNT; t++)
    {
        if (row >= tile_pos[t].row && row < tile_pos[t].row + TILE_H &&
            col >= tile_pos[t].col && col < tile_pos[t].col + TILE_W)
        {
            *in_r = row - tile_pos[t].row;
            *in_c = col - tile_pos[t].col;
            return t;
        }
    }
    return -1;
}

static uint8_t selected_tile(void)
{
    uint8_t tile = sel_row * 2 + sel_col;
    return (tile < TILE_COUNT) ? tile : TILE_COUNT - 1;
}

void menu_move(joy_mv_t mv)
{
    /* Avec l'orientation du joystick, X = -1 descend d'une rangée, Y = 1 va à droite */
    uint8_t last_row = (TILE_COUNT - 1) / 2;

    if (mv.x == -1 && sel_row < last_row)
        sel_row++;
    if (mv.x == 1 && sel_row > 0)
        sel_row--;
    if (mv.y != 0)
        sel_col = (mv.y == 1) ? 1 : 0;
}

void print_menu(void)
{
    uint8_t sel = selected_tile();

    for (uint8_t row = 0; row < MENU_ROWS; row++)
    {
        for (uint8_t col = 0; col < MENU_COLS; col++)
        {
            char c = menu[row][col];
            cell_t cell = {c, COLOR_WHITE, COLOR_BLACK};
            uint8_t in_r, in_c;
            int8_t tile;

            if (row == 0 || row == MENU_ROWS - 1 || col == 0 || col == MENU_COLS - 1)
            {
                cell.fg = COLOR_FRAME;
            }
            else if (row >= TITLE_FIRST_ROW && row <= TITLE_LAST_ROW)
            {
                /* '#' = bloc plein ; la ligne vide entre GAME et GIRL n'a pas de '#' */
                if (c == '#')
                    cell = (cell_t){' ', COLOR_BLACK,
                                    title_gradient[(row - TITLE_FIRST_ROW) % (TITLE_LETTER_H + 1)]};
            }
            else if (row == HINT_ROW)
            {
                cell.fg = COLOR_TEXT_DIM;
            }
            else if ((tile = find_tile(row, col, &in_r, &in_c)) >= 0)
            {
                cell = tile_cell(c, tile, in_r, in_c, tile == sel);
            }

            display_draw_char(col * FONT_W, row * FONT_H, cell.c, cell.fg, cell.bg);
        }
    }
    display_swap();
}

void start_game(void)
{
    switch (selected_tile())
    {
        case GAME_PUISSANCE4: menu_puissance4(); break;
        case GAME_ROCKET:     menu_rocket();     break;
        default:              break;   /* pas encore de jeu */
    }
}
