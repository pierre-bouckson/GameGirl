#include "menu.h"
#include "puissance4.h"
#include "rocket.h"
#include "2048.h"
#include "snake.h"
#include "pong.h"
#include "tetris.h"
#include "flappy.h"
#include "reglages.h"
#include "battery.h"
#include "backlight.h"
#include "buzzer.h"
#include "game_exit.h"
#include "easy.h"

extern volatile uint8_t button_pressed;   /* main.c */
#include <stdio.h>
#include <string.h>

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
    RGB565(180, 120, 255), RGB565(60, 220, 230),   /* Rocket, Tetris */
    RGB565(255, 120, 80), RGB565(200, 200, 215)};  /* Flappy, Réglages */

/* Texte de batterie affiché au dernier print_menu(), pour ne redessiner que s'il change */
static char battery_text[8] = "";

/* Appels de menu_tick() entre deux mesures de batterie (boucle du menu = 100 ms) */
#define BATTERY_TICKS 20

/* Tuile sélectionnée : rangée 0..3 et colonne 0..1 (la rangée 3 n'a qu'une tuile) */
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
        case 'C': return (cell_t){' ', COLOR_BLACK, RGB565(60, 220, 230)};
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

/* Texte (" 87% " ou " USB ") et couleur du pourcentage de batterie */
static uint16_t battery_label(char *text, size_t size)
{
    if (battery_on_usb())
    {
        snprintf(text, size, " USB ");
        return COLOR_CYAN;
    }

    uint8_t pct = battery_percent();
    snprintf(text, size, " %u%% ", pct);

    if (pct >= 50) return RGB565(80, 220, 100);
    if (pct >= 20) return COLOR_ORANGE;
    return COLOR_RED;
}

void menu_tick(void)
{
    static uint8_t ticks = 0;

    battery_mv();   /* nourrit la moyenne glissante à chaque tour */

    if (++ticks < BATTERY_TICKS)
        return;
    ticks = 0;

    char text[sizeof battery_text];
    battery_label(text, sizeof text);
    if (strcmp(text, battery_text) != 0)
        print_menu();
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

    /* Pourcentage de batterie en haut à droite, aligné sur le bord droit du cadre */
    uint16_t color = battery_label(battery_text, sizeof battery_text);
    uint16_t col = BATTERY_COL + 6 - strlen(battery_text);
    display_draw_string(col * FONT_W, BATTERY_ROW * FONT_H, battery_text, color, COLOR_BLACK);

    /* Mode facile : badge en haut à gauche, en face de la batterie */
    if (easy_mode())
        display_draw_string(EASY_COL * FONT_W, EASY_ROW * FONT_H, " FACILE ",
                            COLOR_BLACK, EASY_COLOR);

    display_swap();
}

void start_game(void)
{
    void (*entry)(void) = NULL;

    switch (selected_tile())
    {
        case GAME_PUISSANCE4: entry = menu_puissance4; break;
        case GAME_ROCKET:     entry = menu_rocket;     break;
        case GAME_2048:       entry = menu_2048;       break;
        case GAME_SNAKE:      entry = game;            break;
        case GAME_PONG:       entry = menu_pong;       break;
        case GAME_TETRIS:     entry = menu_tetris;     break;
        case GAME_FLAPPY:     entry = menu_flappy;     break;
        case GAME_REGLAGES:   entry = menu_reglages;   break;
        default:              return;   /* pas encore de jeu */
    }

    /* Revient à la fin du jeu, ou tout de suite sur appui de START */
    game_run(entry);

    buzzer_play(SND_BACK);
    backlight_save();     /* START depuis les réglages : sans effet si rien n'a changé */
    buzzer_save();
    easy_save();
    button_pressed = 0;   /* un appui fait pendant le jeu ne relance pas le jeu */
}
