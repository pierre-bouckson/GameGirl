#ifndef __PONG_H
#define __PONG_H

#include "main.h"

#define PONG_WIN_SCORE   5       /* premier à 5 points */
#define PONG_FRAME_MS    20      /* une image toutes les 20 ms (50 Hz) */
#define PONG_SPEED       3.0f    /* vitesse de départ de la balle (px par image) */
#define PONG_SPEED_MAX   7.0f    /* < hauteur d'une raquette : la balle ne la traverse pas */
#define PONG_ACCEL       1.05f   /* chaque renvoi : vitesse *= 1.05 */
#define PONG_MAX_ANGLE   60.0f   /* angle max par rapport à la verticale, au bord de la raquette */
#define PONG_PLAYER_SPD  5.0f    /* vitesse max de la raquette du joueur (px par image) */

/* Choix du niveau du bot puis partie ; bloque jusqu'au retour au menu */
void menu_pong(void);

extern volatile uint8_t button_pressed;

#endif /* __PONG_H */
