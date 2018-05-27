
#define NO_BIT_DEFINES
#include <pic14regs.h>
#include <stdint.h>
//#include <eeprom.h>

//internal RC freq: 4MHz
//zero crossing freq: 100Hz
//cycles per ZC: 40k




union {
	__PORTCbits_t bits;
	uint8_t raw;
} PORTCshadow;

#define LED_1_PORT PORTCshadow.bits.RC7
#define LED_1_TRIS TRISCbits.TRISC7
#define LED_2_PORT PORTCshadow.bits.RC6
#define LED_2_TRIS TRISCbits.TRISC6
#define LED_3_PORT PORTCshadow.bits.RC5
#define LED_3_TRIS TRISCbits.TRISC5
//tris=0 -> output

#define BUTTON1_PORT PORTCbits.RC2
#define BUTTON1_SHPORT PORTCshadow.bits.RC2
#define BUTTON1_TRIS TRISCbits.TRISC2
#define BUTTON2_PORT PORTCbits.RC1
#define BUTTON2_SHPORT PORTCshadow.bits.RC1
#define BUTTON2_TRIS TRISCbits.TRISC1
#define BUTTON3_PORT PORTCbits.RC0
#define BUTTON3_SHPORT PORTCshadow.bits.RC0
#define BUTTON3_TRIS TRISCbits.TRISC0

#define RFDATA_PORT PORTBbits.RB5
#define RFDATA_TRIS TRISBbits.TRISB5

#define TRIAC_PORT PORTBbits.RB6
#define TRIAC_TRIS TRISBbits.TRISB6

#define ZEROCROS_PORT PORTAbits.RA2
#define ZEROCROS_TRIS TRISAbits.TRISA2


__code uint16_t __at (_CONFIG) __configword = _WDTE_OFF & _INTRC_OSC_NOCLKOUT;

#define CLRWDT() __asm CLRWDT __endasm

//#include "ws2812b.h"
//#include "definitions.h"

void delay(uint16_t msec)
{
	uint16_t i;
	//uint8_t j;
	uint8_t k;
	for (i = 0; i < msec; i++) {
		// one millisecond:
		//for (j=0; j<F_OSC / 16000 / 256; j++) {
		//for (j=0; j<4; j++) {
			for(k=0;k!=255;k++) {
				__asm nop
CLRWDT
nop
nop
nop
nop
nop
nop
nop
nop  __endasm;
			}
		//}
	}
}



uint8_t csectimer,timerevent;
uint8_t tmr1State;
uint8_t tmr1startH, tmr1startL;
void setLightLevel(uint8_t lightlevel){
	uint16_t timerdelay,tmr1start;
	if(lightlevel<5){
		T1CONbits.TMR1ON=0; PIE1bits.TMR1IE=0;
		tmr1startH=0; tmr1startL=0;
		TRIAC_PORT=0;TRIAC_PORT=0;TRIAC_PORT=0;TRIAC_PORT=0;
		return;
	}
	// Timer1 counts until next ZC: 1MHz / 100Hz = 10000
	// therefore this needs to hold true: 0xFFFF - timerdelay < 10000
	// this is accomplished because lightlevel is between 0-255 -> 255<<5 = 255*32 = 8160 < 10000
	timerdelay = ((uint16_t)(255-lightlevel)) << 5;
	tmr1start = 0xffff - timerdelay;

	INTCONbits.INTE=0;// critical section
	tmr1startH = tmr1start >> 8; tmr1startL = tmr1start & 0xff;


	INTCONbits.INTE=1;PIE1bits.TMR1IE=1;
}
static void irqHandler(void) __interrupt 0 {
    if (INTCONbits.INTF) { // zero crossing interrupt

        // Clear interrupt flag
        INTCONbits.INTF = 0;
				csectimer++;
				if(tmr1startH==0){TRIAC_PORT=0;return;}
				// set up Timer1 according to light level
				T1CONbits.TMR1ON = 0;
				TMR1H=tmr1startH;
				TMR1L=tmr1startL;
				tmr1State=1;
				T1CONbits.TMR1ON = 1;
    }
		if (INTCONbits.T0IF) {
				timerevent++;
				INTCONbits.T0IF=0;
		}
		if (PIR1bits.TMR1IF) {
			T1CONbits.TMR1ON = 0;
			if(tmr1State==1||tmr1State==3) {
				// delay until switch off -> let it on for 100 uS
				// 250uS -> 0xffff-250 = 0xff05
				TMR1H=0xfc; TMR1L=0x17;
				T1CONbits.TMR1ON = 1;
				TRIAC_PORT=1;
			} else if(tmr1State==2 || tmr1State==4){
				//delay until next half phase -> 100ms - 250uS -> 0xffff-10000+250 = 0xd9e9
				TMR1H=0xdc; TMR1L=0xd7;
				T1CONbits.TMR1ON = 1;
				TRIAC_PORT=0;
			//} else if(tmr1State==4){
			//	TRIAC_PORT=0;
			}
			tmr1State++;
			PIR1bits.TMR1IF=0;
		}
}

