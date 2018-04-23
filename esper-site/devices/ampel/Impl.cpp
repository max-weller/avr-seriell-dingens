#include "../features/OnOffFeature.h"
#include "Device.h"


constexpr const char RED_NAME[] = "red";
constexpr const char YELLOW_NAME[] = "yellow";
constexpr const char GREEN_NAME[] = "green";

constexpr const uint16_t RED_GPIO = 12; //D6
constexpr const uint16_t YELLOW_GPIO = 14; //D5
constexpr const uint16_t GREEN_GPIO = 13; //D7


class Ampel : public Device {
public:

    Ampel() :
            lightRed(this),
            lightYellow(this),
            lightGreen(this) {
        this->add(&(this->lightRed));
        this->add(&(this->lightYellow));
        this->add(&(this->lightGreen));

        // startup animation
        startupAnimTimer.initializeMs(3000, TimerDelegate(&Ampel::startupAnim, this)).start();
    }

    void start() {
        Device::start();
	    http.addPath("/ampel", HttpPathDelegate(&Ampel::onHttp_Ampel, this));
    }

    void onHttp_Ampel(HttpRequest &request, HttpResponse &response)
    {
        response.setContentType(MIME_HTML);
        response.setHeader("Server", "ham");
        if (request.method == HTTP_POST) {
            if (request.getPostParameter("speed") != "") {
                int spd=atoi(request.getPostParameter("speed").c_str());
                if (spd<1000) spd=1000;
                startupAnimTimer.setIntervalMs(spd);
                startupAnimTimer.start();
                startupAnimStep = -1;
                startupAnim();
                explicitAnim = true;
            }
            if (request.getPostParameter("on") != "") {
                startupAnimTimer.stop();
                lightRed.set(true);
                lightYellow.set(true);
                lightGreen.set(true);
                explicitAnim = true;
            }
            if (request.getPostParameter("off") != "") {
                startupAnimTimer.stop();
                lightRed.set(false);
                lightYellow.set(false);
                lightGreen.set(false);
                explicitAnim = true;
            }
        }
        response.sendString("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
        "<form action=/ampel method=post>AMPEL!!!<hr>"
        "<input type=range name=speed min=1000 max=60000 step=1000 value=20000><input type=submit name=go value=Start><br><br>"
        //"<input type=submit name=stop value=Stop>"
        "<input type=submit name=on value='Alle an'>"
        "<input type=submit name=off value='Alle aus'>"
        "</form>");
    }
private:
    Timer startupAnimTimer;
    int startupAnimStep = 0;
    bool explicitAnim = false;

    void startupAnim() {
        switch(startupAnimStep) {
        case 0:
            lightGreen.set(false);
            lightRed.set(true);
            break;
        case 1:
            lightRed.set(false);
            lightYellow.set(true);
            break;
        case 2:
            lightYellow.set(false);
            lightGreen.set(true);
            break;
        case 3:
            lightGreen.set(false);
            if (!explicitAnim){
                if (this->wifiConnectionManager.getState() == WifiConnectionManager::State::CONNECTED) {
                        startupAnimTimer.stop();
                } else {
                        startupAnimTimer.setIntervalMs(20000);
                        lightRed.set(true);
                }
            } else {
                        lightRed.set(true);

            }
            startupAnimStep = 0;
            break;
        }
        startupAnimStep++;

    }

    OnOffFeature<RED_NAME, RED_GPIO, false, 0> lightRed;
    OnOffFeature<YELLOW_NAME, YELLOW_GPIO, true, 0> lightYellow;
    OnOffFeature<GREEN_NAME, GREEN_GPIO, true, 0> lightGreen;
};

Device *createDevice() {
    return new Ampel();
}
