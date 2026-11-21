#include "interface.h"

/* Mise en page du menu, 1 caractère = 1 case de 8x8 px.
 * menu.c colorie chaque case selon sa zone (voir interface.h) :
 *  - titre : '#' = bloc plein, en dégradé
 *  - icônes des tuiles : G/W/R/Y/B/D = bloc vert/blanc/rouge/jaune/bleu/gris, '.' = trou,
 *    C = bloc cyan, chiffres = cases de 2048 colorées, ':' = filet, 'o' = balle */
const char menu[MENU_ROWS][MENU_COLS + 1] =
{
    {"+============================+"},   /*  0 */
    {"|   ###   ###  #   # #####   |"},   /*  1 */
    {"|  #     #   # ## ## #       |"},   /*  2 */
    {"|  # ### ##### # # # ####    |"},   /*  3 */
    {"|  #   # #   # #   # #       |"},   /*  4 */
    {"|   ###  #   # #   # #####   |"},   /*  5 */
    {"|                            |"},   /*  6 */
    {"|   ###  ##### ####  #       |"},   /*  7 */
    {"|  #       #   #   # #       |"},   /*  8 */
    {"|  # ###   #   ####  #       |"},   /*  9 */
    {"|  #   #   #   #  #  #       |"},   /* 10 */
    {"|   ###  ##### #   # #####   |"},   /* 11 */
    {"|                            |"},   /* 12 */
    {"| +----------+  +----------+ |"},   /* 13 */
    {"| |R    GGGW |  | W  : o   | |"},   /* 14 */
    {"| |  GGGG    |  | W  :   W | |"},   /* 15 */
    {"| |  Snake   |  |   Pong   | |"},   /* 16 */
    {"| +----------+  +----------+ |"},   /* 17 */
    {"|                            |"},   /* 18 */
    {"| +----------+  +----------+ |"},   /* 19 */
    {"| |   2  4   |  | B.BRBRB.B| |"},   /* 20 */
    {"| |   8  16  |  | BYBRBYBRB| |"},   /* 21 */
    {"| |   2048   |  |Puissance4| |"},   /* 22 */
    {"| +----------+  +----------+ |"},   /* 23 */
    {"|                            |"},   /* 24 */
    {"| +----------+  +----------+ |"},   /* 25 */
    {"| |DD   R  DD|  |  R  YY  C| |"},   /* 26 */
    {"| |D   RWR  D|  | RRR YY  C| |"},   /* 27 */
    {"| |  Rocket  |  |  Tetris  | |"},   /* 28 */
    {"| +----------+  +----------+ |"},   /* 29 */
    {"|                            |"},   /* 30 */
    {"| +----------+  +----------+ |"},   /* 31 */
    {"| |GG   YWR  |  | Y.Y GGGD | |"},   /* 32 */
    {"| |    YY  GG|  |  Y  GGGD | |"},   /* 33 */
    {"| |  Flappy  |  | Reglages | |"},   /* 34 */
    {"| +----------+  +----------+ |"},   /* 35 */
    {"|                            |"},   /* 36 */
    {"|   < JOYSTICK : CHOISIR >   |"},   /* 37 */
    {"|                            |"},   /* 38 */
    {"+============================+"},   /* 39 */
};

/* Coin haut-gauche de chaque tuile, dans l'ordre de game_id_t */
const tile_pos_t tile_pos[TILE_COUNT] =
{
    {13, 2}, {13, 16},   /* Snake, Pong */
    {19, 2}, {19, 16},   /* 2048, Puissance4 */
    {25, 2}, {25, 16},   /* Rocket, Tetris */
    {31, 2}, {31, 16},   /* Flappy, Réglages */
};
