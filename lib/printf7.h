#ifndef H_DISPLAY
#define H_DISPLAY

#include <stdint.h>
#include "tm1637.h"

// schematic depended (not necessarily segment A fits byte 0)
#define SEGA  (1 << 0)
#define SEGB  (1 << 2)
#define SEGC  (1 << 4)
#define SEGD  (1 << 6)
#define SEGE  (1 << 7)
#define SEGF  (1 << 1)
#define SEGG  (1 << 3)
#define SEGDP (1 << 5)

#define DIGITS DISPLAY_SIZE

void myprintf(char *format, ... );

#endif
