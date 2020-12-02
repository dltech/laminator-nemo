#include "../libopencm3/include/libopencm3/stm32/rcc.h"
#include "../libopencm3/include/libopencm3/stm32/gpio.h"
#include "../libopencm3/include/libopencm3/stm32/timer.h"
#include "../libopencm3/include/libopencm3/stm32/dma.h"
#include "freq_dr.h"
#include "cmsis_sin.h"

volatile uint32_t positive[SIN_RESOL_HP*2];
volatile uint32_t negative[SIN_RESOL_HP*2];

void sinGenerator(uint32_t arr, uint8_t isOffs, uint8_t isFirst, volatile uint32_t *sinus);

volatile uint32_t otl1;
volatile uint32_t otl2;
volatile uint32_t otl3;
volatile uint32_t otl4;

// just suppose pin4 for forward direction, PINF, and rely on 14 timer
//                pin7 is for reverse direction, PINR, timer 3 ch 2
void initPwm(void)
{
    RCC_AHBENR   |= RCC_AHBENR_GPIOAEN;
    GPIOA_MODER  |= GPIO_MODE_AF << (PINF*2) \
                 |  GPIO_MODE_AF << (PINR*2);
    GPIOA_OTYPER |= GPIO_OTYPE_PP << (PINF) \
                  | GPIO_OTYPE_PP << (PINR);
    GPIOA_OSPEEDR|= GPIO_OSPEED_100MHZ << (PINF*2) \
                  | GPIO_OSPEED_100MHZ << (PINR*2);
    GPIOA_PUPDR  |= GPIO_PUPD_NONE << (PINF*2) \
                  | GPIO_PUPD_NONE << (PINR*2);
    GPIOA_AFRL   |= GPIO_AF4 << (PINF*4) \
                  | GPIO_AF1 << (PINR*4);
    // timer configure for outputs
    RCC_APB1ENR|= RCC_APB1ENR_TIM14EN | RCC_APB1ENR_TIM3EN;
    TIM14_CR1   = (uint32_t) TIM_CR1_CKD_CK_INT;
    TIM14_PSC   = (uint32_t) PSC;
    TIM3_CR1    = (uint32_t) TIM_CR1_CKD_CK_INT;
    TIM3_PSC    = (uint32_t) PSC;
    setSpd(30, forward_dir);
    TIM14_CCMR1 = (uint32_t) TIM_CCMR1_OC1M_PWM2;
    TIM14_CCER  = (uint32_t) TIM_CCER_CC1E;
    TIM14_CCR1  = (uint32_t) 0;
    TIM3_CCMR1  = (uint32_t) TIM_CCMR1_OC2M_PWM2 | TIM_CCMR1_CC2S_OUT;
    TIM3_CCER   = (uint32_t) TIM_CCER_CC2E;
    TIM3_CCR2   = (uint32_t) 0;
    // dma configure for sinus output
    // on dma timer clock
    TIM3_CCMR1 |= (uint32_t) TIM_CCMR1_OC1M_FROZEN;
    TIM3_CCER  |= (uint32_t) TIM_CCER_CC1E;
    TIM3_CCR1   = 0;
    TIM3_DIER   = (uint32_t) TIM_DIER_UDE | TIM_DIER_CC1DE;
    // forward dma
    RCC_AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_CPAR3  = (uint32_t) &TIM14_CCR1;
    DMA1_CMAR3  = (uint32_t) positive;
    DMA1_CNDTR3 = (uint32_t) SIN_RESOL_HP*2;
    uint32_t ccr= DMA_CCR_MINC | DMA_CCR_MSIZE_32BIT | DMA_CCR_PSIZE_32BIT;
    ccr |= DMA_CCR_PL_LOW | DMA_CCR_DIR | DMA_CCR_CIRC;
    ccr |= DMA_CCR_TEIE;
    DMA1_CCR3   = ccr;
    // reverse dma
    DMA1_CPAR4  = (uint32_t) &TIM3_CCR2;
    DMA1_CMAR4  = (uint32_t) negative;
    DMA1_CNDTR4 = (uint32_t) SIN_RESOL_HP*2;
    ccr  = DMA_CCR_MINC | DMA_CCR_MSIZE_32BIT | DMA_CCR_PSIZE_32BIT;
    ccr |= DMA_CCR_PL_LOW | DMA_CCR_DIR | DMA_CCR_CIRC;
    ccr |= DMA_CCR_TEIE;
    DMA1_CCR4   = ccr;

    // timer synch (reason is in timer)
    // start
    TIM14_CR1  |= (uint32_t) TIM_CR1_CEN;
    TIM3_CR1   |= (uint32_t) TIM_CR1_CEN;
    DMA1_CCR3 |= DMA_CCR_EN;
    DMA1_CCR4 |= DMA_CCR_EN;
    TIM14_EGR  |= TIM_EGR_UG;
    TIM3_EGR   |= TIM_EGR_UG;
}

void setSpd(uint32_t hz, uint8_t dir)
{
    const uint32_t arrr = ARR(hz);
    if(dir == forward_dir) {
        sinGenerator(arrr/2, 1, 1, positive);
        sinGenerator(arrr/2, 0, 0, negative);
    } else {
        sinGenerator(arrr/2, 0, 1, positive);
        sinGenerator(arrr/2, 1, 0, negative);
    }
    TIM14_ARR = arrr;
    TIM3_ARR  = arrr;
    otl1 = arrr;
}

void sinGenerator(uint32_t arr, uint8_t isOffs, uint8_t isFirst, volatile uint32_t *sinus)
{
    uint64_t iterQ31;
    uint64_t sinQ31;
    for(int i=0 ; i<SIN_RESOL_HP*2 ; ++i) {
        sinus[i] = 0;
    }
    if(isFirst) {
        for(int i=2 ; i<SIN_RESOL_HP-2 ; ++i) {
            iterQ31   = ((uint64_t)i*BASEQ31)/(SIN_RESOL_HP-1)/2;
            sinQ31    = (uint64_t)arm_sin_q31((uint32_t)iterQ31);
            sinus[i]  = (uint32_t)(sinQ31 * (uint64_t)arr / BASEQ31);
            sinus[i] += DIR_OFFS * isOffs;
        }
    } else {
        for(int i=SIN_RESOL_HP+2 ; i<SIN_RESOL_HP*2-2 ; ++i) {
            iterQ31   = ((uint64_t)(i-SIN_RESOL_HP)*BASEQ31)/(SIN_RESOL_HP-1)/2;
            sinQ31    = (uint64_t)arm_sin_q31((uint32_t)iterQ31);
            sinus[i]  = (uint32_t)(sinQ31 * arr / BASEQ31);
            sinus[i] += DIR_OFFS * isOffs;
        }
    }
}
