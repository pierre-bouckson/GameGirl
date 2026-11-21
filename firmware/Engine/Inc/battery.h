#ifndef __BATTERY_H
#define __BATTERY_H

#include <stdint.h>
#include <stdbool.h>

/* Mesure de la batterie : PA1 (ADC1_IN1) lit VBAT_SENSE = VSW / 2 (pont R7/R8 de 100 kΩ).
 * Sur batterie, VSW ≈ VBAT. Sur USB, VSW ≈ VBUS - diode D1 ≈ 4,6-4,8 V, au-dessus de
 * la tension max d'une Li-ion : c'est comme ça qu'on détecte l'USB.
 * VDDA est recalculé à chaque mesure avec VREFINT, donc la mesure reste juste même si
 * le 3V3 n'est pas exactement à 3,3 V. */

/* Au-dessus de ce seuil, on considère que la console est sur USB (en charge) */
#define BATTERY_USB_MV 4350

/* Tension filtrée sur VSW en mV ; à appeler régulièrement (moyenne glissante) */
uint16_t battery_mv(void);

/* Pourcentage 0..100 estimé sur la courbe de décharge d'une Li-ion */
uint8_t battery_percent(void);

/* Vrai si l'USB est branché (le pourcentage n'a alors pas de sens) */
bool battery_on_usb(void);

#endif /* __BATTERY_H */
