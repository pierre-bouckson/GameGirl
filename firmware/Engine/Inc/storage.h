#ifndef __STORAGE_H
#define __STORAGE_H

#include <stdint.h>

/* Sauvegardes persistantes (réglages et records), toutes dans le secteur 11
 * de la flash (0x080E0000, 128 Ko). Ce secteur existe sur le STM32F429ZIT6
 * (2 Mo) comme sur le ZGT6 (1 Mo) : le même binaire marche sur les deux.
 * Le linker limite le programme aux secteurs 0 à 10 (896 Ko).
 *
 * Le secteur est un journal : chaque storage_set() ajoute un enregistrement
 * (clé, valeur) à la suite, sans effacer. Il n'est effacé (et compacté) que
 * quand il est plein, soit environ 16 000 écritures : seule cette écriture-là
 * bloque ~1 à 2 s.
 *
 * storage_get() renvoie STORAGE_EMPTY pour une clé jamais écrite, comme une
 * flash effacée. */

#define STORAGE_EMPTY 0xFFFFFFFFUL

typedef enum {
    STORAGE_EASY = 0,       /* mode facile : 1 = actif */
    STORAGE_MUTE,           /* son coupé : 1 = muet */
    STORAGE_BACKLIGHT,      /* luminosité 1..10 */
    STORAGE_BEST_FLAPPY,
    STORAGE_BEST_TETRIS,
    STORAGE_BEST_SNAKE,
    STORAGE_BEST_2048,
    STORAGE_NB_KEYS
} storage_key_t;

uint32_t storage_get(storage_key_t key);
void storage_set(storage_key_t key, uint32_t value);   /* ne fait rien si inchangé */

#endif /* __STORAGE_H */
