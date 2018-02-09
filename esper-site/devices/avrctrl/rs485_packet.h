

#define C_ON_INPUT  				 	0x02
#define C_SET_OUTPUT				 	0x05
#define C_INFORM_OUTPUT				 	0x06
#define C_GET_TEMPERATURE				0x40
#define C_SET_DISPLAY				 	0x41
#define C_SET_MENU				 		0x42
#define C_SET_MENU_ITEM				 	0x43
#define C_BUZZER				 		0x49
#define C_BMP_FILL_CONST				0x50
#define C_BMP_WRITE_RANGE				0x51
#define C_BMP_RANGE_FILL_CONST			0x52
#define C_BMP_RANGE_ADD_CONST			0x53
#define C_BMP_SCROLL_LEFT				0x54
#define C_BMP_SCROLL_RIGHT				0x55
#define C_BMP_TO_STRIPE_SINGLE			0x5e
#define C_BMP_TO_STRIPE_SET_INTERVAL	0x5f
#define C_PING							0xf1
#define C_SET_TIME   					0xf3
#define C_SET_BAUD_RATE					0xfc
#define C_REBOOT_TO_BOOTLOADER			0xbf

typedef union  {
	uint8_t raw[RS485_BUFLEN];
	struct {
		uint8_t fromaddr;
		uint8_t length;
		uint8_t command;
		uint8_t data[RS485_BUFLEN-3];
	} st;
} packetbuf;

#define PI_FROMADDR 0

#define PI_LENGTH 1
#define PM_LENGTH 0b00011111
#define PM_FLAGS  0b11100000

#define PI_CMD 2
#define PI_DATA 3
#define PI_TOADDR -1
#define PI_STARTBYTE -2

#define PF_RESERVED 0b10000000
#define PF_ACK 0b01000000
#define PF_ERROR 0b00100000

