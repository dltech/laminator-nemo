#include "mystmbackend.h"
#include "printf7.h"
#include "freq_dr.h"
#include "ds18b20.h"


volatile int temp = 0;

int main(void) {
	clkInit();
	dsInit();

	while(1){
		temp = tempBlocking();
		delay_s(1);
	}
}
