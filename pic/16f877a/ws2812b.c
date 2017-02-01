#define NO_BIT_DEFINES
#include <pic14regs.h>
#include <stdint.h> 

#include "ws2812b.h"

uint8_t bmp_r[STRIPE_LENGTH];
uint8_t bmp_g[STRIPE_LENGTH];
uint8_t bmp_b[STRIPE_LENGTH];


#define ws2812b_wait_bit_one __asm nop \
	nop \
	nop \
	__endasm;
#define ws2812b_write_byte(datavar, bits, looplabel) \
	bits = 0b10000000; \
	looplabel: \
		if((datavar)&bits) { \
			WSOUT_PORT = 1; \
			ws2812b_wait_bit_one; \
			WSOUT_PORT = 0; \
		} else { \
			WSOUT_PORT = 1; \
			WSOUT_PORT = 0; \
		} \
		bits>>=1; \
	if(bits!=0)goto looplabel; \

void ws2812b_fadecolor(uint8_t red, uint8_t green, uint8_t blue, int8_t dr, int8_t dg, int8_t db, uint8_t steps, uint8_t delay) {
	
}


void ws2812b_constcolor(uint8_t red, uint8_t green, uint8_t blue, uint8_t count) {
  uint8_t bits; 
  INTCONbits.GIE = 0; //global enable interrupts
while(INTCONbits.GIE); //wait until really switched off

//reset
loop:
	ws2812b_write_byte(blue, bits, blueloop);
count--;
	ws2812b_write_byte(red, bits, redloop);
	ws2812b_write_byte(green, bits, greenloop);
 if (count != 0) goto loop;
fini: 
  INTCONbits.GIE = 1; //global enable interrupts
}

void ws2812b_fromarray(uint8_t count) {
  uint8_t bits,value,idx; 

//wait a little for the reset
idx = 100; while (idx-->0) __asm nop __endasm;
idx = 0;
  INTCONbits.GIE = 0; //global enable interrupts
while(INTCONbits.GIE); //wait until really switched off

//reset
loop:
	value = bmp_b[idx];
	ws2812b_write_byte(value, bits, blueloop);
	value = bmp_r[idx];
	ws2812b_write_byte(value, bits, redloop);
	value = bmp_g[idx];
	ws2812b_write_byte(value, bits, greenloop);
idx++;
 if (count != idx) goto loop;
fini: 
  INTCONbits.GIE = 1; //global enable interrupts
}

