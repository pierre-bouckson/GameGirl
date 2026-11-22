#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "main.h"
#include "power.h"
#include "display.h"
#include "font8x8.h"
#include "input.h"
#include "battery.h"
#include "backlight.h"
#include "buzzer.h"
#include "easy.h"

extern volatile uint8_t button_pressed;   /* main.c */

#define COLOR_TITLE  RGB565(210, 80, 215)
#define COLOR_LABEL  RGB565(150, 150, 170)
#define COLOR_FRAME  RGB565(60, 70, 120)

#define RELEASE_TIMEOUT_MS 3000   /* attente max du relâchement des commandes */

static volatile uint32_t last_activity = 0;   /* HAL_GetTick() de la dernière action */
static bool ready = false;      /* power_init() fait : SDRAM et écran prêts */
static bool busy = false;       /* message en cours : pas de power_poll() imbriqué */
static bool low_warned = false; /* message batterie faible déjà affiché */
static uint8_t low_count = 0;   /* mesures de suite sous POWER_LOW_PCT */
static uint8_t cutoff_count = 0;/* mesures de suite sous POWER_CUTOFF_MV */
static uint32_t last_check = 0;

/* ---- Commandes ------------------------------------------------------------- */

/* Une commande est-elle actionnée ? Joystick hors zone morte, A ou START */
static bool input_active(void)
{
    if (abs((int)joy_raw_x() - JOY_CENTER) > JOY_DEADZONE ||
        abs((int)joy_raw_y() - JOY_CENTER) > JOY_DEADZONE)
        return true;
    return HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET ||
           HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_SET;
}

/* Une action = un changement : bouton appuyé ou relâché, joystick déplacé de
 * plus de ACTIVITY_DELTA depuis la dernière action. Une commande coincée
 * (bouton bloqué par le boîtier, console posée sur le stick) n'empêche donc
 * pas la mise en veille. */
#define ACTIVITY_DELTA 400   /* ~10 % de la course de l'ADC 12 bits */

void power_tick(void)
{
    static uint16_t ref_x = JOY_CENTER, ref_y = JOY_CENTER;
    static uint8_t prev_buttons = 0;

    uint16_t x = joy_raw_x(), y = joy_raw_y();
    uint8_t buttons = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) |
                      (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_4) == GPIO_PIN_SET) << 1;

    if (buttons != prev_buttons ||
        abs((int)x - ref_x) > ACTIVITY_DELTA || abs((int)y - ref_y) > ACTIVITY_DELTA)
    {
        prev_buttons = buttons;
        ref_x = x;
        ref_y = y;
        last_activity = HAL_GetTick();
    }
}

/* Attente sans HAL_Delay() ni display_swap() : ni power_poll() ni retour au
 * menu (longjmp) au milieu d'un message */
static void wait_ms(uint32_t ms)
{
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < ms);
}

/* Attend que plus rien ne soit actionné : l'appui qui ferme un message ne doit
 * pas être repris par le jeu, et PA0 doit être bas avant le Standby (sinon le
 * réveil est immédiat) */
static void wait_release(void)
{
    uint32_t start = HAL_GetTick();
    while (input_active() && HAL_GetTick() - start < RELEASE_TIMEOUT_MS);
    wait_ms(50);   /* rebonds */
    button_pressed = 0;
    new_selection();   /* oublie l'inclinaison du joystick */
}

/* ---- Écrans ---------------------------------------------------------------- */

static void draw_centered(uint8_t row, const char *text, uint16_t fg, uint16_t bg)
{
    display_draw_string((LCD_W - strlen(text) * FONT_W) / 2, row * FONT_H, text, fg, bg);
}

/* Fenêtre centrée au cadre ASCII, comme le menu : lignes 12 à 27 */
static void draw_box(const char *title, uint16_t title_color)
{
    display_clear(COLOR_BLACK);
    for (uint8_t r = 12; r <= 27; r++)
    {
        const char *line = (r == 12 || r == 27) ? "+======================+"
                                                 : "|                      |";
        draw_centered(r, line, COLOR_FRAME, COLOR_BLACK);
    }
    draw_centered(14, title, title_color, COLOR_BLACK);
}

/* Barre de batterie de 10 cases, comme dans les réglages */
static void draw_battery_bar(uint8_t row, uint8_t pct, uint16_t color)
{
    uint8_t filled = (pct + 5) / 10;
    for (uint8_t i = 0; i < 10; i++)
        display_draw_char((10 + i) * FONT_W, row * FONT_H, ' ', COLOR_BLACK,
                          (i < filled || i == 0) ? color : RGB565(50, 50, 65));
}

/* ---- Veille ---------------------------------------------------------------- */

/* Coupe tout ce qui peut l'être puis passe en Standby. Ne revient pas : le
 * réveil par PA0 redémarre le MCU depuis le reset. */
static void enter_standby(void)
{
    /* Réglages modifiés puis console oubliée dans l'écran Réglages */
    backlight_save();
    buzzer_save();
    easy_save();

    while (buzzer_playing());   /* laisse finir le son de mise en veille */
    buzzer_stop();

    backlight_off();
    display_sleep();   /* ILI9341 : display off + sleep in */
    wait_release();

    /* Plus aucune interruption : une IRQ en attente (DMA de l'ADC, SysTick...)
     * ferait ressortir le WFI tout de suite au lieu d'entrer en Standby */
    __disable_irq();
    SysTick->CTRL = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;

    /* PA0 = WKUP : un front montant (bouton A) réveille la console */
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1);

    while (1)
        HAL_PWR_EnterSTANDBYMode();
}

