#include "Device.h"

#include "features/NeopixelFeature.h"
#include "features/LightsdUDPFeature.h"
#include "features/Socket.h"
#include "features/ToggleButton.h"
#include "features/Light.h"


constexpr const char NEOPIXEL_NAME[] = "stripe";
constexpr const char LIGHTSD_NAME[] = "receiver";

constexpr const char SOCKET_NAME[] = "socket";
constexpr const uint16_t SOCKET_GPIO = 12;

constexpr const char BUTTON_NAME[] = "button";
constexpr const uint16_t BUTTON_GPIO = 0;

constexpr const char LIGHT_NAME[] = "light";
constexpr const uint16_t LIGHT_GPIO = 13;

template<size_t pixel_count>
class LedStripeDevice : public Device {
    using LightsdFeature = LightsdUDPFeature<LIGHTSD_NAME, 1337>;
    using StripeFeature = NeopixelFeature<NEOPIXEL_NAME, pixel_count>;

public:
    LedStripeDevice() :
            stripe(this),
            lightsd(this, LightsdFeature::DataCallback(&LedStripeDevice::onData, this)),
            socket(this),
            light(this),
            button(this, typename decltype(button)::Callback(&LedStripeDevice::onStateChanged, this)) {
        this->add(&(this->lightsd));
        this->add(&(this->stripe));
        this->add(&(this->socket));
        this->add(&(this->light));
        this->add(&(this->button));
    }

private:
    void onData(const LightsdFeature::pixel_t* const pixels, const size_t& count) {
        this->stripe.set((typename StripeFeature::pixel_t*) pixels, min(count, pixel_count));
    }
    
    void onStateChanged(const bool& state) {
        this->socket.set(state);
        this->light.set(state);
    }

    StripeFeature stripe;
    LightsdFeature lightsd;

    Socket<SOCKET_NAME, SOCKET_GPIO> socket;
    Light<LIGHT_NAME, LIGHT_GPIO> light;
    ToggleButton<BUTTON_NAME, BUTTON_GPIO, true> button;
};


Device* createDevice() {
    // Max LEDs in one UDP Paket = MTU - HEADER_SIZE / BYTES_PRE_LED
    static constexpr const size_t MAX_LED_COUNT = (1500 - 28) / 3;

    return new LedStripeDevice<MAX_LED_COUNT>();
}
