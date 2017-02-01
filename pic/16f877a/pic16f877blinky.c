// Copyright (C) 2014 Diego Herranz

#define NO_BIT_DEFINES
#include <pic14regs.h>
#include <stdint.h> 

// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN),
// disable watchdog,
// and disable low voltage programming.
// The rest of fuses are left as default.
__code uint16_t __at (_CONFIG) __configword = _WDTE_OFF & _LVP_OFF;

#include "definitions.h"

// Uncalibrated delay, just waits a number of for-loop iterations
void delay(uint16_t iterations)
{
	uint16_t i;
	for (i = 0; i < iterations; i++) {
		// Prevent this loop from being optimized away.
		__asm nop __endasm;
	}
}
/*
static void irqHandler(void) __interrupt 0 {
    if (INTCON.INTF) { // external interrupt from 433 mhz recv
        if (OPTION_REG.INTEDG){ //rising edge
		LED_RED_PORT = 1;
		OPTION_REG.INTEDG = 0;
	} else { //falling edge
		LED_RED_PORT = 0;
		OPTION_REG.INTEDG = 1;
	} 
        // Clear interrupt flag
        INTCON.INTF = 0;
    }
}*/

void main(void)
{
	LED_RED_TRIS = 0; // Pin as output
	LED_RED_PORT = 0; // LED off
	LED_RED2_TRIS = 0; // Pin as output
	LED_RED2_PORT = 0; // LED off
	LED_YELLOW_TRIS = 0; // Pin as output
	LED_YELLOW_PORT = 0; // LED off
/*
	RECV_TRIS = 1; //reciever is input
	RECV_PORT = 0;
	INTCON.INTE = 1; // enable external interrupt for receiver
	OPTION_REG.INTEDG = 1; //trigger on rising edge
	INTCON.GIE = 1; //global enable interrupts
	INTCON.PIE = 1; //enable peripheral interrupts
*/

	while (1) {
		LED_YELLOW_PORT = 1;
		delay(30000); // ~500ms @ 4MHz
		__asm CLRWDT __endasm;
		LED_YELLOW_PORT = 0;

		LED_RED_PORT = 1;
		delay(30000);
		__asm CLRWDT __endasm;
		LED_RED_PORT = 0;

		LED_RED2_PORT = 1;
		delay(30000);
		__asm CLRWDT __endasm;
		LED_RED2_PORT = 0;
	}
}

