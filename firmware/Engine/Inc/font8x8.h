#ifndef __FONT8X8_H
#define __FONT8X8_H

#include <stdint.h>

#define FONT_W          8
#define FONT_H          8
#define FONT_FIRST_CHAR 0x20   /* ' ' */
#define FONT_LAST_CHAR  0x7F   /* DEL */

/* Police 8x8 : 1 octet par ligne, bit 0 = pixel le plus à gauche.
 * Index = caractère ASCII - FONT_FIRST_CHAR. */
extern const uint8_t font8x8[FONT_LAST_CHAR - FONT_FIRST_CHAR + 1][FONT_H];

#endif /* __FONT8X8_H */
