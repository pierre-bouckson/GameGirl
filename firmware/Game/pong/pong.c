#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "pong.h"
#include "input.h"
#include "display.h"
#include "font8x8.h"
#include "main.h"

/* Terrain vertical de 240x240 px sous le titre, quadrillé en cases de 8x8
 * comme Snake. La balle et les raquettes, elles, bougent au pixel près
 * (positions en float) : la balle peut partir sous n'importe quel angle.
 * Le bot joue en haut, le joueur en bas. Coordonnées relatives au terrain. */
#define FIELD_PX    240
#define F_Y0        (5 * FONT_H)
#define CELLS       (FIELD_PX / FONT_W)   /* 30 cases de côté */
#define NET_ROW     (CELLS / 2)
#define BALL        8                     /* côté de la balle (px) */
#define PAD_W       40                    /* largeur des raquettes (px) */
#define PAD_H       8                     /* épaisseur, une ligne de cases */
#define PLAYER_Y    (FIELD_PX - PAD_H)    /* haut de la raquette du joueur */
#define BALL_MAX_X  (FIELD_PX - BALL)
#define PAD_MAX_X   (FIELD_PX - PAD_W)

#define DEG2RAD     (3.14159265f / 180.0f)

#define P_EMPTY     RGB565(20, 20, 30)
#define P_NET       RGB565(60, 60, 80)
#define P_BALL      COLOR_WHITE
#define P_PLAYER    RGB565(80, 200, 255)

#define DEBOUNCE_MS 50
#define POINT_MS    700   /* pause après un point, pour voir la balle sortir */

/* Niveaux du bot :
 *  speed   = vitesse max de sa raquette (px par image)
 *  error   = écart max (px) entre le point visé et le centre de la balle :
 *            au-delà de PAD_W / 2 + BALL / 2 = 24 px, il peut la rater
 *  predict = calcule où la balle arrivera (rebonds compris) au lieu de la suivre */
typedef struct
{
    const char *name;
    float speed;
    uint8_t error;
    bool predict;
    uint16_t color;
} level_t;

static const level_t levels[] = {
    {"FACILE",    1.6f, 26, false, RGB565(80, 220, 100)},
    {"MOYEN",     2.4f, 18, false, RGB565(255, 160, 60)},
    {"DIFFICILE", 3.2f, 16, true,  RGB565(230, 60, 60)},
};
#define LEVEL_COUNT (sizeof levels / sizeof levels[0])

static const level_t *level;

/* Balle : coin haut-gauche et vitesse (px par image) ; raquettes : bord gauche */
static float ball_x, ball_y, ball_vx, ball_vy, ball_speed;
static float player_x, bot_x;
static int bot_aim;   /* décalage du point visé par le bot, tiré à chaque renvoi */
static uint8_t score_player, score_bot;

/* Attend que le bouton soit relâché puis efface les rebonds */
static void consume_button(void)
{
    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET);
    HAL_Delay(DEBOUNCE_MS);
    button_pressed = 0;
}

static void draw_centered(uint8_t row, const char *str, uint16_t fg, uint16_t bg)
{
    display_draw_string((LCD_W - strlen(str) * FONT_W) / 2, row * FONT_H, str, fg, bg);
}

/* ---- Choix du niveau ------------------------------------------------------ */

static void print_levels(uint8_t sel)
{
    display_clear(COLOR_BLACK);
    draw_centered(2, "= PONG =", P_PLAYER, COLOR_BLACK);
    draw_centered(8, "CHOISIS TON BOT", COLOR_GRAY, COLOR_BLACK);

    for (uint8_t i = 0; i < LEVEL_COUNT; i++)
    {
        /* Barre de 13 cases, pleine de la couleur du niveau si sélectionnée */
        char line[14];
        snprintf(line, sizeof line, "  %-9s  ", levels[i].name);
        if (i == sel)
            draw_centered(14 + 4 * i, line, COLOR_BLACK, levels[i].color);
        else
            draw_centered(14 + 4 * i, line, levels[i].color, COLOR_BLACK);
    }

    draw_centered(34, "bouton : jouer", COLOR_GRAY, COLOR_BLACK);
    display_swap();
}

static uint8_t choose_level(void)
{
    uint8_t sel = 1;   /* MOYEN par défaut */

    consume_button();
    new_selection();   /* oublie l'inclinaison qui a servi dans le menu */
    print_levels(sel);

    while (!button_pressed)
    {
        if (!new_selection())
            continue;

        /* Comme dans le menu : X = -1 descend, X = 1 monte */
        joy_mv_t mv = selection_joy();
        if (mv.x == -1 && sel < LEVEL_COUNT - 1)
            sel++;
        if (mv.x == 1 && sel > 0)
            sel--;
        print_levels(sel);
    }
    consume_button();
    return sel;
}

