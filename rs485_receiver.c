

int8_t recvidx;
packetbuf recvpkg;
uint8_t recvcksum;


uint8_t recvbyte;
struct {
  unsigned int escape : 1;
  unsigned int broadcast : 1;
  unsigned int cksumerr : 1;
} recvflags;

inline void receiveRS485() {
	if (recvbyte == 0xfc) { //start of packet
		recvidx = -1;
		//LED_RED2_PORT = 1;
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
				//LED_YELLOW_PORT=1;
				//LED_RED2_PORT=0;
				recvpkg.fromaddr = recvbyte;
				recvpkg.length = 31;
				recvcksum ^= recvbyte;
				recvidx++;
				break;
			default: //waiting for content
				if (recvidx >= (recvpkg.length & PM_LENGTH)+2) { //just received the checksum, done
					if (recvcksum == recvbyte) 
						recvidx = -3;
					else
						recvidx = -2; //ignore packet
					//LED_YELLOW_PORT = 1;
				} else {
					recvpkg.raw[recvidx++] = recvbyte;
					recvcksum ^= recvbyte;
					//LED_YELLOW_PORT=0;
				}
				break;
			}
		}
	}
}