void setled(uint8_t ledidx){
	if (ledidx&1){
		LED_1_PORT=0;
	} else {
		LED_1_PORT=1;
	}
	if (ledidx&2){
		LED_2_PORT=0;
	} else {
		LED_2_PORT=1;
	}
	if (ledidx&4){
		LED_3_PORT=0;
	} else {
		LED_3_PORT=1;
	}
	PORTC = PORTCshadow.raw;
}

void ledtest() {
	uint8_t test;

		TRIAC_PORT=1;
		setled(7);
		delay(300);
		setled(0);
		delay(150);

				TRIAC_PORT=1;
		for(test=0;test<3;test++){
			setled(1);
			delay(45);
			setled(0);
			delay(45);
		}
		for(test=0;test<4;test++){
			setled(2);
			delay(30);
			setled(0);
			delay(30);
		}
		for(test=0;test<5;test++){
			setled(4);
			delay(15);
			setled(0);
			delay(15);
		}
/*
			TRIAC_PORT=0;
		for(test=0;test<5;test++){
			setled(1);
			delay(20);
			setled(2);
			delay(20);
			setled(4);
			delay(20);
		}
					TRIAC_PORT=1;
		setled(0);
		delay(250);
					TRIAC_PORT=0;*/
}

uint8_t demo_speed;

uint8_t receiver_state, receiver_cmd, received_data;
void serialReceive() {
	uint8_t recv;
	if(RCSTAbits.FERR)receiver_state=0;  //reset receive state on Framing Error

	recv = RCREG;  //read next char

	// state machine for parsing data packets
	// format:
	// preamble   command      data           trailer
	// 0x55* 0xAA <cmd> <~cmd> <data> <~data> 0xEE
	// the command byte and the data byte are each followed by their binary inverse
	if(receiver_state==0){
		if(recv==0x55)receiver_state=1;
	} else if (receiver_state == 1) {
		if(recv==0xAA)receiver_state=2;
		else if(recv==0x55)receiver_state=1;
		else receiver_state=0;
	} else if (receiver_state == 2) {
		receiver_cmd=recv;
		receiver_state=3;
	} else if (receiver_state == 3) {
		if (receiver_cmd== (recv^0xff)) receiver_state=4;
		else receiver_state=0;
	} else if (receiver_state == 4) {
		received_data = recv;
		receiver_state=5;
	} else if (receiver_state == 5) {
		if (received_data== (recv^0xff)) receiver_state=6;
		else receiver_state=0;
	} else if (receiver_state == 6) {
		if (recv==0xee) {
			runCommand();
		}
		receiver_state=0;
	}
}

//supported commands:
// Set Light Level:  0x01 <light level>
// Set Demo Speed (0 = disabled):  0x02 <demo speed>
// Set LED state:  0x03 <led state>
// LED Test:  0x04 <ignored>
// Set Config: 0xf? <data>  (cmd = 0xf0 + index)
void runCommand() {
	if(receiver_cmd==0x01) {
		setLightLevel(received_data);
	}
	if(receiver_cmd==0x02) {
		demo_speed=received_data;
	}
	if(receiver_cmd==0x03) {
		setled(received_data);
	}
	if(receiver_cmd==0x04) {
		ledtest();
	}
	/*if(receiver_cmd>=0xf0) {
		ee_write_byte(0x10 + (receiver_cmd&0x0f), &received_data);
	}*/
}

uint16_t touchThreshold;
void readTouchButton() {
	//1. Drive   secondary   channel   to   VDD as   digital output.
	BUTTON1_TRIS=0; BUTTON1_SHPORT=1;
	//2. Point  ADC  to  the  secondary  VDD pin  (charges	C	HOLD	 to VDD).
	ADCON0bits.CHS=6; //AN6 = button 1 (used to charge CHOLD)
	ADCON0bits.ADON=1;

	//3. Ground sensor line.
	BUTTON2_TRIS=0; BUTTON2_SHPORT=0;
	PORTC = PORTCshadow.raw;

ADCON0bits.ADFM=1;

		//TODO wait a little?
__asm
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
__endasm;

	//4. Turn sensor line as input (TRISx = 1	).
	BUTTON2_TRIS=1;
	//5. Point  ADC  to  sensor  channel  (voltage  divider from sensor to CHOLD).
	ADCON0bits.CHS=5; //AN5 = button 2
	ADCON0bits.ADON=1;
	BUTTON1_TRIS=1;

			//TODO wait a little?
	__asm
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	__endasm;

	//6. Begin ADC conversion.
	ADCON0bits.GO=1;
	//7. Reading is in ADRESH:ADRESL.
	while(ADCON0bits.GO);

}

