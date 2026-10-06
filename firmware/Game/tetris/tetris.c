#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tetris.h"
#include "display.h"
#include "font8x8.h"
#include "input.h"

/* tab[x][y] : x = colonne (0 à gauche), y = ligne (0 en bas, la pièce descend en y-1).
 * 0 = vide, > 0 = bloc posé, < 0 = bloc de la pièce qui tombe.
 * |valeur| - 1 = forme (shape_id_t), qui donne la couleur.
 * static : main.c a déjà un tab global */
static int tab[BOARD_W][BOARD_H] = {0};

/* Pièce qui tombe : sa forme et le coin haut-gauche de sa grille de forme dans tab
 * (box_x = colonne, box_y = ligne de la 1re ligne de la forme). Tenus à jour par
 * spawn_entity(), fall() et mouv_piece(), utilisés par turn_piece(). */
static int cur_shape = 0;
static int box_x = 0;
static int box_y = 0;

static uint32_t score = 0;
static uint32_t best  = 0;    /* meilleur score, gardé en flash */
static uint32_t lines = 0;    /* lignes effacées depuis le début de la partie */
static uint32_t level = 0;    /* lines / LINES_PER_LEVEL */
static bool game_over = false;

/* Points gagnés selon le nombre de lignes effacées d'un coup (0 à 4), x (niveau + 1) */
static const uint16_t line_points[SHAPE_SIZE + 1] = {0, 100, 300, 500, 800};

/* Sac de 7 : les 7 formes mélangées, tirées une par une, puis on remélange.
 * Chaque forme sort exactement une fois tous les 7 tirages. */
static uint8_t bag[NB_SHAPES];
static int     bag_left = 0;     /* formes restant dans le sac */
static int     next_shape = 0;   /* pièce suivante, affichée dans le panneau */

/* ---- Meilleur score en flash -------------------------------------------- */

/* Secteur 21 (128 Ko) : 22 et 23 sont déjà pris par Snake et 2048 */
#define BEST_ADDR    0x081A0000UL
#define BEST_SECTOR  FLASH_SECTOR_21

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

/* Une case du plateau = 2x2 caractères (16x16 px) : 10x20 cases = 160x320 px,
 * toute la hauteur de l'écran. Mur à gauche (colonne 0) et à droite (colonne 21),
 * panneau d'infos sur les colonnes 22 à 29. */
#define T_X0        1                     /* 1re colonne de caractères du plateau */
#define T_WALL_R    (T_X0 + 2 * BOARD_W)  /* colonne du mur droit */
#define T_PANEL     (T_WALL_R + 1)        /* 1re colonne du panneau */
#define T_EMPTY     RGB565(20, 20, 30)
#define T_GRID      RGB565(40, 40, 55)    /* point au centre des cases vides */
#define T_LINE      RGB565(40, 40, 55)    /* traits entre les cases vides */
#define T_WALL      RGB565(60, 70, 120)

/* Couleur de chaque forme, dans l'ordre de shape_id_t (I, O, T, S, Z, J, L) */
static const uint16_t piece_color[NB_SHAPES] = {
    RGB565(60, 220, 230),  RGB565(255, 215, 60), RGB565(180, 80, 240),
    RGB565(80, 220, 100),  RGB565(230, 60, 60),  RGB565(40, 90, 220),
    RGB565(255, 160, 60)};

/* Couleur deux fois plus sombre (chaque composante RGB565 divisée par 2) */
static uint16_t darker(uint16_t c)
{
    return (c >> 1) & 0x7BEF;
}

/* Bloc plein de size x size px (multiple de 8), avec un liseré sombre de 1 px
 * en bas et à droite : sépare les blocs voisins de même couleur */
static void draw_block(uint16_t px, uint16_t py, uint16_t size, uint16_t c)
{
    for (uint16_t dy = 0; dy < size; dy += FONT_H)
        for (uint16_t dx = 0; dx < size; dx += FONT_W)
            display_draw_char(px + dx, py + dy, ' ', c, c);

    for (uint16_t k = 0; k < size; k++)
    {
        display_draw_pixel(px + k, py + size - 1, darker(c));
        display_draw_pixel(px + size - 1, py + k, darker(c));
    }
}

