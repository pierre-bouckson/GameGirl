#include "main.h"
#include "easy.h"

/* Secteur 17 (128 Ko) : 18 à 23 sont pris par le buzzer, le rétroéclairage et
 * les records. Le programme (~60 Ko) reste loin, dans la banque 1. */
#define EASY_ADDR    0x08120000UL
#define EASY_SECTOR  FLASH_SECTOR_17

static bool easy = false;

void easy_init(void)
{
    easy = (*(volatile uint32_t *)EASY_ADDR == 1);   /* flash effacée = mode normal */
}

void easy_set(bool on)
{
    easy = on;
}

bool easy_mode(void)
{
    return easy;
}

void easy_save(void)
{
    uint32_t stored = *(volatile uint32_t *)EASY_ADDR;
    if ((stored == 1) == easy)
        return;

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = EASY_SECTOR,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
    };
    uint32_t err;

    HAL_FLASH_Unlock();
    if (HAL_FLASHEx_Erase(&erase, &err) == HAL_OK)
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, EASY_ADDR, easy ? 1 : 0);
    HAL_FLASH_Lock();
}
