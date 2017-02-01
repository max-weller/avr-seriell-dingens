
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

#include "definitions.h"


int8_t recvidx;
uint8_t recvpkg[37];
uint8_t recvcksum;

#define rs485_send_byte(bt) TXREG=bt; while(!PIR1bits.TXIF);
//#define rs485_send_byte_ck(bt) TXREG=bt; cksum^=bt; while(!PIR1bits.TXIF);

//const char [] MY_ADDR = {'L', 'E', 'D', 'S'};
#define MY_ADDR 0x40


uint8_t cksum;


 __at(0x1bff) void jumpToApplication() __naked {
__asm
jumpToApplication:
	BANKSEL PORTA
	BCF PORTA,5
	BSF PORTA,5
	GOTO $-2
	NOP
	NOP
	NOP
	NOP
__endasm;
}

//escape: 0xfb
//begin: 0xfc
void rs485_send_byte_ck(uint8_t bt);
void rs485_message_trailer();


uint8_t recvbyte;
struct {
  unsigned int escape : 1;
  unsigned int broadcast : 1;
  unsigned int cksumerr : 1;
} recvflags;


void bootload(void) __naked
{
	uint8_t i;
	uint32_t countdown;

countdown=2000000;
i=3;
__asm
GOTO forceBootload
maybeBootload: ;the init vector jumps here
__endasm;
if (PCONbits.NOT_POR == 0) {
	//regular power-on reset
	PCONbits.NOT_POR = 1;
	i=0;
} else {
	//other reset type!
	i=0;
	if (STATUSbits.NOT_TO == 0) {i=1;countdown=1000000;}
	STATUSbits.NOT_TO = 1;
}
BOOT_TRIS=1; OPTION_REGbits.NOT_RBPU=0; //enable pull up on boot pin
if(BOOT_PORT==0) {i=2;countdown=3000000;} //if jumpered to GND -> boot loader
OPTION_REGbits.NOT_RBPU=1;
if (i==0) {
	__asm
	CLRWDT
	PAGESEL jumpToApplication;
	GOTO jumpToApplication;
	__endasm;
}
__asm
forceBootload:
__endasm;

	LED_YELLOW_TRIS = 0; // Pin as output
	LED_RED_TRIS = 0; // Pin as output
	LED_RED2_TRIS = 0; // Pin as output
	LED_YELLOW_PORT = 1; // LED on

	RECV_TRIS = 1; //reciever is input
	RECV_PORT = 0;
	INTCONbits.GIE = 0; //global enable interrupts
	INTCONbits.PEIE = 0; //enable peripheral interrupts

	// Initialize UART
	SPBRG = BAUD_REG_VALUE; //set baud rate
	TXSTAbits.BRGH = 1; // high baud rate mode (prescaler 16x)
	RCSTAbits.SPEN = 1; // enable serial port
	TXSTAbits.SYNC = 0; // async mode
//	TXSTAbits.TX9 = 1;  // enable nine bit mode

	RCSTAbits.CREN = 1;
	recvidx = -2;
	recvflags.escape = 0;

	RS485_RE_TRIS = 0; RS485_DE_TRIS = 0;
	RS485_RE_PORT = 0; RS485_DE_PORT = 0; //RE is inverted

	CLRWDT();
	cksum = 0x2A;
	RS485_DE_PORT = 1; //enable rs485 driver
	TXSTAbits.TXEN = 1;
	rs485_send_byte(0xfc);
	rs485_send_byte_ck(0xff);
	rs485_send_byte_ck(MY_ADDR);
	rs485_send_byte_ck(3|PF_ACK); rs485_send_byte_ck(0xfd); rs485_send_byte_ck(0xbb); rs485_send_byte_ck(i);
	rs485_message_trailer();

	LED_YELLOW_PORT=0;

	while (1) {
		CLRWDT();
		if(countdown==0) {
					__asm
					CLRWDT
					PAGESEL jumpToApplication;
					GOTO jumpToApplication;
					__endasm;
		}
		countdown--;
		if ((countdown&0xffff) == 0) {
			if(LED_RED2_PORT)LED_RED2_PORT=0; else LED_RED2_PORT=1;
		}
		
		if (RCSTAbits.OERR) { // on overflow - skip current received packet, clear OERR by toggling CREN
			recvidx = -2;
			RCSTAbits.CREN = 0; RCSTAbits.CREN = 1;
		}
		if (PIR1bits.RCIF) {
			recvbyte = RCREG;
			if (recvbyte == 0xfc) { //start of packet
				recvidx = -1;
		//LED_RED2_PORT = 1;
			} else {
				if (recvbyte == 0xfb) {//escape
					recvflags.escape = 1; goto skipRcv;
				} else if (recvflags.escape) {
					recvbyte += 0x10;
					recvflags.escape = 0;
				} 
				switch (recvidx) {
				case -3: //successful received, waiting for main loop parsing
				case PI_STARTBYTE: //waiting for packet start
					break;
				case PI_TOADDR: //waiting for receiver address
					if (recvbyte == MY_ADDR /*|| recvbyte == 0xff*/) {
						recvidx = 0;
						recvcksum = 0x2a ^ recvbyte;
						/*recvflags.broadcast = (recvbyte == 0xff)?1:0;*/
					} else recvidx = -2;
					break;
				case PI_FROMADDR: //waiting for sender addr
		//LED_YELLOW_PORT=1;
		//LED_RED2_PORT=0;
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
						//LED_YELLOW_PORT = 1;
					} else {
						recvpkg[recvidx++] = recvbyte;
						recvcksum ^= recvbyte;
						//LED_YELLOW_PORT=0;
					}
					break;
				}
			}
			//PIR1bits.RCIF=0;    --- cleared by reading RCREG
		}

		skipRcv:
		if (recvidx == -3) { // incoming message in recvpkg
			countdown|=0x0f000000;
			//PIE1bits.RCIE = 0;
			//RCSTAbits.CREN = 0;
			if (recvpkg[PI_LENGTH] & (PF_ACK|PF_ERROR)) {
				//handle replies...
				//for now: ignore them
			} else {
				  cksum = 0x2A;
				  RS485_DE_PORT = 1; //enable rs485 driver
				  
				  TXSTAbits.TXEN = 1;

				  rs485_send_byte(0xfc);
				  rs485_send_byte_ck(recvpkg[PI_FROMADDR]);
				  rs485_send_byte_ck(MY_ADDR);

				switch (recvpkg[PI_CMD]) { //command
				case 0xb1: //jump to application
				  	rs485_send_byte_ck(1|PF_ACK);
				  	rs485_send_byte_ck(0xb1);
				  	rs485_message_trailer();

					__asm
					CLRWDT
					PAGESEL jumpToApplication;
					GOTO jumpToApplication;
					__endasm;
					//execution does not continue here

				case 0xb0: //data record
				LED_YELLOW_PORT=1;
					if (recvpkg[PI_DATA]==0 && recvpkg[PI_DATA+1]<4){rs485_send_byte_ck(2|PF_ACK|PF_ERROR);rs485_send_byte_ck(0xb0);rs485_send_byte_ck(0xCC);break;}
					if (recvpkg[PI_DATA]>MAX_FLASH_WRITE_ADDR){rs485_send_byte_ck(2|PF_ACK|PF_ERROR);rs485_send_byte_ck(0xb0);rs485_send_byte_ck(0xCC);break;}
					EEADRH = recvpkg[PI_DATA];
					EEADR = recvpkg[PI_DATA+1];

					i=PI_DATA+2;
					do{
						EEDATA=recvpkg[i];
						i++;
						EEDATH=recvpkg[i];
						i++;

						__asm CLRWDT __endasm;
						__asm
						BANKSEL EECON1
						BSF EECON1,EEPGD ; Point to program memory
						BSF EECON1,WREN ; Enable writes
						MOVLW 0x55 ; Start of required write sequence:
						MOVWF EECON2 ; Write 55h
						MOVLW 0xAA ;
						MOVWF EECON2 ; Write AAh
						BSF EECON1,WR ; Set WR bit to begin write
						NOP ; Any instructions here are ignored as processor
						; halts to begin write sequence
						NOP ; processor will stop here and wait for write complete
						; after write processor continues with 3rd instruction
						BCF EECON1,WREN ; Disable writes
						__endasm;

						EEADR++;
					}while(i<=recvpkg[PI_LENGTH]);

					rs485_send_byte_ck(4|PF_ACK);rs485_send_byte_ck(0xb0);rs485_send_byte_ck(recvpkg[PI_LENGTH]);rs485_send_byte_ck(recvcksum);rs485_send_byte_ck(i);
					LED_YELLOW_PORT=0;
					break;

				case 0xb9: //read back
					rs485_send_byte_ck(31|PF_ACK);rs485_send_byte_ck(0xb9);
					EEADRH = recvpkg[PI_DATA];
					EEADR = recvpkg[PI_DATA+1];
					EECON1bits.EEPGD = 1;
					i=15;
					while (i>0) {
						__asm
						BANKSEL EECON1
						BSF EECON1, RD
						NOP
						NOP
						__endasm;
						rs485_send_byte_ck(EEDATA);
						rs485_send_byte_ck(EEDATH);
						EEADR++;
						if (EEADR==0)EEADRH++;
						i--;
					}

					break;
				case 0xbf: // jump to bootloader: ignore, handle like echo request
					rs485_send_byte_ck(1|PF_ACK); rs485_send_byte_ck(0xbf);
					break;
				case 0xf1: // echo request
					rs485_send_byte_ck(2|PF_ACK); rs485_send_byte_ck(0xf1); rs485_send_byte_ck(recvpkg[PI_DATA]);
					break;
				default:
					rs485_send_byte_ck(1|PF_ACK|PF_ERROR); rs485_send_byte_ck(recvpkg[PI_CMD]);
					break;
				}
			}
			rs485_message_trailer();
			recvidx = -2; //start listening again
			//PIE1bits.RCIE = 1;
			//RCSTAbits.CREN = 1;
		}

	}




}


void rs485_send_byte_ck(uint8_t bt) {
  cksum ^= bt;
  if (bt == 0xfb || bt == 0xfc) {
    TXREG=0xfb;
    while(!PIR1bits.TXIF);
    bt -= 0x10;
  }
  TXREG=bt;
  while(!PIR1bits.TXIF);
}
void rs485_message_trailer() {
  rs485_send_byte_ck(cksum);
  rs485_send_byte(0x00);

  while(TXSTAbits.TRMT);
__asm nop 
nop
nop __endasm;
  TXSTAbits.TXEN = 0;
  RS485_DE_PORT = 0; //disable rs485 driver

}


