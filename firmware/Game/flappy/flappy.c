#include <stdlib.h>
#include "buzzer.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "flappy.h"
#include "input.h"
#include "display.h"
#include "font8x8.h"
#include "main.h"
#include "easy.h"

/* Terrain de 240x256 px sous le titre : 240 px de ciel puis 16 px de sol.
 * L'oiseau reste à une abscisse fixe et ne bouge qu'en hauteur (position
 * en float, au pixel près) ; ce sont les tuyaux qui défilent vers la gauche.
 * Coordonnées relatives au terrain. */
#define FIELD_W     240
#define FIELD_H     256
#define F_Y0        (4 * FONT_H)
#define GROUND_H    16
#define SKY_H       (FIELD_H - GROUND_H)
#define GRASS_H     3                     /* liseré d'herbe en haut du sol */

#define BIRD_X      56                    /* bord gauche de l'oiseau */
#define BIRD_W      12
#define BIRD_H      9

#define PIPE_W      28
#define CAP_W       (PIPE_W + 4)          /* chapeau du tuyau, 2 px de chaque côté */
#define CAP_H       6
#define GAP_MARGIN  24                    /* ouverture jamais collée au haut ni au sol */
#define NB_PIPES    3                     /* assez pour couvrir 240 px à 130 ou 150 px d'écart */

#define P_SKY        RGB565(20, 20, 40)
#define P_PIPE       RGB565(80, 200, 80)
#define P_PIPE_LIGHT RGB565(150, 240, 130)
#define P_PIPE_DARK  RGB565(30, 120, 40)
#define P_GRASS      RGB565(80, 220, 100)
#define P_DIRT_A     RGB565(150, 110, 60)
#define P_DIRT_B     RGB565(120, 85, 45)
#define P_BIRD       RGB565(255, 215, 60)

#define DEBOUNCE_MS 50
#define DEATH_MS    800   /* pause sur l'image de la collision */

/* Oiseau tourné vers la droite : Y = corps, w = aile, W = œil, K = pupille,
 * O = bec, '.' = transparent */
static const char bird_sprite[BIRD_H][BIRD_W + 1] = {
    "....YYYY....",
    "..YYYYYWW...",
    ".YYYYYWWKW..",
    "wwwwYYWWKW..",
    "wwwwwYYWWW..",
    ".wwwYYYOOOOO",
    "..YYYYOOOOO.",
    "...YYYYOOO..",
    ".....YYY....",
};

typedef struct
{
    float x;        /* bord gauche du tuyau (sans le chapeau) */
    int gap_y;      /* haut de l'ouverture */
    bool passed;    /* déjà compté dans le score */
} pipe_t;

static pipe_t pipes[NB_PIPES];
static float bird_y, bird_vy, speed;
static float ground_off;   /* décalage des rayures du sol, pour le défilement */
static uint32_t score, best;

/* Réglages de la partie, choisis selon le mode au lancement */
static bool easy;
static float gravity, flap_v, max_fall, speed_start, speed_up;
static int gap, spacing, margin;

/* ---- Meilleur score en flash -------------------------------------------- */

/* Secteur 20 (128 Ko) : 21, 22 et 23 sont déjà pris par Tetris, Snake et 2048 */
#define BEST_ADDR    0x08180000UL
#define BEST_SECTOR  FLASH_SECTOR_20

static uint32_t best_read(void)
{
    uint32_t v = *(volatile uint32_t *)BEST_ADDR;
    return (v == 0xFFFFFFFF) ? 0 : v;   /* flash effacée = jamais écrit */
}

/* Efface le secteur puis écrit la valeur : bloque ~1 à 2 s */
static void best_write(uint32_t value)
{
    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = BEST_SECTOR,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,   /* alim 2.7-3.6 V : écriture par mots de 32 bits */
    };
    uint32_t err;

    HAL_FLASH_Unlock();
    if (HAL_FLASHEx_Erase(&erase, &err) == HAL_OK)
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, BEST_ADDR, value);
    HAL_FLASH_Lock();
}

