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

/* Mode facile : ouverture bien plus haute, oiseau plus léger, tuyaux plus
 * lents et plus espacés qui n'accélèrent pas, et le sol fait rebondir */
#define FLAPPY_EASY_GRAVITY      0.20f
#define FLAPPY_EASY_FLAP        -3.8f
#define FLAPPY_EASY_MAX_FALL     4.0f
#define FLAPPY_EASY_SPEED        1.4f
#define FLAPPY_EASY_GAP          120
#define FLAPPY_EASY_SPACING      150
#define FLAPPY_EASY_MARGIN       4       /* boîte de collision réduite de 4 px de chaque côté */

/* Écran d'attente puis partie ; bloque jusqu'au retour au menu */
void menu_flappy(void);

extern volatile uint8_t button_pressed;

#endif /* __FLAPPY_H */
