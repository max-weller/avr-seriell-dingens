
#define rs485_send_byte(x) Serial.write(((uint8_t)(x)))

uint8_t cksum;

void rs485_send_byte_ck(uint8_t bt) {
    cksum ^= bt;
    if (bt == 0xfb || bt == 0xfc) {
        rs485_send_byte(0xfb);
        bt -= 0x10;
    }
    rs485_send_byte(bt);
}

void rs485_message_send(uint8_t destaddr, uint8_t length, uint8_t* data) {
    cksum = 0x2A;
	debugf("rs485_message_send to=%02x, len=%02x, cmd=%02x", destaddr, length, *data);
    rs485_send_byte(0xfc);
    rs485_send_byte_ck(destaddr);

    // TXSTAbits.TX9D = 0;
    // for(idx=0; idx!=4; idx++) { rs485_send_byte_ck(MY_ADDR[idx]) }
    rs485_send_byte_ck(MY_ADDR);

    rs485_send_byte_ck(length);
    length &= PM_LENGTH;
    for(; length!=0; length--) {
        rs485_send_byte_ck(*data);
        data++;
    }
    rs485_send_byte_ck(cksum);
    rs485_send_byte(0x00);
}




int8_t recvidx;
packetbuf recvpkg;
uint8_t recvcksum;

char recvpkghexbuf[(RS485_BUFLEN+2)*2];
int8_t recvhexbufidx;

uint8_t recvbyte;
struct {
  unsigned int escape : 1;
  unsigned int forme : 1;
  unsigned int broadcast : 1;
  unsigned int cksumerr : 1;
} recvflags;

void receiveRS485(void) {
	if (recvbyte == 0xfc) { //start of packet
		recvidx = -1;
		recvhexbufidx = 1;
	} else if (recvidx >= -1) {
        sprintf(&recvpkghexbuf[(recvhexbufidx++)<<1], "%02x", recvbyte);
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
				recvidx = 0;
				recvcksum = 0x2a ^ recvbyte;
				recvflags.broadcast = (recvbyte == 0xff)?1:0;
				recvflags.forme = (recvbyte == MY_ADDR | recvbyte == 0xff)?1:0;
				break;
			case PI_FROMADDR: //waiting for sender addr
				recvpkg.st.fromaddr = recvbyte;
				recvpkg.st.length = 31;
				recvcksum ^= recvbyte;
				recvidx++;
				break;
			default: //waiting for content
				if (recvidx >= (recvpkg.st.length & PM_LENGTH)+2) { //just received the checksum, done
					if (recvcksum == recvbyte) 
						recvidx = -3;
					else
						recvidx = -2; //ignore packet
				} else {
					recvpkg.raw[recvidx++] = recvbyte;
					recvcksum ^= recvbyte;
				}
				break;
			}
		}
	}
}

