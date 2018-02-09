#include "../features/SwitchedSocket.h"
#include "../features/StatusLed.h"
#include "Device.h"


constexpr const char L1_NAME[] = "l1";
constexpr const char L2_NAME[] = "l2";
constexpr const char L3_NAME[] = "l3";
constexpr const char L4_NAME[] = "l4";
constexpr const char STATUS_NAME[] = "status";

struct Sonoff4chConfig {
    struct SwitchedSocketConfig l1Config;
    struct SwitchedSocketConfig l2Config;
    struct SwitchedSocketConfig l3Config;
    struct SwitchedSocketConfig l4Config;
}

class Sonoff4ch : public Device {
public:

    Sonoff4ch() :
            l1(this, L1_NAME, 12,12,  0, config->l1Config),
            l2(this, L2_NAME, 5 , 5,  9, config->l2Config),
            l3(this, L3_NAME, 4 , 4, 10, config->l3Config),
            l4(this, L4_NAME, 15,15, 14, config->l4Config),
            status(this) {
        this->add(&(this->l1));
        this->add(&(this->l2));
        this->add(&(this->l3));
        this->add(&(this->l4));
        this->add(&(this->status));
    }

    void start() {
        Device::start();
    }

private:
    SwitchedSocket l1;
    SwitchedSocket l2;
    SwitchedSocket l3;
    SwitchedSocket l4;
    StatusLed<STATUS_NAME, 13, true> status;
};

Device *createDevice() {
    return new Sonoff4ch();
}
