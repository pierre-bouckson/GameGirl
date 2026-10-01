#include <string.h>
#include "p4_jeux.h"

void p4_jeux_init(p4_jeux_t *jeu)
{
    memset(jeu->buffer, P4_VIDE, sizeof(jeu->buffer));
}

int p4_jeux_update_buffer(p4_jeux_t *jeu, int x, uint8_t id)
{
    for (int i = P4_ROWS - 1; i >= 0; i--)
    {
        if (jeu->buffer[i][x] == P4_VIDE)
        {
            jeu->buffer[i][x] = id;
            return i;
        }
    }
    return -1;
}

bool p4_jeux_check_full(const p4_jeux_t *jeu, int x)
{
    return jeu->buffer[0][x] != P4_VIDE;
}

bool p4_jeux_grille_pleine(const p4_jeux_t *jeu)
{
    for (int x = 0; x < P4_COLS; x++)
        if (!p4_jeux_check_full(jeu, x))
            return false;
    return true;
}

bool p4_jeux_check_victory(const p4_jeux_t *jeu, uint8_t id)
{
    /* Horizontal, vertical, diagonale '\', diagonale '/' */
    static const int8_t dir[4][2] = {{0, 1}, {1, 0}, {1, 1}, {-1, 1}};

    for (int i = 0; i < P4_ROWS; i++)
    {
        for (int j = 0; j < P4_COLS; j++)
        {
            if (jeu->buffer[i][j] != id)
                continue;

            for (int d = 0; d < 4; d++)
            {
                int k = 1;
                for (; k < 4; k++)
                {
                    int r = i + dir[d][0] * k;
                    int c = j + dir[d][1] * k;
                    if (r < 0 || r >= P4_ROWS || c >= P4_COLS || jeu->buffer[r][c] != id)
                        break;
                }
                if (k == 4)
                    return true;
            }
        }
    }
    return false;
}
