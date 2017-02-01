;--------------------------------------------------------
; File Created by SDCC : free open source ANSI-C Compiler
; Version 3.6.0 #9615 (Mac OS X x86_64)
;--------------------------------------------------------
; PIC port for the 14-bit core
;--------------------------------------------------------
;	.file	"bootloader.c"
	list	p=16f877a
	radix dec
	include "p16f877a.inc"
;--------------------------------------------------------
; config word(s)
;--------------------------------------------------------
	; __config 0x3f7e
	__config  _WDTE_ON & _LVP_OFF & _FOSC_HS & _DEBUG_ON

;--------------------------------------------------------
; external declarations
;--------------------------------------------------------
	extern	_STATUSbits
	extern	_PORTAbits
	extern	_PORTBbits
	extern	_PORTCbits
	extern	_PORTDbits
	extern	_PORTEbits
	extern	_INTCONbits
	extern	_PIR1bits
	extern	_PIR2bits
	extern	_T1CONbits
	extern	_T2CONbits
	extern	_SSPCONbits
	extern	_CCP1CONbits
	extern	_RCSTAbits
	extern	_CCP2CONbits
	extern	_ADCON0bits
	extern	_OPTION_REGbits
	extern	_TRISAbits
	extern	_TRISBbits
	extern	_TRISCbits
	extern	_TRISDbits
	extern	_TRISEbits
	extern	_PIE1bits
	extern	_PIE2bits
	extern	_PCONbits
	extern	_SSPCON2bits
	extern	_SSPSTATbits
	extern	_TXSTAbits
	extern	_CMCONbits
	extern	_CVRCONbits
	extern	_ADCON1bits
	extern	_EECON1bits
	extern	_INDF
	extern	_TMR0
	extern	_PCL
	extern	_STATUS
	extern	_FSR
	extern	_PORTA
	extern	_PORTB
	extern	_PORTC
	extern	_PORTD
	extern	_PORTE
	extern	_PCLATH
	extern	_INTCON
	extern	_PIR1
	extern	_PIR2
	extern	_TMR1
	extern	_TMR1L
	extern	_TMR1H
	extern	_T1CON
	extern	_TMR2
	extern	_T2CON
	extern	_SSPBUF
	extern	_SSPCON
	extern	_CCPR1
	extern	_CCPR1L
	extern	_CCPR1H
	extern	_CCP1CON
	extern	_RCSTA
	extern	_TXREG
	extern	_RCREG
	extern	_CCPR2
	extern	_CCPR2L
	extern	_CCPR2H
	extern	_CCP2CON
	extern	_ADRESH
	extern	_ADCON0
	extern	_OPTION_REG
	extern	_TRISA
	extern	_TRISB
	extern	_TRISC
	extern	_TRISD
	extern	_TRISE
	extern	_PIE1
	extern	_PIE2
	extern	_PCON
	extern	_SSPCON2
	extern	_PR2
	extern	_SSPADD
	extern	_SSPSTAT
	extern	_TXSTA
	extern	_SPBRG
	extern	_CMCON
	extern	_CVRCON
	extern	_ADRESL
	extern	_ADCON1
	extern	_EEDATA
	extern	_EEADR
	extern	_EEDATH
	extern	_EEADRH
	extern	_EECON1
	extern	_EECON2

;--------------------------------------------------------
; global declarations
;--------------------------------------------------------
	global	_rs485_message_trailer
	global	_jumpToApplication
	global	_bootload
	global	_rs485_send_byte_ck
	global	_recvidx
	global	_recvpkg
	global	_recvcksum
	global	_cksum
	global	_recvbyte
	global	_recvflags

	global PSAVE
	global SSAVE
	global WSAVE
	global STK12
	global STK11
	global STK10
	global STK09
	global STK08
	global STK07
	global STK06
	global STK05
	global STK04
	global STK03
	global STK02
	global STK01
	global STK00

sharebank udata_ovr 0x0070
PSAVE	res 1
SSAVE	res 1
WSAVE	res 1
STK12	res 1
STK11	res 1
STK10	res 1
STK09	res 1
STK08	res 1
STK07	res 1
STK06	res 1
STK05	res 1
STK04	res 1
STK03	res 1
STK02	res 1
STK01	res 1
STK00	res 1
;--------------------------------------------------------
; global definitions
;--------------------------------------------------------
UD_bootloader_0	udata
_recvidx	res	1

UD_bootloader_1	udata
_recvpkg	res	37

UD_bootloader_2	udata
_recvcksum	res	1

UD_bootloader_3	udata
_cksum	res	1

UD_bootloader_4	udata
_recvbyte	res	1

UD_bootloader_5	udata
_recvflags	res	1

;--------------------------------------------------------
; absolute symbol definitions
;--------------------------------------------------------
;--------------------------------------------------------
; compiler-defined variables
;--------------------------------------------------------
UDL_bootloader_0	udata
r0x102F	res	1
r0x1030	res	1
r0x1031	res	1
r0x1032	res	1
r0x1033	res	1
r0x1034	res	1
r0x1035	res	1
r0x1036	res	1
r0x1037	res	1
r0x1038	res	1
r0x102A	res	1
r0x102B	res	1
r0x102C	res	1
r0x102D	res	1
r0x102E	res	1
;--------------------------------------------------------
; initialized data
;--------------------------------------------------------
;--------------------------------------------------------
; overlayable items in internal ram 
;--------------------------------------------------------
;	udata_ovr
;--------------------------------------------------------
; code
;--------------------------------------------------------
code_bootloader	code
;***
;  pBlock Stats: dbName = C
;***
;has an exit
;functions called:
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;   _sendByteCk
;7 compiler assigned registers:
;   r0x102B
;   STK00
;   r0x102C
;   STK01
;   r0x102D
;   STK02
;   r0x102E
;; Starting pCode block





