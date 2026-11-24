#include <stdbool.h>
#include "main.h"
#include "storage.h"

/* Secteur 11 : dernier secteur de la banque 1, présent sur le ZIT6 et le ZGT6 */
#define SECTOR_ADDR   0x080E0000UL
#define SECTOR_SIZE   (128UL * 1024UL)
#define SECTOR_NUM    FLASH_SECTOR_11

/* Premier mot du secteur : marque un journal valide (sinon le secteur est
 * effacé avant usage, au cas où il contiendrait autre chose) */
#define MAGIC         0x47475354UL   /* "GGST" */

/* Enregistrement de 2 mots : la valeur est écrite d'abord, la clé ensuite.
 * Une coupure entre les deux laisse une clé effacée : l'enregistrement est
 * ignoré. La fin du journal est le premier emplacement entièrement effacé. */
typedef struct {
    uint32_t value;
    uint32_t key;
} record_t;

#define RECORDS     ((volatile record_t *)(SECTOR_ADDR + sizeof(record_t)))
#define NB_RECORDS  (SECTOR_SIZE / sizeof(record_t) - 1)

/* Taille de la flash en Ko, programmée en usine */
#define FLASH_SIZE_KB (*(volatile uint16_t *)FLASHSIZE_BASE)

static uint32_t cache[STORAGE_NB_KEYS];
static uint32_t next = 0;      /* index du premier emplacement libre */
static bool ready = false;

static bool program(uint32_t addr, uint32_t value)
{
    return HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, value) == HAL_OK;
}

/* Efface le secteur et réécrit le cache (bloque ~1 à 2 s) */
static void rewrite(void)
{
    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Sector       = SECTOR_NUM,
        .NbSectors    = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,   /* alim 2.7-3.6 V : écriture par mots de 32 bits */
    };
    uint32_t err;

    next = 0;
    HAL_FLASH_Unlock();
    if (HAL_FLASHEx_Erase(&erase, &err) == HAL_OK)
    {
        for (uint32_t k = 0; k < STORAGE_NB_KEYS; k++)
        {
            if (cache[k] == STORAGE_EMPTY)
                continue;
            uint32_t addr = (uint32_t)&RECORDS[next];
            program(addr, cache[k]);
            program(addr + 4, k);
            next++;
        }
        program(SECTOR_ADDR, MAGIC);   /* en dernier : journal complet */
    }
    HAL_FLASH_Lock();
}

/* Anciens emplacements (un secteur par valeur, banque 2) : n'existent que sur
 * les puces de 2 Mo. Repris une seule fois, au premier démarrage. */
static void migrate(void)
{
    static const uint32_t old_addr[STORAGE_NB_KEYS] = {
        [STORAGE_EASY]        = 0x08120000UL,   /* secteur 17 */
        [STORAGE_MUTE]        = 0x08140000UL,   /* secteur 18 */
        [STORAGE_BACKLIGHT]   = 0x08160000UL,   /* secteur 19 */
        [STORAGE_BEST_FLAPPY] = 0x08180000UL,   /* secteur 20 */
        [STORAGE_BEST_TETRIS] = 0x081A0000UL,   /* secteur 21 */
        [STORAGE_BEST_SNAKE]  = 0x081C0000UL,   /* secteur 22 */
        [STORAGE_BEST_2048]   = 0x081E0000UL,   /* secteur 23 */
    };
    if (FLASH_SIZE_KB < 2048)
        return;   /* lire la banque 2 absente provoquerait une faute */
    for (uint32_t k = 0; k < STORAGE_NB_KEYS; k++)
        cache[k] = *(volatile uint32_t *)old_addr[k];
}

static void load(void)
{
    ready = true;
    for (uint32_t k = 0; k < STORAGE_NB_KEYS; k++)
        cache[k] = STORAGE_EMPTY;

    if (*(volatile uint32_t *)SECTOR_ADDR != MAGIC)
    {
        migrate();
        rewrite();
        return;
    }

    for (next = 0; next < NB_RECORDS; next++)
    {
        uint32_t key   = RECORDS[next].key;
        uint32_t value = RECORDS[next].value;
        if (key == STORAGE_EMPTY && value == STORAGE_EMPTY)
            break;
        if (key < STORAGE_NB_KEYS)
            cache[key] = value;   /* le dernier enregistrement d'une clé gagne */
    }
}

uint32_t storage_get(storage_key_t key)
{
    if (!ready)
        load();
    return (key < STORAGE_NB_KEYS) ? cache[key] : STORAGE_EMPTY;
}

void storage_set(storage_key_t key, uint32_t value)
{
    if (!ready)
        load();
    if (key >= STORAGE_NB_KEYS || cache[key] == value)
        return;

    cache[key] = value;
    if (next >= NB_RECORDS)
    {
        rewrite();   /* journal plein : compactage */
        return;
    }

    uint32_t addr = (uint32_t)&RECORDS[next++];
    HAL_FLASH_Unlock();
    if (program(addr, value))
        program(addr + 4, key);
    HAL_FLASH_Lock();
}
