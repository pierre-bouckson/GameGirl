#include <stdio.h>
#include <string.h>
#include "main.h"
#include "display.h"
#include "font8x8.h"
#include "rocket.h"

/* Port du jeu exam2_stm32 : la version d'origine dessinait dans un terminal
 * (1 ligne de route en haut, puis scroll vers le bas). On garde le même
 * écran de caractères, émulé ici puis dessiné sur le LCD. */

/* Géométrie du jeu d'origine */
#define SCREEN_W   64       /* largeur de la piste en colonnes */
#define CAR_ROW    30       /* ligne de la fusée (1 = haut du terminal) */
#define ADC_MAX    4095     /* pleine échelle de l'ADC 12 bits */

/* Période d'une image : TIM6 à 45 ms, 10 ms quand le bouton est maintenu */
#define PERIOD_MS        45
#define PERIOD_BOOST_MS  10

/* Rendu LCD : 64 colonnes de 3 px = 192 px, le score à droite */
#define CELL_W     3
#define CELL_H     FONT_H
#define TERM_ROWS  (LCD_H / CELL_H)              /* 40 lignes */
#define SCORE_X    (SCREEN_W * CELL_W + 4)

#define ROAD_COLOR  COLOR_WHITE                  /* ANSI 1;37 */
#define CAR_COLOR   RGB565(80, 120, 255)         /* ANSI 1;34 */
#define SCORE_BG    RGB565(200, 0, 0)            /* ANSI 41 */
#define OVER_COLOR  RGB565(255, 60, 60)          /* ANSI 1;31 */

/* Définis dans main.c : DMA de l'ADC joystick et flag levé par l'EXTI0 (PA0) */
extern volatile uint16_t joy_adc[2];
extern volatile uint8_t button_pressed;

typedef struct
{
    char c;
    uint16_t fg;
} term_cell_t;

/* Écran du terminal : [0] = ligne du haut */
static term_cell_t term[TERM_ROWS][SCREEN_W];

static void term_clear(void)
{
    for (int r = 0; r < TERM_ROWS; r++)
        for (int c = 0; c < SCREEN_W; c++)
            term[r][c] = (term_cell_t){' ', COLOR_BLACK};
}

/* Équivalent de "\e[1T" : tout descend d'une ligne, la ligne du haut est vide */
static void term_scroll_down(void)
{
    memmove(&term[1], &term[0], sizeof(term) - sizeof(term[0]));
    for (int c = 0; c < SCREEN_W; c++)
        term[0][c] = (term_cell_t){' ', COLOR_BLACK};
}

static void fill_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            display_draw_pixel(x + i, y + j, color);
}

/* Dessine l'écran du terminal et le score dans le back buffer (sans swap) */
static void render(uint32_t score)
{
    display_clear(COLOR_BLACK);

    for (int r = 0; r < TERM_ROWS; r++)
    {
        for (int c = 0; c < SCREEN_W; c++)
        {
            term_cell_t cell = term[r][c];
            int x = c * CELL_W;
            int y = r * CELL_H;

            /* Une colonne fait 3 px : chaque caractère devient un motif plein */
            if (cell.c == '-')
                fill_rect(x, y + CELL_H / 2, CELL_W, 1, cell.fg);
            else if (cell.c != ' ')
                fill_rect(x, y, CELL_W, CELL_H, cell.fg);
        }
    }

    char text[12];
    snprintf(text, sizeof(text), "%lu", (unsigned long)score);
    display_draw_string(SCORE_X, 0, text, COLOR_WHITE, SCORE_BG);
}

/* Attend que le bouton soit relâché puis efface les rebonds */
static void consume_button(void)
{
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
    HAL_Delay(50);
    button_pressed = 0;
}

static void game_over(uint32_t score)
{
    /* Même cadre que "\e[20;32H" dans le terminal, centré sur la piste */
    const int x = (SCREEN_W * CELL_W - 13 * FONT_W) / 2;
    const int y = 19 * CELL_H;

    render(score);
    display_draw_string(x, y,              "#############", OVER_COLOR, COLOR_BLACK);
    display_draw_string(x, y + CELL_H,     "##GAME OVER##", OVER_COLOR, COLOR_BLACK);
    display_draw_string(x, y + 2 * CELL_H, "#############", OVER_COLOR, COLOR_BLACK);
    display_draw_string(x, y + 4 * CELL_H, "bouton : menu", COLOR_GRAY, COLOR_BLACK);
    display_swap();

    /* Le bouton a pu servir au boost pendant la partie : on attend un nouvel appui */
    consume_button();
    while (!button_pressed);
    consume_button();
}

void menu_rocket(void)
{
    uint32_t j = 0;
    uint8_t coef = 1;
    uint32_t period = PERIOD_MS;
    uint32_t last_tick;

    consume_button();
    term_clear();
    last_tick = HAL_GetTick();

    while (1)
    {
        while (HAL_GetTick() - last_tick < period);
        last_tick = HAL_GetTick();

        /* Ligne de route : '#' hors des bords, '-' une fois le circuit fini */
        uint32_t idx = (j < ROCKET_TRACK_LEN) ? j : (ROCKET_TRACK_LEN - 1);
        for (int i = 0; i < SCREEN_W; i++)
        {
            char c = (i < rocket_edges[idx][0] || i >= rocket_edges[idx][1]) ? '#' : ' ';
            if (j >= ROCKET_TRACK_LEN)
                c = '-';
            term[0][i] = (term_cell_t){c, ROAD_COLOR};
        }
        term_scroll_down();

        j++;
        uint32_t score = j * coef;

        /* Comme dans le menu, l'axe Y du joystick donne la gauche / droite de l'écran */
        uint16_t x_position = (uint32_t)joy_adc[1] * (SCREEN_W - 1) / ADC_MAX;

        /* Collision, une fois la fusée entrée sur le circuit */
        if (j > CAR_ROW)
        {
            uint32_t coll = j - CAR_ROW;
            if (coll >= ROCKET_TRACK_LEN)
                coll = ROCKET_TRACK_LEN - 1;

            if (!(x_position > rocket_edges[coll][0] && x_position < rocket_edges[coll][1]))
            {
                game_over(score);
                return;
            }
        }

        /* La fusée n'est pas effacée : le scroll laisse sa traînée derrière elle */
        term[CAR_ROW - 1][x_position] = (term_cell_t){'I', CAR_COLOR};

        render(score);
        display_swap();

        /* Boost tant que le bouton est maintenu : jeu plus rapide, score x2 */
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
        {
            period = PERIOD_BOOST_MS;
            coef = 2;
        }
        else
        {
            period = PERIOD_MS;
            coef = 1;
        }
    }
}
