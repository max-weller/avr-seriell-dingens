
#define NO_BIT_DEFINES
#include <pic14regs.h>
#include <stdint.h> 

// crystal 17,734475 mhz
//crystal 18,432 mhz
#define F_OSC 18432000
#define BAUDRATE 19200
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

static void dbg(uint8_t msec) {
			LED_YELLOW_PORT = 1;
			delay(msec);
			LED_YELLOW_PORT = 0;
			delay(msec);
}



/******** configuration / macros for rs485 receiver+transmitter */

#define rs485_start_sending() do{RS485_DE_PORT = 1; /*enable rs485 driver*/  TXSTAbits.TXEN = 1;}while(0);
#define begin_critical_section() do{  INTCONbits.GIE = 0; /*global enable interrupts*/while(INTCONbits.GIE); /*wait until really switched off*/}while(0);
#define end_critical_section() do{  INTCONbits.GIE = 1; /*global enable interrupts*//*while(!INTCONbits.GIE); *//*wait until really switched off*/}while(0);
#define rs485_finish_sending() do{ while(TXSTAbits.TRMT); TXSTAbits.TXEN = 0;RS485_DE_PORT = 0;\
__asm nop \
nop \
nop __endasm;}while(0);


#define rs485_send_byte(bt) do{TXREG=bt; while(!PIR1bits.TXIF); }while(0)
//#define sendByteCk(bt) TXREG=bt; cksum^=bt; while(!PIR1bits.TXIF);

//const char [] MY_ADDR = {'L', 'E', 'D', 'S'};
#define MY_ADDR 0x40

#define RS485_BUFLEN 37

inline void receiveRS485(void);
#include "../../rs485_receiver.c"




#define USEC_TO_TIMER *1736/1000

uint16_t onTime, offTime;

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
		receiveRS485();
    }
}

