
#include "Device.h"

struct artnet_header {
    char magic[8];
    uint16_t opcode; //little endian
    uint16_t version; //network byte order (BE)
    uint8_t sequence;
    uint8_t physical;
    uint16_t universe; //little endian
    uint16_t length; //network byte order (BE)
};

HardwareSerial *Serial1;

//timer
os_timer_t brk_timer;
char dmx_data[512];
int dmx_data_len = 0;
void ICACHE_FLASH_ATTR clr_brk_bit()
{
    CLEAR_PERI_REG_MASK(UART_CONF0(UART0), UART_TXD_BRK);
    for(int i=0;i<250;i++) { __asm("nop"); }
    Serial.write(dmx_data, dmx_data_len);
}

LOCAL void ICACHE_FLASH_ATTR dmx_uart_config()
{
    os_timer_setfn(&brk_timer, (os_timer_func_t *)clr_brk_bit, NULL);
}

class DmxNode : public Device {
public:
    DmxNode() :
            artnetUdp(UdpConnectionDataDelegate(&DmxNode::onUDPReceive, this))
            {
        
        debug_d("\n\nswitching serial...");
        Serial1 = new HardwareSerial(UART1);
        Serial1->begin(76800);
        delay(50);
        Serial1->systemDebugOutput(true); // Redirect debug output to Serial1
        delay(20);
        Serial.begin(250000); // change baud rate
        //Serial.swap(); //GPIO15 (TX) and GPIO13 (RX) 
        delay(100);
        debug_d("\n\nhello, debug serial");
        
        artnetUdp.listen(ArtnetPort);

        //this->registerSubscription(Device::TOPIC_BASE + "/led3/set", Device::MessageCallback(&DmxNode::onAction_Output, this));
        pinMode(4, OUTPUT);
        digitalWrite(4, HIGH);
        dmx_uart_config();
    }

    void onUDPReceive(UdpConnection& connection, char *data, int size, IPAddress remoteIP, uint16_t remotePort)
    {
        debug_d("[ArtNet] UDP Server callback from %s:%d, %d bytes", remoteIP.toString().c_str(), remotePort, size);
        if (size < sizeof(struct artnet_header)) {
            debug_w("[ArtNet] invalid len %d<%d", size, sizeof(struct artnet_header));
            return;
        }
        struct artnet_header *header = (struct artnet_header *)data;
        if (strcmp("Art-Net", header->magic) != 0) {
            debug_w("[ArtNet] invalid magic");
            return;
        }

        char *payload = data + sizeof(struct artnet_header);
        int payload_len = size - sizeof(struct artnet_header);
        if (payload_len > ntohs(header->length)) payload_len = ntohs(header->length);
        if (payload_len > 512) payload_len = 512;
        debug_d("[ArtNet] op %04x, ver %d, seq %d, phy %d, uni %d - Forwarding %d bytes", 
            header->opcode, ntohs(header->version), header->sequence, header->physical, header->universe, payload_len);
        //   1. set UART_TXFIFO_RST,         
        SET_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);
        CLEAR_PERI_REG_MASK(UART_CONF0(UART0), UART_RXFIFO_RST | UART_TXFIFO_RST);
        //   2. set UART_TXD_BRK
        SET_PERI_REG_MASK(UART_CONF0(UART0), UART_TXD_BRK);
        //   3. set a timer to RESET UART_TXD_BRK
        os_timer_arm(&brk_timer,1,0);
        memcpy(dmx_data, payload, payload_len);
        dmx_data_len = payload_len;

        
    }


private:
    IPAddress lastUdpIP;
    uint16_t lastUdpPort;

    // UDP server
    const uint16_t ArtnetPort = 0x1936;
    UdpConnection artnetUdp;

};

Device *createDevice() {
    return new DmxNode();
}
