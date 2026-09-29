#ifndef __INTERFACE_H
#define __INTERFACE_H

#include "display.h"
#include "font8x8.h"

#define MENU_ROWS (LCD_H / FONT_H)   /* 40 lignes */
#define MENU_COLS (LCD_W / FONT_W)   /* 30 colonnes */

/* +1 pour le '\0' de fin de chaque ligne */
extern const char menu[MENU_ROWS][MENU_COLS + 1];

#endif /* __INTERFACE_H */
