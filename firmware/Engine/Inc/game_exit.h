#ifndef __GAME_EXIT_H
#define __GAME_EXIT_H

/* Bouton START (PE4, actif haut, pull-down 10 kΩ sur le PCB) : quitte le jeu
 * en cours et revient au menu, quel que soit l'endroit où le jeu se trouve.
 *
 * game_run() pose un point de retour (setjmp) puis lance le jeu. L'EXTI4 ne
 * fait que lever une demande ; elle est honorée au prochain HAL_Delay() ou
 * display_swap() du jeu, qui appellent game_exit_poll() et font un longjmp
 * vers game_run(). Toute boucle d'attente d'un jeu doit donc passer par l'un
 * des deux (ex. `while (!button_pressed) HAL_Delay(1);`). */

/* Configure PE4 en EXTI front montant ; à appeler une fois dans main.c */
void game_exit_init(void);

/* Lance un jeu bloquant ; revient à la fin du jeu ou sur appui de START */
void game_run(void (*entry)(void));

/* Appelée par HAL_GPIO_EXTI_Callback sur PE4 */
void game_exit_request(void);

/* Quitte le jeu si START a été pressé (sans effet hors d'un jeu) */
void game_exit_poll(void);

#endif /* __GAME_EXIT_H */
