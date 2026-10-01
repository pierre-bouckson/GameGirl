#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

#define JOY_CENTER 2048   /* valeur ADC 12 bits au repos */
#define JOY_MAX    4095   /* pleine échelle de l'ADC */

/* Force de la courbe exponentielle : 0.0 = linéaire, 1.0 = cubique pure.
 * Plus elle est forte, moins le stick est sensible autour du centre. */
#define JOY_EXPO   0.6f

typedef struct joy_mv
{
    int x;
    int y;
} joy_mv_t;

joy_mv_t get_joy(uint16_t x, uint16_t y);

joy_mv_t selection_joy(uint16_t x, uint16_t y);

/* Applique la courbe exponentielle à une valeur ADC brute (0..4095) : même
 * centre et mêmes extrêmes, mais plus de précision sur les petits mouvements. */
uint16_t joy_expo(uint16_t raw);

#endif /* __INPUT_H */