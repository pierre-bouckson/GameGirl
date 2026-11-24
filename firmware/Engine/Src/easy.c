#include "main.h"
#include "easy.h"
#include "storage.h"

static bool easy = false;

void easy_init(void)
{
    easy = (storage_get(STORAGE_EASY) == 1);   /* jamais écrit = mode normal */
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
    storage_set(STORAGE_EASY, easy ? 1 : 0);
}