;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;
;;;;          reset vector
;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


S_bootloader__resetvector	code 0x0000
STARTUP	code 0x0000
	nop
	pagesel maybeBootload
	goto	maybeBootload







;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;
;;;;          _jumpToApplication
;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



S_bootloader__bugtrap	code 0x1bea
bugtrap_begin:
	BANKSEL	PORTD
	BCF	PORTD,0
	NOP
	BSF	PORTD,0
	NOP
	GOTO	bugtrap_begin
	NOP



S_bootloader__jumpToApplication	code 0x1bf8
_jumpToApplication:
; 2 exit points
jumpToApplication:
	BANKSEL	PORTA
	BCF	PORTA,5
	BSF	PORTA,5
	GOTO	jumpToApplication
	NOP
	NOP
	NOP

; exit point of _jumpToApplication






;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;
;;;;          RS485 message send
;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;***
;  pBlock Stats: dbName = C
;***
;has an exit
;functions called:
;   _rs485_send_byte_ck
;   _rs485_send_byte_ck
;; Starting pCode block
S_bootloader__rs485_message_trailer	code
_rs485_message_trailer:
; 2 exit points
;	.line	332; "bootloader.c"	rs485_send_byte_ck(cksum);
	BANKSEL	_cksum
	MOVF	_cksum,W
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	333; "bootloader.c"	rs485_send_byte(0x00);
	BANKSEL	_TXREG
	CLRF	_TXREG
_00346_DS_:
	BANKSEL	_PIR1bits
	BTFSS	_PIR1bits,4
	GOTO	_00346_DS_
_00349_DS_:
;	.line	335; "bootloader.c"	while(TXSTAbits.TRMT);
	BANKSEL	_TXSTAbits
	BTFSC	_TXSTAbits,1
	GOTO	_00349_DS_
	nop
	nop
	nop	
;	.line	339; "bootloader.c"	TXSTAbits.TXEN = 0;
	BANKSEL	_TXSTAbits
	BCF	_TXSTAbits,5
;	.line	340; "bootloader.c"	RS485_DE_PORT = 0; //disable rs485 driver
	BANKSEL	_PORTBbits
	BCF	_PORTBbits,5
	RETURN	
; exit point of _rs485_message_trailer