/* Batterie vide : message puis veille */
static void battery_empty(void)
{
    display_overlay_begin();
    draw_box("BATTERIE VIDE", COLOR_RED);
    draw_battery_bar(17, 0, COLOR_RED);
    draw_centered(20, "RECHARGE-MOI !", COLOR_WHITE, COLOR_BLACK);
    draw_centered(22, "ou eteins la console", COLOR_LABEL, COLOR_BLACK);
    draw_centered(25, "mise en veille...", COLOR_LABEL, COLOR_BLACK);
    display_swap_nopoll();
    buzzer_play(SND_LOSE);
    wait_ms(5000);
    enter_standby();
}

/* Batterie faible : message jusqu'à un appui ou POWER_LOW_MSG_MS, puis le jeu reprend */
static void battery_low(uint8_t pct)
{
    char line[24];

    display_overlay_begin();
    draw_box("BATTERIE FAIBLE", COLOR_ORANGE);
    draw_battery_bar(17, pct, COLOR_ORANGE);
    snprintf(line, sizeof line, "%u %%", pct);
    draw_centered(19, line, COLOR_ORANGE, COLOR_BLACK);
    draw_centered(22, "pense a me recharger", COLOR_WHITE, COLOR_BLACK);
    draw_centered(25, "bouton : continuer", COLOR_LABEL, COLOR_BLACK);
    display_swap_nopoll();
    buzzer_play(SND_BACK);

    wait_release();   /* une commande tenue avant le message ne le ferme pas */
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < POWER_LOW_MSG_MS && !input_active());
    wait_release();

    display_overlay_end();
}

/* Inactivité : compte à rebours, annulé par n'importe quelle commande */
static void idle_countdown(void)
{
    char line[24];
    uint32_t start = HAL_GetTick();
    uint32_t activity_at_start = last_activity;
    int shown = -1;

    display_overlay_begin();
    while (last_activity == activity_at_start)   /* une action : on reprend */
    {
        uint32_t elapsed = HAL_GetTick() - start;
        if (elapsed >= POWER_IDLE_WARN_MS)
        {
            draw_box("BONNE NUIT !", COLOR_TITLE);
            draw_centered(19, "mise en veille", COLOR_WHITE, COLOR_BLACK);
            draw_centered(22, "bouton A : rallumer", COLOR_LABEL, COLOR_BLACK);
            display_swap_nopoll();
            buzzer_play(SND_BACK);
            wait_ms(1500);
            enter_standby();
        }

        int left = (POWER_IDLE_WARN_MS - elapsed + 999) / 1000;
        if (left != shown)
        {
            shown = left;
            draw_box("VEILLE", COLOR_TITLE);
            snprintf(line, sizeof line, "dans %d s", left);
            draw_centered(18, line, COLOR_WHITE, COLOR_BLACK);
            draw_centered(22, "touche une commande", COLOR_LABEL, COLOR_BLACK);
            draw_centered(24, "pour continuer", COLOR_LABEL, COLOR_BLACK);
            display_swap_nopoll();
            buzzer_play(SND_MOVE);
        }
    }

    wait_release();
    display_overlay_end();
    last_activity = HAL_GetTick();
}

/* ---- Surveillance ---------------------------------------------------------- */

void power_init(void)
{
    /* Réveil par PA0 depuis le Standby : on rend PA0 à l'EXTI0 du bouton */
    HAL_PWR_DisableWakeUpPin(PWR_WAKEUP_PIN1);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_SB);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* Amorce la moyenne glissante, puis refuse de démarrer sur une batterie
     * vide (hystérésis : sans rétroéclairage, la tension remonte un peu) */
    uint16_t mv = 0;
    for (uint8_t i = 0; i < 8; i++)
        mv = battery_mv();

    last_activity = HAL_GetTick();
    last_check = HAL_GetTick();
    ready = true;

    if (!battery_on_usb() && mv < POWER_BOOT_MIN_MV)
    {
        busy = true;
        battery_empty();
    }
}

static void check_battery(void)
{
    uint16_t mv = battery_mv();

    if (battery_on_usb())
    {
        low_warned = false;
        low_count = cutoff_count = 0;
        return;
    }

    cutoff_count = (mv < POWER_CUTOFF_MV) ? cutoff_count + 1 : 0;
    if (cutoff_count >= POWER_CUTOFF_CHECKS)
        battery_empty();

    uint8_t pct = battery_percent();
    if (pct >= POWER_LOW_REARM_PCT)
        low_warned = false;

    low_count = (pct <= POWER_LOW_PCT) ? low_count + 1 : 0;
    if (!low_warned && low_count >= 3)
    {
        low_warned = true;
        battery_low(pct);
    }
}

void power_poll(void)
{
    if (!ready || busy)
        return;
    busy = true;

    uint32_t now = HAL_GetTick();

    if (now - last_check >= POWER_CHECK_MS)
    {
        last_check = now;
        check_battery();
    }

    if (HAL_GetTick() - last_activity >= POWER_IDLE_MS - POWER_IDLE_WARN_MS)
        idle_countdown();

    busy = false;
}
