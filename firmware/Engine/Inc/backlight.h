#ifndef __BACKLIGHT_H
#define __BACKLIGHT_H

#include <stdint.h>

/* Rétroéclairage : PB4 = TIM3_CH1 en PWM 20 kHz sur l'entrée CTRL du TPS61165.
 * La luminosité va de BACKLIGHT_MIN à BACKLIGHT_MAX (rapport cyclique = niveau x 10 %).
 * Le niveau 0 n'existe pas : CTRL à 0 V éteindrait complètement l'écran. */
#define BACKLIGHT_MIN 1
#define BACKLIGHT_MAX 10

/* Démarre le PWM avec le niveau sauvegardé en flash (BACKLIGHT_MAX si jamais réglé) */
void backlight_init(void);

/* Change la luminosité tout de suite (borne à MIN..MAX), sans sauvegarder */
void backlight_set(uint8_t level);

uint8_t backlight_get(void);

/* Éteint complètement le rétroéclairage (avant la mise en veille) */
void backlight_off(void);

/* Écrit le niveau courant en flash (storage.c) s'il a changé */
void backlight_save(void);

#endif /* __BACKLIGHT_H */