/* Libellé gris puis valeur blanche en dessous, dans le panneau de droite */
static void draw_stat(uint8_t row, const char *label, uint32_t value, uint16_t color)
{
    char line[16];
    snprintf(line, sizeof line, "%lu", (unsigned long)value);
    display_draw_string((T_PANEL + 1) * FONT_W, row * FONT_H, label, COLOR_GRAY, COLOR_BLACK);
    display_draw_string((T_PANEL + 1) * FONT_W, (row + 1) * FONT_H, line, color, COLOR_BLACK);
}

/* Dessine le plateau tab[][] et l'affiche (display_swap) */
void print_tab(void)
{
    display_clear(COLOR_BLACK);

    /* Murs sur toute la hauteur */
    for (int row = 0; row < LCD_H / FONT_H; row++)
    {
        display_draw_char(0, row * FONT_H, '|', T_WALL, COLOR_BLACK);
        display_draw_char(T_WALL_R * FONT_W, row * FONT_H, '|', T_WALL, COLOR_BLACK);
    }

    for (int x = 0; x < BOARD_W; x++)
    {
        for (int y = 0; y < BOARD_H; y++)
        {
            uint16_t px  = (T_X0 + 2 * x) * FONT_W;
            uint16_t py  = (BOARD_H - 1 - y) * 2 * FONT_H;   /* y = 0 en bas de l'écran */
            int      v   = tab[x][y];

            if (v != 0)
            {
                draw_block(px, py, 2 * FONT_W, piece_color[abs(v) - 1]);
            }
            else
            {
                /* Case vide : fond sombre, point au centre et traits de grille */
                display_draw_char(px,          py,          ' ', T_EMPTY, T_EMPTY);
                display_draw_char(px + FONT_W, py,          ' ', T_EMPTY, T_EMPTY);
                display_draw_char(px,          py + FONT_H, ' ', T_EMPTY, T_EMPTY);
                display_draw_char(px + FONT_W, py + FONT_H, ' ', T_EMPTY, T_EMPTY);

                /* Point de 2x2 px pile au centre de la case de 16x16 px
                 * (le '.' de la police est en bas du caractère, donc décentré) */
                display_draw_pixel(px + FONT_W - 1, py + FONT_H - 1, T_GRID);
                display_draw_pixel(px + FONT_W,     py + FONT_H - 1, T_GRID);
                display_draw_pixel(px + FONT_W - 1, py + FONT_H,     T_GRID);
                display_draw_pixel(px + FONT_W,     py + FONT_H,     T_GRID);

                /* Traits de 1 px en bas et à droite : avec ceux des cases voisines,
                 * ils forment la grille (comme le liseré des blocs pleins) */
                for (int k = 0; k < 2 * FONT_W; k++)
                {
                    display_draw_pixel(px + k, py + 2 * FONT_H - 1, T_LINE);
                    display_draw_pixel(px + 2 * FONT_W - 1, py + k, T_LINE);
                }
            }
        }
    }

    /* Panneau d'infos : titre lettre par lettre aux couleurs des pièces */
    const char *title = "TETRIS";
    for (int i = 0; title[i]; i++)
        display_draw_char((T_PANEL + 1 + i) * FONT_W, 2 * FONT_H, title[i],
                          piece_color[i % NB_SHAPES], COLOR_BLACK);

    draw_stat(5,  "SCORE", score, COLOR_WHITE);
    draw_stat(8,  "LEVEL", level, COLOR_WHITE);
    draw_stat(11, "LINES", lines, COLOR_WHITE);
    draw_stat(14, "BEST",  (score > best) ? score : best, COLOR_YELLOW);

    /* Pièce suivante : sa grille 4x4, 1 caractère (8 px) par case */
    display_draw_string((T_PANEL + 1) * FONT_W, 18 * FONT_H, "NEXT", COLOR_GRAY, COLOR_BLACK);
    for (int i = 0; i < SHAPE_SIZE; i++)
        for (int j = 0; j < SHAPE_SIZE; j++)
            if (shapes[next_shape][i][j] == 1)
                draw_block((T_PANEL + 2 + j) * FONT_W, (20 + i) * FONT_H, FONT_W,
                           piece_color[next_shape]);

    display_swap();
}

