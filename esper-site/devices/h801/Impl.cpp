#include "../features/Socket.h"
#include "../features/TimedButton.h"
#include "../features/Light.h"
#include "Device.h"
#include "HardwarePWM.h"

uint8_t PWM_PINS[] = {15,13,12,14,4};


class H801Device : public Device {
public:
    H801Device() : pwm(PWM_PINS, sizeof(PWM_PINS)) {
        
        this->registerProperty(Device::TOPIC_BASE + "/led/red",
                NodeProperty(Device::MessageCallback(&H801Device::onAction_Output, this), PropertyDataType::Integer, "", "0:200", ""));
        this->registerProperty(Device::TOPIC_BASE + "/led/green",
                NodeProperty(Device::MessageCallback(&H801Device::onAction_Output, this), PropertyDataType::Integer, "", "0:200", ""));
        this->registerProperty(Device::TOPIC_BASE + "/led/blue",
                NodeProperty(Device::MessageCallback(&H801Device::onAction_Output, this), PropertyDataType::Integer, "", "0:200", ""));
        this->registerProperty(Device::TOPIC_BASE + "/led/white",
                NodeProperty(Device::MessageCallback(&H801Device::onAction_Output, this), PropertyDataType::Integer, "", "0:200", ""));
        this->registerProperty(Device::TOPIC_BASE + "/led/white2",
                NodeProperty(Device::MessageCallback(&H801Device::onAction_Output, this), PropertyDataType::Integer, "", "0:200", ""));
        pwm.setPeriod(200);
    }

    void onAction_Output(const String& topic, const String& message) {
        int pin = 0;
        if (topic == "led/red") pin = 15;
        else if (topic == "led/green") pin = 13;
        else if (topic == "led/blue") pin = 12;
        else if (topic == "led/white") pin = 14;
        else if (topic == "led/white2") pin = 4;
        else debug_e("ERROR: unknown topic %s", topic.c_str());
        
        uint32_t value = atoi(message.c_str());
        pwm.analogWrite(pin, value);
        publish(Device::TOPIC_BASE + "/" + topic, message, true, true);
    }
private:
    void onStateChanged(const bool& state) {
    }

    HardwarePWM pwm;
};


Device* createDevice() {
    return new H801Device();
}
