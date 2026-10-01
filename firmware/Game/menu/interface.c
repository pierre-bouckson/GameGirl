#include "interface.h"

/* Mise en page du menu, 1 caractère = 1 case de 8x8 px.
 * menu.c colorie chaque case selon sa zone (voir interface.h) :
 *  - titre : '#' = bloc plein, en dégradé
 *  - icônes des tuiles : G/W/R/Y/B/D = bloc vert/blanc/rouge/jaune/bleu/gris, '.' = trou,
 *    chiffres = cases de 2048 colorées, ':' = filet, 'o' = balle */
const char menu[MENU_ROWS][MENU_COLS + 1] =
{
    {"+============================+"},   /*  0 */
    {"|                            |"},   /*  1 */
    {"|   ###   ###  #   # #####   |"},   /*  2 */
    {"|  #     #   # ## ## #       |"},   /*  3 */
    {"|  # ### ##### # # # ####    |"},   /*  4 */
    {"|  #   # #   # #   # #       |"},   /*  5 */
    {"|   ###  #   # #   # #####   |"},   /*  6 */
    {"|                            |"},   /*  7 */
    {"|   ###  ##### ####  #       |"},   /*  8 */
    {"|  #       #   #   # #       |"},   /*  9 */
    {"|  # ###   #   ####  #       |"},   /* 10 */
    {"|  #   #   #   #  #  #       |"},   /* 11 */
    {"|   ###  ##### #   # #####   |"},   /* 12 */
    {"|                            |"},   /* 13 */
    {"|      SELECT YOUR GAME      |"},   /* 14 */
    {"|                            |"},   /* 15 */
    {"| +----------+  +----------+ |"},   /* 16 */
    {"| | R   GW   |  | W  :     | |"},   /* 17 */
    {"| |     G    |  | W  : o W | |"},   /* 18 */
    {"| |  GGGG    |  |    :   W | |"},   /* 19 */
    {"| |  Snake   |  |   Pong   | |"},   /* 20 */
    {"| +----------+  +----------+ |"},   /* 21 */
    {"|                            |"},   /* 22 */
    {"| +----------+  +----------+ |"},   /* 23 */
    {"| |   2  4   |  | B.B.BYB.B| |"},   /* 24 */
    {"| |          |  | B.BRBRB.B| |"},   /* 25 */
    {"| |   8  16  |  | BYBRBYBRB| |"},   /* 26 */
    {"| |   2048   |  |Puissance4| |"},   /* 27 */
    {"| +----------+  +----------+ |"},   /* 28 */
    {"|                            |"},   /* 29 */
    {"|        +----------+        |"},   /* 30 */
    {"|        |DD   R  DD|        |"},   /* 31 */
    {"|        |D   RWR  D|        |"},   /* 32 */
    {"|        |DD   Y  DD|        |"},   /* 33 */
    {"|        |  Rocket  |        |"},   /* 34 */
    {"|        +----------+        |"},   /* 35 */
    {"|                            |"},   /* 36 */
    {"|   < JOYSTICK : CHOISIR >   |"},   /* 37 */
    {"|                            |"},   /* 38 */
    {"+============================+"},   /* 39 */
};

/* Coin haut-gauche de chaque tuile, dans l'ordre de game_id_t */
const tile_pos_t tile_pos[TILE_COUNT] =
{
    {16, 2}, {16, 16},   /* Snake, Pong */
    {23, 2}, {23, 16},   /* 2048, Puissance4 */
    {30, 9},             /* Rocket, centrée */
};
