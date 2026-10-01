#ifndef __P4_JEUX_H
#define __P4_JEUX_H

#include <stdint.h>
#include <stdbool.h>

#define P4_ROWS 6
#define P4_COLS 7

/* Contenu d'une case : 0 = vide, 1 = joueur 1 ('O'), 2 = joueur 2 ou bot ('X') */
#define P4_VIDE 0
#define P4_J1   1
#define P4_J2   2

typedef struct
{
    uint8_t buffer[P4_ROWS][P4_COLS];   /* ligne 0 = haut de la grille */
} p4_jeux_t;

void p4_jeux_init(p4_jeux_t *jeu);

/* Fait tomber un pion de id dans la colonne x. Retourne la ligne, ou -1 si pleine */
int p4_jeux_update_buffer(p4_jeux_t *jeu, int x, uint8_t id);

bool p4_jeux_check_full(const p4_jeux_t *jeu, int x);
bool p4_jeux_grille_pleine(const p4_jeux_t *jeu);

/* 1 si le joueur id a aligné 4 pions (horizontal, vertical ou diagonal) */
bool p4_jeux_check_victory(const p4_jeux_t *jeu, uint8_t id);

#endif /* __P4_JEUX_H */
