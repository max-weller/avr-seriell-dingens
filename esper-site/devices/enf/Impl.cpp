#include "../features/Ds1820Sensor.h"

#include "Device.h"

#define RS485_BUFLEN 37
#define MY_ADDR 0
#include "rs485_packet.h"
#include "rs485.h"

uint16_t adc_buffer[1000];
uint16_t adc_idx;

class EnfNode : public Device {
public:
    EnfNode() :
            udp(UdpConnectionDataDelegate(&EnfNode::onUDPReceive, this))
            {
        
        debug_d("\n\nhello, debug serial");
        
        udp.listen(UdpSerialPort);

        this->registerProperty(Device::TOPIC_BASE + "/board/buzzer",
                NodeProperty(Device::MessageCallback(&EnfNode::onAction_Buzzer, this), PropertyDataType::String, "", "", ""));

        adc_idx = 0;
        avrClockTimer.initializeMs(1, TimerDelegate(&EnfNode::readAdc, this)).startOnce();
    }

    void checkAdr() {
        int a = system_adc_read();
        adc_buffer[adc_idx++] = a;
        if (adc_idx >= sizeof(adc_buffer) / sizeof(uint16_t)) {
            udp.send
        }
    }

    void onUDPReceive(UdpConnection& connection, char *data, int size, IPAddress remoteIP, uint16_t remotePort)
    {
    }

private:
    const uint16_t UdpSerialPort = 40485;
    UdpConnection udp;
    Timer avrClockTimer;

};

Device *createDevice() {
    return new EnfNode();
}
