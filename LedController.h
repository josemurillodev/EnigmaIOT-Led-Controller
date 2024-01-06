// BasicController.h

#ifndef _BASICCONTROLLER_h
#define _BASICCONTROLLER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//#define DEBUG_SERIAL

#ifdef ESP32
#include <SPIFFS.h>
#endif

#include <EnigmaIOTjsonController.h>
#define LedController LedController
static const char* CONTROLLER_NAME = "LEDWS2811";

#if SUPPORT_HA_DISCOVERY    
#include <haSwitch.h>
#endif

// --------------------------------------------------
// You may define data structures and constants here
// --------------------------------------------------
#define LED_PIN 2
#define _LED_ON LOW
#define _LED_OFF !_LED_ON

enum LED_CONTROLLER_EVENT {
  SE_TYPE_DATA = 10,
  SE_TYPE_STATUS = 0,
  SE_TYPE_COLOR = 1,
  SE_TYPE_BPM = 2,
  SE_TYPE_PALETTE = 3,
  SE_TYPE_CONNECTION = 4,
  SE_TYPE_TOGGLE = 5,
  SE_TYPE_INFO = 6,
  SE_TYPE_CONFIG = 7,
  SE_TYPE_RESET = 8,
  SE_TYPE_REVERSE = 9
};

class LedController : EnigmaIOTjsonController {
protected:
	// --------------------------------------------------
	// add all parameters that your project needs here
	// --------------------------------------------------
	int led = _LED_OFF;

public:
	void setup (EnigmaIOTNodeClass* node, void* data = NULL);

	bool processRxCommand (const uint8_t* address, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding);

	void loop ();

	~LedController ();

	/**
	 * @brief Called when wifi manager starts config portal
	 * @param node Pointer to EnigmaIOT gateway instance
	 */
	void configManagerStart ();

	/**
	 * @brief Called when wifi manager exits config portal
	 * @param status `true` if configuration was successful
	 */
	void configManagerExit (bool status);

	/**
	 * @brief Loads output module configuration
	 * @return Returns `true` if load was successful. `false` otherwise
	 */
	bool loadConfig ();

    void connectInform ();

protected:
	/**
	  * @brief Saves output module configuration
	  * @return Returns `true` if save was successful. `false` otherwise
	  */
	bool saveConfig ();

	bool sendCommandResp (const char* command, bool result);

    bool sendStartAnouncement () {
        // You can send a 'hello' message when your node starts. Useful to detect unexpected reboot
        const size_t capacity = JSON_OBJECT_SIZE (10);
        DynamicJsonDocument json (capacity);
        json["status"] = "start";
        json["device"] = CONTROLLER_NAME;
        char version_buf[10];
        snprintf (version_buf, 10, "%d.%d.%d",
                  ENIGMAIOT_PROT_VERS[0], ENIGMAIOT_PROT_VERS[1], ENIGMAIOT_PROT_VERS[2]);
        json["version"] = String (version_buf);

        return sendJson (json);
    }

    void buildHADiscovery ();
    
    // ------------------------------------------------------------
	// You may add additional method definitions that you need here
	// ------------------------------------------------------------

	bool sendLedStatus ();
};

#endif

