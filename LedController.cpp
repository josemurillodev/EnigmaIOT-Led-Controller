// 
// 
// 

#include <functional>
#include "LedController.h"
#include "LedStripConfig.h"

using namespace std;
using namespace placeholders;

constexpr auto CONFIG_FILE = "/customconf.json"; ///< @brief Custom configuration file name

// -----------------------------------------
// You may add some global variables you need here,
// like serial port instances, I2C, etc
// -----------------------------------------

// const char* ledKey = "led";
const char* commandKey = "status";

LedStripConfig ledstrip;

bool LedController::processRxCommand (const uint8_t* address, const uint8_t* buffer, uint8_t length, nodeMessageType_t command, nodePayloadEncoding_t payloadEncoding) {
	// Process incoming messages here
	// They are normally encoded as MsgPack so you can confert them to JSON very easily
	uint8_t _command = command;

	_command = (_command & 0x7F);

	if (_command != nodeMessageType_t::DOWNSTREAM_DATA_GET && _command != nodeMessageType_t::DOWNSTREAM_DATA_SET) {
		DEBUG_WARN ("Wrong message type");
		return false;
	}
	// Check payload encoding
	if (payloadEncoding != MSG_PACK) {
		DEBUG_WARN ("Wrong payload encoding");
		return false;
	}

	// Decode payload
	DynamicJsonDocument doc (256);
	uint8_t tempBuffer[MAX_MESSAGE_LENGTH];

	memcpy (tempBuffer, buffer, length);
	DeserializationError error = deserializeMsgPack (doc, tempBuffer, length);
	// Check decoding
	if (error != DeserializationError::Ok) {
		DEBUG_WARN ("Error decoding command: %s", error.c_str ());
		return false;
	}

	DEBUG_WARN ("Command: %d = %s", _command, _command == nodeMessageType_t::DOWNSTREAM_DATA_GET ? "GET" : "SET");

	// Dump debug data
	size_t strLen = measureJson (doc) + 1;
	char* strBuffer = (char*)malloc (strLen);
	serializeJson (doc, strBuffer, strLen);
	DEBUG_WARN ("Data: %s", strBuffer);
	free (strBuffer);

	// // Check cmd field on JSON data
	// if (!doc.containsKey (commandKey)) {
	// 	DEBUG_WARN ("Wrong format");
	// 	return false;
	// }

	// Get state
	if (_command == nodeMessageType_t::DOWNSTREAM_DATA_GET) {
			if (!sendLedStatus ()) {
				DEBUG_WARN ("Error sending LED status");
				return false;
			}
	}

  LED_CONTROLLER_EVENT _type = (LED_CONTROLLER_EVENT)doc[commandKey].as<int>();

	// Set state
	if (_command == nodeMessageType_t::DOWNSTREAM_DATA_SET) {
    switch (_type) {
      case SE_TYPE_STATUS: {
        DEBUG_WARN("SE_TYPE_STATUS");
        ls_Modes _status = (ls_Modes)doc["status"].as<int>();
				static time_t clock;
				clock = EnigmaIOTNode.clock ();
        if (ledstrip.ledstatus != _status) {
          ledstrip.setStatus(_status, clock);
        }
        break;
      }
      case SE_TYPE_COLOR: {
        DEBUG_WARN("SE_TYPE_COLOR");
        JsonObject _hsv = doc["hsv"];
        double hue = _hsv["h"];
        double saturation = _hsv["s"];
        double value = _hsv["v"];
        ledstrip.updateHsv(hue, saturation, value);
        break;
      }
      case SE_TYPE_PALETTE: {
        DEBUG_WARN("SE_TYPE_PALETTE");
        ls_Palette _palette = (ls_Palette)doc["palette"].as<int>();
        if (ledstrip.ledpalette != _palette) {
          ledstrip.ledpalette = _palette;
        }
        break;
      }
      case SE_TYPE_BPM: {
        DEBUG_WARN("SE_TYPE_BPM");
        uint8_t _bpm = doc["bpm"];
        if (ledstrip.bpm != _bpm) {
          ledstrip.bpm = _bpm > 0 ? _bpm : ledstrip.bpm;
        }
        break;
      }
      // case SE_TYPE_TOGGLE: {
      //   DEBUG_WARN("SE_TYPE_TOGGLE");
      //   if (isSelected) {
      //     ledstrip.isOn = !ledstrip.isOn;
      //   }
      //   break;
      // }
      case SE_TYPE_CONFIG: {
        DEBUG_WARN("SE_TYPE_CONFIG");
        uint16_t _numLeds = doc["ledcount"];
        // const char * _deviceName = doc["deviceName"];
        // Serial.println(_deviceName);
        ledstrip.setLeds(_numLeds);
        // nodeConfig.setNodeName(_deviceName);
        // nodeConfig.updateNodeConfig();
        break;
      }
      // case SE_TYPE_RESET: {
      //   DEBUG_WARN("SE_TYPE_RESET");
      //   if (isSelected) {
      //     handleSettingsDelete();
      //     return;
      //   }
      //   break;
      // }
      default:
				DEBUG_WARN("Wrong command type");
				return false;
        break;
    }
		// if (!strcmp (doc[commandKey], ledKey)) {
		// 	if (doc.containsKey (ledKey)) {
		// 		if (doc[ledKey].as<int> () == 1) {
		// 			led = _LED_ON;
		// 		} else if (doc[ledKey].as<int> () == 0) {
		// 			led = _LED_OFF;
		// 		} else {
		// 			DEBUG_WARN ("Wrong LED value: %d", doc[ledKey].as<int> ());
		// 			return false;
		// 		}
		// 		DEBUG_WARN ("Set LED status to %s", led == _LED_ON ? "ON" : "OFF");
		// 	} else {
		// 		DEBUG_WARN ("Wrong format");
		// 		return false;
		// 	}

		// 	// Confirm command execution with send state
		// 	if (!sendLedStatus ()) {
		// 		DEBUG_WARN ("Error sending LED status");
		// 		return false;
		// 	}
		// }
    if (!sendLedStatus ()) {
      DEBUG_WARN ("Error sending LED status 2");
      return false;
    }
	}


	return true;
}

