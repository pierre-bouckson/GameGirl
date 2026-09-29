#ifndef __INPUT_H
#define __INPUT_H

typedef joy_mv 
{
    int x;
    int y;
} joy_mv_t;

joy_mv_t get_joy();

#endif /* __INPUT_H */