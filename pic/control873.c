// Copyright (C) 2014 Diego Herranz

#define NO_BIT_DEFINES
#include <pic14regs.h>
#include <stdint.h> 

// crystal 17,734475 mhz
//crystal 18,432 mhz
#define F_OSC 18432000
#define BAUDRATE 9600
#define BAUD_REG_VALUE (F_OSC/BAUDRATE/16)

// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN),
// disable watchdog,
// and disable low voltage programming.
// The rest of fuses are left as default.

// FOSC_HS -> HS oscillator : external crystal > 4MHz
__code uint16_t __at (_CONFIG) __configword = _WDTE_ON & _LVP_OFF & _FOSC_HS;

#define CLRWDT() __asm CLRWDT __endasm

#include "ws2812b.h"
#include "definitions.h"

static void delay(uint16_t msec)
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
nop
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

static void dbg(uint8_t msec) {
			LED_YELLOW_PORT = 1;
			delay(msec);
			LED_YELLOW_PORT = 0;
			delay(msec);
}




int8_t recvidx;
uint8_t recvpkg[37];
uint8_t recvcksum;

#define sendByte(bt) TXREG=bt; while(!PIR1bits.TXIF);
//#define sendByteCk(bt) TXREG=bt; cksum^=bt; while(!PIR1bits.TXIF);

//const char [] MY_ADDR = {'L', 'E', 'D', 'S'};
#define MY_ADDR 0x40


uint8_t cksum;

//escape: 0xfb
//begin: 0xfc

void sendByteCk(uint8_t bt) {
  cksum ^= bt;
  if (bt == 0xfb || bt == 0xfc) {
    TXREG=0xfb;
    while(!PIR1bits.TXIF);
    bt -= 0x10;
  }
  TXREG=bt;
  while(!PIR1bits.TXIF);
}
void rs485_message_send(uint8_t destaddr, uint8_t length, uint8_t* data) {
  cksum = 0x2A;
  // uint8_t idx ;
  RS485_DE_PORT = 1; //enable rs485 driver
  
  TXSTAbits.TXEN = 1;
 // TXSTAbits.TX9D = 1;
  INTCONbits.GIE = 0; //global enable interrupts
while(INTCONbits.GIE); //wait until really switched off

__asm nop 
nop
nop __endasm;

  sendByte(0xfc);
  sendByteCk(destaddr);
  
 // TXSTAbits.TX9D = 0;
 // for(idx=0; idx!=4; idx++) { sendByteCk(MY_ADDR[idx]) }
  sendByteCk(MY_ADDR);

  sendByteCk(length);
  length &= PM_LENGTH;
  for(; length!=0; length--) {
    sendByteCk(*data);
    data++;
  }
  sendByteCk(cksum);
  sendByte(0x00);

  while(TXSTAbits.TRMT);
__asm nop 
nop
nop __endasm;
  TXSTAbits.TXEN = 0;
  RS485_DE_PORT = 0; //disable rs485 driver
  INTCONbits.GIE = 1; //global enable interrupts
}

void rs485_ack(uint8_t arg) {
  recvpkg[PI_DATA] = recvpkg[PI_LENGTH]; recvpkg[PI_DATA+1] = recvcksum; recvpkg[PI_DATA+2] = arg;
  rs485_message_send(recvpkg[PI_FROMADDR], (4 & PM_LENGTH) | PF_ACK, &recvpkg[PI_CMD]);
}

void rs485_error() {
  rs485_message_send(recvpkg[PI_FROMADDR], (recvpkg[PI_LENGTH] & PM_LENGTH) | PF_ACK | PF_ERROR, &recvpkg[PI_CMD]);
}



#define USEC_TO_TIMER *1736/1000

uint16_t onTime, offTime;