bool LedController::sendLedStatus () {
	const size_t capacity = JSON_OBJECT_SIZE (11);
	DynamicJsonDocument json (capacity);
  json["ledcount"] = ledstrip.getLeds();
  json["status"] = ledstrip.ledstatus;
  json["palette"] = (uint8_t)ledstrip.ledpalette;
  // json["bpm"] = ledstrip.bpm;
  JsonObject hsv_r = json.createNestedObject("hsv");
  hsv_r["h"] = ledstrip.hue;
  hsv_r["s"] = ledstrip.saturation;
  hsv_r["v"] = ledstrip.value;

	char gwAddress[ENIGMAIOT_ADDR_LEN * 3];
  json["nodeName"] = enigmaIotNode->getNode()->getNodeName();
  json["nodeAddress"] = mac2str (enigmaIotNode->getNode()->getMacAddress(), gwAddress);

	return sendJson (json);
}


bool LedController::sendCommandResp (const char* command, bool result) {
	// Respond to command with a result: true if successful, false if failed 
	return true;
}

void LedController::connectInform () {

#if SUPPORT_HA_DISCOVERY    
    // Register every HAEntity discovery function here. As many as you need
    addHACall (std::bind (&LedController::buildHADiscovery, this));
    addHACall (std::bind (&LedController::sendLedStatus, this));
#endif

    EnigmaIOTjsonController::connectInform ();
}

void LedController::setup (EnigmaIOTNodeClass* node, void* data) {
	enigmaIotNode = node;

	// You do node setup here. Use it as it was the normal setup() Arduino function

  ledstrip.initLedStrip();

	// Send a 'hello' message when initalizing is finished
    sendStartAnouncement ();
    
    sendLedStatus ();
    
	DEBUG_DBG ("Finish begin");

	// If your node should sleep after sending data do all remaining tasks here
}

void LedController::loop () {
	static time_t clock;
	clock = EnigmaIOTNode.clock ();

	if (EnigmaIOTNode.hasClockSync () && EnigmaIOTNode.isRegistered ()) {
		ledstrip.update(clock);
	} else {
		ledstrip.update(millis());
	}
	// If your node stays allways awake do your periodic task here
	// digitalWrite (LED_PIN, led);
}

LedController::~LedController () {
	// It your class uses dynamic data free it up here
	// This is normally not needed but it is a good practice
}

void LedController::configManagerStart () {
	DEBUG_INFO ("==== CCost Controller Configuration start ====");
	// If you need to add custom configuration parameters do it here
}

void LedController::configManagerExit (bool status) {
	DEBUG_INFO ("==== CCost Controller Configuration result ====");
	// You can read configuration paramenter values here
}

bool LedController::loadConfig () {
	// If you need to read custom configuration data do it here
	return true;
}

bool LedController::saveConfig () {
	// If you need to save custom configuration data do it here
	return true;
}

#if SUPPORT_HA_DISCOVERY   
// Repeat this method for every entity
void LedController::buildHADiscovery () {
    // Select corresponding HAEntiny type
    HASwitch* haEntity = new HASwitch ();

    uint8_t* msgPackBuffer;

    if (!haEntity) {
        DEBUG_WARN ("JSON object instance does not exist");
        return;
    }

    // *******************************
    // Add your characteristics here
    // There is no need to futher modify this function

    //haEntity->setNameSufix ("switch");
    haEntity->setPayloadOff ("{\"cmd\":\"led\",\"led\":0}");
    haEntity->setPayloadOn ("{\"cmd\":\"led\",\"led\":1}");
    haEntity->setValueField ("led");
    haEntity->setStateOff (0);
    haEntity->setStateOn (1);

    // *******************************

    size_t bufferLen = haEntity->measureMessage ();

    msgPackBuffer = (uint8_t*)malloc (bufferLen);

    size_t len = haEntity->getAnounceMessage (bufferLen, msgPackBuffer);

    DEBUG_INFO ("Resulting MSG pack length: %d", len);

    if (!sendHADiscovery (msgPackBuffer, len)) {
        DEBUG_WARN ("Error sending HA discovery message");
    }

    if (haEntity) {
        delete (haEntity);
    }

    if (msgPackBuffer) {
        free (msgPackBuffer);
    }
}
#endif // SUPPORT_HA_DISCOVERY
