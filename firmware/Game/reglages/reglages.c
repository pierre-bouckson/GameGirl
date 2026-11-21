#include <stdio.h>
#include <string.h>
#include "main.h"
#include "display.h"
#include "font8x8.h"
#include "input.h"
#include "backlight.h"
#include "battery.h"
#include "reglages.h"

extern volatile uint8_t button_pressed;

#define FRAME_MS        50     /* boucle à 20 Hz */
#define BATTERY_MS      1000   /* rafraîchissement de la batterie */
#define DEBOUNCE_MS     50

#define BAR_COL         5      /* barres de 20 cases, centrées sur les 30 colonnes */
#define BAR_LEN         20

#define COLOR_TITLE     RGB565(210, 80, 215)
#define COLOR_LABEL     RGB565(150, 150, 170)
#define COLOR_BAR_OFF   RGB565(50, 50, 65)
#define COLOR_SUN       RGB565(255, 215, 60)

/* ---- Outils ----------------------------------------------------------------- */

/* Texte centré sur la ligne row (en cases) */
static void draw_centered(uint8_t row, const char *text, uint16_t fg)
{
    uint16_t col = (LCD_W / FONT_W - strlen(text)) / 2;
    display_draw_string(col * FONT_W, row * FONT_H, text, fg, COLOR_BLACK);
}

/* Barre de BAR_LEN cases pleines, dont filled allumées */
static void draw_bar(uint8_t row, uint8_t filled, uint16_t color)
{
    for (uint8_t i = 0; i < BAR_LEN; i++)
    {
        uint16_t bg = (i < filled) ? color : COLOR_BAR_OFF;
        display_draw_char((BAR_COL + i) * FONT_W, row * FONT_H, ' ', COLOR_BLACK, bg);
    }
}

/* Attend que le bouton soit relâché puis efface les rebonds */
static void consume_button(void)
{
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
    HAL_Delay(DEBOUNCE_MS);
    button_pressed = 0;
}

/* ---- Écran ------------------------------------------------------------------ */

static void draw_screen(void)
{
    char line[31];

    display_clear(COLOR_BLACK);

    draw_centered(3, "REGLAGES", COLOR_TITLE);
    draw_centered(4, "--------", COLOR_TITLE);

    /* Luminosité */
    uint8_t level = backlight_get();
    draw_centered(9, "LUMINOSITE", COLOR_LABEL);
    draw_bar(12, level * BAR_LEN / BACKLIGHT_MAX, COLOR_SUN);
    snprintf(line, sizeof line, "%c  %2u / %u  %c",
             (level > BACKLIGHT_MIN) ? '<' : ' ', level, BACKLIGHT_MAX,
             (level < BACKLIGHT_MAX) ? '>' : ' ');
    draw_centered(15, line, COLOR_WHITE);

    /* Batterie */
    uint16_t mv = battery_mv();
    draw_centered(21, "BATTERIE", COLOR_LABEL);
    if (battery_on_usb())
    {
        draw_bar(24, BAR_LEN, COLOR_CYAN);
        draw_centered(27, "USB : EN CHARGE", COLOR_CYAN);
    }
    else
    {
        uint8_t pct = battery_percent();
        uint16_t color = (pct >= 50) ? RGB565(80, 220, 100)
                       : (pct >= 20) ? COLOR_ORANGE : COLOR_RED;
        draw_bar(24, (pct * BAR_LEN + 50) / 100, color);
        snprintf(line, sizeof line, "%u %%", pct);
        draw_centered(27, line, color);
    }
    snprintf(line, sizeof line, "%u.%02u V", mv / 1000, (mv % 1000) / 10);
    draw_centered(29, line, COLOR_LABEL);

    draw_centered(35, "JOYSTICK < > : REGLER", COLOR_LABEL);
    draw_centered(37, "BOUTON : RETOUR", COLOR_LABEL);

    display_swap();
}

void menu_reglages(void)
{
    uint8_t level_at_start = backlight_get();
    uint32_t last_battery = HAL_GetTick();

    consume_button();   /* le bouton qui a ouvert l'écran */
    draw_screen();

    while (1)
    {
        if (button_pressed)
        {
            consume_button();
            break;
        }

        /* Même sens que le menu : Y = 1 va à droite, donc plus lumineux */
        if (new_selection())
        {
            joy_mv_t mv = selection_joy();
            if (mv.y != 0)
            {
                backlight_set(backlight_get() + mv.y);
                draw_screen();
            }
        }

        if (HAL_GetTick() - last_battery >= BATTERY_MS)
        {
            last_battery = HAL_GetTick();
            draw_screen();   /* mesure la batterie et met à jour l'affichage */
        }

        HAL_Delay(FRAME_MS);
    }

    if (backlight_get() != level_at_start)
    {
        display_clear(COLOR_BLACK);
        draw_centered(20, "SAUVEGARDE...", COLOR_LABEL);
        display_swap();
        backlight_save();   /* efface un secteur flash : ~1 à 2 s */
    }
}
