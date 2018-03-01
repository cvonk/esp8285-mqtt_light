/*
* ESP8266 framework that provides
*   - Online WiFi SSID/passwd configuration
*   - Saves fatal exception details to non-volatile memory
*   - Over-the-air software updates
*
* Platform: ESP8266/8285 using Arduino IDE, min 1 MByte of flash for OTA updates
* Documentation: https://github.com/cvonk/ESP8266_Framework
* Tested with: Arduino IDE 1.8.5, board package esp8266 2.4.0, Adafruit huzzah (feather) esp8266 / general esp8285
*
* GNU GENERAL PUBLIC LICENSE Version 3, check the file LICENSE.md for more information
* (c) Copyright 2018, Coert Vonk
* All text above must be included in any redistribution
*/

#include "wificonnect.h"
#include "statusled.h"

using namespace WiFiConnect;
ESP8266WebServer WiFiConnect::server(80);  // declared
bool WiFiConnect::shouldSaveConfig = false;  // declared

// called when WiFiManager enters configuration mode
void
configModeCallback(WiFiManager *myWiFiManager)
{
	Serial.printf("Config AP SSID: %s\n", myWiFiManager->getConfigPortalSSID().c_str());
	setStatusLED(STATUS_WIFI_CFG);
}

// called when WiFiManager exists configuration mode
void
configSaveCallback(void)
{
	Serial.println("Save config");
	WiFiConnect::shouldSaveConfig = true;
}

static void
handleFatal(void)
{
	WiFiClient client = server.client();
	client.print(F("HTTP/1.1 200 OK\r\n"
				   "Content-Type: text/plain\r\n"
				   "Connection: close\r\n"
				   "\r\n"));
	Fatal::print(client);  // send crash information to the web browser
	Fatal::clear();
}

static void
handleNotFound(void)
{
	Serial.println("handleNotFound");
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for (uint8_t i = 0; i < server.args(); i++) {
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}
	server.send(404, "text/plain", message);
}

uint8_t const
WiFiConnect::begin(WiFiManagerParameter *params, void(*saveParamFunc)(void))
{
	int const startTime = millis();

	// WiFiManager
	Serial.println("\nWiFi connect ...");
	setStatusLED(STATUS_NO_WIFI);

	{
		WiFiManager wifiManager;

		//wifiManager.resetSettings();  // for testing
		wifiManager.setAPCallback(configModeCallback);  // called when previous WiFi fails, when entering AP mode
														// fetches ssid and pass and tries to connect
														// on failure, starts an access point and wait for further instructions (wifi SSID/passwd)
		for (WiFiManagerParameter * param = params; *(int *)param; param++) {
			wifiManager.addParameter(param);
		}
		wifiManager.setSaveConfigCallback(saveParamFunc);

		if (!wifiManager.autoConnect()) {
			Serial.println("timeout, no connection");
			ESP.reset();  //reset and try again, or maybe put it to deep sleep
			delay(1000);
		}
		Serial.printf("Connect time: %lu ms\n", millis() - startTime);
		Serial.print("IP address: "); Serial.print(WiFi.localIP());  // we're connected
		struct station_config conf;
		wifi_station_get_config(&conf);
		Serial.printf(", MAC: %02X:%02X:%02X:%02X:%02X:%02X\n", conf.bssid[0], conf.bssid[1], conf.bssid[2], conf.bssid[3], conf.bssid[4], conf.bssid[5]);
	}

	// Be receptive to over-the-air (OTA) updates
	Serial.println("Start OTA ... ");
	yield(); delay(500);
	{
		ArduinoOTA.onStart([]() {
			//Serial.printf("\nOTA start %s", (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem");
		});
		ArduinoOTA.onEnd([]() {
			Serial.printf("\nOTA end\n");
		});
		ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
			Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
		});
		ArduinoOTA.onError([](ota_error_t error) {
			Serial.printf("OTA error %u\n", error);
		});
		ArduinoOTA.begin();
	}

	// Fatal
	Serial.println("Start Fatal ... ");
	yield(); delay(500);
	{
		Fatal::begin(0x0010, 0x0200);
		server.on("/fatal", handleFatal);
		server.onNotFound(handleNotFound);
	}

	WiFi.persistent(true);
	setStatusLED(STATUS_OK);
	return 0;
}


uint8_t const
WiFiConnect::handle(void) 
{
	ArduinoOTA.handle();
	server.handleClient();
	return 0;
}
