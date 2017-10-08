#include "../features/Ds1820Sensor.h"

#include "Device.h"

#define RS485_BUFLEN 37
#define MY_ADDR 0
#include "rs485_packet.h"
#include "rs485.h"

constexpr const char THERMO_NAME[] = "ds";
constexpr const int THERMO_GPIO = 4;

int char2int(char input){
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  return 0;
}

// This function assumes src to be a zero terminated sanitized string with
// an even number of [0-9a-f] characters, and target to be sufficiently large
void hex2bin(const char* src, uint8_t* target){
  while(*src && src[1]) {
    *(target++) = char2int(*src)*16 + char2int(src[1]);
    src += 2;
  }
}


HardwareSerial *Serial1;

class AvrCtrl : public Device {
public:
    AvrCtrl() :
            dsSensor(this),
            udp(UdpConnectionDataDelegate(&AvrCtrl::onUDPReceive, this)),
            ntp("0.de.pool.ntp.org", 86400)
            {
        

        Serial1 = new HardwareSerial(UART1);
        Serial1->begin(76800);
        delay(50);
        Serial1->systemDebugOutput(true); // Redirect debug output to Serial1
        delay(20);
        Serial.begin(38400); // change baud rate
        Serial.swap(); //GPIO15 (TX) and GPIO13 (RX) 
        delay(100);
        rs485_message_send(0xff, 6, (uint8_t*) "\375proxy");
        debug_d("\n\nhello, debug serial");
        
        udp.listen(UdpSerialPort);

        this->add(&(this->dsSensor));
        this->registerProperty(Device::TOPIC_BASE + "buzzer",
                NodeProperty(Device::MessageCallback(&AvrCtrl::onAction_Buzzer, this), PropertyDataType::String, "", "", ""));
        this->registerProperty(Device::TOPIC_BASE + "display",
                NodeProperty(Device::MessageCallback(&AvrCtrl::onAction_Display, this), PropertyDataType::String, "", "", ""));
        
        this->registerProperty(Device::TOPIC_BASE + "door",
                NodeProperty(Device::MessageCallback(&AvrCtrl::onAction_Output, this), PropertyDataType::Boolean, "", "", ""));
        this->registerProperty(Device::TOPIC_BASE + "backlight",
                NodeProperty(Device::MessageCallback(&AvrCtrl::onAction_Output, this), PropertyDataType::Boolean, "", "", ""));
        this->registerProperty(Device::TOPIC_BASE + "led1",
                NodeProperty(Device::MessageCallback(&AvrCtrl::onAction_Output, this), PropertyDataType::Boolean, "", "", ""));
        this->registerProperty(Device::TOPIC_BASE + "led2",
                NodeProperty(Device::MessageCallback(&AvrCtrl::onAction_Output, this), PropertyDataType::Boolean, "", "", ""));
        this->registerProperty(Device::TOPIC_BASE + "led3",
                NodeProperty(Device::MessageCallback(&AvrCtrl::onAction_Output, this), PropertyDataType::Boolean, "", "", ""));
        
        if(!Serial.setCallback(StreamDataReceivedDelegate(&AvrCtrl::onSerialData, this))) {
            debug_e("ERROR: Serial callback registration failed!");
        }

        avrClockTimer.initializeMs(10000, TimerDelegate(&AvrCtrl::updateAvrClock, this)).startOnce();
        onAction_Display("", "esper ini done");
    }

    void updateAvrClock() {
        DateTime dt = SystemClock.now();
        uint8_t cmd [] = { C_SET_TIME, (uint8_t)dt.Hour, (uint8_t)dt.Minute, (uint8_t)dt.Second };
        rs485_message_send(0xff, 4, cmd);
        avrClockTimer.setIntervalMs(600000);
        avrClockTimer.startOnce();
    }

