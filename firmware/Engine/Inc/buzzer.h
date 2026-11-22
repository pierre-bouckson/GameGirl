#ifndef __BUZZER_H
#define __BUZZER_H

#include <stdint.h>
#include <stdbool.h>

/* Buzzer magnétique passif BZ1 (MLT-8530, résonance ~2,7 kHz) piloté par le
 * N-MOS Q2 sur PB14 = TIM12_CH1 (AF9). Le PWM à 50 % donne la note ; un
 * rapport cyclique nul coupe le son.
 *
 * Tout est non bloquant : buzzer_play() lance une mélodie que buzzer_tick()
 * fait avancer chaque milliseconde depuis le SysTick (HAL_IncTick, buzzer.c). */

typedef struct
{
    uint16_t freq;   /* Hz, 0 = silence */
    uint16_t ms;     /* durée ; une note de 0 ms termine la mélodie */
} note_t;

/* Configure PB14 et TIM12 ; à appeler une fois dans main.c */
void buzzer_init(void);

/* Lance une mélodie (tableau terminé par {0, 0}) en coupant celle en cours */
void buzzer_play(const note_t *melody);

/* Joue une seule note */
void buzzer_tone(uint16_t freq, uint16_t ms);

void buzzer_stop(void);

/* Mode muet : les mélodies avancent toujours, mais sans son. buzzer_init()
 * relit l'état sauvegardé en flash (secteur 18), buzzer_save() l'y écrit
 * seulement s'il a changé (bloque ~1 à 2 s). */
void buzzer_set_mute(bool mute);
bool buzzer_muted(void);
void buzzer_save(void);

/* Appelée chaque milliseconde par HAL_IncTick */
void buzzer_tick(void);

/* Sons communs aux jeux */
extern const note_t SND_BOOT[];     /* démarrage de la console */
extern const note_t SND_MOVE[];     /* déplacement d'un curseur / d'une pièce */
extern const note_t SND_SELECT[];   /* validation, lancement d'un jeu */
extern const note_t SND_BACK[];     /* retour au menu */
extern const note_t SND_HIT[];      /* rebond, battement d'ailes */
extern const note_t SND_POINT[];    /* point marqué, bonus mangé */
extern const note_t SND_LOSE[];     /* point perdu, game over */
extern const note_t SND_WIN[];      /* victoire, record */

#endif /* __BUZZER_H */