/* Tire la forme suivante dans le sac de 7, remélangé quand il est vide */
static int draw_from_bag(void)
{
    if(bag_left == 0)
    {
        /* Mélange de Fisher-Yates : chaque ordre a la même probabilité */
        for(int i = 0; i<NB_SHAPES; i++)
        {
            bag[i] = i;
        }
        for(int i = NB_SHAPES - 1; i > 0; i--)
        {
            int k = rand() % (i + 1);
            uint8_t t = bag[i];
            bag[i] = bag[k];
            bag[k] = t;
        }
        bag_left = NB_SHAPES;
    }
    return bag[--bag_left];
}

/* Fait apparaître la pièce suivante, en haut et centrée, et en tire une nouvelle.
 * Renvoie false si la place est déjà prise (game over). */
static bool spawn_entity(void)
{
    int shape = next_shape;
    int x0 = (BOARD_W - SHAPE_SIZE) / 2;   /* colonnes 3 à 6 */

    /* shapes[forme][ligne][colonne] : la ligne 0 de la forme va tout en haut */
    for(int i = 0; i<SHAPE_SIZE;i++)
    {
        for(int j = 0; j<SHAPE_SIZE; j++)
        {
            if(shapes[shape][i][j] == 1 && tab[x0 + j][BOARD_H - 1 - i] != 0)
            {
                return false;
            }
        }
    }

    for(int i = 0; i<SHAPE_SIZE;i++)
    {
        for(int j = 0; j<SHAPE_SIZE; j++)
        {
            if(shapes[shape][i][j] == 1)
            {
                tab[x0 + j][BOARD_H - 1 - i] = -(shape + 1);
            }
        }
    }
    next_shape = draw_from_bag();
    cur_shape = shape;
    box_x = x0;
    box_y = BOARD_H - 1;
    return true;
}

/* true si toutes les cases de la pièce qui tombe peuvent descendre d'une ligne */
static bool can_fall(void)
{
    for(int x = 0; x<BOARD_W;x++)
    {
        for(int y = 0; y<BOARD_H; y++)
        {
            if(tab[x][y] < 0)
            {
                if(y == 0 || tab[x][y-1] > 0)   /* sol ou bloc posé dessous */
                {
                    return false;
                }
            }
        }
    }
    return true;
}

/* Descend toute la pièce d'une ligne (à n'appeler que si can_fall()) */
static void fall(void)
{
    /* De bas en haut : la case du dessous a déjà été libérée */
    for(int x = 0; x<BOARD_W;x++)
    {
        for(int y = 1; y<BOARD_H; y++)
        {
            if(tab[x][y] < 0)
            {
                tab[x][y-1] = tab[x][y];
                tab[x][y] = 0;
            }
        }
    }
    box_y--;
}

/* Pose la pièce : ses cases deviennent des blocs fixes */
static void land(void)
{
    for(int x = 0; x<BOARD_W;x++)
    {
        for(int y = 0; y<BOARD_H; y++)
        {
            if(tab[x][y] < 0)
            {
                tab[x][y] = -tab[x][y];
            }
        }
    }
}

/* Efface les lignes pleines, fait descendre celles du dessus et ajoute les points */
static void clear_lines(void)
{
    int cleared = 0;

    for(int y = 0; y<BOARD_H; y++)
    {
        bool full = true;
        for(int x = 0; x<BOARD_W;x++)
        {
            if(tab[x][y] == 0)
            {
                full = false;
                break;
            }
        }
        if(!full) continue;

        for(int x = 0; x<BOARD_W;x++)
        {
            for(int yy = y; yy<BOARD_H - 1; yy++)
            {
                tab[x][yy] = tab[x][yy+1];
            }
            tab[x][BOARD_H - 1] = 0;
        }
        cleared++;
        y--;   /* la ligne du dessus est descendue ici : la revérifier */
    }

    score += line_points[cleared] * (level + 1);
    lines += cleared;
    level = lines / LINES_PER_LEVEL;
}

