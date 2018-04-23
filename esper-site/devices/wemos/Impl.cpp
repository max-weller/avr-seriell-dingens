#include "../features/BmpSensor.h"
//#include "../features/Ws2812Light.h"
#include "Device.h"


constexpr const char BAROMETER_NAME[] = "barometer";
constexpr const char LEDLIGHT_NAME[] = "lights";



class WemosD1 : public Device {
public:
    WemosD1() :
            bmpSensor(this)
            //,ledlights(this) 
            {
        this->add(&(this->bmpSensor));
        //this->add(&(this->ledlights));
    }

private:
    BmpSensor<BAROMETER_NAME> bmpSensor;
    //Ws2812Light<LEDLIGHT_NAME, 2, 1337> ledlights;
};

Device *createDevice() {
    return new WemosD1();
}
