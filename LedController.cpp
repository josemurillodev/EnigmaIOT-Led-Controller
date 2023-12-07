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

// const char* nodeType = "ledstrip";
const char* commandKey = "event";

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
	DynamicJsonDocument doc (2048);
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

	// Set state
	if (_command == nodeMessageType_t::DOWNSTREAM_DATA_SET) {
    uint16_t nodeId = enigmaIotNode->getNode()->getNodeId();
    DEBUG_WARN("nodeId, %d", nodeId);
    bool hasIds = doc.containsKey ("ids");
    std::string nodeIdKey = to_string(nodeId);
    bool isSelected = doc.containsKey (nodeIdKey);
    if (hasIds && !isSelected) {
      ledstrip.isOn = false;
      return false;
    }
    JsonObject data = isSelected ? doc[nodeIdKey] : doc.as<JsonObject>();
    DEBUG_WARN("NODE_SELECTED");
    
    LED_CONTROLLER_EVENT _type = (LED_CONTROLLER_EVENT)doc[commandKey].as<int>();
    switch (_type) {
      case SE_TYPE_DATA: {
        DEBUG_WARN("SE_TYPE_DATA");
        if (data.containsKey ("col")) {
          JsonArray _rgb = data["col"].as<JsonArray>();
          uint8_t r = _rgb[0].as<int>();
          uint8_t g = _rgb[1].as<int>();
          uint8_t b = _rgb[2].as<int>();
          DEBUG_WARN("color, [%d, %d, %d]", r, g, b);
          ledstrip.setRgb(r, g, b);
        }
        if (data.containsKey ("rgb")) {
          JsonObject _rgb = data["rgb"];
          uint8_t r = _rgb["r"];
          uint8_t g = _rgb["g"];
          uint8_t b = _rgb["b"];
          ledstrip.setRgb(r, g, b);
        }
        if (data.containsKey("intensity")) {
          double _value = data["intensity"];
          ledstrip.value = _value;
        }
        if (data.containsKey ("ledMode")) {
          ls_Modes _ledMode = (ls_Modes)data["ledMode"].as<int>();
          if (ledstrip.ledMode != _ledMode) {
            static time_t clock;
            clock = EnigmaIOTNode.clock ();
            ledstrip.setStatus(_ledMode, clock);
          }
        }
        if (data.containsKey ("palette")) {
          uint8_t _palette = (uint8_t)data["palette"].as<int>();
          if (ledstrip.ledpalette != _palette) {
            ledstrip.ledpalette = _palette;
          }
        }
        if (data.containsKey ("bpm")) {
          float _bpm = data["bpm"];
          if (ledstrip.bpm != _bpm) {
            ledstrip.bpm = _bpm > 1.0f ? _bpm : ledstrip.bpm;
          }
        }
        if (data.containsKey ("isOn")) {
          bool _isOn = data["isOn"];
          ledstrip.isOn = _isOn;
        }
        bool _reverse = data["reverse"];
        bool _mirror = data["mirror"];
        DEBUG_WARN("mirror, %s", _mirror ? "true" : "false");
        ledstrip.reverse = _reverse;
        ledstrip.mirror = _mirror;
        if (data.containsKey ("ledCount")) {
          uint16_t _ledCount = data["ledCount"];
          ledstrip.setLeds(_ledCount);
          if (saveConfig ()) {
            DEBUG_WARN ("Config updated.");
          } else {
            DEBUG_ERROR ("Error saving config");
          }
        }
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

		// 	// Confirm command execution with send state
		// 	if (!sendLedStatus ()) {
		// 		DEBUG_WARN ("Error sending LED status");
		// 		return false;
		// 	}
		// }
	}

	return true;
}

// TODO: send state array

bool LedController::sendLedStatus () {
	const size_t capacity = JSON_OBJECT_SIZE (16);
	DynamicJsonDocument json (capacity);
  json["name"] = enigmaIotNode->getNode()->getNodeName();
  json["nodeId"] = enigmaIotNode->getNode()->getNodeId();

	char gwAddress[ENIGMAIOT_ADDR_LEN * 3];
  json["address"] = mac2str (enigmaIotNode->getNode()->getMacAddress(), gwAddress);

  json["ledCount"] = ledstrip.getLeds();
  JsonObject rgb = json.createNestedObject("rgb");
  rgb["r"] = ledstrip.rgb_r;
  rgb["g"] = ledstrip.rgb_g;
  rgb["b"] = ledstrip.rgb_b;

  json["device"] = CONTROLLER_NAME;
  json["isOn"] = ledstrip.isOn;
  json["reverse"] = ledstrip.reverse;
  json["mirror"] = ledstrip.mirror;

  json["ledMode"] = ledstrip.ledMode;
  json["palette"] = (uint8_t)ledstrip.ledpalette;
  json["bpm"] = ledstrip.bpm;

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
	// static time_t clock;
	// clock = EnigmaIOTNode.clock ();

  if (EnigmaIOTNode.getOTArunning()) {
    ledstrip.writeHsv(0, 0, 0);
    return;
  }

  ledstrip.update(get_millisecond_timer());

	// if (EnigmaIOTNode.hasClockSync () && EnigmaIOTNode.isRegistered ()) {
	// 	ledstrip.update(clock);
	// } else {
	// 	ledstrip.update(millis());
	// }
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
  // // If you need to read custom configuration data do it here
  bool json_correct = false;

  if (!FILESYSTEM.begin()) {
    DEBUG_WARN("Error starting filesystem. Formatting");
    FILESYSTEM.format();
  }

  // // FILESYSTEM.remove (CONFIG_FILE); // Only for testing

  if (FILESYSTEM.exists(CONFIG_FILE)) {
    DEBUG_WARN("Opening %s file", CONFIG_FILE);
    File configFile = FILESYSTEM.open(CONFIG_FILE, "r");
    if (configFile) {
        size_t size = configFile.size();
        DEBUG_WARN("%s opened. %u bytes", CONFIG_FILE, size);
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, configFile);
        if (error) {
          DEBUG_WARN("Failed to parse file");
        } else {
          DEBUG_WARN("JSON file parsed");
        }
        if (doc.containsKey("ledCount")) {
          uint16_t _ledCount = doc["ledCount"];
          ledstrip.setLeds(_ledCount);
        }
        if (doc.containsKey("ledMode")) {
          ls_Modes _ledMode = (ls_Modes)doc["ledMode"].as<int>();
          ledstrip.setStatus(_ledMode);
        }
        if (doc.containsKey("palette")) {
          uint8_t _palette = (uint8_t)doc["palette"].as<int>();
          ledstrip.ledpalette = _palette;
        }
        if (doc.containsKey("bpm")) {
          float _bpm = doc["bpm"].as<float>();
          ledstrip.bpm = _bpm;
        }
        if (doc.containsKey("rgb")) {
          JsonObject _rgb = doc["rgb"];
          double r = _rgb["r"];
          double g = _rgb["g"];
          double b = _rgb["b"];
          ledstrip.setHsv(r, g, b);
        }
        if (doc.containsKey("intensity")) {
          uint8_t _value = (uint8_t)doc["intensity"].as<int>();
          ledstrip.value = _value;
        }

        json_correct = true;

        configFile.close();

        if (json_correct) {
          DEBUG_WARN("Smart switch controller configuration successfuly read");
        } else {
          DEBUG_WARN("Smart switch controller configuration error");
        }
        // DEBUG_WARN("==== Smart switch Controller Configuration ====");
        // DEBUG_WARN("Button pin: %d", config.buttonPin);
        // DEBUG_WARN("Relay pin: %d", config.relayPin);
        // DEBUG_WARN("Linked: %s", config.linked ? "true" : "false");
        // DEBUG_WARN("ON level: %s ", config.ON_STATE ? "HIGH" : "LOW");
        // DEBUG_WARN("Boot relay status: %d ", config.bootStatus);

        size_t jsonLen = measureJsonPretty(doc) + 1;
        char *output = (char *)malloc(jsonLen);
        serializeJsonPretty(doc, output, jsonLen);

        DEBUG_WARN("File content:\n%s", output);

        free(output);
    } else {
        DEBUG_WARN("Error opening %s", CONFIG_FILE);
        // defaultConfig();
    }
  } else {
    DEBUG_WARN("%s do not exist", CONFIG_FILE);
    // defaultConfig();
  }

  return json_correct;
}

