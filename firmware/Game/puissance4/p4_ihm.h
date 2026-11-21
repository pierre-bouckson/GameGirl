#ifndef __P4_IHM_H
#define __P4_IHM_H

/* Valeur retournée par p4_ihm_choose_line() quand le bouton est appuyé */
#define P4_SELECT 10

/* Oublie les appuis et mouvements en cours (à appeler en entrant dans le jeu) */
void p4_ihm_init(void);

/* Bloque jusqu'à une action : -1 = gauche, 1 = droite (joystick), P4_SELECT = bouton PA0 */
int p4_ihm_choose_line(void);

/* Bloque jusqu'à un appui sur le bouton PA0 */
void p4_ihm_wait_button(void);

#endif /* __P4_IHM_H */