/* ---- Outils ----------------------------------------------------------------- */

/* Attend que le bouton soit relâché puis efface les rebonds */
static void consume_button(void)
{
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
    HAL_Delay(DEBOUNCE_MS);
    button_pressed = 0;
}

/* Bouton ou stick vers le haut (x = 1, comme dans le menu) : battement d'ailes */
static bool flap_requested(void)
{
    bool flap = false;

    if (button_pressed)
    {
        button_pressed = 0;
        flap = true;
    }
    if (new_selection() && selection_joy().x == 1)
        flap = true;
    return flap;
}

static void draw_centered(uint8_t row, const char *str, uint16_t fg, uint16_t bg)
{
    display_draw_string((LCD_W - strlen(str) * FONT_W) / 2, row * FONT_H, str, fg, bg);
}

/* Rectangle en coordonnées terrain, coupé aux bords du terrain */
static void fill_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int j = (y < 0 ? 0 : y); j < y + h && j < FIELD_H; j++)
        for (int i = (x < 0 ? 0 : x); i < x + w && i < FIELD_W; i++)
            display_draw_pixel(i, F_Y0 + j, color);
}

static bool overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

/* ---- Dessin ----------------------------------------------------------------- */

/* Tronçon de tuyau : reflet à gauche, ombre à droite */
static void draw_pipe_part(int x, int y, int w, int h)
{
    fill_rect(x, y, w, h, P_PIPE);
    fill_rect(x + 2, y, 3, h, P_PIPE_LIGHT);
    fill_rect(x + w - 4, y, 4, h, P_PIPE_DARK);
}

static void draw_pipe(const pipe_t *p)
{
    int x = (int)p->x;
    int bottom = p->gap_y + gap;

    draw_pipe_part(x, 0, PIPE_W, p->gap_y - CAP_H);
    draw_pipe_part(x - 2, p->gap_y - CAP_H, CAP_W, CAP_H);
    draw_pipe_part(x - 2, bottom, CAP_W, CAP_H);
    draw_pipe_part(x, bottom + CAP_H, PIPE_W, SKY_H - bottom - CAP_H);
}

/* Sol rayé en diagonale, les rayures avancent avec les tuyaux */
static void draw_ground(void)
{
    int off = (int)ground_off;

    fill_rect(0, SKY_H, FIELD_W, GRASS_H, P_GRASS);
    for (int y = SKY_H + GRASS_H; y < FIELD_H; y++)
        for (int x = 0; x < FIELD_W; x++)
            display_draw_pixel(x, F_Y0 + y, ((x + off + y) / 6) % 2 ? P_DIRT_A : P_DIRT_B);
}

static void draw_bird(int y)
{
    for (int r = 0; r < BIRD_H; r++)
        for (int c = 0; c < BIRD_W; c++)
        {
            uint16_t color;
            switch (bird_sprite[r][c])
            {
                case 'Y': color = P_BIRD;                break;
                case 'w': color = RGB565(255, 245, 180); break;
                case 'W': color = COLOR_WHITE;           break;
                case 'K': color = COLOR_BLACK;           break;
                case 'O': color = RGB565(255, 120, 40);  break;
                default:  continue;   /* transparent */
            }
            if (y + r >= 0 && y + r < SKY_H)
                display_draw_pixel(BIRD_X + c, F_Y0 + y + r, color);
        }
}