;***
;  pBlock Stats: dbName = C
;***
;has an exit
;1 compiler assigned register :
;   r0x102A
;; Starting pCode block
S_bootloader__rs485_send_byte_ck	code
_rs485_send_byte_ck:
; 2 exit points
;	.line	321; "bootloader.c"	void rs485_send_byte_ck(uint8_t bt) {
	BANKSEL	r0x102A
	MOVWF	r0x102A
;	.line	322; "bootloader.c"	cksum ^= bt;
	BANKSEL	_cksum
	XORWF	_cksum,F
;	.line	323; "bootloader.c"	if (bt == 0xfb || bt == 0xfc) {
	BANKSEL	r0x102A
	MOVF	r0x102A,W
	XORLW	0xfb
	BTFSC	STATUS,2
	GOTO	_00321_DS_
	MOVF	r0x102A,W
	XORLW	0xfc
	BTFSS	STATUS,2
	GOTO	_00322_DS_
_00321_DS_:
;	.line	324; "bootloader.c"	TXREG=0xfb;
	MOVLW	0xfb
	BANKSEL	_TXREG
	MOVWF	_TXREG
_00318_DS_:
;	.line	325; "bootloader.c"	while(!PIR1bits.TXIF);
	BANKSEL	_PIR1bits
	BTFSS	_PIR1bits,4
	GOTO	_00318_DS_
;	.line	326; "bootloader.c"	bt -= 0x10;
	MOVLW	0xf0
	BANKSEL	r0x102A
	ADDWF	r0x102A,F
_00322_DS_:
;	.line	328; "bootloader.c"	TXREG=bt;
	BANKSEL	r0x102A
	MOVF	r0x102A,W
	BANKSEL	_TXREG
	MOVWF	_TXREG
_00324_DS_:
;	.line	329; "bootloader.c"	while(!PIR1bits.TXIF);
	BANKSEL	_PIR1bits
	BTFSS	_PIR1bits,4
	GOTO	_00324_DS_
	RETURN	
; exit point of _rs485_send_byte_ck














;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;
;;;;          MAIN METHOD   BOOTLOAD
;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


S_bootloader__bootload	code  0x1c00

_bootload:
; 2 exit points
;	.line	74; "bootloader.c"	countdown=2000000;
	MOVLW	0x80
	BANKSEL	r0x102B
	MOVWF	r0x102B
	MOVLW	0x84
	MOVWF	r0x102C
	MOVLW	0x1e
	MOVWF	r0x102D
	CLRF	r0x102E
	GOTO	forceBootload
maybeBootload:
	;the init vector jumps here
;	.line	80; "bootloader.c"	if (PCONbits.NOT_POR == 0) {
	BANKSEL	_PCONbits
	BTFSC	_PCONbits,1
	GOTO	_00112_DS_
;	.line	82; "bootloader.c"	PCONbits.NOT_POR = 1;
	BSF	_PCONbits,1
;	.line	83; "bootloader.c"	i=0;
	BANKSEL	r0x102F
	CLRF	r0x102F
	GOTO	_00113_DS_
_00112_DS_:
;	.line	86; "bootloader.c"	i=0;
	BANKSEL	r0x102F
	CLRF	r0x102F
;	.line	87; "bootloader.c"	if (STATUSbits.NOT_TO == 0) {i=1;countdown=1000000;}
	BANKSEL	_STATUSbits
	BTFSC	_STATUSbits,4
	GOTO	_00110_DS_
	MOVLW	0x01
	BANKSEL	r0x102F
	MOVWF	r0x102F
	MOVLW	0x40
	MOVWF	r0x102B
	MOVLW	0x42
	MOVWF	r0x102C
	MOVLW	0x0f
	MOVWF	r0x102D
	CLRF	r0x102E
_00110_DS_:
;	.line	88; "bootloader.c"	STATUSbits.NOT_TO = 1;
	BANKSEL	_STATUSbits
	BSF	_STATUSbits,4
_00113_DS_:
;	.line	90; "bootloader.c"	BOOT_TRIS=1; OPTION_REGbits.NOT_RBPU=0; //enable pull up on boot pin
	BANKSEL	_TRISBbits
	BSF	_TRISBbits,7
	BCF	_OPTION_REGbits,7
;	.line	91; "bootloader.c"	if(BOOT_PORT==0) {i=2;countdown=3000000;} //if jumpered to GND -> boot loader
	BANKSEL	_PORTBbits
	BTFSC	_PORTBbits,7
	GOTO	_00115_DS_
	MOVLW	0x02
	BANKSEL	r0x102F
	MOVWF	r0x102F
	MOVLW	0xc0
	MOVWF	r0x102B
	MOVLW	0xc6
	MOVWF	r0x102C
	MOVLW	0x2d
	MOVWF	r0x102D
	CLRF	r0x102E
_00115_DS_:
;	.line	92; "bootloader.c"	OPTION_REGbits.NOT_RBPU=1;
	BANKSEL	_OPTION_REGbits
	BSF	_OPTION_REGbits,7
;	.line	93; "bootloader.c"	if (i==0) {
	BANKSEL	r0x102F
	MOVF	r0x102F,W
	BTFSS	STATUS,2
	GOTO	_00117_DS_
	CLRWDT
	PAGESEL	jumpToApplication;
	GOTO	jumpToApplication;
	
_00117_DS_:
forceBootload:
;	.line	104; "bootloader.c"	LED_YELLOW_TRIS = 0; // Pin as output
	BANKSEL	_TRISAbits
	BCF	_TRISAbits,5
;	.line	105; "bootloader.c"	LED_RED_TRIS = 0; // Pin as output
	BCF	_TRISCbits,1
;	.line	106; "bootloader.c"	LED_RED2_TRIS = 0; // Pin as output
	BCF	_TRISDbits,0
;	.line	107; "bootloader.c"	LED_YELLOW_PORT = 1; // LED on
	BANKSEL	_PORTAbits
	BSF	_PORTAbits,5
;	.line	109; "bootloader.c"	RECV_TRIS = 1; //reciever is input
	BANKSEL	_TRISBbits
	BSF	_TRISBbits,0
;	.line	110; "bootloader.c"	RECV_PORT = 0;
	BANKSEL	_PORTBbits
	BCF	_PORTBbits,0
;	.line	111; "bootloader.c"	INTCONbits.GIE = 0; //global enable interrupts
	BCF	_INTCONbits,7
;	.line	112; "bootloader.c"	INTCONbits.PEIE = 0; //enable peripheral interrupts
	BCF	_INTCONbits,6
;	.line	115; "bootloader.c"	SPBRG = BAUD_REG_VALUE; //set baud rate
	MOVLW	0x78
	BANKSEL	_SPBRG
	MOVWF	_SPBRG
;	.line	116; "bootloader.c"	TXSTAbits.BRGH = 1; // high baud rate mode (prescaler 16x)
	BSF	_TXSTAbits,2
;	.line	117; "bootloader.c"	RCSTAbits.SPEN = 1; // enable serial port
	BANKSEL	_RCSTAbits
	BSF	_RCSTAbits,7
;	.line	118; "bootloader.c"	TXSTAbits.SYNC = 0; // async mode
	BANKSEL	_TXSTAbits
	BCF	_TXSTAbits,4
;	.line	121; "bootloader.c"	RCSTAbits.CREN = 1;
	BANKSEL	_RCSTAbits
	BSF	_RCSTAbits,4
;	.line	122; "bootloader.c"	recvidx = -2;
	MOVLW	0xfe
	BANKSEL	_recvidx
	MOVWF	_recvidx
;	.line	123; "bootloader.c"	recvflags.escape = 0;
	BANKSEL	_recvflags
	BCF	_recvflags,0
;	.line	125; "bootloader.c"	RS485_RE_TRIS = 0; RS485_DE_TRIS = 0;
	BANKSEL	_TRISBbits
	BCF	_TRISBbits,4
	BCF	_TRISBbits,5
;	.line	126; "bootloader.c"	RS485_RE_PORT = 0; RS485_DE_PORT = 0; //RE is inverted
	BANKSEL	_PORTBbits
	BCF	_PORTBbits,4
	BCF	_PORTBbits,5
	CLRWDT	
;	.line	129; "bootloader.c"	cksum = 0x2A;
	MOVLW	0x2a
	BANKSEL	_cksum
	MOVWF	_cksum
;	.line	130; "bootloader.c"	RS485_DE_PORT = 1; //enable rs485 driver
	BANKSEL	_PORTBbits
	BSF	_PORTBbits,5
;	.line	131; "bootloader.c"	TXSTAbits.TXEN = 1;
	BANKSEL	_TXSTAbits
	BSF	_TXSTAbits,5
;	.line	132; "bootloader.c"	rs485_send_byte(0xfc);
	MOVLW	0xfc
	BANKSEL	_TXREG
	MOVWF	_TXREG
_00118_DS_:
	BANKSEL	_PIR1bits
	BTFSS	_PIR1bits,4
	GOTO	_00118_DS_
;	.line	133; "bootloader.c"	rs485_send_byte_ck(0xff);
	MOVLW	0xff
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	134; "bootloader.c"	rs485_send_byte_ck(MY_ADDR);
	MOVLW	0x40
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	135; "bootloader.c"	rs485_send_byte_ck(3|PF_ACK); rs485_send_byte_ck(0xfd); rs485_send_byte_ck(0xbb); rs485_send_byte_ck(i);
	MOVLW	0x43
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xfd
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xbb
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	BANKSEL	r0x102F
	MOVF	r0x102F,W
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	136; "bootloader.c"	rs485_message_trailer();
	PAGESEL	_rs485_message_trailer
	CALL	_rs485_message_trailer
	PAGESEL	$
;	.line	138; "bootloader.c"	LED_YELLOW_PORT=0;
	BANKSEL	_PORTAbits
	BCF	_PORTAbits,5
_00185_DS_:
	CLRWDT	
;	.line	142; "bootloader.c"	if(countdown==0) {
	BANKSEL	r0x102B
	MOVF	r0x102B,W
	IORWF	r0x102C,W
	IORWF	r0x102D,W
	IORWF	r0x102E,W
	BTFSS	STATUS,2
	GOTO	_00122_DS_
	CLRWDT
	PAGESEL	jumpToApplication;
	GOTO	jumpToApplication;
	
_00122_DS_:
;	.line	149; "bootloader.c"	countdown--;
	MOVLW	0xff
	BANKSEL	r0x102B
	ADDWF	r0x102B,F
	MOVLW	0xff
	BTFSS	STATUS,0
	ADDWF	r0x102C,F
	MOVLW	0xff
	BTFSS	STATUS,0
	ADDWF	r0x102D,F
	MOVLW	0xff
	BTFSS	STATUS,0
	ADDWF	r0x102E,F
;	.line	150; "bootloader.c"	if ((countdown&0xffff) == 0) {
	MOVF	r0x102B,W
	ANDLW	0xff
	BTFSS	STATUS,2
	GOTO	_00127_DS_
	MOVF	r0x102C,W
	ANDLW	0xff
	BTFSS	STATUS,2
	GOTO	_00127_DS_
;	.line	151; "bootloader.c"	if(LED_RED2_PORT)LED_RED2_PORT=0; else LED_RED2_PORT=1;
	BANKSEL	_PORTDbits
	BTFSS	_PORTDbits,0
	GOTO	_00124_DS_
	BCF	_PORTDbits,0
	GOTO	_00127_DS_
_00124_DS_:
	BANKSEL	_PORTDbits
	BSF	_PORTDbits,0
_00127_DS_:
;	.line	154; "bootloader.c"	if (RCSTAbits.OERR) { // on overflow - skip current received packet, clear OERR by toggling CREN
	BANKSEL	_RCSTAbits
	BTFSS	_RCSTAbits,1
	GOTO	_00129_DS_
;	.line	155; "bootloader.c"	recvidx = -2;
	MOVLW	0xfe
	BANKSEL	_recvidx
	MOVWF	_recvidx
;	.line	156; "bootloader.c"	RCSTAbits.CREN = 0; RCSTAbits.CREN = 1;
	BANKSEL	_RCSTAbits
	BCF	_RCSTAbits,4
	BSF	_RCSTAbits,4
_00129_DS_:
;	.line	158; "bootloader.c"	if (PIR1bits.RCIF) {
	BANKSEL	_PIR1bits
	BTFSS	_PIR1bits,5
	GOTO	_00155_DS_
;	.line	159; "bootloader.c"	recvbyte = RCREG;
	MOVF	_RCREG,W
	BANKSEL	_recvbyte
	MOVWF	_recvbyte
;	.line	160; "bootloader.c"	if (recvbyte == 0xfc) { //start of packet
	MOVF	_recvbyte,W
	XORLW	0xfc
	BTFSS	STATUS,2
	GOTO	_00151_DS_
;	.line	161; "bootloader.c"	recvidx = -1;
	MOVLW	0xff
	BANKSEL	_recvidx
	MOVWF	_recvidx
	GOTO	_00155_DS_
_00151_DS_:
;	.line	164; "bootloader.c"	if (recvbyte == 0xfb) {//escape
	BANKSEL	_recvbyte
	MOVF	_recvbyte,W
	XORLW	0xfb
	BTFSS	STATUS,2
	GOTO	_00133_DS_
;	.line	165; "bootloader.c"	recvflags.escape = 1; goto skipRcv;
	BANKSEL	_recvflags
	BSF	_recvflags,0
	GOTO	_00155_DS_
_00133_DS_:
;	.line	166; "bootloader.c"	} else if (recvflags.escape) {
	BANKSEL	_recvflags
	BTFSS	_recvflags,0
	GOTO	_00134_DS_
;	.line	167; "bootloader.c"	recvbyte += 0x10;
	MOVLW	0x10
	BANKSEL	_recvbyte
	ADDWF	_recvbyte,F
;	.line	168; "bootloader.c"	recvflags.escape = 0;
	BANKSEL	_recvflags
	BCF	_recvflags,0
;;signed compare: left < lit(0xFFFFFFFD=-3), size=1, mask=ff
_00134_DS_:
;	.line	170; "bootloader.c"	switch (recvidx) {
	BANKSEL	_recvidx
	MOVF	_recvidx,W
	ADDLW	0x80
	ADDLW	0x83
	BTFSS	STATUS,0
	GOTO	_00142_DS_
;;genSkipc:3257: created from rifx:0x7fff53da83b8
;;swapping arguments (AOP_TYPEs 1/3)
;;signed compare: left >= lit(0x1=1), size=1, mask=ff
	MOVF	_recvidx,W
	ADDLW	0x80
	ADDLW	0x7f
	BTFSC	STATUS,0
	GOTO	_00142_DS_
;;genSkipc:3257: created from rifx:0x7fff53da83b8
	MOVLW	0x03
	ADDWF	_recvidx,W
	BANKSEL	r0x1030
	MOVWF	r0x1030
	MOVLW	HIGH(_00305_DS_)
	BANKSEL	PCLATH
	MOVWF	PCLATH
	MOVLW	_00305_DS_
	BANKSEL	r0x1030
	ADDWF	r0x1030,W
	BTFSS	STATUS,0
	GOTO	_00001_DS_
	BANKSEL	PCLATH
	INCF	PCLATH,F
_00001_DS_:
	MOVWF	PCL
_00305_DS_:
	GOTO	_00136_DS_
	GOTO	_00136_DS_
	GOTO	_00137_DS_
	GOTO	_00141_DS_
_00136_DS_:
;	.line	173; "bootloader.c"	break;
	GOTO	_00155_DS_
_00137_DS_:
;	.line	175; "bootloader.c"	if (recvbyte == MY_ADDR /*|| recvbyte == 0xff*/) {
	BANKSEL	_recvbyte
	MOVF	_recvbyte,W
	XORLW	0x40
	BTFSS	STATUS,2
	GOTO	_00139_DS_
;	.line	176; "bootloader.c"	recvidx = 0;
	BANKSEL	_recvidx
	CLRF	_recvidx
;	.line	177; "bootloader.c"	recvcksum = 0x2a ^ recvbyte;
	MOVLW	0x2a
	BANKSEL	_recvbyte
	XORWF	_recvbyte,W
	BANKSEL	_recvcksum
	MOVWF	_recvcksum
	GOTO	_00155_DS_
_00139_DS_:
;	.line	179; "bootloader.c"	} else recvidx = -2;
	MOVLW	0xfe
	BANKSEL	_recvidx
	MOVWF	_recvidx
;	.line	180; "bootloader.c"	break;
	GOTO	_00155_DS_
;;gen.c:6559: size=0, offset=0, AOP_TYPE(res)=8
_00141_DS_:
;	.line	184; "bootloader.c"	recvpkg[PI_FROMADDR] = recvbyte;
	BANKSEL	_recvbyte
	MOVF	_recvbyte,W
	BANKSEL	_recvpkg
	MOVWF	(_recvpkg + 0)
;;gen.c:6559: size=0, offset=0, AOP_TYPE(res)=8
;	.line	185; "bootloader.c"	recvpkg[PI_LENGTH] = 31;
	MOVLW	0x1f
	MOVWF	(_recvpkg + 1)
;	.line	186; "bootloader.c"	recvcksum ^= recvbyte;
	BANKSEL	_recvbyte
	MOVF	_recvbyte,W
	BANKSEL	_recvcksum
	XORWF	_recvcksum,F
;	.line	187; "bootloader.c"	recvidx++;
	BANKSEL	_recvidx
	INCF	_recvidx,F
;	.line	188; "bootloader.c"	break;
	GOTO	_00155_DS_
_00142_DS_:
;	.line	190; "bootloader.c"	if (recvidx >= (recvpkg[PI_LENGTH] & PM_LENGTH)+2) { //just received the checksum, done
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 1),W
	BANKSEL	r0x1030
	MOVWF	r0x1030
	MOVLW	0x1f
	ANDWF	r0x1030,F
	MOVF	r0x1030,W
	MOVWF	r0x1031
	CLRF	r0x1032
	MOVLW	0x02
	ADDWF	r0x1031,W
	MOVWF	r0x1030
	CLRF	r0x1033
	RLF	r0x1033,F
	MOVLW	0x00
	ADDWF	r0x1033,F
	BANKSEL	_recvidx
	MOVF	_recvidx,W
	BANKSEL	r0x1031
	MOVWF	r0x1031
	CLRF	r0x1032
	BTFSC	r0x1031,7
	DECF	r0x1032,F
	MOVF	r0x1032,W
	ADDLW	0x80
	MOVWF	r0x1034
	MOVF	r0x1033,W
	ADDLW	0x80
	SUBWF	r0x1034,W
	BTFSS	STATUS,2
	GOTO	_00306_DS_
	MOVF	r0x1030,W
	SUBWF	r0x1031,W
_00306_DS_:
	BTFSS	STATUS,0
	GOTO	_00147_DS_
;;genSkipc:3257: created from rifx:0x7fff53da83b8
;	.line	191; "bootloader.c"	if (recvcksum == recvbyte) 
	BANKSEL	_recvbyte
	MOVF	_recvbyte,W
	BANKSEL	_recvcksum
	XORWF	_recvcksum,W
	BTFSS	STATUS,2
	GOTO	_00144_DS_
;	.line	192; "bootloader.c"	recvidx = -3;
	MOVLW	0xfd
	BANKSEL	_recvidx
	MOVWF	_recvidx
	GOTO	_00155_DS_
_00144_DS_:
;	.line	194; "bootloader.c"	recvidx = -2; //ignore packet
	MOVLW	0xfe
	BANKSEL	_recvidx
	MOVWF	_recvidx
	GOTO	_00155_DS_
_00147_DS_:
;	.line	197; "bootloader.c"	recvpkg[recvidx++] = recvbyte;
	BANKSEL	_recvidx
	MOVF	_recvidx,W
	BANKSEL	r0x1030
	MOVWF	r0x1030
	BANKSEL	_recvidx
	INCF	_recvidx,F
	BANKSEL	r0x1030
	MOVF	r0x1030,W
	ADDLW	(_recvpkg + 0)
	MOVWF	r0x1030
	MOVLW	high (_recvpkg + 0)
	BTFSC	STATUS,0
	ADDLW	0x01
	MOVWF	r0x1031
	MOVF	r0x1030,W
	BANKSEL	FSR
	MOVWF	FSR
	BCF	STATUS,7
	BANKSEL	r0x1031
	BTFSC	r0x1031,0
	BSF	STATUS,7
	BANKSEL	_recvbyte
	MOVF	_recvbyte,W
	BANKSEL	INDF
	MOVWF	INDF
;	.line	198; "bootloader.c"	recvcksum ^= recvbyte;
	BANKSEL	_recvbyte
	MOVF	_recvbyte,W
	BANKSEL	_recvcksum
	XORWF	_recvcksum,F
_00155_DS_:
;	.line	208; "bootloader.c"	if (recvidx == -3) { // incoming message in recvpkg
	BANKSEL	_recvidx
	MOVF	_recvidx,W
	XORLW	0xfd
	BTFSS	STATUS,2
	GOTO	_00185_DS_
;	.line	209; "bootloader.c"	countdown|=0x0f000000;
	MOVLW	0x0f
	BANKSEL	r0x102E
	IORWF	r0x102E,F
;	.line	212; "bootloader.c"	if (recvpkg[PI_LENGTH] & (PF_ACK|PF_ERROR)) {
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 1),W
	BANKSEL	r0x1030
	MOVWF	r0x1030
	ANDLW	0x60
	BTFSS	STATUS,2
	GOTO	_00181_DS_
;	.line	216; "bootloader.c"	cksum = 0x2A;
	MOVLW	0x2a
	BANKSEL	_cksum
	MOVWF	_cksum
;	.line	217; "bootloader.c"	RS485_DE_PORT = 1; //enable rs485 driver
	BANKSEL	_PORTBbits
	BSF	_PORTBbits,5
;	.line	219; "bootloader.c"	TXSTAbits.TXEN = 1;
	BANKSEL	_TXSTAbits
	BSF	_TXSTAbits,5
;	.line	221; "bootloader.c"	rs485_send_byte(0xfc);
	MOVLW	0xfc
	BANKSEL	_TXREG
	MOVWF	_TXREG
_00156_DS_:
	BANKSEL	_PIR1bits
	BTFSS	_PIR1bits,4
	GOTO	_00156_DS_
;	.line	222; "bootloader.c"	rs485_send_byte_ck(recvpkg[PI_FROMADDR]);
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 0),W
	BANKSEL	r0x1030
	MOVWF	r0x1030
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	223; "bootloader.c"	rs485_send_byte_ck(MY_ADDR);
	MOVLW	0x40
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	225; "bootloader.c"	switch (recvpkg[PI_CMD]) { //command
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 2),W
	BANKSEL	r0x1030
	MOVWF	r0x1030
	XORLW	0xb0
	BTFSC	STATUS,2
	GOTO	_00160_DS_
	MOVF	r0x1030,W
	XORLW	0xb1
	BTFSC	STATUS,2
	GOTO	_00159_DS_
	MOVF	r0x1030,W
	XORLW	0xb9
	BTFSC	STATUS,2
	GOTO	_00169_DS_
	MOVF	r0x1030,W
	XORLW	0xbf
	BTFSC	STATUS,2
	GOTO	_00175_DS_
	MOVF	r0x1030,W
	XORLW	0xf1
	BTFSC	STATUS,2
	GOTO	_00176_DS_
	GOTO	_00177_DS_
_00159_DS_:
;	.line	227; "bootloader.c"	rs485_send_byte_ck(1|PF_ACK);
	MOVLW	0x41
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	228; "bootloader.c"	rs485_send_byte_ck(0xb1);
	MOVLW	0xb1
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	229; "bootloader.c"	rs485_message_trailer();
	PAGESEL	_rs485_message_trailer
	CALL	_rs485_message_trailer
	PAGESEL	$
	CLRWDT
	PAGESEL	jumpToApplication;
	GOTO	jumpToApplication;
	
_00160_DS_:
;	.line	239; "bootloader.c"	LED_YELLOW_PORT=1;
	BANKSEL	_PORTAbits
	BSF	_PORTAbits,5
;	.line	240; "bootloader.c"	if (recvpkg[PI_DATA]==0 && recvpkg[PI_DATA+1]<4){rs485_send_byte_ck(2|PF_ACK|PF_ERROR);rs485_send_byte_ck(0xb0);rs485_send_byte_ck(0xCC);break;}
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 3),W
	BANKSEL	r0x1030
	MOVWF	r0x1030
	BTFSS	STATUS,2
	GOTO	_00162_DS_
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 4),W
	BANKSEL	r0x1030
	MOVWF	r0x1030
;;unsigned compare: left < lit(0x4=4), size=1
	MOVLW	0x04
	SUBWF	r0x1030,W
	BTFSC	STATUS,0
	GOTO	_00162_DS_
;;genSkipc:3257: created from rifx:0x7fff53da83b8
	MOVLW	0x62
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xb0
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xcc
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	GOTO	_00181_DS_
_00162_DS_:
;	.line	241; "bootloader.c"	if (recvpkg[PI_DATA]>0x1b){rs485_send_byte_ck(2|PF_ACK|PF_ERROR);rs485_send_byte_ck(0xb0);rs485_send_byte_ck(0xCC);break;}
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 3),W
	BANKSEL	r0x1030
	MOVWF	r0x1030
;;swapping arguments (AOP_TYPEs 1/2)
;;unsigned compare: left >= lit(0x1C=28), size=1
	MOVLW	0x1c
	SUBWF	r0x1030,W
	BTFSS	STATUS,0
	GOTO	_00165_DS_
;;genSkipc:3257: created from rifx:0x7fff53da83b8
	MOVLW	0x62
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xb0
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xcc
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	GOTO	_00181_DS_
_00165_DS_:
;	.line	242; "bootloader.c"	EEADRH = recvpkg[PI_DATA];
	BANKSEL	r0x1030
	MOVF	r0x1030,W
	BANKSEL	_EEADRH
	MOVWF	_EEADRH
;	.line	243; "bootloader.c"	EEADR = recvpkg[PI_DATA+1];
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 4),W
	BANKSEL	_EEADR
	MOVWF	_EEADR
;	.line	245; "bootloader.c"	i=PI_DATA+2;
	MOVLW	0x05
	BANKSEL	r0x102F
	MOVWF	r0x102F
_00166_DS_:
;	.line	247; "bootloader.c"	EEDATA=recvpkg[i];
	BANKSEL	r0x102F
	MOVF	r0x102F,W
	ADDLW	(_recvpkg + 0)
	MOVWF	r0x1030
	MOVLW	high (_recvpkg + 0)
	BTFSC	STATUS,0
	ADDLW	0x01
	MOVWF	r0x1031
	MOVF	r0x1030,W
	BANKSEL	FSR
	MOVWF	FSR
	BCF	STATUS,7
	BANKSEL	r0x1031
	BTFSC	r0x1031,0
	BSF	STATUS,7
	BANKSEL	INDF
	MOVF	INDF,W
	BANKSEL	_EEDATA
	MOVWF	_EEDATA
;	.line	248; "bootloader.c"	i++;
	BANKSEL	r0x102F
	INCF	r0x102F,F
;	.line	249; "bootloader.c"	EEDATH=recvpkg[i];
	MOVF	r0x102F,W
	ADDLW	(_recvpkg + 0)
	MOVWF	r0x1030
	MOVLW	high (_recvpkg + 0)
	BTFSC	STATUS,0
	ADDLW	0x01
	MOVWF	r0x1031
	MOVF	r0x1030,W
	BANKSEL	FSR
	MOVWF	FSR
	BCF	STATUS,7
	BANKSEL	r0x1031
	BTFSC	r0x1031,0
	BSF	STATUS,7
	BANKSEL	INDF
	MOVF	INDF,W
	BANKSEL	_EEDATH
	MOVWF	_EEDATH
;	.line	250; "bootloader.c"	i++;
	BANKSEL	r0x102F
	INCF	r0x102F,F
	CLRWDT	
	BANKSEL	EECON1
	BSF	EECON1,EEPGD ; Point to program memory
	BSF	EECON1,WREN ; Enable writes
	MOVLW	0x55 ; Start of required write sequence:
	MOVWF	EECON2 ; Write 55h
	MOVLW	0xAA ;
	MOVWF	EECON2 ; Write AAh
	BSF	EECON1,WR ; Set WR bit to begin write
	NOP	; Any instructions here are ignored as processor
	;	halts to begin write sequence
	NOP	; processor will stop here and wait for write complete
	;	after write processor continues with 3rd instruction
	BCF	EECON1,WREN ; Disable writes
	
;	.line	269; "bootloader.c"	EEADR++;
	BANKSEL	_EEADR
	INCF	_EEADR,F
;	.line	270; "bootloader.c"	}while(i<=recvpkg[PI_LENGTH]);
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 1),W
	BANKSEL	r0x1030
	MOVWF	r0x1030
	MOVF	r0x102F,W
	SUBWF	r0x1030,W
	BTFSC	STATUS,0
	GOTO	_00166_DS_
;;genSkipc:3257: created from rifx:0x7fff53da83b8
;	.line	272; "bootloader.c"	rs485_send_byte_ck(4|PF_ACK);rs485_send_byte_ck(0xb0);rs485_send_byte_ck(recvpkg[PI_LENGTH]);rs485_send_byte_ck(recvcksum);rs485_send_byte_ck(i);
	MOVLW	0x44
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xb0
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 1),W
	BANKSEL	r0x1030
	MOVWF	r0x1030
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	BANKSEL	_recvcksum
	MOVF	_recvcksum,W
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	BANKSEL	r0x102F
	MOVF	r0x102F,W
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	273; "bootloader.c"	LED_YELLOW_PORT=0;
	BANKSEL	_PORTAbits
	BCF	_PORTAbits,5
;	.line	274; "bootloader.c"	break;
	GOTO	_00181_DS_
_00169_DS_:
;	.line	277; "bootloader.c"	rs485_send_byte_ck(31|PF_ACK);rs485_send_byte_ck(0xb9);
	MOVLW	0x5f
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xb9
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	278; "bootloader.c"	EEADRH = recvpkg[PI_DATA];
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 3),W
	BANKSEL	_EEADRH
	MOVWF	_EEADRH
;	.line	279; "bootloader.c"	EEADR = recvpkg[PI_DATA+1];
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 4),W
	BANKSEL	_EEADR
	MOVWF	_EEADR
;	.line	280; "bootloader.c"	EECON1bits.EEPGD = 1;
	BANKSEL	_EECON1bits
	BSF	_EECON1bits,7
;	.line	282; "bootloader.c"	while (i>0) {
	MOVLW	0x0f
	BANKSEL	r0x102F
	MOVWF	r0x102F
_00172_DS_:
	MOVLW	0x00
	BANKSEL	r0x102F
	IORWF	r0x102F,W
	BTFSC	STATUS,2
	GOTO	_00181_DS_
	BANKSEL	EECON1
	BSF	EECON1, RD
	NOP
	NOP
	
;	.line	289; "bootloader.c"	rs485_send_byte_ck(EEDATA);
	BANKSEL	_EEDATA
	MOVF	_EEDATA,W
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	290; "bootloader.c"	rs485_send_byte_ck(EEDATH);
	BANKSEL	_EEDATH
	MOVF	_EEDATH,W
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	291; "bootloader.c"	EEADR++;
	BANKSEL	_EEADR
	INCF	_EEADR,F
;	.line	292; "bootloader.c"	if (EEADR==0)EEADRH++;
	MOVLW	0x00
;	.line	293; "bootloader.c"	i--;
	IORWF	_EEADR,W
	BTFSC	STATUS,2
	INCF	_EEADRH,F
	BANKSEL	r0x102F
	DECF	r0x102F,F
	GOTO	_00172_DS_
_00175_DS_:
;	.line	298; "bootloader.c"	rs485_send_byte_ck(1|PF_ACK); rs485_send_byte_ck(0xbf);
	MOVLW	0x41
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xbf
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	299; "bootloader.c"	break;
	GOTO	_00181_DS_
_00176_DS_:
;	.line	301; "bootloader.c"	rs485_send_byte_ck(2|PF_ACK); rs485_send_byte_ck(0xf1); rs485_send_byte_ck(recvpkg[PI_DATA]);
	MOVLW	0x42
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	MOVLW	0xf1
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 3),W
	BANKSEL	r0x102F
	MOVWF	r0x102F
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
;	.line	302; "bootloader.c"	break;
	GOTO	_00181_DS_
_00177_DS_:
;	.line	304; "bootloader.c"	rs485_send_byte_ck(1|PF_ACK|PF_ERROR); rs485_send_byte_ck(recvpkg[PI_CMD]);
	MOVLW	0x61
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
	BANKSEL	_recvpkg
	MOVF	(_recvpkg + 2),W
	BANKSEL	r0x102F
	MOVWF	r0x102F
	PAGESEL	_rs485_send_byte_ck
	CALL	_rs485_send_byte_ck
	PAGESEL	$
_00181_DS_:
;	.line	308; "bootloader.c"	rs485_message_trailer();
	PAGESEL	_rs485_message_trailer
	CALL	_rs485_message_trailer
	PAGESEL	$
;	.line	309; "bootloader.c"	recvidx = -2; //start listening again
	MOVLW	0xfe
	BANKSEL	_recvidx
	MOVWF	_recvidx
	GOTO	_00185_DS_
	RETURN	
; exit point of _bootload
















;	code size estimation:
;	  368+  142 =   510 instructions ( 1304 byte)

	end
