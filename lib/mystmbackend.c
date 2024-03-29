#include "mystmbackend.h"
#include "../libopencm3/include/libopencm3/stm32/i2c.h"
#include "../libopencm3/include/libopencm3/stm32/rcc.h"
#include "../libopencm3/include/libopencm3/stm32/gpio.h"


void clkInit()
{
    // не знаю зачем, на всякий случай
    RCC_CR  |= RCC_CR_HSION;
    RCC_CFGR2 = RCC_CFGR2_PREDIV_NODIV;
    // такирование от встроенных RC генераторов, SYS 48 МГц, AHB 48МГц, APB 48 МГц
    uint32_t timeout = TIMEOUT_CLKS;
    if ( (RCC_CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL )
    {
        RCC_CFGR &= ~RCC_CFGR_SW;
        while ( ((RCC_CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) || \
                (--timeout > 1) );
    }
    RCC_CFGR |= RCC_CFGR_PLLSRC_HSI_CLK_DIV2;
    RCC_CR &= (uint32_t)(~RCC_CR_PLLON);
    timeout = TIMEOUT_CLKS;
    while( ((RCC_CR & RCC_CR_PLLRDY) != 0) && (--timeout > 1) );
    RCC_CFGR &= ~RCC_CFGR_PLLMUL;
    RCC_CFGR |= RCC_CFGR_PLLMUL_MUL12;
    RCC_CR |= RCC_CR_PLLON;
    timeout = TIMEOUT_CLKS;
    while( ((RCC_CR & RCC_CR_PLLRDY) == 0) && (--timeout > 1) );
    RCC_CFGR |= (uint32_t) (RCC_CFGR_SW_PLL);
    timeout = TIMEOUT_CLKS;
    while ( ((RCC_CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) && (--timeout > 1) );
    RCC_CFGR |= (uint32_t) RCC_CFGR_HPRE_NODIV | RCC_CFGR_PPRE_NODIV;
    RCC_CSR  |= (uint32_t) RCC_CSR_LSION;
    timeout = TIMEOUT_CLKS;
    while ( ((RCC_CSR & RCC_CSR_LSIRDY) == 0 ) && (--timeout > 0) );
}

void delay_s(uint16_t s)
{
    uint16_t cnt = s*20;
    while(cnt-- > 0) rough_delay_us(50000);
}

void rough_delay_us(uint16_t us)
{
    // podbiral na glazok
    volatile uint32_t cnt = (uint32_t)us*(uint32_t)5;
    while(cnt-- > 0);
}

void delay_ms(uint16_t ms)
{
    while(ms-- > 0) rough_delay_us(1000);
}

void f0I2cInit()
{
    // rcc and ports
    RCC_CFGR3 &= ~RCC_CFGR3_I2C1SW;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;
    RCC_AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA_MODER |= GPIO_MODE_AF << (SCL*2) \
                 | GPIO_MODE_AF << (SDA*2);
    GPIOA_OTYPER |= GPIO_OTYPE_OD << (SCL) \
                  | GPIO_OTYPE_OD << (SDA);
    GPIOA_OSPEEDR |= GPIO_OSPEED_100MHZ << (SCL*2) \
                  | GPIO_OSPEED_100MHZ << (SDA*2);
    GPIOA_PUPDR |= GPIO_PUPD_PULLUP << (SCL*2) \
                 | GPIO_PUPD_PULLUP << (SDA*2);
    GPIOA_AFRH |= GPIO_AF4 << ((SCL-8)*4) \
                | GPIO_AF4 << ((SDA-8)*4);
    // reset
    I2C1_CR1 &= ~I2C_CR1_PE;
    // digital filter disabled, analogue enabled
//    I2C1_CR1 &= ~(0 << I2C_CR1_DNF_SHIFT);
    I2C1_CR1 &= ~I2C_CR1_ANFOFF;
    // clocks for 200k transfer
    I2C1_TIMINGR = (0x01) << I2C_TIMINGR_PRESC_SHIFT | \
                (0x04) << I2C_TIMINGR_SCLDEL_SHIFT | \
                (0x02) << I2C_TIMINGR_SDADEL_SHIFT | \
                (0xc3) << I2C_TIMINGR_SCLH_SHIFT | \
                (0xc7) << I2C_TIMINGR_SCLL_SHIFT;
    // I2C1_TIMINGR = (0x01) << I2C_TIMINGR_PRESC_SHIFT | \
    //                (0x02) << I2C_TIMINGR_SCLDEL_SHIFT | \
    //                (0x01) << I2C_TIMINGR_SDADEL_SHIFT | \
    //                (0x08) << I2C_TIMINGR_SCLH_SHIFT | \
    //                (0x0a) << I2C_TIMINGR_SCLL_SHIFT;
   I2C1_CR1 &= ~I2C_CR1_NOSTRETCH;
   I2C1_CR1 |=  I2C_CR1_PE;
}

volatile uint32_t isr[16] = {0xee, 0xee, 0xee, 0xee, \
                             0xee, 0xee, 0xee, 0xee, \
                             0xee, 0xee, 0xee, 0xee, \
                             0xee, 0xee, 0xee, 0xee};
volatile uint32_t tout[16] = {0xee, 0xee, 0xee, 0xee, \
                             0xee, 0xee, 0xee, 0xee, \
                             0xee, 0xee, 0xee, 0xee, \
                             0xee, 0xee, 0xee, 0xee};
volatile uint32_t cr2;

void f0I2cSend(uint8_t addr, uint8_t *data, uint8_t nBytes)
{
    I2C1_CR1 &= ~I2C_CR1_PE; // reset na vsyakiy sluchay
    rough_delay_us(10);
    I2C1_CR1 |=  I2C_CR1_PE;
    // empty 7-bit address
    I2C1_CR2 &= ~I2C_CR2_ADD10;
    I2C1_CR2 |= (addr << I2C_CR2_SADD_7BIT_SHIFT) & I2C_CR2_SADD_7BIT_MASK;
    // tak nado
    I2C1_CR2 |= I2C_CR2_AUTOEND;
//    I2C1_CR2 |= I2C_CR2_STOP;
    I2C1_CR2 &= ~I2C_CR2_RD_WRN; // write
    I2C1_CR2 |= (0x06 << I2C_CR2_NBYTES_SHIFT) & I2C_CR2_NBYTES_MASK;
    isr[nBytes+1] = I2C1_ISR;
    isr[15] = nBytes;
    I2C1_CR2 |= I2C_CR2_START;

    cr2 = I2C1_CR2;
    uint32_t timeout = 1e6;
    for(int i=0 ; i<nBytes ; ++i) {
        timeout = 1e6;
        while( ((I2C1_ISR & (I2C_ISR_NACKF + I2C_ISR_TXIS)) == 0) && (--timeout > 0) );
        isr[i] = I2C1_ISR;
        tout[i] = timeout;
        if((I2C1_ISR & I2C_ISR_NACKF) > 0)  {
            return;
        }
        I2C1_TXDR = data[i];
    }

    timeout = 1e6;
    while( ((I2C1_ISR & (I2C_ISR_NACKF + I2C_ISR_TXIS)) == 0) && (--timeout > 0) );
    isr[15] = I2C1_ISR;
    tout[15] = timeout;
}
/*
uint8_t f0I2cTransferGet()
{

}

void f0I2cDmaInit()
{

}
*/
