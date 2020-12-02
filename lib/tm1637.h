#ifndef H_TM1637
#define H_TM1637

#include "inttypes.h"

#define SCL 9
#define SDA 10

#define DISPLAY_SIZE        3

// data command
#define WRITE_DATA_CMD_AVT  0x70
#define WRITE_DATA_CMD_FIX  0x74
#define READ_KEY_DATA_CMD   0x72
// address command
#define SET_0H              0xf0
#define SET_1H              0xf1
#define SET_2H              0xf2
#define SET_3H              0xf3
#define SET_4H              0xf4
#define SET_5H              0xf5
#define MAXDIGITS           6
// display control
#define DISP_OFF            0xb0
#define BRIGHT6P            0xb8
#define BRIGHT13P           0xb9
#define BRIGHT25P           0xba
#define BRIGHT63P           0xbb
#define BRIGHT69P           0xbc
#define BRIGHT75P           0xbd
#define BRIGHT81P           0xbe
#define BRIGHT88P           0xbf

void tmInit(void);
void setBrightness(uint8_t percent);
void tmUpd(uint8_t *data);

#endif
