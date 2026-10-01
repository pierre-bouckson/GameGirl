#include "display.h"
#include "font8x8.h"
#include "p4_grille.h"

/* Même rendu que la version terminal : 1 caractère = 1 case de 8x8 px */
#define P4_TEXT_COLOR  COLOR_LIGHTGRAY
#define P4_ROW0        2    /* première ligne utilisée, pour centrer verticalement */

/* Ligne de l'écran de jeu (en cases) */
#define TITLE_ROW      P4_ROW0
#define ARROW_ROW      (TITLE_ROW + 8)
#define GRID_ROW       (ARROW_ROW + 3)
#define HELP_ROW       (GRID_ROW + 2 * P4_ROWS + 2)
#define MESSAGE_ROW    (HELP_ROW + 3)

static void put_str(int row, int col, const char *s, uint16_t fg, uint16_t bg)
{
    display_draw_string(col * FONT_W, row * FONT_H, s, fg, bg);
}

static void put_char(int row, int col, char c, uint16_t fg)
{
    display_draw_char(col * FONT_W, row * FONT_H, c, fg, COLOR_BLACK);
}

static uint16_t couleur_pion(char cara)
{
    return (cara == 'O') ? P4_COLOR_O : P4_COLOR_X;
}

void p4_grille_afficher_menu(int select)
{
    static const char *const menu[] = {
        "     ******************     ",
        "    ***              ***    ",
        "    *** PUISSANCE 4  ***    ",
        "    ***     by :     ***    ",
        "    *** Pierre       ***    ",
        "    ***     Bouckson ***    ",
        "   ****              ****   ",
        "  ****  Select Game   ****  ",
        "  ***                  ***  ",
        "  ***  Bot   2 player  ***  ",
        "   ***                ***   ",
        "    ********************    ",
    };
    const int nb = sizeof(menu) / sizeof(menu[0]);
    const int choix_row = P4_ROW0 + 9;

    display_clear(COLOR_BLACK);
    for (int i = 0; i < nb; i++)
        put_str(P4_ROW0 + i, 0, menu[i], P4_TEXT_COLOR, COLOR_BLACK);

    /* Option sélectionnée en noir sur blanc, comme le \033[30;47m du terminal */
    if (select == 0)
        put_str(choix_row, 7, "Bot", COLOR_BLACK, COLOR_WHITE);
    else
        put_str(choix_row, 13, "2 player", COLOR_BLACK, COLOR_WHITE);

    put_str(36, 1, "joystick =  <--   -->", P4_TEXT_COLOR, COLOR_BLACK);
    put_str(37, 1, "bouton   =  select", P4_TEXT_COLOR, COLOR_BLACK);
    display_swap();
}

static void afficher_choix(int choix, char cara)
{
    static const char *const titre[] = {
        "    ********************    ",
        "    ***              ***    ",
        "    *** PUISSANCE 4  ***    ",
        "    ***     by :     ***    ",
        "    *** Pierre       ***    ",
        "    ***     Bouckson ***    ",
        "    ***              ***    ",
        "    ********************    ",
    };

    for (int i = 0; i < 8; i++)
        put_str(TITLE_ROW + i, 0, titre[i], P4_TEXT_COLOR, COLOR_BLACK);

    if (choix < 0)
        return;

    int col = choix * 4 + 2;
    put_char(ARROW_ROW,     col, cara, couleur_pion(cara));
    put_char(ARROW_ROW + 1, col, '|',  P4_TEXT_COLOR);
    put_char(ARROW_ROW + 2, col, 'v',  P4_TEXT_COLOR);
}

static void afficher_grille(const p4_jeux_t *jeu)
{
    /* 13 lignes x 29 colonnes : "+---+" pour les bords, "| O |" pour les cases */
    for (int i = 0; i < 2 * P4_ROWS + 1; i++)
    {
        for (int j = 0; j < 4 * P4_COLS + 1; j++)
        {
            char c = ' ';
            uint16_t fg = P4_TEXT_COLOR;

            if (i % 2 == 0)
            {
                c = (j % 4 == 0) ? '+' : '-';
            }
            else if (j % 4 == 0)
            {
                c = '|';
            }
            else if (j % 4 == 2)
            {
                uint8_t v = jeu->buffer[i / 2][j / 4];
                if (v == P4_J1)
                    c = 'O';
                else if (v == P4_J2)
                    c = 'X';
                fg = couleur_pion(c);
            }
            put_char(GRID_ROW + i, j, c, fg);
        }
    }

    put_str(HELP_ROW,     1, "joystick =  <--   -->", P4_TEXT_COLOR, COLOR_BLACK);
    put_str(HELP_ROW + 1, 1, "bouton   =  select", P4_TEXT_COLOR, COLOR_BLACK);
}

void p4_grille_afficher_jeu(const p4_jeux_t *jeu, int choix, char cara,
                            const char *message, uint16_t message_color)
{
    display_clear(COLOR_BLACK);
    afficher_choix(choix, cara);
    afficher_grille(jeu);

    if (message)
    {
        put_str(MESSAGE_ROW, 1, message, message_color, COLOR_BLACK);
        put_str(MESSAGE_ROW + 2, 1, "bouton = retour menu", P4_TEXT_COLOR, COLOR_BLACK);
    }
    display_swap();
}
