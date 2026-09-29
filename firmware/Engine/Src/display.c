#include "main.h"
#include "display.h"
#include "font8x8.h"

#define SDRAM_ADDR_1 0xD0000000UL
#define SDRAM_ADDR_2 (SDRAM_ADDR_1 + LCD_W * LCD_H * 2)

extern LTDC_HandleTypeDef hltdc;

static uint16_t *const framebuffer_1 = (uint16_t *)SDRAM_ADDR_1;   // 153 600 octets, en SDRAM externe
static uint16_t *const framebuffer_2 = (uint16_t *)SDRAM_ADDR_2;   // 153 600 octets, en SDRAM externe

static uint16_t *front_buffer = framebuffer_1;   // lu par le LTDC
static uint16_t *back_buffer  = framebuffer_2;   // celui dans lequel on dessine

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

  back_buffer[y * LCD_W + x] = color;
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

void display_swap(void)
{
  HAL_LTDC_SetAddress_NoReload(&hltdc, (uint32_t)back_buffer, 0);
  HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING);

  /* Attendre que le LTDC ait vraiment basculé avant de redessiner dans l'ancien front */
  while (hltdc.Instance->SRCR & LTDC_SRCR_VBR);

  uint16_t *tmp = front_buffer;
  front_buffer = back_buffer;
  back_buffer = tmp;
}
