

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

