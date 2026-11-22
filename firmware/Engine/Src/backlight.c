#include "main.h"
#include "backlight.h"

extern TIM_HandleTypeDef htim3;

/* Secteur 19 (128 Ko) : 20 à 23 sont déjà pris par Flappy, Tetris, Snake et 2048 */
#define LEVEL_ADDR    0x08160000UL
#define LEVEL_SECTOR  FLASH_SECTOR_19

static uint8_t level = BACKLIGHT_MAX;

static uint8_t clamp(int v)
{
  if (v < BACKLIGHT_MIN) return BACKLIGHT_MIN;
  if (v > BACKLIGHT_MAX) return BACKLIGHT_MAX;
  return (uint8_t)v;
}

static uint8_t level_read(void)
{
  uint32_t v = *(volatile uint32_t *)LEVEL_ADDR;
  if (v < BACKLIGHT_MIN || v > BACKLIGHT_MAX)   /* flash effacée (0xFFFFFFFF) = jamais réglé */
    return BACKLIGHT_MAX;
  return (uint8_t)v;
}

void backlight_set(uint8_t new_level)
{
  level = clamp(new_level);

  /* Période = ARR + 1 ; CCR = période => 100 % */
  uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3) + 1;
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, period * level / BACKLIGHT_MAX);
}

uint8_t backlight_get(void)
{
  return level;
}

void backlight_init(void)
{
  /* Le TPS61165 passe en mode PWM tant que CTRL ne reste jamais bas plus de
   * ~260 µs au démarrage (sinon il entre en mode EasyScale) : à 20 kHz, la
   * phase basse dure au plus 45 µs, donc pas de risque. */
  backlight_set(level_read());
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void backlight_save(void)
{
  if (level_read() == level)
    return;

  FLASH_EraseInitTypeDef erase = {
      .TypeErase    = FLASH_TYPEERASE_SECTORS,
      .Sector       = LEVEL_SECTOR,
      .NbSectors    = 1,
      .VoltageRange = FLASH_VOLTAGE_RANGE_3,   /* alim 2.7-3.6 V : écriture par mots de 32 bits */
  };
  uint32_t err;

  HAL_FLASH_Unlock();
  if (HAL_FLASHEx_Erase(&erase, &err) == HAL_OK)
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, LEVEL_ADDR, level);
  HAL_FLASH_Lock();
}

void backlight_off(void)
{
  /* CTRL bas plus de 2,5 ms : le TPS61165 s'arrête. En Standby, PB4 passe en
   * haute impédance et c'est le pull-down interne du TPS61165 qui le garde bas. */
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}
