#ifndef H_DS18B20
#define H_DS18B20
/*
 * stm32 usart based ds18b20 lib
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

#define NUMBER_OF_DEVICES  0   // if there are multiple devices, this number set here

// Rom commands
#define SEARCH_ROM      0xf0
#define READ_ROM        0x33
#define MATCH_ROM       0xf0
#define SKIP_ROM        0xf0
#define ALARM_SEARCH    0xf0

// Function commands
#define CONVERT_T           0x44
#define READ_SCRATCHPAD     0xbe
#define WRITE_SCRATCHPAD    0x4e
#define COPY_SCRATCHPAD     0x48
#define RECALL_E2           0xb8
#define READ_POWER_SUPPLY   0xb4

// mcu related
#define DS_PORT GPIOA
#define RX_PIN  GPIO3
#define TX_PIN  GPIO2

// usart related
#define BAUD9600    0x1388
#define BAUT115200  0x01A1

#endif