/* Période de chute du niveau courant : -15 % par niveau, avec un plancher */
static uint32_t fall_period(void)
{
    uint32_t p = TETRIS_PERIOD_MS;
    for(uint32_t l = 0; l < level && p > TETRIS_PERIOD_MIN_MS; l++)
    {
        p = p * TETRIS_ACCEL / 100;
    }
    return (p < TETRIS_PERIOD_MIN_MS) ? TETRIS_PERIOD_MIN_MS : p;
}

void mouv_piece(joy_mv_t mv)
{
    /* Pas de déplacement latéral : sans ce test, tab[x+0] = tab[x] puis
     * tab[x] = 0 effacerait la pièce */
    if(mv.y == 0)
    {
        return;
    }

    /* Si une seule case de la pièce sortirait du plateau ou tomberait sur un bloc
     * posé, on ne bouge rien. Le test des bords passe en premier : avec ||, tab[]
     * n'est lu que si x + mv.y est dans le plateau. */
    for(int x = 0; x<BOARD_W;x++)
    {
        for(int y = 0; y<BOARD_H; y++)
        {
            if(tab[x][y] < 0 && (x + mv.y < 0 || x + mv.y >= BOARD_W || tab[x + mv.y][y] > 0))
            {
                return;
            }
        }
    }

    /* Parcours dans le sens opposé au mouvement : une case déplacée ne doit pas
     * être retrouvée plus loin dans la boucle et redéplacée (vers la droite,
     * on part de la colonne 9 ; vers la gauche, de la colonne 0) */
    for(int a = 0; a<BOARD_W;a++)
    {
        int x = (mv.y > 0) ? BOARD_W - 1 - a : a;
        for(int y = 0; y<BOARD_H; y++)
        {
            if(tab[x][y] < 0)
            {
                tab[x+mv.y][y] = tab[x][y];
                tab[x][y] = 0;
            }
        }
    }
    box_x += mv.y;
}

/* true si la forme r (n x n), posée avec son coin haut-gauche en (bx, box_y),
 * reste dans le plateau sans toucher de bloc posé */
static bool fits(uint8_t r[SHAPE_SIZE][SHAPE_SIZE], int n, int bx)
{
    for(int i = 0; i<n; i++)
    {
        for(int j = 0; j<n; j++)
        {
            if(r[i][j] == 0) continue;

            int x = bx + j;
            int y = box_y - i;
            if(x < 0 || x >= BOARD_W || y < 0 || y >= BOARD_H || tab[x][y] > 0)
            {
                return false;
            }
        }
    }
    return true;
}

/* Tourne la pièce d'un quart de tour dans le sens horaire, dans sa grille de forme
 * (4x4 pour le I, 3x3 pour les autres ; le O ne tourne pas). Si la place est prise,
 * essaie de la décaler d'1 puis 2 colonnes (contre un mur) ; sinon ne fait rien. */
static void turn_piece(void)
{
    if(cur_shape == SHAPE_O)
    {
        return;
    }
    int n = (cur_shape == SHAPE_I) ? 4 : 3;

    /* Lit la pièce dans tab : ligne i de la forme = ligne box_y - i du plateau */
    uint8_t m[SHAPE_SIZE][SHAPE_SIZE] = {{0}};
    for(int i = 0; i<n; i++)
    {
        for(int j = 0; j<n; j++)
        {
            int x = box_x + j;
            int y = box_y - i;
            if(x >= 0 && x < BOARD_W && y >= 0 && y < BOARD_H && tab[x][y] < 0)
            {
                m[i][j] = 1;
            }
        }
    }

    /* Quart de tour horaire : la colonne de gauche devient la ligne du haut */
    uint8_t r[SHAPE_SIZE][SHAPE_SIZE] = {{0}};
    for(int i = 0; i<n; i++)
    {
        for(int j = 0; j<n; j++)
        {
            r[i][j] = m[n - 1 - j][i];
        }
    }

    static const int kicks[] = {0, -1, 1, -2, 2};   /* décalages essayés, en colonnes */
    for(int k = 0; k < (int)(sizeof kicks / sizeof kicks[0]); k++)
    {
        int bx = box_x + kicks[k];
        if(!fits(r, n, bx)) continue;

        /* Efface l'ancienne position puis écrit la pièce tournée */
        for(int x = 0; x<BOARD_W;x++)
        {
            for(int y = 0; y<BOARD_H; y++)
            {
                if(tab[x][y] < 0)
                {
                    tab[x][y] = 0;
                }
            }
        }
        for(int i = 0; i<n; i++)
        {
            for(int j = 0; j<n; j++)
            {
                if(r[i][j] == 1)
                {
                    tab[bx + j][box_y - i] = -(cur_shape + 1);
                }
            }
        }
        box_x = bx;
        return;
    }
}

