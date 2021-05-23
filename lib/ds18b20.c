/*
 * stm32 timer based ds18b20 lib
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

#include "ds18b20.h"
#include "mystmbackend.h"
#include "../libopencm3/include/libopencm3/stm32/rcc.h"
#include "../libopencm3/include/libopencm3/stm32/gpio.h"
#include "../libopencm3/include/libopencm3/stm32/timer.h"

volatile uint32_t timeoutGl;

// init subfunctions
void dsPortConfig(void);
void dsTimerConfig(void);
// timer related functions
void dsTimerUpdate(void);
void dsTimerStart(void);
uint8_t dsTimeout(void);
int dsElapsedTime(void);
void dsDelayUs(uint16_t us);
// GPIO related functions
uint8_t dsReadPin(void);
void dsSetPin(void);
void dsResetPin(void);
// DS18B20 relateg functions
int dsInitSequence(void);
uint8_t dsTxBit(uint8_t bit);
uint8_t dsTxByte(uint8_t byte);
uint8_t dsCrc(uint8_t *data, uint8_t size);
int32_t dsTransTemp(uint8_t templ, uint8_t temph, uint8_t conf);
// commands
int dsReadRomCmd(void);
int dsWriteScratchpad(uint16_t Tpar, uint8_t configByte);


// Hardware related functions (backend).

void dsPortConfig()
{
    RCC_AHBENR    |= RCC_AHBENR_GPIOAEN;
    GPIOA_MODER   |= GPIO_MODE_OUTPUT << (DS_PIN*2);
    GPIOA_OTYPER  |= GPIO_OTYPE_OD << (DS_PIN);
    GPIOA_OSPEEDR |= GPIO_OSPEED_100MHZ << (DS_PIN*2);
    GPIOA_PUPDR   |= GPIO_PUPD_PULLUP << (DS_PIN*2);
    GPIOA_BSRR    |= 1 << DS_PIN;
}

void dsTimerConfig()
{
    // timer for tacts
    RCC_APB2ENR |= RCC_APB2ENR_TIM16EN;
    TIM16_CR1    = (uint32_t) TIM_CR1_CKD_CK_INT;
    TIM16_CR1   |= (uint32_t) TIM_CR1_CEN;
    dsTimerUpdate();
}

void dsTimerUpdate()
{
    TIM16_PSC   = (uint32_t) 47;
    TIM16_ARR   = (uint32_t) 65535;
    TIM16_EGR   |= TIM_EGR_UG;
}

void dsTimerStart()
{
    timeoutGl  = 1e6;
    TIM16_EGR |= TIM_EGR_UG;
}

int dsElapsedTime()
{
    if(--timeoutGl < 5) {
        return 65000;
    }
    return TIM16_CNT;
}

void dsDelayUs(uint16_t us)
{
    TIM16_EGR |= TIM_EGR_UG;
    timeoutGl = 1e6;
    while((TIM16_CNT < us) && (--timeoutGl > 1));
}

uint8_t dsReadPin()
{
    if((GPIOA_IDR & (1 << DS_PIN)) > 0) {
        return 1;
    }
    return 0;
}

void dsSetPin()
{
    GPIOA_BSRR |= 1 << DS_PIN;
}

void dsResetPin()
{
    GPIOA_BRR |= 1 << DS_PIN;
}

uint8_t dsTxBit(uint8_t bit)
{
    uint8_t ret = 0;
    dsResetPin();
    if(bit == 0) {
        dsDelayUs(WRITE0T);
        dsSetPin();
        dsDelayUs(PULSE_DELAY);
        return 0;
    }
    dsDelayUs(WRITE1T1);
    dsSetPin();
    dsTimerStart();
    dsDelayUs(PULSE_DELAY);
    while( (dsElapsedTime() < TIMESLOT) && (dsReadPin() == 0) );
    if(dsElapsedTime() > READT) {
        ret = 0;
    } else {
        ret = 1;
    }
    while(dsElapsedTime() < WRITE1T2);
    return ret;
}

uint8_t dsTxByte(uint8_t byte)
{
    uint8_t ret = 0;
    for(int i=0 ; i<8 ; ++i) {
        if( dsTxBit(byte & (1<<i)) == 1 ) {
            ret |= 1<<i;
        }
    }
    return ret;
}

// DS18B20 related functions based of low-level presented above

void dsInit()
{
    dsPortConfig();
    dsTimerConfig();
    rough_delay_us(100);
    dsWriteScratchpad(0x7ff, DEFAULT_RESOL);
    dsStart();
}

// returns 0 if device is found
int dsInitSequence()
{
    dsResetPin();
    dsDelayUs(RESET_PULSE);
    dsSetPin();
    dsDelayUs(RESET_WAIT);
    dsTimerStart();
    while((dsReadPin() == 0) && (dsElapsedTime() < INIT_PULSE_MAX));
    uint16_t time = dsElapsedTime();
    if( (time > INIT_PULSE_MAX) || (time < INIT_PULSE_MIN) ) {
        return -1;
    }
    while( dsElapsedTime() < RESET_PULSE );
    return 0;
}

// This function calculates the cumulative Maxim 1-Wire CRC of the message.
// The result will be 0 if ok. The table was obtained from maximintegrated.com.
uint8_t dsCrc(uint8_t *data, uint8_t size)
{
    const uint8_t table[256] =
        {0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
        157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
        35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
        190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
        70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
        219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
        101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
        248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
        140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
        17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
        175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
        50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
        202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
        87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
        233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
        116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};
    uint8_t crc = 0;

    for(int i=0 ; i<size ; ++i) {
        crc = table[crc ^ data[i]];
    }
    return crc;
}

int32_t dsTransTemp(uint8_t templ, uint8_t temph, uint8_t conf)
{
    uint32_t tempRaw = ((uint32_t)templ + ((uint32_t)temph << 8)) & 0x07ff;
    int32_t sign = 1;
    if( (temph & 0x80) > 0 ) {
        sign = -1;
    }
    switch(conf)
    {
        case RESOL_9BIT  :
            return (int32_t)((tempRaw >> 3) * 500) * sign;
            break;
        case RESOL_10BIT :
            return (int32_t)((tempRaw >> 2) * 250) * sign;
            break;
        case RESOL_11BIT :
            return (int32_t)((tempRaw >> 1) * 125) * sign;
            break;
        case RESOL_12BIT :
            return (int32_t)((tempRaw * 625)/10) * sign;
            break;
    }
    return -1;
}

// commands

int dsReadRomCmd()
{
    uint8_t buffer[7];
    int ret;
    if( dsInitSequence() < 0 ) {
        return -1;
    }
    dsTxByte(READ_ROM);
    buffer[0] = dsTxByte(0xff);
    if( buffer[0] != FAMILY_CODE ) {
        return -1;
    }
    // ds ROM CODE not needed
    for(int i=0 ; i<6 ; ++i) {
        buffer[i+1] = dsTxByte(0xff);
    }
    uint8_t crc = dsTxByte(0xff);
    if( crc != dsCrc(buffer,7) ) {
        return -1;
    }
    return 0;
}

// reads scratchpad, returns temperature only, multiplied by 1000
int32_t dsReadScratchpad()
{
    dsTimerUpdate();
    if( dsReadRomCmd() < 0 ) {
        return -1;
    }
    dsTxByte(READ_SCRATCHPAD);
    uint8_t scratchpad[9];
    uint8_t crc;
    for(int i=0 ; i<9 ; ++i) {
        scratchpad[i] = dsTxByte(0xff);
    }
    if( scratchpad[8] != dsCrc(scratchpad, 8) ) {
        return -1;
    }
    return dsTransTemp(scratchpad[0], scratchpad[1], scratchpad[4]);
}

int dsWriteScratchpad(uint16_t Tpar, uint8_t configByte)
{
    if( dsReadRomCmd() < 0 ) {
        return -1;
    }
    dsTxByte(WRITE_SCRATCHPAD);
    dsTxByte((uint8_t)((Tpar >> 8) & 0x00ff));
    dsTxByte((uint8_t)(Tpar & 0x00ff));
    dsTxByte(configByte);
    if( dsReadRomCmd() < 0 ) {
        return -1;
    }
    dsTxByte(COPY_SCRATCHPAD);
    uint32_t timeout = 1e6;
    while( (dsTxBit(1) == 0) && (--timeout > 0) );
    if(timeout < 2) {
        return -1;
    }
    return 0;
}

int dsStart()
{
    dsTimerUpdate();
    if( dsReadRomCmd() < 0 ) {
        return -1;
    }
    dsTxByte(CONVERT_T);
    return 0;
}

int32_t tempBlocking()
{
    dsTimerUpdate();
    uint32_t timeout = 1e6;
    if( dsStart() < 0 ) {
        return -1;
    }
    while( (dsTxBit(1) == 0) && (--timeout > 0) );
    if(timeout > 0) {
        return dsReadScratchpad();
    }
    return -1;
}
