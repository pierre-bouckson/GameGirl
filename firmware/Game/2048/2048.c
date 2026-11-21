#include "2048.h"
#include <stdlib.h>
#include "display.h"
#include "font8x8.h"
#include <stdio.h>
#include "main.h"
#include <string.h>
#include <stdbool.h>

static int tab[4][4] = {0};   /* static : main.c a déjà un tab global */

static uint32_t score = 0;
static uint32_t best  = 0;    /* meilleur score, gardé en flash */

/* Cases issues d'une fusion au dernier coup (pour l'animation) */
static bool merged[4][4];

#define POP_MS  100   /* durée de l'effet "pop" sur les tuiles fusionnées */

static void draw_grid(bool pop);

/* ---- Meilleur score en flash -------------------------------------------- */

/* Dernier secteur de la flash (128 Ko), loin du programme */
#define BEST_ADDR    0x081E0000UL
#define BEST_SECTOR  FLASH_SECTOR_23

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


void init_grid()
{
    memset(tab, 0, sizeof tab);
}

void add_grid(int number)
{
    bool empty[4][4] = {{false}};
    int count = 0;
    for(int i = 0; i<4;i++)
    {
        for(int j = 0; j<4; j++)
        {
            if(tab[i][j]== 0) 
            {
                empty[i][j] = true;
                count++;
            }
        }
    }
    if(count == 0) return;   /* grille pleine : rand() % 0 planterait */
    int rand_number = rand() % count;


    int number_of_true = 0;

    for(int i = 0; i<4;i++)
    {
        for(int j = 0; j<4; j++)
        {
            if(empty[i][j]== true)
            {
                if(number_of_true == rand_number)
                {
                    tab[i][j] = number;
                    return;
                }
                number_of_true++;
            }
        }
    }
}

/* Pousse les tuiles dans la direction mv. Renvoie true si au moins une tuile
 * a bougé ou fusionné. */
bool table_shift(joy_mv_t mv)
{
    bool moved = false;
    int di = -mv.x;   /* sens du déplacement en ligne   (x = -1 -> bas)    */
    int dj = mv.y;    /* sens du déplacement en colonne (y =  1 -> droite) */
    memset(merged, 0, sizeof merged);   /* une tuile ne fusionne qu'une fois par coup */

    /* Commence par les cases les plus proches du bord d'arrivée */
    for(int a = 0; a < 4; a++)
    {
        int i = (di > 0) ? 3 - a : a;
        for(int b = 0; b < 4; b++)
        {
            int j = (dj > 0) ? 3 - b : b;
            if(tab[i][j] == 0) continue;

            /* Avance tant que la case suivante existe et est vide */
            int ni = i, nj = j;
            while(ni + di >= 0 && ni + di < 4 && nj + dj >= 0 && nj + dj < 4
                  && tab[ni + di][nj + dj] == 0)
            {
                ni += di;
                nj += dj;
            }

            /* Fusion avec la tuile suivante si elle a la même valeur */
            if(ni + di >= 0 && ni + di < 4 && nj + dj >= 0 && nj + dj < 4
               && tab[ni + di][nj + dj] == tab[i][j] && !merged[ni + di][nj + dj])
            {
                tab[ni + di][nj + dj] *= 2;
                score += tab[ni + di][nj + dj];   /* la fusion rapporte la nouvelle valeur */
                merged[ni + di][nj + dj] = true;
                tab[i][j] = 0;
                moved = true;
            }
            else if(ni != i || nj != j)
            {
                tab[ni][nj] = tab[i][j];
                tab[i][j] = 0;
                moved = true;
            }
        }
    }
    return moved;
}

/* Vrai s'il reste une case vide ou deux voisines égales à fusionner */
static bool can_move(void)
{
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            if(tab[i][j] == 0) return true;
            if(i < 3 && tab[i][j] == tab[i + 1][j]) return true;
            if(j < 3 && tab[i][j] == tab[i][j + 1]) return true;
        }
    }
    return false;
}

void new_number()
{
    int rand_number = (rand() % 10 == 0) ? 4 : 2;   /* 4 dans 10 % des cas, comme le jeu original */
    //printf("%d", rand_number);
    add_grid(rand_number);
}

