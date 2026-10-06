#ifndef __TETRIS_H
#define __TETRIS_H

#include <stdbool.h>
#include <stdint.h>
#include "main.h"

#define NB_SHAPES   7   /* nombre de tétrominos */
#define SHAPE_SIZE  4   /* chaque forme tient dans une grille 4x4 */
#define TETRIS_PERIOD_MS 500   /* ms entre deux descentes, au niveau 0 */
#define TETRIS_FAST_MS   50    /* idem, stick tenu vers le bas */
#define TETRIS_PERIOD_MIN_MS 80   /* plancher de la période de chute */
#define TETRIS_ACCEL     85    /* chaque niveau : période *= 85 % */
#define LINES_PER_LEVEL  10    /* lignes à effacer pour passer un niveau */

typedef enum { SHAPE_I, SHAPE_O, SHAPE_T, SHAPE_S, SHAPE_Z, SHAPE_J, SHAPE_L } shape_id_t;

/* Formes des tétrominos (shape.c) : 1 = case pleine, 0 = vide */
extern const uint8_t shapes[NB_SHAPES][SHAPE_SIZE][SHAPE_SIZE];

#define BOARD_W     10  /* largeur du plateau (cases) */
#define BOARD_H     20  /* hauteur du plateau (cases) */

/* Lance une partie ; bloque jusqu'au retour au menu */
void menu_tetris(void);

/* Dessine le plateau tab[][] et l'affiche (display_swap) */
void print_tab(void);

extern volatile uint8_t button_pressed;

#endif /* __TETRIS_H */