    void onAction_Output(const String& topic, const String& message) {
        uint8_t cmd [] = { C_SET_OUTPUT };
        if (message == "1") cmd[2] = 0xff; else cmd[2] = 0x00;
        
        if (topic == "door/set") cmd[1] = 0x10;
        else if (topic == "backlight/set") cmd[1] = 0x0f;
        else if (topic == "led1/set") cmd[1] = 0x01;
        else if (topic == "led2/set") cmd[1] = 0x02;
        else if (topic == "led3/set") cmd[1] = 0x03;
        else debug_e("ERROR: unknown topic %s", topic.c_str());
        
        rs485_message_send(0x41, 3, cmd);
        publish(topic.substring(0, topic.length() - 3) + "state", cmd[2] == 0xff ? "1" : "0");
    }

    void onAction_Buzzer(const String& topic, const String& message) {
        uint8_t cmd [] = { C_BUZZER, 0, 0, 0, 0, 0 };
        const char *ptr = message.c_str();
        for (int i = 1; i <= 5 && *ptr; i++)
            cmd[i] = strtol(ptr, (char **) &ptr, 10);

        rs485_message_send(0x41, 6, cmd);
    }
    void onAction_Display(const String& topic, const String& message) {
        uint8_t cmd [33] = { C_SET_DISPLAY };
        strncpy((char*)&cmd[1], message.c_str(), 32);
        rs485_message_send(0x41, 31, cmd);
    }

    void forwardPacket() {
        debugf("received a serial rs485 packet");
        recvpkghexbuf[0] = 'f'; recvpkghexbuf[1] = 'c'; 
        debugf("data: %s", recvpkghexbuf);

        udp.sendStringTo(lastUdpIP, lastUdpPort, recvpkghexbuf);
        if (recvflags.broadcast) {
            udp.sendStringTo(IPAddress(255,255,255,255), UdpSerialPort, recvpkghexbuf);
        }

        if (recvpkg.st.command == C_ON_INPUT) {
            char topic[10];
            sprintf(topic, "button/%d", recvpkg.st.data[0]);
            publish(topic, "");
        }

        recvidx = -2; // set status: no packet available
    }

    void onSerialData(Stream &source, char arrivedChar, uint16_t availableCharsCount) {
        /*recvbyte = arrivedChar;
        receiveRS485();*/
        debugf("onSerialData arrivedChar=%02x, availableCharsCount=%d", arrivedChar, availableCharsCount);
        //if (recvidx == -3) forwardPacket();
        while(availableCharsCount --> 0) {
            recvbyte = source.read();
            receiveRS485();
            m_printf("%02x (%d), ", recvbyte, recvidx);
            if (recvidx == -3) {
                m_printf("\n");
                forwardPacket();
            }
        }
    }

    void onUDPReceive(UdpConnection& connection, char *data, int size, IPAddress remoteIP, uint16_t remotePort)
    {
        debug_d("UDP Server callback from %s:%d, %d bytes", remoteIP.toString().c_str(), remotePort, size);

        uint8_t bindata[40];
        String udpData(data);
        Vector<String> dataLines;
        int numLines = splitString(udpData, '\n' , dataLines);
        for (int i=0;i<numLines;i++)
        {
            const char *ptr = dataLines.at(i).c_str();
            int len = strlen(ptr);
            if (len > 4 && len < 86 && (len & 1) == 0) {
                hex2bin(ptr, bindata);
                if (bindata[0] == 0xFC) {
                    // packet looks kinda valid, send it onto serial port
                    Serial.write(bindata, len>>1);
                    debug_d("Forwarded UDP packet to avr");
                    // and store sender IP for the answer
                    lastUdpIP = remoteIP; lastUdpPort = remotePort;
                } else debug_w("UDP packet with invalid startchar 0x%02x", bindata[0]);
            } else debug_w("UDP packet with invalid len %d", len);
        }
        // Send echo to remote sender
        //String text = String("echo: ") + data;
        //udp.sendStringTo(remoteIP, remotePort, text);
    }


private:
    IPAddress lastUdpIP;
    uint16_t lastUdpPort;

    Ds1820Sensor<THERMO_NAME,THERMO_GPIO> dsSensor;
    // UDP server
    const uint16_t UdpSerialPort = 40485;
    UdpConnection udp;

    NtpClient ntp;
    Timer avrClockTimer;

};

Device *createDevice() {
    return new AvrCtrl();
}