/* Écran de fin : game over et/ou nouveau record */
static void end_screen(bool game_over, bool record)
{
    char line[16];
    snprintf(line, sizeof line, "%lu", (unsigned long)score);

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

void menu_2048()
{
    srand(HAL_GetTick());
    score = 0;
    best = best_read();
    init_grid();
    new_number();
    print_grid();

    bool game_over = false;
    while(!button_pressed && !game_over)
    {
        HAL_Delay(100);
        joy_mv_t mov = selection_joy();
        if(new_selection() && table_shift(mov))   /* nouvelle tuile seulement si la grille a changé */
        {
            /* Animation : les tuiles fusionnées grossissent un instant */
            draw_grid(true);
            display_swap();
            HAL_Delay(POP_MS);

            new_number();
            print_grid();
            game_over = !can_move();
        }
    }

    if (game_over)
        HAL_Delay(1500);   /* laisse voir la grille bloquée */

    bool record = score > best;
    if (game_over || record)
        end_screen(game_over, record);
    if (record)
        best_write(score);   /* l'écran reste figé pendant l'effacement */

    if (game_over)
    {
        button_pressed = 0;   /* ignore les appuis faits pendant la partie */
        while(!button_pressed);
    }
    button_pressed = 0;
}


/* ---- Affichage ---------------------------------------------------------- */

/* Grille ASCII : 4 cases de 6 x 5 caractères + bordures = 29 x 25 caractères */
#define G_CELL_W   6
#define G_CELL_H   5
#define G_W        (4 * (G_CELL_W + 1) + 1)
#define G_H        (4 * (G_CELL_H + 1) + 1)
#define G_X0       ((LCD_W - G_W * FONT_W) / 2)   /* en pixels, grille centrée */
#define G_Y0       (6 * FONT_H)
#define G_BORDER   COLOR_GRAY
#define G_EMPTY    RGB565(40, 40, 40)

/* Couleur de fond d'une case selon sa valeur */
static uint16_t tile_color(int v)
{
    switch (v)
    {
        case 0:    return G_EMPTY;
        case 2:    return RGB565(238, 228, 218);
        case 4:    return RGB565(237, 224, 200);
        case 8:    return RGB565(242, 177, 121);
        case 16:   return RGB565(245, 149, 99);
        case 32:   return RGB565(246, 124, 95);
        case 64:   return RGB565(246, 94, 59);
        case 128:  return RGB565(237, 207, 114);
        case 256:  return RGB565(237, 204, 97);
        case 512:  return RGB565(237, 200, 80);
        case 1024: return RGB565(237, 197, 63);
        case 2048: return RGB565(237, 194, 46);
        default:   return COLOR_PURPLE;
    }
}

static void put_char_g(int col, int row, char c, uint16_t fg, uint16_t bg)
{
    display_draw_char(G_X0 + col * FONT_W, G_Y0 + row * FONT_H, c, fg, bg);
}

/* Écrit la valeur v centrée sur la ligne du milieu de la case (i, j) */
static void draw_value(int i, int j, int v)
{
    char buf[8];
    int len = 0;
    for (int n = v; n > 0 && len < 7; n /= 10)
        buf[len++] = '0' + n % 10;

    uint16_t fg = (v <= 4) ? RGB565(119, 110, 101) : COLOR_WHITE;
    int col = j * (G_CELL_W + 1) + 1 + (G_CELL_W - len + 1) / 2;
    int row = i * (G_CELL_H + 1) + 1 + G_CELL_H / 2;
    for (int k = 0; k < len; k++)
        put_char_g(col + k, row, buf[len - 1 - k], fg, tile_color(v));
}

/* Dessine tout l'écran de jeu dans le back buffer, sans l'afficher.
 * pop = true : les tuiles fusionnées débordent d'un caractère sur leurs
 * bordures, ce qui les fait paraître plus grosses. */
static void draw_grid(bool pop)
{
    display_clear(COLOR_BLACK);

    /* Titre */
    display_draw_string((LCD_W - 8 * FONT_W) / 2, 2 * FONT_H, "= 2048 =", COLOR_YELLOW, COLOR_BLACK);

    for (int row = 0; row < G_H; row++)
    {
        for (int col = 0; col < G_W; col++)
        {
            int on_hline = (row % (G_CELL_H + 1)) == 0;
            int on_vline = (col % (G_CELL_W + 1)) == 0;

            if (on_hline && on_vline)
                put_char_g(col, row, '+', G_BORDER, COLOR_BLACK);
            else if (on_hline)
                put_char_g(col, row, '-', G_BORDER, COLOR_BLACK);
            else if (on_vline)
                put_char_g(col, row, '|', G_BORDER, COLOR_BLACK);
            else
            {
                int v = tab[row / (G_CELL_H + 1)][col / (G_CELL_W + 1)];
                put_char_g(col, row, ' ', COLOR_BLACK, tile_color(v));
            }
        }
    }

    /* Valeurs centrées dans chaque case */
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (tab[i][j] != 0)
                draw_value(i, j, tab[i][j]);
        }
    }

    /* Tuiles fusionnées agrandies : on recouvre aussi leur cadre */
    if (pop)
    {
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                if (!merged[i][j])
                    continue;
                for (int row = i * (G_CELL_H + 1); row <= (i + 1) * (G_CELL_H + 1); row++)
                    for (int col = j * (G_CELL_W + 1); col <= (j + 1) * (G_CELL_W + 1); col++)
                        put_char_g(col, row, ' ', COLOR_BLACK, tile_color(tab[i][j]));
                draw_value(i, j, tab[i][j]);
            }
        }
    }

    /* Score et record sous la grille */
    char line[16];
    snprintf(line, sizeof line, "%lu", (unsigned long)score);
    display_draw_string(G_X0, 33 * FONT_H, "SCORE", COLOR_GRAY, COLOR_BLACK);
    display_draw_string(G_X0 + 7 * FONT_W, 33 * FONT_H, line, COLOR_WHITE, COLOR_BLACK);

    snprintf(line, sizeof line, "%lu", (unsigned long)((score > best) ? score : best));
    display_draw_string(G_X0, 35 * FONT_H, "BEST", COLOR_GRAY, COLOR_BLACK);
    display_draw_string(G_X0 + 7 * FONT_W, 35 * FONT_H, line, COLOR_YELLOW, COLOR_BLACK);
}

void print_grid(void)
{
    draw_grid(false);
    display_swap();
}
