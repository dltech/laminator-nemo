#ifndef H_FREQDR
#define H_FREQDR

#define SIN_RESOL_HP    16
#define DIR_OFFS        5

#define PINF    4
#define PINR    7

#define SYSCLK  48000000
#define PSC     16
#define ARR(out_clk)     SYSCLK / PSC / SIN_RESOL_HP / out_clk

#define forward_dir 0
#define reverse_dir 1

void setSpd(uint32_t hz, uint8_t reverse);
void initPwm(void);

#endif