/* Dessine le terrain, l'oiseau et le score, sans afficher (pas de display_swap) */
static void draw_scene(int bird_px)
{
    char line[16];

    display_clear(COLOR_BLACK);
    draw_centered(2, "= FLAPPY =", P_BIRD, COLOR_BLACK);

    fill_rect(0, 0, FIELD_W, SKY_H, P_SKY);
    for (int i = 0; i < NB_PIPES; i++)
        draw_pipe(&pipes[i]);
    draw_ground();
    draw_bird(bird_px);

    /* Score sous le terrain (lignes 36 à 39 libres) */
    display_draw_string(FONT_W, 37 * FONT_H, "SCORE", COLOR_GRAY, COLOR_BLACK);
    snprintf(line, sizeof line, "%lu", (unsigned long)score);
    display_draw_string(7 * FONT_W, 37 * FONT_H, line, COLOR_WHITE, COLOR_BLACK);

    if (easy)
    {
        display_draw_string(22 * FONT_W, 37 * FONT_H, "FACILE", EASY_COLOR, COLOR_BLACK);
        return;
    }
    display_draw_string(17 * FONT_W, 37 * FONT_H, "BEST", COLOR_GRAY, COLOR_BLACK);
    snprintf(line, sizeof line, "%lu", (unsigned long)((score > best) ? score : best));
    display_draw_string(22 * FONT_W, 37 * FONT_H, line, COLOR_YELLOW, COLOR_BLACK);
}

/* ---- Partie ---------------------------------------------------------------- */

static int random_gap(void)
{
    return GAP_MARGIN + rand() % (SKY_H - gap - 2 * GAP_MARGIN + 1);
}

/* Tuyaux alignés à droite de l'écran, hors champ */
static void reset_pipes(void)
{
    for (int i = 0; i < NB_PIPES; i++)
        pipes[i] = (pipe_t){FIELD_W + 40 + i * spacing, random_gap(), false};
}

/* Fait défiler les tuyaux ; un tuyau sorti à gauche repart derrière le dernier.
 * Compte les tuyaux passés par l'oiseau. */
static void pipes_step(void)
{
    for (int i = 0; i < NB_PIPES; i++)
    {
        pipe_t *p = &pipes[i];
        p->x -= speed;

        if (!p->passed && p->x + PIPE_W < BIRD_X)
        {
            p->passed = true;
            score++;
            buzzer_play(SND_POINT);
            speed = fminf(speed + speed_up, FLAPPY_SPEED_MAX);
        }

        if (p->x + CAP_W < 0)
        {
            float last = pipes[0].x;
            for (int j = 1; j < NB_PIPES; j++)
                last = fmaxf(last, pipes[j].x);
            *p = (pipe_t){last + spacing, random_gap(), false};
        }
    }

    ground_off += speed;
    if (ground_off >= 12.0f)   /* période des rayures : 2 x 6 px */
        ground_off -= 12.0f;
}

/* L'oiseau touche-t-il un tuyau ou le sol ? Boîte réduite de margin px de
 * chaque côté (1 px, 4 en mode facile où le sol ne compte pas) */
static bool collision(void)
{
    int bx = BIRD_X + margin, by = (int)bird_y + margin;
    int bw = BIRD_W - 2 * margin, bh = BIRD_H - 2 * margin;

    if (!easy && by + bh >= SKY_H)
        return true;

    for (int i = 0; i < NB_PIPES; i++)
    {
        int x = (int)pipes[i].x;
        int top = pipes[i].gap_y;
        int bottom = top + gap;

        if (overlap(bx, by, bw, bh, x, 0, PIPE_W, top) ||
            overlap(bx, by, bw, bh, x - 2, top - CAP_H, CAP_W, CAP_H) ||
            overlap(bx, by, bw, bh, x - 2, bottom, CAP_W, CAP_H) ||
            overlap(bx, by, bw, bh, x, bottom, PIPE_W, SKY_H - bottom))
            return true;
    }
    return false;
}

static void bird_step(void)
{
    bird_vy = fminf(bird_vy + gravity, max_fall);
    bird_y += bird_vy;

    /* Mode facile : le sol fait rebondir l'oiseau au lieu de le tuer */
    if (easy && bird_y + BIRD_H >= SKY_H)
    {
        bird_y = SKY_H - BIRD_H;
        bird_vy = flap_v;
        buzzer_play(SND_HIT);
    }

    /* Plafond : l'oiseau s'y cogne sans mourir */
    if (bird_y < 0)
    {
        bird_y = 0;
        bird_vy = 0;
    }
}

