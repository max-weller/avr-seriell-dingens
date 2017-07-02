
#include "rs485_packet.h"

int8_t recvidx;
packetbuf recvpkg;
uint8_t recvcksum;


uint8_t recvbyte;
struct {
  unsigned int escape : 1;
  unsigned int broadcast : 1;
  unsigned int cksumerr : 1;
} recvflags;

inline void receiveRS485(void) {
	if (recvbyte == 0xfc) { //start of packet
		recvidx = -1;
		//LED_RED2_PORT = 1;
	} else {
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
  rs485_start_sending();
  begin_critical_section();

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

  rs485_finish_sending();
  end_critical_section();
}


void rs485_ack(uint8_t arg) {
  recvpkg.st.data[0] = recvpkg.st.length; recvpkg.st.data[1] = recvcksum; recvpkg.st.data[2] = arg;
  rs485_message_send(recvpkg.st.fromaddr, (4 & PM_LENGTH) | PF_ACK, &recvpkg.st.command);
}

void rs485_error(void) {
	rs485_message_send(recvpkg.st.fromaddr, (recvpkg.st.length & PM_LENGTH) | PF_ACK | PF_ERROR, &recvpkg.st.command);
}