/* Écran de fin : game over, score, niveau et nouveau record éventuel */
static void end_screen(bool record)
{
    char line[40];   /* "LEVEL n  LINES n" avec des nombres 32 bits : jusqu'à 35 caractères */

    display_clear(COLOR_BLACK);
    display_draw_string((LCD_W - 9 * FONT_W) / 2, 11 * FONT_H, "GAME OVER", COLOR_RED, COLOR_BLACK);
    if (record)
        display_draw_string((LCD_W - 16 * FONT_W) / 2, 14 * FONT_H, "Nouveau record !", COLOR_YELLOW, COLOR_BLACK);

    snprintf(line, sizeof line, "%lu", (unsigned long)score);
    display_draw_string((LCD_W - 5 * FONT_W) / 2, 17 * FONT_H, "SCORE", COLOR_GRAY, COLOR_BLACK);
    display_draw_string((LCD_W - strlen(line) * FONT_W) / 2, 19 * FONT_H, line, COLOR_WHITE, COLOR_BLACK);

    snprintf(line, sizeof line, "LEVEL %lu  LINES %lu", (unsigned long)level, (unsigned long)lines);
    display_draw_string((LCD_W - strlen(line) * FONT_W) / 2, 22 * FONT_H, line, COLOR_GRAY, COLOR_BLACK);

    display_draw_string((LCD_W - 13 * FONT_W) / 2, 27 * FONT_H, "bouton : menu", COLOR_GRAY, COLOR_BLACK);
    display_swap();
}

void menu_tetris(void)
{
    srand(HAL_GetTick());

    /* Plateau et compteurs remis à zéro à chaque partie */
    memset(tab, 0, sizeof tab);
    score = 0;
    lines = 0;
    level = 0;
    best = best_read();
    bag_left = 0;
    next_shape = draw_from_bag();
    game_over = !spawn_entity();

    uint32_t period = fall_period();
    uint32_t last_tick = HAL_GetTick();
    bool drop_locked = false;   /* descente rapide bloquée jusqu'au relâchement du stick */

    button_pressed = 0;   /* l'appui qui a lancé le jeu ne doit pas tourner la pièce */
    print_tab();

    /* Le bouton sert à tourner la pièce : la partie ne s'arrête qu'au game over */
    while(!game_over)
    {
        while (HAL_GetTick() - last_tick < period)
        {
            /* Stick vers le bas (x = -1) : descente rapide. La période est relue à
             * chaque tour, donc pousser le stick écourte aussi l'attente en cours.
             * Après une pièce posée, il faut relâcher le stick pour la réactiver. */
            joy_mv_t joy = selection_joy();
            if(joy.x >= 0)
            {
                drop_locked = false;
            }
            period = (joy.x < 0 && !drop_locked) ? TETRIS_FAST_MS : fall_period();

            if(new_selection())
            {
                joy_mv_t mv = selection_joy();
                if(mv.x > 0)   /* stick vers le haut : rotation */
                {
                    turn_piece();
                }
                mouv_piece(mv);
                print_tab();
            }
            if(button_pressed)
            {
                turn_piece();
                button_pressed = 0;
                print_tab();
            }
        }
        last_tick = HAL_GetTick();


        if(can_fall())
        {
            fall();
        }
        else
        {
            land();
            clear_lines();

            /* Pièce posée : la suivante repart à l'allure normale du niveau */
            period = fall_period();
            drop_locked = true;
            game_over = !spawn_entity();
        }

        print_tab();
    }

    HAL_Delay(1000);   /* laisse voir la position finale */

    bool record = score > best;
    end_screen(record);
    if (record)
        best_write(score);   /* l'écran reste figé pendant l'effacement */

    button_pressed = 0;   /* ignore les appuis faits pendant la partie */
    while(!button_pressed);
    button_pressed = 0;
}
