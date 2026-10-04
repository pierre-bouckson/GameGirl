#ifndef __SNAKE_H
#define __SNAKE_H

#include "main.h"

#define PERIOD_MS       300   /* période de départ (ms par pas) */
#define PERIOD_MIN_MS   150   /* plancher : reste jouable au joystick */
#define PERIOD_ACCEL    97    /* chaque bonus mangé : period *= 97 % */

void game();

/* Dessine la grille tab[][] et l'affiche (display_swap) */
void print_game(void);

extern volatile uint8_t button_pressed;

#endif /* __SNAKE_H */