#include "mystmbackend.h"
#include "printf7.h"
#include "freq_dr.h"


volatile uint8_t cycles = 0;

int main(void) {
	clkInit();
	tmInit();
	setBrightness(100);
	initPwm();

	while(1){
		myprintf("%03d",cycles);
		rough_delay_us(10000);

		++cycles;
	}
}
