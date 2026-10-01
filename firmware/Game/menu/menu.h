#ifndef __MENU_H
#define __MENU_H

#include "display.h"
#include "input.h"
#include "interface.h"

void print_menu(joy_mv_t select);

/* Lance le jeu de la tuile sélectionnée (bloquant jusqu'à la fin du jeu) */
void start_game(joy_mv_t select);

#endif /* __MENU_H */
