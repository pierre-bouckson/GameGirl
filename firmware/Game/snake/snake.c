#include <stdlib.h>
#include "buzzer.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "snake.h"
#include "input.h"
#include "display.h"
#include "font8x8.h"
#include "main.h"
#include "easy.h"


static int tab[30][30] = {[15][15] = -1};   /* static : main.c a déjà un tab global */

/* Grille de grid x grid cases = 240x240 px, sous le titre : 30x30 cases de
 * 8 px, ou 15x15 cases de 16 px en mode facile (seul le coin de tab[][] sert) */
#define S_PX        240
#define S_Y0        (5 * FONT_H)
#define S_EMPTY     RGB565(20, 20, 30)
#define S_BODY      COLOR_DARKGREEN
#define S_HEAD      COLOR_GREEN
#define S_BONUS     COLOR_RED

#define BONUS       6677   /* valeur d'une case bonus dans tab[][] */

static uint32_t len_snake = 0;   /* score : nombre de bonus mangés */
static uint32_t best      = 0;   /* meilleur score, gardé en flash */
static int      grid      = 30;  /* côté de la grille en cases */

/* ---- Meilleur score en flash -------------------------------------------- */

/* Avant-dernier secteur (128 Ko) : le 23 est déjà pris par 2048 */
#define BEST_ADDR    0x081C0000UL
#define BEST_SECTOR  FLASH_SECTOR_22

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

/* Dessine tab[][] (ligne i, colonne j) et l'affiche (display_swap) */
void print_game(void)
{
    display_clear(COLOR_BLACK);
    display_draw_string((LCD_W - 9 * FONT_W) / 2, 2 * FONT_H, "= SNAKE =", COLOR_GREEN, COLOR_BLACK);

    int cell = S_PX / grid;
    for (int i = 0; i < grid; i++)
    {
        for (int j = 0; j < grid; j++)
        {
            uint16_t color;
            if (tab[i][j] == BONUS)   color = S_BONUS;   /* testé avant > 0 */
            else if (tab[i][j] > 0)   color = S_BODY;    /* corps */
            else if (tab[i][j] == -1) color = S_HEAD;    /* tête */
            else                      color = S_EMPTY;   /* vide */
            for (int y = 0; y < cell; y += FONT_H)
                for (int x = 0; x < cell; x += FONT_W)
                    display_draw_char(j * cell + x, S_Y0 + i * cell + y, ' ', color, color);
        }
    }

    /* Score et record sous la grille (lignes 35 à 39 libres) */
    char line[16];
    snprintf(line, sizeof line, "%lu", (unsigned long)len_snake);
    display_draw_string(FONT_W, 36 * FONT_H, "SCORE", COLOR_GRAY, COLOR_BLACK);
    display_draw_string(8 * FONT_W, 36 * FONT_H, line, COLOR_WHITE, COLOR_BLACK);

    if (easy_mode())
    {
        display_draw_string(FONT_W, 38 * FONT_H, "MODE FACILE", EASY_COLOR, COLOR_BLACK);
    }
    else
    {
        snprintf(line, sizeof line, "%lu", (unsigned long)((len_snake > best) ? len_snake : best));
        display_draw_string(FONT_W, 38 * FONT_H, "BEST", COLOR_GRAY, COLOR_BLACK);
        display_draw_string(8 * FONT_W, 38 * FONT_H, line, COLOR_YELLOW, COLOR_BLACK);
    }

    display_swap();
}

void bonus_add()
{
    int count = 0;
    for(int i = 0; i<grid;i++)
    {
        for(int j = 0; j<grid; j++)
        {
            if(tab[i][j] == 0)
            {
                count++;
            }
        }
    }
    if (count == 0) return;   /* plus de case vide : pas de bonus */
    int rand_number = rand() % count;
    count = 0;
    for(int i = 0; i<grid;i++)
    {
        for(int j = 0; j<grid; j++)
        {
            if(tab[i][j] == 0)
            {
                if(count == rand_number) tab[i][j] = BONUS;
                count++;
            }

        }
    }
}

/* Écran de fin : game over et/ou nouveau record */
static void end_screen(bool game_over, bool record)
{
    char line[16];
    snprintf(line, sizeof line, "%lu", (unsigned long)len_snake);

    display_clear(COLOR_BLACK);
    if (game_over)
        display_draw_string((LCD_W - 9 * FONT_W) / 2, 13 * FONT_H, "GAME OVER", COLOR_RED, COLOR_BLACK);
    if (record)
        display_draw_string((LCD_W - 16 * FONT_W) / 2, 16 * FONT_H, "Nouveau record !", COLOR_YELLOW, COLOR_BLACK);
    display_draw_string((LCD_W - 5 * FONT_W) / 2, 19 * FONT_H, "SCORE", COLOR_GRAY, COLOR_BLACK);
    display_draw_string((LCD_W - strlen(line) * FONT_W) / 2, 21 * FONT_H, line, COLOR_WHITE, COLOR_BLACK);
    if (game_over)
        display_draw_string((LCD_W - 13 * FONT_W) / 2, 26 * FONT_H, "bouton : menu", COLOR_GRAY, COLOR_BLACK);
    display_swap();
}


