#ifndef __ROCKET_H
#define __ROCKET_H

#include <stdint.h>

#define ROCKET_TRACK_LEN 1024   /* nombre de lignes du circuit */

/* Pour chaque ligne du circuit : colonne du bord gauche et du bord droit */
extern const uint8_t rocket_edges[ROCKET_TRACK_LEN][2];

/* Lance Rocket Space. Bloquant : retourne au menu GameGirl après le
 * GAME OVER, quand le bouton est appuyé. */
void menu_rocket(void);

#endif /* __ROCKET_H */
