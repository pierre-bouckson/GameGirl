#include "menu.h"

#define COLOR_FRAME     RGB565(60, 70, 120)
#define COLOR_TEXT_DIM  RGB565(150, 150, 170)
#define COLOR_TILE_IDLE RGB565(70, 70, 90)

/* Dégradé du titre, une couleur par ligne de lettre */
static const uint16_t title_gradient[TITLE_LETTER_H] = {
    RGB565(255, 80, 160), RGB565(235, 80, 190), RGB565(210, 80, 215),
    RGB565(180, 80, 240), RGB565(150, 80, 255)};

/* Couleur de chaque jeu, dans le même ordre que les tuiles de interface.c */
static const uint16_t tile_accent[TILE_GRID_ROWS][TILE_GRID_COLS] = {
    {RGB565(80, 220, 100), RGB565(80, 200, 255)},    /* Snake, Pong */
    {RGB565(255, 160, 60), RGB565(255, 215, 60)}};   /* 2048, Puissance4 */

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
static cell_t tile_cell(char c, uint8_t tile_r, uint8_t tile_c, uint8_t in_r, uint8_t in_c, uint8_t selected)
{
    uint16_t accent = tile_accent[tile_r][tile_c];
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

/* Retourne 1 si (row, col) est dans une tuile, et donne la tuile et la position dedans */
static uint8_t find_tile(uint8_t row, uint8_t col, uint8_t *tile_r, uint8_t *tile_c, uint8_t *in_r, uint8_t *in_c)
{
    if (row < TILE_ROW0 || col < TILE_COL0)
        return 0;

    uint8_t r = (row - TILE_ROW0) / TILE_STEP_ROW;
    uint8_t c = (col - TILE_COL0) / TILE_STEP_COL;
    *in_r = (row - TILE_ROW0) % TILE_STEP_ROW;
    *in_c = (col - TILE_COL0) % TILE_STEP_COL;

    if (r >= TILE_GRID_ROWS || c >= TILE_GRID_COLS || *in_r >= TILE_H || *in_c >= TILE_W)
        return 0;

    *tile_r = r;
    *tile_c = c;
    return 1;
}

void print_menu(joy_mv_t select)
{
    /* Avec l'orientation du joystick, l'axe X choisit la ligne et l'axe Y la colonne */
    uint8_t sel_r = (select.x == -1) ? 1 : 0;
    uint8_t sel_c = (select.y == 1) ? 1 : 0;

    for (uint8_t row = 0; row < MENU_ROWS; row++)
    {
        for (uint8_t col = 0; col < MENU_COLS; col++)
        {
            char c = menu[row][col];
            cell_t cell = {c, COLOR_WHITE, COLOR_BLACK};
            uint8_t tile_r, tile_c, in_r, in_c;

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
            else if (find_tile(row, col, &tile_r, &tile_c, &in_r, &in_c))
            {
                cell = tile_cell(c, tile_r, tile_c, in_r, in_c, tile_r == sel_r && tile_c == sel_c);
            }

            display_draw_char(col * FONT_W, row * FONT_H, cell.c, cell.fg, cell.bg);
        }
    }
    display_swap();
}

int select_game(int joy_mv)
{
    (void)joy_mv;
    return 0; /* TODO */
}
