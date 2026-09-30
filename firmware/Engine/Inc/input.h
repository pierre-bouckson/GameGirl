#ifndef __INPUT_H
#define __INPUT_H

#include <stdint.h>

typedef struct joy_mv
{
    int x;
    int y;
} joy_mv_t;

joy_mv_t get_joy(uint16_t x, uint16_t y);

joy_mv_t selection_joy(uint16_t x, uint16_t y);

#endif /* __INPUT_H */