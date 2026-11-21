#ifndef __FLAPPY_H
#define __FLAPPY_H

#include "main.h"

#define FLAPPY_FRAME_MS   20      /* une image toutes les 20 ms (50 Hz) */
#define FLAPPY_GRAVITY    0.30f   /* px par image², vers le bas */
#define FLAPPY_FLAP      -4.6f    /* vitesse donnée par un battement d'ailes (px par image) */
#define FLAPPY_MAX_FALL   6.0f    /* vitesse de chute max */
#define FLAPPY_SPEED      2.0f    /* vitesse de départ des tuyaux (px par image) */
#define FLAPPY_SPEED_UP   0.05f   /* chaque tuyau passé : vitesse += 0.05 */
#define FLAPPY_SPEED_MAX  3.2f
#define FLAPPY_GAP        76      /* hauteur de l'ouverture entre deux tuyaux (px) */
#define FLAPPY_SPACING    130     /* distance entre deux tuyaux (px) */

/* Écran d'attente puis partie ; bloque jusqu'au retour au menu */
void menu_flappy(void);

extern volatile uint8_t button_pressed;

#endif /* __FLAPPY_H */
