#ifndef __2048_H
#define __2048_H

#include "input.h"

extern volatile uint8_t button_pressed;

void init_grid(void);

void menu_2048(void);

/* Dessine la grille tab[][] et l'affiche (display_swap) */
void print_grid(void);

void table_shift(joy_mv_t dir);

#endif /* __2048_H */