uint8_t recvbyte;
struct {
  unsigned int escape : 1;
  unsigned int broadcast : 1;
  unsigned int cksumerr : 1;
} recvflags;
static void irqHandler(void) __interrupt 0 {
    /*if (INTCONbits.INTF) { // external interrupt from 433 mhz recv
        if (OPTION_REGbits.INTEDG){ //rising edge
		LED_RED_PORT = 1;
		OPTION_REGbits.INTEDG = 0;
		onTime = 0;
		offTime = (TMR1H<<8) | TMR1L;
		TMR1L = 0;
		TMR1H = 0;
	} else { //falling edge
		LED_RED_PORT = 0;
		OPTION_REGbits.INTEDG = 1;
		onTime = (TMR1H<<8) | TMR1L;
		TMR1L = 0;
		TMR1H = 0;
	} 
        // Clear interrupt flag
        INTCONbits.INTF = 0;
    }*/
    if (PIR1bits.RCIF) {
		recvbyte = RCREG;
		if (recvbyte == 0xfc) { //start of packet
			recvidx = -1;
			LED_RED2_PORT = 1;
		} else {
			if (recvbyte == 0xfb) {//escape
				recvflags.escape = 1;
			} else {
				if (recvflags.escape) {
					recvbyte += 0x10;
					recvflags.escape = 0;
				} 
				switch (recvidx) {
				case -3: //successful received, waiting for main loop parsing
				case PI_STARTBYTE: //waiting for packet start
					break;
				case PI_TOADDR: //waiting for receiver address
					if (recvbyte == MY_ADDR || recvbyte == 0xff) {
						recvidx = 0;
						recvcksum = 0x2a ^ recvbyte;
						recvflags.broadcast = (recvbyte == 0xff)?1:0;
					} else recvidx = -2;
					break;
				case PI_FROMADDR: //waiting for sender addr
					LED_YELLOW_PORT=1;
					LED_RED2_PORT=0;
					recvpkg[PI_FROMADDR] = recvbyte;
					recvpkg[PI_LENGTH] = 31;
					recvcksum ^= recvbyte;
					recvidx++;
					break;
				default: //waiting for content
					if (recvidx >= (recvpkg[PI_LENGTH] & PM_LENGTH)+2) { //just received the checksum, done
						if (recvcksum == recvbyte) 
							recvidx = -3;
						else
							recvidx = -2; //ignore packet
						LED_YELLOW_PORT = 1;
					} else {
						recvpkg[recvidx++] = recvbyte;
						recvcksum ^= recvbyte;
						LED_YELLOW_PORT=0;
					}
					break;
				}
			}
		}
    }
}

