#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <stdint.h>

#define LCD_W 240
#define LCD_H 320

/* 1 = image tournée de 180° : l'écran est monté à l'envers sur le PCB de la GameGirl.
 * 0 = orientation de la carte STM32F429I-DISC1. */
#define DISPLAY_ROTATE_180 1

/* Couleurs RGB565 (5 bits R, 6 bits G, 5 bits B) */
#define RGB565(r, g, b)  ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#define COLOR_BLACK      0x0000
#define COLOR_WHITE      0xFFFF
#define COLOR_RED        0xF800
#define COLOR_GREEN      0x07E0
#define COLOR_BLUE       0x001F
#define COLOR_YELLOW     0xFFE0
#define COLOR_CYAN       0x07FF
#define COLOR_MAGENTA    0xF81F
#define COLOR_ORANGE     0xFD20
#define COLOR_PURPLE     0x8010
#define COLOR_PINK       0xFE19
#define COLOR_BROWN      0xA145
#define COLOR_GRAY       0x8410
#define COLOR_LIGHTGRAY  0xC618
#define COLOR_DARKGRAY   0x4208
#define COLOR_NAVY       0x0010
#define COLOR_DARKGREEN  0x03E0
#define COLOR_MAROON     0x8000
#define COLOR_OLIVE      0x8400
#define COLOR_TEAL       0x0410

/* À appeler après l'init de la SDRAM : init ILI9341 + affiche un écran noir */
void display_init(void);

/* Toutes les fonctions de dessin écrivent dans le back buffer (invisible) */
void display_clear(uint16_t color);
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void display_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg);
void display_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg);

/* Affiche le back buffer au prochain vertical blanking, puis échange les buffers.
 * Point de passage des jeux : surveille aussi la batterie / l'inactivité
 * (power_poll) et le retour au menu par START (game_exit_poll). */
void display_swap(void);

/* Pour l'engine seulement : display_swap() sans power_poll() ni game_exit_poll() */
void display_swap_nopoll(void);

/* Message par-dessus un jeu : _begin sauvegarde les deux buffers en SDRAM,
 * _end les restaure, et le jeu retrouve l'écran exactement comme avant */
void display_overlay_begin(void);
void display_overlay_end(void);

/* Écran éteint et ILI9341 en Sleep In, avant la mise en veille du MCU */
void display_sleep(void);

#endif /* __DISPLAY_H */
