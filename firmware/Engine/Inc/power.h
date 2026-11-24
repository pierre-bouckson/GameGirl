#ifndef __POWER_H
#define __POWER_H

#include <stdint.h>

/* Gestion de l'énergie, pour ne pas laisser une Li-ion se vider à fond quand
 * la console est oubliée allumée :
 *
 *  - batterie faible : à POWER_LOW_PCT %, message « batterie faible » une
 *    fois (réarmé si on remonte à POWER_LOW_REARM_PCT % ou sur USB) ;
 *  - batterie vide : sous POWER_CUTOFF_MV pendant POWER_CUTOFF_CHECKS
 *    mesures de suite, message « recharge-moi » puis mise en veille ;
 *  - inactivité : sans action (joystick, A, START) pendant POWER_IDLE_MS,
 *    compte à rebours de POWER_IDLE_WARN_MS (une action l'annule) puis veille.
 *
 * Veille = rétroéclairage coupé, ILI9341 en sleep, MCU en mode Standby
 * (quelques µA). Le LDO, la SDRAM et l'écran restent alimentés : ça ne remplace
 * pas un interrupteur, mais la consommation tombe fortement. Le bouton A (PA0 =
 * broche WKUP) réveille la console, qui redémarre comme à la mise sous tension.
 *
 * Les messages s'affichent par-dessus le jeu : les deux framebuffers sont
 * sauvegardés puis restaurés (display_overlay_begin/end), le jeu reprend où il
 * en était si on annule la veille. */

#define POWER_LOW_PCT        10
#define POWER_LOW_REARM_PCT  15
#define POWER_CUTOFF_MV      3400                 /* ~1 % sur la courbe de battery.c */
#define POWER_BOOT_MIN_MV    (POWER_CUTOFF_MV + 50) /* hystérésis au démarrage, sans charge */
#define POWER_CUTOFF_CHECKS  5                    /* mesures de suite sous le seuil */
#define POWER_CHECK_MS       1000                 /* une mesure de batterie par seconde */
#define POWER_IDLE_MS        (5UL * 60 * 1000)    /* veille après 5 min sans action */
#define POWER_IDLE_WARN_MS   10000                /* dont 10 s de compte à rebours */
#define POWER_LOW_MSG_MS     4000                 /* durée du message batterie faible */

/* À appeler une fois dans main.c après display_init() : remet à zéro le
 * réveil, et renvoie en veille si la batterie est déjà vide au démarrage */
void power_init(void);

/* Appelée chaque milliseconde par HAL_IncTick : repère les actions du joueur */
void power_tick(void);

/* Appelée par display_swap() et HAL_Delay() : surveille la batterie et
 * l'inactivité, affiche les messages et met en veille. Ne revient pas si la
 * console part en veille. */
void power_poll(void);

#endif /* __POWER_H */
