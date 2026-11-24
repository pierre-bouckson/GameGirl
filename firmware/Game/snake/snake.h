#ifndef __SNAKE_H
#define __SNAKE_H

#include "main.h"

#define PERIOD_MS       300   /* période de départ (ms par pas) */
#define PERIOD_MIN_MS   150   /* plancher : reste jouable au joystick */
#define PERIOD_ACCEL    97    /* chaque bonus mangé : period *= 97 % */

/* Mode facile : grille 15x15 aux cases deux fois plus grosses, serpent plus
 * lent qui accélère à peine, et il traverse les murs au lieu de mourir */
#define SNAKE_EASY_GRID          15
#define SNAKE_EASY_PERIOD_MS     450
#define SNAKE_EASY_PERIOD_MIN_MS 320
#define SNAKE_EASY_PERIOD_ACCEL  99

void game(void);

/* Dessine la grille tab[][] et l'affiche (display_swap) */
void print_game(void);

extern volatile uint8_t button_pressed;

#endif /* __SNAKE_H */