#ifndef __INTERFACE_H
#define __INTERFACE_H

#include "display.h"
#include "font8x8.h"

#define MENU_ROWS (LCD_H / FONT_H)   /* 40 lignes */
#define MENU_COLS (LCD_W / FONT_W)   /* 30 colonnes */

/* Zones de la grille menu[][] (en cases) */
#define TITLE_FIRST_ROW 1    /* "GAME" lignes 1-5, "GIRL" lignes 7-11 */
#define TITLE_LAST_ROW  11
#define TITLE_LETTER_H  5
#define HINT_ROW        37

/* Pourcentage de batterie, écrit par-dessus le cadre du haut (" 100% " = 6 cases) */
#define BATTERY_ROW     0
#define BATTERY_COL     22

/* Tuiles des jeux : cadres de 12 x 5 cases, positions dans tile_pos[] */
#define TILE_W          12
#define TILE_H          5
#define TILE_ICON_FIRST 1    /* lignes de l'icône, relatives à la tuile */
#define TILE_ICON_LAST  2
#define TILE_NAME_ROW   3    /* ligne du nom, relative à la tuile */

/* Une tuile par jeu (+ les réglages), dans cet ordre : 2 par rangée */
typedef enum
{
    GAME_SNAKE,
    GAME_PONG,
    GAME_2048,
    GAME_PUISSANCE4,
    GAME_ROCKET,
    GAME_TETRIS,
    GAME_FLAPPY,
    GAME_REGLAGES,   /* pas un jeu : luminosité et batterie */
    TILE_COUNT
} game_id_t;

typedef struct
{
    uint8_t row;   /* case du coin haut-gauche */
    uint8_t col;
} tile_pos_t;

extern const tile_pos_t tile_pos[TILE_COUNT];

/* +1 pour le '\0' de fin de chaque ligne */
extern const char menu[MENU_ROWS][MENU_COLS + 1];

#endif /* __INTERFACE_H */
