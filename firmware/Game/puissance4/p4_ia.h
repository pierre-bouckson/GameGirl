#ifndef __P4_IA_H
#define __P4_IA_H

#include "p4_jeux.h"

/* IA facile (bot = P4_J2) : gagne si possible en 1 coup, sinon bloque le
 * joueur s'il gagne en 1 coup, sinon joue une colonne non pleine au hasard. */
int p4_ia_check_V_in_1(p4_jeux_t *jeu);

#endif /* __P4_IA_H */
