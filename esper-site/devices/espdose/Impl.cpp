#include "../features/Socket.h"
#include "../features/BuiltinLED.h"
#include "../features/ToggleButton.h"
#include "../features/RotaryEncoder.h"
#include "Device.h"


constexpr const char SOCKET1_NAME[] = "socket1";
constexpr const uint16_t SOCKET1_GPIO = 12; //D6

constexpr const char SOCKET2_NAME[] = "socket2";
constexpr const uint16_t SOCKET2_GPIO = 13; //D5

constexpr const char BUILTINLED_NAME[] = "led";

constexpr const char ROTENC_NAME[] = "rotaryencoder";
constexpr const uint16_t ROTENC_GPIOA =  5; //D1
constexpr const uint16_t ROTENC_GPIOB =  4; //D2

constexpr const char BUTTON_NAME[] = "button";
constexpr const uint16_t BUTTON_GPIO =  0; //D3

class ESPDose : public Device {
public:
    ESPDose() :
            socket1(this),
            socket2(this),
            builtinled(this),
            button(this, decltype(button)::Callback(&ESPDose::onStateChanged, this)),
            rotenc(this, decltype(rotenc)::Callback(&ESPDose::onRotaryAction, this)) {
        this->add(&(this->socket1));
        this->add(&(this->socket2));
        this->add(&(this->builtinled));
        this->add(&(this->button));
        this->add(&(this->rotenc));
    }

private:
    void onStateChanged(const bool& state) {
        this->socket1.set(state);
        this->socket2.set(state);
        this->builtinled.set(state);
    }
    void onRotaryAction(bool state) {
    }
    OnOffFeature <SOCKET1_NAME, SOCKET1_GPIO, false, 0> socket1;
    OnOffFeature <SOCKET2_NAME, SOCKET2_GPIO, false, 0> socket2;
    BuiltinLED <BUILTINLED_NAME> builtinled;
    ToggleButton<BUTTON_NAME, BUTTON_GPIO, true> button;
    RotaryEncoder<ROTENC_NAME, ROTENC_GPIOA, ROTENC_GPIOB> rotenc;
};

Device *createDevice() {
    return new ESPDose();
}
