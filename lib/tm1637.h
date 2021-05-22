#ifndef H_TM1637
#define H_TM1637

/*
 * printf for TM1637 display-driver based 7-segment displays
 * connected to STM32.
 *
 * Copyright 2021 Mikhail Belkin <dltech174@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "inttypes.h"

// ports
#define TM_SCL 10
#define TM_SDA 9

// how much digits on your display
#define DISPLAY_SIZE        3

// TM1637 registers
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

// Init the MCU and driver before transmit digits.
void tmInit(void);
// You can set brightness of the display in percent.
void setBrightness(uint8_t percent);
// Send new data (blocking api).
void tmUpd(uint8_t *data);

#endif