void game()
{
    srand(HAL_GetTick());

    bool easy = easy_mode();
    grid = easy ? SNAKE_EASY_GRID : 30;

    /* Grille remise à zéro à chaque partie, tête au centre */
    memset(tab, 0, sizeof tab);
    tab[grid / 2][grid / 2] = -1;

    bonus_add();

    joy_mv_t mouv = {-1,0};
    joy_mv_t applied = mouv;   /* direction du dernier pas */
    uint32_t period     = easy ? SNAKE_EASY_PERIOD_MS     : PERIOD_MS;
    uint32_t period_min = easy ? SNAKE_EASY_PERIOD_MIN_MS : PERIOD_MIN_MS;
    uint32_t accel      = easy ? SNAKE_EASY_PERIOD_ACCEL  : PERIOD_ACCEL;
    uint32_t last_tick = HAL_GetTick();

    len_snake = 0;
    best = best_read();

    bool game_over = false;

    while(!button_pressed && !game_over)
    {
        /* Lit le joystick en continu pendant l'attente : la dernière
         * direction valide est appliquée dès le prochain pas */
        while (HAL_GetTick() - last_tick < period)
        {
            joy_mv_t joy = selection_joy();
            /* Comparé au dernier pas joué, pas à la dernière direction lue :
             * haut puis gauche avant le pas suivant reste un demi-tour */
            bool reverse = (joy.x == -applied.x && joy.y == -applied.y);
            if((joy.x == 0) != (joy.y == 0)    /* un seul axe : pas de diagonale */
               && !(easy && reverse && len_snake > 0))   /* facile : pas de demi-tour mortel */
                mouv = joy;
        }
        last_tick = HAL_GetTick();
        applied = mouv;

        /* Trouve la tête et vieillit le corps */
        int hi = 0, hj = 0;
        for(int i = 0; i<grid;i++)
        {
            for(int j = 0; j<grid; j++)
            {
                if(tab[i][j] == -1)
                {
                    hi = i;
                    hj = j;
                }
                else if(tab[i][j] > 0 && tab[i][j] != BONUS)
                {
                    tab[i][j]--;
                }
            }
        }

        /* Déplace la tête une seule fois ; sortie de la grille = game over,
         * sauf en mode facile où le serpent ressort de l'autre côté */
        int ni = hi - mouv.x;   /* x = -1 -> bas, comme dans le menu */
        int nj = hj + mouv.y;   /* y =  1 -> droite */
        if(easy)
        {
            ni = (ni + grid) % grid;
            nj = (nj + grid) % grid;
        }
        if(ni < 0 || ni >= grid || nj < 0 || nj >= grid)
        {
            game_over = true;
            break;
        }
        if(tab[ni][nj] > 0 && tab[ni][nj] != 6677)
        {
            game_over = true;
            break;
        }
        bool eaten = (tab[ni][nj] == BONUS);
        if(eaten)
        {
            len_snake++;   /* mange le bonus : le serpent grandit */
            buzzer_play(SND_POINT);

            /* Accélère de 3 % par bonus : ~22 bonus pour atteindre le plancher */
            period = period * accel / 100;
            if(period < period_min) period = period_min;
        }
        tab[ni][nj] = -1;
        tab[hi][hj] = len_snake;
        if(eaten)
            bonus_add();   /* un seul bonus à la fois : le suivant apparaît dès qu'il est mangé */
        print_game();   /* affiche le pas qui vient d'être calculé */
    }

    if (game_over)
    {
        buzzer_play(SND_LOSE);
        HAL_Delay(1000);   /* laisse voir la position finale */
    }

    bool record = !easy && len_snake > best;   /* pas de record en mode facile */
    if (record)
        buzzer_play(SND_WIN);
    if (game_over || record)
        end_screen(game_over, record);
    if (record)
        best_write(len_snake);   /* l'écran reste figé pendant l'effacement */

    if (game_over)
    {
        button_pressed = 0;   /* ignore les appuis faits pendant la partie */
        while (!button_pressed) HAL_Delay(1);   /* HAL_Delay : START reste actif */
    }
    button_pressed = 0;
}