/* ---- Partie ---------------------------------------------------------------- */

static float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* Vitesse de la raquette du joueur : axe Y proportionnel (courbe expo), 1 = droite */
static float joy_speed(void)
{
    int v = (int)joy_expo(joy_adc[1]) - JOY_CENTER;
    if (abs(v) < JOY_DEADZONE / 2)
        return 0.0f;
    return PONG_PLAYER_SPD * v / (float)JOY_CENTER;
}

/* Rectangle en coordonnées terrain, coupé aux bords du terrain */
static void fill_rect(int x, int y, int w, int h, uint16_t color)
{
    for (int j = (y < 0 ? 0 : y); j < y + h && j < FIELD_PX; j++)
        for (int i = (x < 0 ? 0 : x); i < x + w && i < FIELD_PX; i++)
            display_draw_pixel(i, F_Y0 + j, color);
}

/* Dessine le terrain et le score, puis l'affiche (display_swap) */
static void print_game(void)
{
    display_clear(COLOR_BLACK);
    draw_centered(2, "= PONG =", P_PLAYER, COLOR_BLACK);

    for (int r = 0; r < CELLS; r++)
        for (int c = 0; c < CELLS; c++)
        {
            uint16_t color = (r == NET_ROW && c % 2 == 0) ? P_NET : P_EMPTY;
            display_draw_char(c * FONT_W, F_Y0 + r * FONT_H, ' ', color, color);
        }

    fill_rect((int)bot_x, 0, PAD_W, PAD_H, level->color);
    fill_rect((int)player_x, PLAYER_Y, PAD_W, PAD_H, P_PLAYER);
    fill_rect((int)ball_x, (int)ball_y, BALL, BALL, P_BALL);

    /* Score sous le terrain (lignes 35 à 39 libres) */
    char line[8];
    display_draw_string(FONT_W, 36 * FONT_H, "BOT", COLOR_GRAY, COLOR_BLACK);
    display_draw_string(5 * FONT_W, 36 * FONT_H, level->name, level->color, COLOR_BLACK);
    snprintf(line, sizeof line, "%u", score_bot);
    display_draw_string(17 * FONT_W, 36 * FONT_H, line, COLOR_WHITE, COLOR_BLACK);

    display_draw_string(FONT_W, 38 * FONT_H, "TOI", COLOR_GRAY, COLOR_BLACK);
    snprintf(line, sizeof line, "%u", score_player);
    display_draw_string(17 * FONT_W, 38 * FONT_H, line, COLOR_WHITE, COLOR_BLACK);

    display_swap();
}

/* Nouveau point visé par le bot, dans [-error, error] autour de la balle */
static void bot_new_aim(void)
{
    bot_aim = rand() % (2 * level->error + 1) - level->error;
}

/* Lance la balle à `angle` degrés de la verticale, dir = 1 vers le joueur */
static void launch(float angle, int dir)
{
    ball_vx = ball_speed * sinf(angle * DEG2RAD);
    ball_vy = dir * ball_speed * cosf(angle * DEG2RAD);
}

/* Balle au centre, lancée vers dir (1 = vers le joueur) avec un angle au hasard */
static void serve(int dir)
{
    ball_x = (FIELD_PX - BALL) / 2;
    ball_y = (FIELD_PX - BALL) / 2;
    ball_speed = PONG_SPEED;
    launch((float)(rand() % 61 - 30), dir);   /* -30° à +30° */
    bot_new_aim();
}

/* Abscisse où arrivera la balle devant le bot, en repliant les rebonds sur les bords */
static float predict_x(void)
{
    float t = (ball_y - PAD_H) / -ball_vy;   /* images avant d'arriver */
    float x = fmodf(ball_x + ball_vx * t, 2.0f * BALL_MAX_X);
    if (x < 0)          x += 2.0f * BALL_MAX_X;
    if (x > BALL_MAX_X) x = 2.0f * BALL_MAX_X - x;
    return x;
}

/* Le bot vise la balle quand elle vient vers lui, sinon revient au centre */
static void bot_move(void)
{
    float target;   /* abscisse visée pour le centre de la raquette */

    if (ball_vy < 0)
        target = (level->predict ? predict_x() : ball_x) + BALL / 2 + bot_aim;
    else
        target = FIELD_PX / 2;

    float d = clampf(target - (bot_x + PAD_W / 2), -level->speed, level->speed);
    bot_x = clampf(bot_x + d, 0, PAD_MAX_X);
}

