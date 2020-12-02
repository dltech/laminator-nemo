#ifndef H_MYSTM
#define H_MYSTM

#include "inttypes.h"

#define SCL 9
#define SDA 10

#define I2C_ADD_NE_ADD 0x00
#define TIMEOUT_CLKS    10000

void clkInit(void);
void rough_delay_us(uint16_t us);
void delay_s(uint16_t s);
void f0I2cInit(void);
void f0I2cSend(uint8_t addr, uint8_t *data, uint8_t nBytes);

#endif