bool LedController::saveConfig () {
  // If you need to save custom configuration data do it here
  if (!FILESYSTEM.begin()) {
    DEBUG_WARN("Error opening filesystem");
    return false;
  }
  DEBUG_INFO("Filesystem opened");

  File configFile = FILESYSTEM.open(CONFIG_FILE, "w");
  if (!configFile) {
    DEBUG_WARN("Failed to open config file %s for writing", CONFIG_FILE);
    return false;
  } else {
    DEBUG_INFO("%s opened for writting", CONFIG_FILE);
  }

  DynamicJsonDocument doc(512);

  doc["ledCount"] = ledstrip.getLeds();
  doc["ledMode"] = ledstrip.ledMode;
  doc["palette"] = (uint8_t)ledstrip.ledpalette;
  JsonObject rgb = doc.createNestedObject("rgb");
  rgb["r"] = ledstrip.rgb_r;
  rgb["g"] = ledstrip.rgb_g;
  rgb["b"] = ledstrip.rgb_b;
  doc["bpm"] = ledstrip.bpm;

	// char gwAddress[ENIGMAIOT_ADDR_LEN * 3];
  // doc["address"] = mac2str (enigmaIotNode->getNode()->getMacAddress(), gwAddress);
  // doc["name"] = enigmaIotNode->getNode()->getNodeName();
  // doc["isOn"] = ledstrip.isOn;
  // doc["device"] = CONTROLLER_NAME;

  if (serializeJson(doc, configFile) == 0)
  {
    DEBUG_ERROR("Failed to write to file");
    configFile.close();
    // FILESYSTEM.remove (CONFIG_FILE);
    return false;
  }

  size_t jsonLen = measureJsonPretty(doc) + 1;
  char *output = (char *)malloc(jsonLen);
  serializeJsonPretty(doc, output, jsonLen);

  DEBUG_ERROR("File content:\n%s", output);

  free(output);

  configFile.flush();
  // size_t size = configFile.size();

  // configFile.write ((uint8_t*)(&mqttgw_config), sizeof (mqttgw_config));
  configFile.close();
  // DEBUG_DBG("Smart Switch controller configuration saved to flash. %u bytes", size);

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
