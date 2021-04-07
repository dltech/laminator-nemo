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

#include "../libopencm3/include/libopencm3/stm32/rcc.h"
#include "../libopencm3/include/libopencm3/stm32/usart.h"

// Hardware related functions (backend).

void dsPortConfig()
{
    RCC_AHBENR    |= RCC_AHBENR_GPIOAEN;
    GPIOA_MODER   |= GPIO_MODE_AF << (TX_PIN*2) \
                   | GPIO_MODE_AF << (RX_PIN*2);
    GPIOA_OTYPER  |= GPIO_OTYPE_OD << (TX_PIN);
    GPIOA_OSPEEDR |= GPIO_OSPEED_100MHZ << (TX_PIN*2) \
                   | GPIO_OSPEED_100MHZ << (RX_PIN*2);
    GPIOA_PUPDR   |= GPIO_PUPD_PULLUP << (TX_PIN*2) \
                   | GPIO_PUPD_PULLUP << (RX_PIN*2);
                   GPIOA_AFRH |= GPIO_AF4 << ((SCL-8)*4) \
                               | GPIO_AF4 << ((SDA-8)*4);
    GPIOA_AFRL    |= GPIO_AF1 << ((TX_PIN-8)*4) \
                   | GPIO_AF1 << ((RX_PIN-8)*4);
}

void dsUsartConfig()
{
    RCC_APB2ENR |= RCC_APB2ENR_USART1EN;
    USART1_CR1 &= ~(USART_CR1_M0);  // 8-bit len
    USART1_CR1 &= ~(USART_CR1_PCE); // off parity
    USART1_CR2 |= USART_CR2_STOPBITS_1 // stop bits
    USART1_CR1 &= ~(USART_CR1_OVER8);
    USART1_BRR |= BAUD9600;
    USART1_CR1 |= USART_CR1_UE;
}

uint8_t dsUsartTxSingleByte(uint8_t byte)
{
    uint32_t timeout = 1e6;
    uint8_t ret = 0xff;
    USART1_CR1 |= USART_CR1_RE;
    USART1_CR1 |= USART_CR1_TE;
    USART_TDR = byte;
    while(((USART1_ISR & USART_ISR_TC) == 0) || (--timeout > 0));
    if( USART1_ISR & USART_ISR_RXNE ){
        ret = USART1_RDR;
    }
    USART1_CR1 &= ~USART_CR1_TE;
    USART1_CR1 &= ~USART_CR1_RE;
    return ret;
}


// DS18B20 related functions based of low-level presented above

void dsInit()
{
    dsPortConfig();
    dsUsartConfig();
    dsInitSequence();
}

// returns 0 if device is found
int dsInitSequence()
{
    const uint8_t init_word = 0x0f;
    const uint8_t presence_bit = 1 << 6;
    USART1_BRR |= BAUD9600;
    uint8_t ret = dsUsartTxSingleByte(init_word);
    USART1_BRR |= BAUD115200;
    if( (ret & presence_bit == 0) ) {
        return 0;
    } else {
        return -1;
    }
}


// This function calculates the cumulative Maxim 1-Wire CRC of the message.
// The result will be 0 if ok. The table was obtained from maximintegrated.com.
uint8_t dsCrc(uint8_t data, uint8_t size)
{
    const uint8_t table[255] =
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
    uint8_t crc;

    for(int i=0 ; i<size ; ++i) {
        crc = table[crc ^ data[i]];
    }
    return crc;
}

void dsSendCmd(uint8_t cmd)
{
    USART_TDR |=

}

void dsFoundDev()
{

}

void dsRomCycle(uint64_t romCode)
{
    int isSuccess = 0;
    dsInitSequence();
    if(isOne) {
        dsSendCmd(READ_ROM);
    } else {
        dsSendCmd(SEARCH_ROM);
    }
    return isSuccess;
}


int dsInitStandalone()
{
    hardwareInit();
    if(NDEVICE == 1) {
        return;
    } else {
        dsFoundDev();
    }
}


int dsInitMultiple()
{

}
