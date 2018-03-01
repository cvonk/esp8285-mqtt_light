#pragma once

#include <FS.h>           // needs to be first
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>  // https://github.com/tzapu/WiFiManager
#include <Fatal.h>        // https://github.com/cvonk/ESP8266_Fatal
#include <ArduinoOTA.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

namespace WiFiConnect
{
	extern ESP8266WebServer server;   // declared, not defined
	extern bool shouldSaveConfig;  // declared (defined in .cpp file)
	uint8_t const begin(WiFiManagerParameter *params, void(*saveParamFunc)(void) = NULL);
	uint8_t const handle(void);
};

