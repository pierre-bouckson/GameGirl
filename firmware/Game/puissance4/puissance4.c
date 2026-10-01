#include <stdlib.h>
#include "main.h"
#include "display.h"
#include "puissance4.h"
#include "p4_jeux.h"
#include "p4_ia.h"
#include "p4_ihm.h"
#include "p4_grille.h"

#define P4_COL_DEPART 3     /* colonne de la flèche au début de chaque tour */
#define P4_BOT_DELAY  400   /* ms avant que le bot joue, pour voir le coup du joueur */

static int print_game(void);
static void ia_easy_play(void);
static void player_2_play(void);

void menu_puissance4(void)
{
    p4_ihm_init();
    /* Le moment de l'appui sur le bouton sert de graine aléatoire pour le bot */
    srand(HAL_GetTick());

    if (print_game() == 0)
        ia_easy_play();
    else
        player_2_play();
}

/* Choix du mode : 0 = Bot, 1 = 2 player */
static int print_game(void)
{
    int select = 0;

    while (1)
    {
        p4_grille_afficher_menu(select);

        int touche = p4_ihm_choose_line();
        if (touche == P4_SELECT)
            return select;
        select = (touche < 0) ? 0 : 1;
    }
}

/* Le joueur id déplace la flèche puis valide une colonne non pleine, qui est jouée */
static void tour_joueur(p4_jeux_t *jeu, uint8_t id)
{
    char cara = (id == P4_J1) ? 'O' : 'X';
    int choix = P4_COL_DEPART;

    while (1)
    {
        p4_grille_afficher_jeu(jeu, choix, cara, NULL, 0);

        int touche = p4_ihm_choose_line();
        if (touche == P4_SELECT)
        {
            if (!p4_jeux_check_full(jeu, choix))
                break;
            continue;
        }

        choix += touche;
        if (choix < 0)
            choix = 0;
        if (choix > P4_COLS - 1)
            choix = P4_COLS - 1;
    }
    p4_jeux_update_buffer(jeu, choix, id);
}

/* Affiche le résultat si la partie est finie, attend le bouton et retourne 1 */
static int fin_partie(const p4_jeux_t *jeu, uint8_t id, const char *gagnant)
{
    const char *message;
    uint16_t color;

    if (p4_jeux_check_victory(jeu, id))
    {
        message = gagnant;
        color = (id == P4_J1) ? P4_COLOR_O : P4_COLOR_X;
    }
    else if (p4_jeux_grille_pleine(jeu))
    {
        message = "Egalite";
        color = COLOR_WHITE;
    }
    else
    {
        return 0;
    }

    p4_grille_afficher_jeu(jeu, -1, ' ', message, color);
    p4_ihm_wait_button();
    return 1;
}

static void ia_easy_play(void)
{
    p4_jeux_t jeu;
    p4_jeux_init(&jeu);

    while (1)
    {
        tour_joueur(&jeu, P4_J1);
        if (fin_partie(&jeu, P4_J1, "Player 1 WIN"))
            return;

        p4_grille_afficher_jeu(&jeu, -1, ' ', NULL, 0);
        HAL_Delay(P4_BOT_DELAY);

        p4_jeux_update_buffer(&jeu, p4_ia_check_V_in_1(&jeu), P4_J2);
        if (fin_partie(&jeu, P4_J2, "Bot WIN"))
            return;
    }
}

static void player_2_play(void)
{
    p4_jeux_t jeu;
    p4_jeux_init(&jeu);

    while (1)
    {
        tour_joueur(&jeu, P4_J1);
        if (fin_partie(&jeu, P4_J1, "Player 1 WIN"))
            return;

        tour_joueur(&jeu, P4_J2);
        if (fin_partie(&jeu, P4_J2, "Player 2 WIN"))
            return;
    }
}
