#ifndef __INTERFACE_H
#define __INTERFACE_H

#include "display.h"
#include "font8x8.h"

#define MENU_ROWS (LCD_H / FONT_H)   /* 40 lignes */
#define MENU_COLS (LCD_W / FONT_W)   /* 30 colonnes */

/* Zones de la grille menu[][] (en cases) */
#define TITLE_FIRST_ROW 2    /* "GAME" lignes 2-6, "GIRL" lignes 8-12 */
#define TITLE_LAST_ROW  12
#define TITLE_LETTER_H  5
#define SUBTITLE_ROW    14
#define HINT_ROW        37

/* Tuiles des jeux : cadres de 12 x 6 cases, positions dans tile_pos[] */
#define TILE_W          12
#define TILE_H          6
#define TILE_ICON_FIRST 1    /* lignes de l'icône, relatives à la tuile */
#define TILE_ICON_LAST  3
#define TILE_NAME_ROW   4    /* ligne du nom, relative à la tuile */

/* Une tuile par jeu, dans cet ordre : 2 par rangée, Rocket seule sur la 3e */
typedef enum
{
    GAME_SNAKE,
    GAME_PONG,
    GAME_2048,
    GAME_PUISSANCE4,
    GAME_ROCKET,
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