/* Écran d'attente : l'oiseau flotte jusqu'au premier battement */
static void wait_start(void)
{
    uint32_t last_tick = HAL_GetTick();
    uint32_t frame = 0;

    consume_button();
    new_selection();   /* oublie l'inclinaison qui a servi dans le menu */

    while (!flap_requested())
    {
        while (HAL_GetTick() - last_tick < FLAPPY_FRAME_MS);
        last_tick = HAL_GetTick();

        ground_off += speed;
        if (ground_off >= 12.0f)
            ground_off -= 12.0f;

        draw_scene((int)(bird_y + 4.0f * sinf(frame++ * 0.12f)));
        draw_centered(12, "PRET ?", COLOR_WHITE, P_SKY);
        draw_centered(25, "bouton : voler", COLOR_GRAY, P_SKY);
        display_swap();
    }
}

/* Écran de fin : game over, score et nouveau record éventuel */
static void end_screen(bool record)
{
    char line[16];

    display_clear(COLOR_BLACK);
    draw_centered(11, "GAME OVER", COLOR_RED, COLOR_BLACK);
    if (record)
        draw_centered(14, "Nouveau record !", COLOR_YELLOW, COLOR_BLACK);

    draw_centered(17, "SCORE", COLOR_GRAY, COLOR_BLACK);
    snprintf(line, sizeof line, "%lu", (unsigned long)score);
    draw_centered(19, line, COLOR_WHITE, COLOR_BLACK);

    if (!easy)
    {
        snprintf(line, sizeof line, "BEST %lu", (unsigned long)(record ? score : best));
        draw_centered(22, line, COLOR_GRAY, COLOR_BLACK);
    }

    draw_centered(27, "bouton : menu", COLOR_GRAY, COLOR_BLACK);
    display_swap();
}

void menu_flappy(void)
{
    easy        = easy_mode();
    gravity     = easy ? FLAPPY_EASY_GRAVITY  : FLAPPY_GRAVITY;
    flap_v      = easy ? FLAPPY_EASY_FLAP     : FLAPPY_FLAP;
    max_fall    = easy ? FLAPPY_EASY_MAX_FALL : FLAPPY_MAX_FALL;
    speed_start = easy ? FLAPPY_EASY_SPEED    : FLAPPY_SPEED;
    speed_up    = easy ? 0.0f          : FLAPPY_SPEED_UP;
    gap         = easy ? FLAPPY_EASY_GAP      : FLAPPY_GAP;
    spacing     = easy ? FLAPPY_EASY_SPACING  : FLAPPY_SPACING;
    margin      = easy ? FLAPPY_EASY_MARGIN   : 1;

    score = 0;
    best = best_read();
    speed = speed_start;
    ground_off = 0;
    bird_y = (SKY_H - BIRD_H) / 2;
    bird_vy = 0;
    reset_pipes();

    wait_start();

    /* Le moment du premier battement sert de graine aléatoire pour les tuyaux */
    srand(HAL_GetTick());
    reset_pipes();
    bird_vy = flap_v;

    /* Le bouton sert à voler : la partie ne s'arrête qu'à la collision */
    uint32_t last_tick = HAL_GetTick();
    while (!collision())
    {
        while (HAL_GetTick() - last_tick < FLAPPY_FRAME_MS);
        last_tick = HAL_GetTick();

        if (flap_requested())
        {
            bird_vy = flap_v;
            buzzer_play(SND_HIT);
        }
        bird_step();
        pipes_step();

        draw_scene((int)bird_y);
        display_swap();
    }

    buzzer_play(SND_LOSE);
    HAL_Delay(DEATH_MS);   /* laisse voir la collision */

    bool record = !easy && score > best;   /* pas de record en mode facile */
    if (record)
        buzzer_play(SND_WIN);
    end_screen(record);
    if (record)
        best_write(score);   /* l'écran reste figé pendant l'effacement */

    button_pressed = 0;   /* ignore les appuis faits pendant la partie */
    while (!button_pressed) HAL_Delay(1);   /* HAL_Delay : START reste actif */
    consume_button();
}
