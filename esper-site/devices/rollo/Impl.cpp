#include "../features/Stepper.h"
#include "../features/ToggleButton.h"
#include "Device.h"



constexpr const char BUTTON_NAME[] = "button";
constexpr const uint16_t BUTTON_GPIO = 0;

constexpr const char STEPPER_NAME[] = "rollo";


class RolloDevice : public Device {
public:
    RolloDevice() :
            rollo(this),
            button(this, decltype(button)::Callback(&RolloDevice::onStateChanged, this)) {
        this->add(&(this->rollo));
        this->add(&(this->button));
    }

private:
    void onStateChanged(const bool& state) {
    }

    Stepper<STEPPER_NAME> rollo;
    ToggleButton<BUTTON_NAME, BUTTON_GPIO, true> button;
};


Device* createDevice() {
    return new RolloDevice();
}
