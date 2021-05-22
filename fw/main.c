#include "mystmbackend.h"
#include "printf7.h"
#include "freq_dr.h"
#include "ds18b20.h"


volatile int32_t temp = 0;

int main(void) {
	delay_ms(10);
	clkInit();
	tmInit();
	dsInit();

	while(1){
		temp = tempBlocking();
		if(temp == -1) {
			myprintf("err");
		} else {
			myprintf("%03d",temp/1000);
		}
		delay_s(1);
	}
}
