#include <stdlib.h>
#include "p4_ia.h"

/* Colonne où id gagne en 1 coup, ou -1. Le pion est posé puis retiré. */
static int coup_gagnant(p4_jeux_t *jeu, uint8_t id)
{
    for (int k = 0; k < P4_COLS; k++)
    {
        int ligne = p4_jeux_update_buffer(jeu, k, id);
        if (ligne < 0)
            continue;

        bool gagne = p4_jeux_check_victory(jeu, id);
        jeu->buffer[ligne][k] = P4_VIDE;
        if (gagne)
            return k;
    }
    return -1;
}

int p4_ia_check_V_in_1(p4_jeux_t *jeu)
{
    int k = coup_gagnant(jeu, P4_J2);
    if (k >= 0)
        return k;

    k = coup_gagnant(jeu, P4_J1);
    if (k >= 0)
        return k;

    /* La grille n'est pas pleine quand le bot joue, la boucle se termine */
    do
        k = rand() % P4_COLS;
    while (p4_jeux_check_full(jeu, k));
    return k;
}