uint32_t test32;
void main(void)
{
	uint16_t test,ticker,autorefreshat;
	uint8_t i,j,k,l;


	WSOUT_TRIS = 0; WSOUT_PORT = 0;

	i=100;
	while(i-->0) {
		ws2812b_constcolor(i,i,i,STRIPE_LENGTH);
		delay(1);
		CLRWDT();
	}
	onTime = 0; offTime = 0; k = 0;ticker=0;autorefreshat=0;

	OPTION_REGbits.PS0=1;
	OPTION_REGbits.PS1=1;
	OPTION_REGbits.PS2=1;

	LED_RED_TRIS = 0; // Pin as output
	LED_RED_PORT = 0; // LED off
	LED_RED2_TRIS = 0; // Pin as output
	LED_RED2_PORT = 0; // LED off
	LED_YELLOW_TRIS = 0; // Pin as output
	LED_YELLOW_PORT = 0; // LED on


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

	CLRWDT();
	ws2812b_constcolor(0,10,0,STRIPE_LENGTH);

dbg(244);
dbg(244);

	test = 0xfffd;
	rs485_message_send(0xff,2,(uint8_t*)&test);

	ws2812b_constcolor(0,0,0,STRIPE_LENGTH);

	LED_YELLOW_PORT=0; LED_RED_PORT=0; LED_RED2_PORT=0;

	PIE1bits.RCIE = 1;
	while (1) {
		CLRWDT();

		if (RCSTAbits.OERR) { // on overflow - skip current received packet, clear OERR by toggling CREN
			recvidx = -2;
			RCSTAbits.CREN = 0; RCSTAbits.CREN = 1;
		}
		if (recvidx == -3) { // incoming message in recvpkg
			//PIE1bits.RCIE = 0;
			//RCSTAbits.CREN = 0;
			if (recvpkg.st.length & (PF_ACK|PF_ERROR)) {
				//handle replies...
				//for now: ignore them
			} else {
				i = 0;
				switch (recvpkg.st.command) { //command
				case 0x05: //set output
					if (recvpkg.st.data[2] == 0xff) j = 1;
					else if (recvpkg.st.data[2] == 0x00) j = 0;
					else { rs485_error(); break; }

					if (recvpkg.st.data[1] == 0x01) {
						LED_YELLOW_PORT = j;
						rs485_ack(0xAA);
					} else if (recvpkg.st.data[1] == 0x02) {
						LED_RED_PORT = j;
						LED_RED2_PORT = j;
						rs485_ack(0xAA);
					} else {
						rs485_error();
					}
					break;
				case 0x50: //set constcolor
					ws2812b_constcolor(recvpkg.raw[3], recvpkg.raw[4], recvpkg.raw[5], recvpkg.raw[6]);

					rs485_ack(0xAA);
					break;
				case 0x51: //set bmp rgb range
					j = PI_DATA+1;
					i = recvpkg.st.data[0];
					while (j<recvpkg.st.length && i<STRIPE_LENGTH) {
						bmp_r[i] = recvpkg.raw[j++];
						bmp_g[i] = recvpkg.raw[j++];
						bmp_b[i] = recvpkg.raw[j++];
						i++;
					}
					rs485_ack(0xaa);
					break;
				case 0x52: //fill bmp const color range
				case 0x53: //dim/lighten bmp range
					i = recvpkg.st.data[0];
					if (i >= STRIPE_LENGTH || recvpkg.st.data[1] >= STRIPE_LENGTH || recvpkg.st.data[1] < i) {
						rs485_error(); break;
					}
					while(i<=recvpkg.st.data[1]) {
						if (recvpkg.st.command == 0x52) {
							bmp_r[i] = recvpkg.st.data[2];
							bmp_g[i] = recvpkg.st.data[3];
							bmp_b[i] = recvpkg.st.data[4];
						} else {
							bmp_r[i] += (int8_t)recvpkg.st.data[2];
							bmp_g[i] += (int8_t)recvpkg.st.data[3];
							bmp_b[i] += (int8_t)recvpkg.st.data[4];
						}
						i++;
					}
					rs485_ack(0xAA);
					break;
				case 0x54: //scroll bmp range left
					i = recvpkg.st.data[0];
					if (i >= STRIPE_LENGTH || recvpkg.st.data[1] >= STRIPE_LENGTH || recvpkg.st.data[1] < i) {
						rs485_error(); break;
					}
					j = bmp_r[i];
					k = bmp_g[i];
					l = bmp_b[i];
					while(i<recvpkg.st.data[1]) {
						bmp_r[i] = bmp_r[i+1];
						bmp_g[i] = bmp_g[i+1];
						bmp_b[i] = bmp_b[i+1];
						i++;
					}
					bmp_r[i] = j;
					bmp_g[i] = k;
					bmp_b[i] = l;
					rs485_ack(0xAA);
					break;
				case 0x55: //scroll bmp range right
					i = recvpkg.st.data[1];
					if (recvpkg.st.data[0] >= STRIPE_LENGTH || i >= STRIPE_LENGTH || recvpkg.st.data[0] > i) {
						rs485_error(); break;
					}
					j = bmp_r[i];
					k = bmp_g[i];
					l = bmp_b[i];
					while(i>recvpkg.st.data[0]) {
						bmp_r[i] = bmp_r[i-1];
						bmp_g[i] = bmp_g[i-1];
						bmp_b[i] = bmp_b[i-1];
						i--;
					}
					bmp_r[i] = j;
					bmp_g[i] = k;
					bmp_b[i] = l;
					rs485_ack(0xAA);
					break;
				case 0x5e: //update ws2812 from bitmap
					ws2812b_fromarray(recvpkg.st.data[0]);
					rs485_ack(0xAA);
					break;
				case 0x5f: //set autorefresh
					autorefreshat = (recvpkg.st.data[0]<<8)|(recvpkg.st.data[1]);
					rs485_ack(0xAA);
					break;
				case 0xfc: //change baud rate
					test = recvpkg.st.data[0]<<8 | recvpkg.st.data[1];
					test32 = F_OSC;
					i = 0; j = 0;
					while(test32 != 0 && i!=255) { test32 -= test; j++; if (j==16) {i++; j=0;} }
					if (i==255) {rs485_error(); break; } //ging nicht auf
					//dbg(100);
					//i = F_OSC/test/16;
					//i = recvpkg.st.data;
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
					rs485_message_send(recvpkg.st.fromaddr, recvpkg.st.length | PF_ACK, &recvpkg.st.command);
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

