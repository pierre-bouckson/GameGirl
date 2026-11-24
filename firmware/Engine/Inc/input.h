#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"   /* adc_values[], ADC_IDX_JOY_X / ADC_IDX_JOY_Y */

#define JOY_CENTER 2048   /* valeur ADC 12 bits au repos */
#define JOY_MAX    4095   /* pleine échelle de l'ADC */

/* Seuil : écart au centre en dessous duquel le stick est considéré au repos.
 * À augmenter si le joystick déclenche tout seul. */
#define JOY_DEADZONE 500

/* Force de la courbe exponentielle : 0.0 = linéaire, 1.0 = cubique pure.
 * Plus elle est forte, moins le stick est sensible autour du centre. */
#define JOY_EXPO   0.6f

/* Sens des axes sur la GameGirl : 1 = valeur ADC retournée (JOY_MAX - brut).
 * L'axe Y (horizontal à l'écran, 1 = droite) est câblé à l'envers. */
#define JOY_INVERT_X 0
#define JOY_INVERT_Y 1

/* Valeurs brutes 0..4095 avec le bon sens : à utiliser au lieu de adc_values[] */
static inline uint16_t joy_raw_x(void)
{
    uint16_t v = adc_values[ADC_IDX_JOY_X];
    return JOY_INVERT_X ? JOY_MAX - v : v;
}

static inline uint16_t joy_raw_y(void)
{
    uint16_t v = adc_values[ADC_IDX_JOY_Y];
    return JOY_INVERT_Y ? JOY_MAX - v : v;
}


typedef struct joy_mv
{
    int x;
    int y;
} joy_mv_t;

joy_mv_t get_joy(void);

joy_mv_t selection_joy();

bool new_selection();

/* Applique la courbe exponentielle à une valeur ADC brute (0..4095) : même
 * centre et mêmes extrêmes, mais plus de précision sur les petits mouvements. */
uint16_t joy_expo(uint16_t raw);

#endif /* __INPUT_H */