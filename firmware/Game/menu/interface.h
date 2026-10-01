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

/* Tuiles des jeux : grille 2 x 2 de cadres 12 x 9 cases */
#define TILE_GRID_ROWS 2
#define TILE_GRID_COLS 2
#define TILE_W         12
#define TILE_H         9
#define TILE_ROW0      17    /* ligne du haut de la première tuile */
#define TILE_COL0      2     /* colonne de gauche de la première tuile */
#define TILE_STEP_ROW  10    /* TILE_H + 1 ligne d'écart */
#define TILE_STEP_COL  14    /* TILE_W + 2 colonnes d'écart */
#define TILE_ICON_FIRST 2    /* lignes de l'icône, relatives à la tuile */
#define TILE_ICON_LAST  4
#define TILE_NAME_ROW   6    /* ligne du nom, relative à la tuile */

/* +1 pour le '\0' de fin de chaque ligne */
extern const char menu[MENU_ROWS][MENU_COLS + 1];

#endif /* __INTERFACE_H */
