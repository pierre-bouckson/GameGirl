#include <stdio.h>
#include "buzzer.h"
#include <string.h>
#include "main.h"
#include "display.h"
#include "font8x8.h"
#include "input.h"
#include "backlight.h"
#include "battery.h"
#include "easy.h"
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
#define COLOR_SOUND_ON  RGB565(80, 220, 100)
#define COLOR_CURSOR    RGB565(210, 80, 215)

/* Réglages choisis avec le joystick haut / bas, dans l'ordre de l'écran */
typedef enum { ITEM_LIGHT, ITEM_SOUND, ITEM_EASY, ITEM_COUNT } item_t;

static item_t cursor = ITEM_LIGHT;

/* ---- Outils ----------------------------------------------------------------- */

/* Texte centré sur la ligne row (en cases) */
static void draw_centered(uint8_t row, const char *text, uint16_t fg)
{
    uint16_t col = (LCD_W / FONT_W - strlen(text)) / 2;
    display_draw_string(col * FONT_W, row * FONT_H, text, fg, COLOR_BLACK);
}

/* Libellé d'un réglage : barre pleine (comme le nom de la tuile choisie dans
 * le menu) quand le curseur est dessus */
static void draw_label(uint8_t row, const char *text, item_t item)
{
    char line[31];
    if (item == cursor)
    {
        snprintf(line, sizeof line, " > %s < ", text);
        uint16_t col = (LCD_W / FONT_W - strlen(line)) / 2;
        display_draw_string(col * FONT_W, row * FONT_H, line, COLOR_BLACK, COLOR_CURSOR);
    }
    else
        draw_centered(row, text, COLOR_LABEL);
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

    draw_centered(2, "REGLAGES", COLOR_TITLE);
    draw_centered(3, "--------", COLOR_TITLE);

    /* Luminosité */
    uint8_t level = backlight_get();
    draw_label(6, "LUMINOSITE", ITEM_LIGHT);
    draw_bar(8, level * BAR_LEN / BACKLIGHT_MAX, COLOR_SUN);
    snprintf(line, sizeof line, "%c  %2u / %u  %c",
             (level > BACKLIGHT_MIN) ? '<' : ' ', level, BACKLIGHT_MAX,
             (level < BACKLIGHT_MAX) ? '>' : ' ');
    draw_centered(10, line, COLOR_WHITE);

    /* Son */
    draw_label(13, "SON", ITEM_SOUND);
    if (buzzer_muted())
        draw_centered(15, "< COUPE >", COLOR_RED);
    else
        draw_centered(15, "< ACTIVE >", COLOR_SOUND_ON);

    /* Mode facile */
    draw_label(18, "MODE FACILE", ITEM_EASY);
    if (easy_mode())
        draw_centered(20, "< OUI >", EASY_COLOR);
    else
        draw_centered(20, "< NON >", COLOR_LABEL);

    /* Batterie */
    uint16_t mv = battery_mv();
    draw_centered(24, "BATTERIE", COLOR_LABEL);
    if (battery_on_usb())
    {
        draw_bar(26, BAR_LEN, COLOR_CYAN);
        draw_centered(28, "USB : EN CHARGE", COLOR_CYAN);
    }
    else
    {
        uint8_t pct = battery_percent();
        uint16_t color = (pct >= 50) ? RGB565(80, 220, 100)
                       : (pct >= 20) ? COLOR_ORANGE : COLOR_RED;
        draw_bar(26, (pct * BAR_LEN + 50) / 100, color);
        snprintf(line, sizeof line, "%u %%", pct);
        draw_centered(28, line, color);
    }
    snprintf(line, sizeof line, "%u.%02u V", mv / 1000, (mv % 1000) / 10);
    draw_centered(30, line, COLOR_LABEL);

    draw_centered(34, "JOYSTICK HAUT/BAS : CHOISIR", COLOR_LABEL);
    draw_centered(36, "JOYSTICK < > : MODIFIER", COLOR_LABEL);
    draw_centered(38, "BOUTON : RETOUR", COLOR_LABEL);

    display_swap();
}

void menu_reglages(void)
{
    uint8_t level_at_start = backlight_get();
    bool muted_at_start = buzzer_muted();
    bool easy_at_start = easy_mode();
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

        if (new_selection())
        {
            joy_mv_t mv = selection_joy();

            if (mv.x != 0)
            {
                /* Comme dans le menu : X = -1 descend, X = 1 monte */
                if (mv.x == -1 && cursor < ITEM_COUNT - 1)
                    cursor++;
                if (mv.x == 1 && cursor > 0)
                    cursor--;
                buzzer_play(SND_MOVE);
            }
            else if (cursor == ITEM_LIGHT)
            {
                backlight_set(backlight_get() + mv.y);   /* Y = 1 = droite = plus lumineux */
                buzzer_play(SND_MOVE);
            }
            else
            {
                if (cursor == ITEM_SOUND)
                    buzzer_set_mute(!buzzer_muted());
                else
                    easy_set(!easy_mode());
                buzzer_play(SND_SELECT);   /* inaudible si on vient de couper le son */
            }
            draw_screen();
        }

        if (HAL_GetTick() - last_battery >= BATTERY_MS)
        {
            last_battery = HAL_GetTick();
            draw_screen();   /* mesure la batterie et met à jour l'affichage */
        }

        HAL_Delay(FRAME_MS);
    }

    if (backlight_get() != level_at_start || buzzer_muted() != muted_at_start ||
        easy_mode() != easy_at_start)
    {
        display_clear(COLOR_BLACK);
        draw_centered(20, "SAUVEGARDE...", COLOR_LABEL);
        display_swap();
        backlight_save();   /* chacun efface son secteur flash, s'il a changé : ~1 à 2 s */
        buzzer_save();
        easy_save();
    }
}
