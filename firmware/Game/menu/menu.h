#ifndef __MENU_H
#define __MENU_H

#include "display.h"
#include "input.h"
#include "interface.h"

/* Déplace la sélection d'une tuile selon la direction du joystick */
void menu_move(joy_mv_t mv);

/* Dessine le menu avec la tuile sélectionnée, puis l'affiche */
void print_menu(void);

/* Lance le jeu de la tuile sélectionnée (bloquant jusqu'à la fin du jeu) */
void start_game(void);

#endif /* __MENU_H */