uint32_t test32;
void main(void)
{
	uint16_t test,ticker,autorefreshat;
	uint8_t i,j,k,l;


	WSOUT_TRIS = 0; WSOUT_PORT = 0;

	if (PCONbits.NOT_POR == 0) {
		//regular power-on reset
		PCONbits.NOT_POR = 1;
		i=100; j=100; k=100;
		test = 100;
	} else {
		//other reset type!
		i=0; j=0; k=255;
		test = 510;
		if (STATUSbits.NOT_TO == 0) {i=255;j=0;k=0; test=765;}
		STATUSbits.NOT_TO = 1;
	}
	while(test-->0) {
	i--; if(j>0)j--; if(k>0)k--;
		ws2812b_constcolor(i,j,k,STRIPE_LENGTH);
		delay(1);
		CLRWDT();
	}
	onTime = 0; offTime = 0; k = 0;ticker=0;autorefreshat=0;

	LED_RED_TRIS = 0; // Pin as output
	LED_RED_PORT = 1; // LED off
	LED_RED2_TRIS = 0; // Pin as output
	LED_RED2_PORT = 1; // LED off
	LED_YELLOW_TRIS = 0; // Pin as output
	LED_YELLOW_PORT = 1; // LED off


	INTCONbits.GIE = 1; //global enable interrupts
	INTCONbits.PEIE = 1; //enable peripheral interrupts

	// Initialize UART
	SPBRG = BAUD_REG_VALUE; //set baud rate
	TXSTAbits.BRGH = 1; // high baud rate mode (prescaler 16x)
	RCSTAbits.SPEN = 1; // enable serial port
	TXSTAbits.SYNC = 0; // async mode
//	TXSTAbits.TX9 = 1;  // enable nine bit mode

	RCSTAbits.CREN = 1;
	recvidx = -2;

	RS485_RE_TRIS = 0; RS485_DE_TRIS = 0;
	RS485_RE_PORT = 0; RS485_DE_PORT = 0; //RE is inverted
	
	T1CONbits.T1CKPS1 = 1; T1CONbits.T1CKPS0 = 1; // 0b11 ^= 1:8 prescaler of 1/4 FOSC
	T1CONbits.T1OSCEN = 1;
	T1CONbits.TMR1ON = 1;
	T1CONbits.TMR1CS = 0; // internal clock (1/4 FOSC)

	test = 0xfffd;
	ws2812b_constcolor(0,1,0,STRIPE_LENGTH);
	delay(500);
	CLRWDT();
	rs485_message_send(0xff,2,(uint8_t*)&test);


	ws2812b_constcolor(0,0,0,STRIPE_LENGTH);

	LED_YELLOW_PORT=0; LED_RED_PORT=0; LED_RED2_PORT=0;

	while (1) {
		__asm CLRWDT __endasm;

		if (RCSTAbits.OERR) { // on overflow - skip current received packet, clear OERR by toggling CREN
			recvidx = -2;
			RCSTAbits.CREN = 0; RCSTAbits.CREN = 1;
		}
		if (recvidx == -3) { // incoming message in recvpkg
			//PIE1bits.RCIE = 0;
			//RCSTAbits.CREN = 0;
			if (recvpkg[PI_LENGTH] & (PF_ACK|PF_ERROR)) {
				//handle replies...
				//for now: ignore them
			} else {
				i = 0;
				switch (recvpkg[PI_CMD]) { //command
				case 0x05: //set output
					if (recvpkg[PI_DATA+2] == 0xff) j = 1;
					else if (recvpkg[PI_DATA+2] == 0x00) j = 0;
					else { rs485_error(); break; }

					if (recvpkg[PI_DATA+1] == 0x01) {
						LED_YELLOW_PORT = j;
						rs485_ack(0xAA);
					} else if (recvpkg[PI_DATA+1] == 0x02) {
						LED_RED_PORT = j;
						LED_RED2_PORT = j;
						rs485_ack(0xAA);
					} else {
						rs485_error();
					}
					break;
					
				case 0x5e: //update ws2812 from bitmap
					ws2812b_fromarray(recvpkg[PI_DATA]);
					rs485_ack(0xAA);
					break;
				case 0x5f: //set autorefresh
					autorefreshat = (recvpkg[PI_DATA]<<8)|(recvpkg[PI_DATA+1]);
					rs485_ack(0xAA);
					break;
				case 0xfc: //change baud rate
					test = recvpkg[PI_DATA]<<8 | recvpkg[PI_DATA+1];
					test32 = F_OSC;
					i = 0; j = 0;
					while(test32 != 0 && i!=255) { test32 -= test; j++; if (j==16) {i++; j=0;} }
					if (i==255) {rs485_error(); break; } //ging nicht auf
					//dbg(100);
					//i = F_OSC/test/16;
					//i = recvpkg[PI_DATA];
					//dbg(300);
					rs485_ack(i);
					//dbg(500);
					RCSTAbits.SPEN = 0; // enable serial port
					RCSTAbits.CREN = 0;
					SPBRG = i;
					RCSTAbits.SPEN = 1; // enable serial port
					TXSTAbits.SYNC = 0; // async mode
					RCSTAbits.CREN = 1;
					break;
				case 0xf1: // echo request
						LED_YELLOW_PORT=0;
					rs485_message_send(recvpkg[PI_FROMADDR], recvpkg[PI_LENGTH] | PF_ACK, &recvpkg[PI_CMD]);
					break;
				case 0xbf: // reboot to bootloader
					while(1);
					break;
				default:
					if (!recvflags.broadcast) rs485_error();
					break;
				}
			}
			recvidx = -2; //start listening again
			//PIE1bits.RCIE = 1;
			//RCSTAbits.CREN = 1;
		}
		ticker++;
		if(autorefreshat!= 0 && ticker==autorefreshat) {
			ws2812b_fromarray(STRIPE_LENGTH);
			ticker=0;
			//dbg(10);
		}
	}
}

