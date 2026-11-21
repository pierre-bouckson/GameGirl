#ifndef __P4_GRILLE_H
#define __P4_GRILLE_H

#include <stdint.h>
#include "display.h"
#include "p4_jeux.h"

#define P4_COLOR_O RGB565(230, 50, 50)    /* joueur 1 */
#define P4_COLOR_X RGB565(60, 120, 255)   /* joueur 2 / bot */

/* Écran de choix du mode : select = 0 (Bot) ou 1 (2 player) */
void p4_grille_afficher_menu(int select);

/* Écran de jeu : titre, flèche au-dessus de la colonne choix avec le pion cara
 * ('O' ou 'X', choix < 0 = pas de flèche), grille, aide, et message optionnel
 * (NULL = aucun). Dessine tout l'écran puis l'affiche (display_swap). */
void p4_grille_afficher_jeu(const p4_jeux_t *jeu, int choix, char cara,
                            const char *message, uint16_t message_color);

#endif /* __P4_GRILLE_H */