/* Renvoi : l'angle dépend du point d'impact, 0° au centre de la raquette,
 * PONG_MAX_ANGLE au bord. dir = sens de départ (1 = vers le joueur). */
static void bounce(float pad_x, int dir)
{
    float off = ((ball_x + BALL / 2) - (pad_x + PAD_W / 2)) / (PAD_W / 2 + BALL / 2);

    ball_speed = clampf(ball_speed * PONG_ACCEL, 0, PONG_SPEED_MAX);
    launch(clampf(off, -1, 1) * PONG_MAX_ANGLE, dir);
}

static bool on_pad(float pad_x)
{
    return ball_x + BALL > pad_x && ball_x < pad_x + PAD_W;
}

/* Avance la balle d'une image. Retourne 1 si le joueur marque, -1 si le bot
 * marque, 0 sinon. */
static int ball_step(void)
{
    /* Rebond sur les bords : on replie ce qui dépasse */
    ball_x += ball_vx;
    if (ball_x < 0)
    {
        ball_x = -ball_x;
        ball_vx = -ball_vx;
    }
    if (ball_x > BALL_MAX_X)
    {
        ball_x = 2 * BALL_MAX_X - ball_x;
        ball_vx = -ball_vx;
    }

    float ny = ball_y + ball_vy;

    /* Franchit la ligne de la raquette du joueur pendant cette image ? */
    if (ball_vy > 0 && ball_y + BALL <= PLAYER_Y && ny + BALL > PLAYER_Y && on_pad(player_x))
    {
        ball_y = PLAYER_Y - BALL;
        bounce(player_x, -1);
        bot_new_aim();
        return 0;
    }
    if (ball_vy < 0 && ball_y >= PAD_H && ny < PAD_H && on_pad(bot_x))
    {
        ball_y = PAD_H;
        bounce(bot_x, 1);
        return 0;
    }

    ball_y = ny;
    if (ball_y >= FIELD_PX) return -1;   /* sortie en bas : point pour le bot */
    if (ball_y + BALL <= 0) return 1;    /* sortie en haut : point pour le joueur */
    return 0;
}

/* Écran de fin : résultat et score, puis attend le bouton */
static void end_screen(void)
{
    char line[16];
    bool win = score_player > score_bot;

    display_clear(COLOR_BLACK);
    draw_centered(13, win ? "GAGNE !" : "PERDU", win ? COLOR_YELLOW : COLOR_RED, COLOR_BLACK);
    draw_centered(16, level->name, level->color, COLOR_BLACK);
    snprintf(line, sizeof line, "%u - %u", score_player, score_bot);
    draw_centered(19, "TOI - BOT", COLOR_GRAY, COLOR_BLACK);
    draw_centered(21, line, COLOR_WHITE, COLOR_BLACK);
    draw_centered(26, "bouton : menu", COLOR_GRAY, COLOR_BLACK);
    display_swap();

    button_pressed = 0;   /* ignore les appuis faits pendant la partie */
    while (!button_pressed);
}

void menu_pong(void)
{
    level = &levels[choose_level()];
    /* Le moment de l'appui sur le bouton sert de graine aléatoire pour le bot */
    srand(HAL_GetTick());

    score_player = 0;
    score_bot = 0;
    player_x = PAD_MAX_X / 2;
    bot_x = player_x;
    serve(1);

    uint32_t last_tick = HAL_GetTick();
    print_game();

    /* Le bouton pendant la partie ramène au menu */
    while (!button_pressed)
    {
        while (HAL_GetTick() - last_tick < PONG_FRAME_MS);
        last_tick = HAL_GetTick();

        player_x = clampf(player_x + joy_speed(), 0, PAD_MAX_X);
        bot_move();
        int point = ball_step();
        print_game();

        if (point != 0)
        {
            if (point > 0) score_player++;
            else           score_bot++;
            HAL_Delay(POINT_MS);

            if (score_player >= PONG_WIN_SCORE || score_bot >= PONG_WIN_SCORE)
            {
                end_screen();
                break;
            }

            /* Engagement vers celui qui vient de perdre le point */
            serve(point > 0 ? -1 : 1);
            print_game();
            HAL_Delay(POINT_MS);
            last_tick = HAL_GetTick();
        }
    }
    consume_button();
}
