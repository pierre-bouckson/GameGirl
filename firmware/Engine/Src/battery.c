#include "main.h"
#include "battery.h"

/* Valeur de VREFINT mesurée en usine avec VDDA = 3,3 V (datasheet STM32F429, tableau 74) */
#define VREFINT_CAL     (*(const uint16_t *)0x1FFF7A2AUL)
#define VREFINT_CAL_MV  3300

#define NB_SAMPLES      32   /* lectures du buffer DMA moyennées par mesure */

/* Courbe de décharge typique d'une Li-ion 1S (mV, %), du plus chargé au plus vide */
static const struct { uint16_t mv; uint8_t pct; } curve[] = {
    {4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80}, {3980, 75},
    {3950,  70}, {3910, 65}, {3870, 60}, {3850, 55}, {3840, 50}, {3820, 45},
    {3800,  40}, {3790, 35}, {3770, 30}, {3750, 25}, {3730, 20}, {3710, 15},
    {3690,  10}, {3610,  5}, {3300,  0},
};
#define CURVE_LEN (sizeof curve / sizeof curve[0])

static uint32_t filtered_mv = 0;   /* 0 = pas encore de mesure */

/* Une mesure brute : moyenne de NB_SAMPLES lectures du buffer DMA, convertie en mV */
static uint32_t measure_mv(void)
{
  uint32_t sum_bat = 0, sum_ref = 0;

  for (uint8_t i = 0; i < NB_SAMPLES; i++)
  {
    sum_bat += adc_values[ADC_IDX_VBAT];
    sum_ref += adc_values[ADC_IDX_VREFINT];
    /* Une séquence de 4 conversions dure ~120 µs : on attend un peu pour
     * moyenner des séquences différentes et pas 32 fois la même valeur */
    for (volatile uint32_t d = 0; d < 1000; d++);
  }

  if (sum_ref == 0)
    return 0;

  /* VDDA = 3300 * CAL / ref, et V_pin = raw * VDDA / 4095, donc :
   * V_pin = raw * 3300 * CAL / (ref * 4095) ; x2 pour le pont diviseur */
  uint64_t mv = (uint64_t)sum_bat * VREFINT_CAL_MV * VREFINT_CAL * 2;
  return (uint32_t)(mv / ((uint64_t)sum_ref * 4095));
}

uint16_t battery_mv(void)
{
  uint32_t mv = measure_mv();

  /* Moyenne glissante (1/4) : le pourcentage ne saute pas quand le courant varie */
  if (filtered_mv == 0)
    filtered_mv = mv;
  else
    filtered_mv = (filtered_mv * 3 + mv) / 4;

  return (uint16_t)filtered_mv;
}

bool battery_on_usb(void)
{
  if (filtered_mv == 0)
    battery_mv();
  return filtered_mv >= BATTERY_USB_MV;
}

uint8_t battery_percent(void)
{
  if (filtered_mv == 0)
    battery_mv();

  uint32_t mv = filtered_mv;

  if (mv >= curve[0].mv)
    return 100;

  for (uint8_t i = 1; i < CURVE_LEN; i++)
  {
    if (mv >= curve[i].mv)
    {
      /* Interpolation linéaire entre les deux points qui encadrent la tension */
      uint32_t span_mv  = curve[i - 1].mv - curve[i].mv;
      uint32_t span_pct = curve[i - 1].pct - curve[i].pct;
      return curve[i].pct + (mv - curve[i].mv) * span_pct / span_mv;
    }
  }
  return 0;
}