void main(void)
{
	uint16_t dimvalue=0, dimctr=0, touchvalue;
	uint8_t zclast=0, zcctr=0,lightlevel,touchduration;
	int8_t dir=4;
	LED_1_TRIS=0;
	LED_2_TRIS=0;
	LED_3_TRIS=0;

	LED_1_PORT=0;
	LED_2_PORT=0;
	LED_3_PORT=0;
	PORTC = PORTCshadow.raw;

	ZEROCROS_TRIS=1; ANSELbits.ANS2 = 0; //configure as digital input
	RFDATA_TRIS=1;

	TRIAC_TRIS=0;
	TRIAC_PORT=1;

	//touch buttons
	BUTTON1_TRIS=1; //buttons are inputs
	BUTTON2_TRIS=1;
	BUTTON3_TRIS=1;
	ANSELbits.ANS5=1;
	ANSELbits.ANS6=1;
	//TODO ANSEL button rechts

	readTouchButton();
	touchThreshold=(ADRESH<<8)|ADRESL;
	ledtest();
	readTouchButton();
	touchThreshold+=(ADRESH<<8)|ADRESL;
	touchThreshold>>=1;

	// configure pin change interrupt for ZeroCrossing detector
	OPTION_REGbits.INTEDG = 0; //falling edge
	INTCONbits.INTE = 1;
	INTCONbits.GIE = 1;


	// configure Timer0 for timer event
	//OPTION_REGbits.PSA = 1; //no prescaler on Timer0, prescaler on WDT
	OPTION_REGbits.PSA = 0; // prescaler on Timer0, no prescaler on WDT
	OPTION_REGbits.T0CS = 0; //clock source: F_OSC/4 -> 1MHz
	INTCONbits.T0IE = 1; //enable Timer0 overflow interrupt


	// configure Timer1 for triac firing timing
	//T1CONbits.TMR1CS = 0; //clock source: F_OSC/4 -> 1MHz
	//T1CONbits.T1CKPS = 0; //prescaler 1:1
	T1CON = 0;  //clock source: F_OSC/4 -> 1MHz, prescaler 1:1
	INTCONbits.PEIE = 1;
	PIE1bits.TMR1IE = 1;


	//enable UART receiver
	SPBRG = 51; // 1200Baud bei FOSC=4MHz
	TXSTAbits.SYNC=0;
	RCSTAbits.SPEN=1;
	RCSTAbits.CREN=1;
	ANSELHbits.ANS11 = 0;
	receiver_state=0;

	csectimer=0;
	lightlevel=0;
	//ee_read_byte(0x10, &lightlevel);
	demo_speed=0;
	//ee_read_byte(0x11, &demo_speed);

	readTouchButton();
	touchThreshold+=(ADRESH<<8)|ADRESL;
	touchThreshold>>=1;

	setLightLevel(lightlevel);


	readTouchButton();
	touchThreshold+=(ADRESH<<8)|ADRESL;
	touchThreshold>>=1;


	//int8_t touchthresh;
	//ee_read_byte(0x12, &touchthresh);
	//if(touchthresh==-128)touchthresh=3;
	//touchThreshold-=touchthresh;
	touchThreshold-=2;

	while(1){

		if(demo_speed!=0 && csectimer>demo_speed) {
			lightlevel += 4;
			setLightLevel(lightlevel);

			if(lightlevel<5){dir=4;}
			if(lightlevel>200){dir=-8;}
			csectimer=0;
		}

		if(timerevent>1){
			readTouchButton();
			touchvalue=(ADRESH<<8)|ADRESL;
			__asm
nop
nop
nop
nop
nop
nop
nop
__endasm;
			readTouchButton();
			if(touchvalue < touchThreshold || ((ADRESH<<8)|ADRESL) < touchThreshold){
				LED_2_PORT = 0;
				touchduration++;
				if(touchduration==3) {
					demo_speed=0;
					lightlevel=(lightlevel+32)&0xE0;
					setLightLevel(lightlevel);
				} else if(touchduration==9) {
					lightlevel=(lightlevel==32)?250:0;
					setLightLevel(lightlevel);
				}
			}else {
				LED_2_PORT = 1;
				touchduration=0;
			}
			//LED_3_PORT=!LED_3_PORT;
			PORTC = PORTCshadow.raw;
			timerevent=0;
		}

		if(RCSTAbits.OERR){ //reset receiver on buffer overrun
			RCSTAbits.CREN=0;
			receiver_state=0;
			RCSTAbits.CREN=1;
		}
		if(PIR1bits.RCIF) { //UART byte received
			serialReceive();
		}
	}

/*
	while(1) {
	}
*/
}
