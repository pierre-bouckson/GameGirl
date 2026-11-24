#include <string.h>
#include "main.h"
#include "display.h"
#include "font8x8.h"
#include "game_exit.h"
#include "power.h"

#define SDRAM_ADDR_1 0xD0000000UL
#define SDRAM_ADDR_2 (SDRAM_ADDR_1 + LCD_W * LCD_H * 2)
#define FB_PIXELS    (LCD_W * LCD_H)

#define LCD_SLEEP_IN 0x10   /* commande ILI9341 absente de ili9341.h (seul Sleep Out y est) */

/* Copies des deux buffers pendant un message par-dessus le jeu (power.c),
 * juste après les framebuffers : la SDRAM fait 8 Mo, on en utilise 600 Ko */
#define SDRAM_SAVE_1 (SDRAM_ADDR_2 + FB_PIXELS * 2)
#define SDRAM_SAVE_2 (SDRAM_SAVE_1 + FB_PIXELS * 2)

extern LTDC_HandleTypeDef hltdc;

static uint16_t *const framebuffer_1 = (uint16_t *)SDRAM_ADDR_1;   // 153 600 octets, en SDRAM externe
static uint16_t *const framebuffer_2 = (uint16_t *)SDRAM_ADDR_2;   // 153 600 octets, en SDRAM externe

static uint16_t *front_buffer = framebuffer_1;   // lu par le LTDC
static uint16_t *back_buffer  = framebuffer_2;   // celui dans lequel on dessine

static uint16_t *const save_front = (uint16_t *)SDRAM_SAVE_1;
static uint16_t *const save_back  = (uint16_t *)SDRAM_SAVE_2;

void display_init(void)
{
  ili9341_Init();

  display_clear(COLOR_BLACK);
  display_swap();
  display_clear(COLOR_BLACK);
}

void display_clear(uint16_t color)
{
  for (uint32_t i = 0; i < LCD_W * LCD_H; i++)
  {
    back_buffer[i] = color;
  }
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
  if (x >= LCD_W || y >= LCD_H)
    return;

#if DISPLAY_ROTATE_180
  /* Rotation de 180° = parcours du buffer à l'envers : (x, y) -> (W-1-x, H-1-y) */
  back_buffer[(LCD_W * LCD_H - 1) - (y * LCD_W + x)] = color;
#else
  back_buffer[y * LCD_W + x] = color;
#endif
}

void display_draw_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg)
{
  if ((uint8_t)c < FONT_FIRST_CHAR || (uint8_t)c > FONT_LAST_CHAR)
    c = '?';

  const uint8_t *glyph = font8x8[(uint8_t)c - FONT_FIRST_CHAR];

  for (uint8_t row = 0; row < FONT_H; row++)
  {
    for (uint8_t col = 0; col < FONT_W; col++)
    {
      /* bit 0 = pixel le plus à gauche */
      uint16_t color = (glyph[row] & (1 << col)) ? fg : bg;
      display_draw_pixel(x + col, y + row, color);
    }
  }
}

void display_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg)
{
  while (*str)
  {
    display_draw_char(x, y, *str, fg, bg);
    x += FONT_W;
    str++;
  }
}

void display_swap_nopoll(void)
{
  HAL_LTDC_SetAddress_NoReload(&hltdc, (uint32_t)back_buffer, 0);
  HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);

  /* Attendre que le LTDC ait vraiment basculé avant de redessiner dans l'ancien front */
  while (hltdc.Instance->SRCR & LTDC_SRCR_VBR);

  uint16_t *tmp = front_buffer;
  front_buffer = back_buffer;
  back_buffer = tmp;
}

void display_swap(void)
{
  display_swap_nopoll();
  power_poll();       /* batterie et inactivité (power.h) */
  game_exit_poll();   /* START pressé : retour au menu (game_exit.h) */
}

void display_overlay_begin(void)
{
  memcpy(save_front, front_buffer, FB_PIXELS * 2);
  memcpy(save_back, back_buffer, FB_PIXELS * 2);
}

void display_overlay_end(void)
{
  /* Ce qui était affiché revient à l'écran, et le buffer de dessin retrouve
   * le contenu que le jeu y avait peut-être commencé */
  memcpy(back_buffer, save_front, FB_PIXELS * 2);
  display_swap_nopoll();
  memcpy(back_buffer, save_back, FB_PIXELS * 2);
}

void display_sleep(void)
{
  ili9341_DisplayOff();
  ili9341_WriteReg(LCD_SLEEP_IN);
  /* Le contrôleur a besoin de 5 ms après Sleep In. Pas de HAL_Delay() : il
   * pourrait faire un retour au menu (START) au milieu de la mise en veille */
  uint32_t start = HAL_GetTick();
  while (HAL_GetTick() - start < 6);
}
