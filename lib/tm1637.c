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

#include "tm1637.h"
#include "../libopencm3/include/libopencm3/stm32/rcc.h"
#include "../libopencm3/include/libopencm3/stm32/gpio.h"
#include "../libopencm3/include/libopencm3/stm32/timer.h"

// little init functions as parts of main init function
void tmPortInit(void);
void tmDelayInit(void);
void tmDelayUpdate(void);
// timer based accurate delays
void quaterTact(void);
void halfTact(void);
// data transmission
int pushByte(uint8_t byte);
int tipoI2cBlockingTx1(uint8_t data);
int tipoI2cBlockingTx(uint8_t *data, uint8_t size);

/* display only, no keys reads */
void tmInit()
{
    tmPortInit();
    tmDelayInit();
    uint8_t data[DISPLAY_SIZE] = {};
    tmUpd(data);
    setBrightness(100);
}

void tmPortInit()
{
    RCC_AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA_MODER |= GPIO_MODE_OUTPUT << (TM_SCL*2) \
                 | GPIO_MODE_OUTPUT << (TM_SDA*2);
    GPIOA_OTYPER |= GPIO_OTYPE_OD << (TM_SCL) \
                  | GPIO_OTYPE_OD << (TM_SDA);
    GPIOA_OSPEEDR |= GPIO_OSPEED_100MHZ << (TM_SCL*2) \
                  | GPIO_OSPEED_100MHZ << (TM_SDA*2);
    GPIOA_PUPDR |= GPIO_PUPD_PULLUP << (TM_SCL*2) \
                 | GPIO_PUPD_PULLUP << (TM_SDA*2);
    GPIOA_BSRR  |= (1 << TM_SDA) | (1 << TM_SCL);
}

void tmDelayInit()
{
    // timer for tacts
    RCC_APB2ENR |= RCC_APB2ENR_TIM16EN;
    TIM16_CR1   = (uint32_t) TIM_CR1_CKD_CK_INT;
    TIM16_CR1  |= (uint32_t) TIM_CR1_CEN;
    tmDelayUpdate();
}

void tmDelayUpdate()
{
    TIM16_PSC   = (uint32_t) 63;
    TIM16_ARR   = (uint32_t) 1;
    TIM16_EGR   |= TIM_EGR_UG;
}

void quaterTact()
{
    uint32_t timeout = 1e7;
    TIM16_EGR  |= TIM_EGR_UG;
    TIM16_SR = 0;
    while( (TIM16_SR == 0) && (--timeout > 0) );
}

void halfTact()
{
    quaterTact();
    quaterTact();
}

int pushByte(uint8_t byte)
{
    int ret;
    for(int i=0 ; i<8 ; ++i)
    {
        GPIOA_BRR |= 1 << TM_SCL;
        quaterTact();
        if((byte & (1<<i)) > 0) {
            GPIOA_BSRR  |= 1 << TM_SDA;
        } else {
            GPIOA_BRR |= 1 << TM_SDA;
        }
        quaterTact();
        GPIOA_BSRR |= 1 << TM_SCL;
        halfTact();
    }
    // ack tact
    GPIOA_BRR |= 1 << TM_SCL;
    quaterTact();
    GPIOA_BSRR |= 1 << TM_SDA;
    quaterTact();
    GPIOA_BSRR  |= 1 << TM_SCL;
    quaterTact();
    if((GPIOA_IDR & (1 << TM_SDA)) > 0) {
        ret = -1;
    } else {
        ret = 0;
    }
    quaterTact();
    return ret;
}

// transmit to tm only one byte
int tipoI2cBlockingTx1(uint8_t data)
{
    int err = 0;
    GPIOA_BSRR |= (1 << TM_SDA) | (1 << TM_SCL);
    halfTact();
    halfTact();
    // start condition
    GPIOA_BRR |= 1 << TM_SDA;
    halfTact();
    // clock data
    err = pushByte(data);
    // stop bit
    GPIOA_BRR |= 1 << TM_SDA;
    GPIOA_BRR |= 1 << TM_SCL;
    halfTact();
    GPIOA_BSRR |= 1 << TM_SCL;
    halfTact();
    GPIOA_BSRR |= (1 << TM_SDA) | (1 << TM_SCL);
    return err;
}

// transmit to tm array of bytes
int tipoI2cBlockingTx(uint8_t *data, uint8_t size)
{
    int err = 0;
    GPIOA_BSRR |= (1 << TM_SDA) | (1 << TM_SCL);
    halfTact();
    halfTact();
    // start condition
    GPIOA_BRR |= 1 << TM_SDA;
    halfTact();
    // clock data
    for(int i=0 ; i<size ; ++i) {
        err += pushByte(data[i]);
    }
    // stop bit
    GPIOA_BRR |= 1 << TM_SDA;
    GPIOA_BRR |= 1 << TM_SCL;
    halfTact();
    GPIOA_BSRR |= 1 << TM_SCL;
    halfTact();
    GPIOA_BSRR |= (1 << TM_SDA) | (1 << TM_SCL);
    return err;
}

void tmUpd(uint8_t *data)
{
    tmDelayUpdate();
    tipoI2cBlockingTx1(WRITE_DATA_CMD_AVT);
    uint8_t cnt = 0;
    uint8_t tx[DISPLAY_SIZE+1];
    tx[cnt++] = SET_0H;
    for(int i=0 ; i<DISPLAY_SIZE ; ++i) {
        tx[cnt++] = data[i];
    }
    tipoI2cBlockingTx(tx, cnt);
}

void setBrightness(uint8_t percent)
{
    tmDelayUpdate();
    uint8_t reg = BRIGHT6P;
    if(percent == 0) {
        reg = DISP_OFF;
    } else if(percent <= 6) {
        reg = BRIGHT6P;
    } else if(percent <= 13) {
        reg = BRIGHT13P;
    } else if(percent <= 25) {
        reg = BRIGHT25P;
    } else if(percent <= 63) {
        reg = BRIGHT63P;
    } else if(percent <= 69) {
        reg = BRIGHT69P;
    } else if(percent <= 75) {
        reg = BRIGHT75P;
    } else if(percent <= 81 ) {
        reg = BRIGHT81P;
    } else if(percent <= 100) {
        reg = BRIGHT88P;
    }
    tipoI2cBlockingTx1(reg);
}
