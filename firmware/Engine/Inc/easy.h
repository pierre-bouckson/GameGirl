#ifndef __EASY_H
#define __EASY_H

#include <stdbool.h>
#include "display.h"

/* Vert du badge " FACILE " (même vert que Snake et la batterie pleine) */
#define EASY_COLOR RGB565(80, 220, 100)

/* Mode facile (enfants) : chaque jeu le lit au lancement avec easy_mode() et
 * se simplifie (plus lent, éléments plus gros, pas de mort aux bords...).
 * Les records ne sont ni comparés ni sauvegardés en mode facile, pour ne pas
 * mélanger les scores des deux modes.
 *
 * easy_init() relit l'état sauvegardé en flash (storage.c), easy_save() l'y
 * écrit seulement s'il a changé. */
void easy_init(void);
void easy_set(bool on);
bool easy_mode(void);
void easy_save(void);

#endif /* __EASY_H */
