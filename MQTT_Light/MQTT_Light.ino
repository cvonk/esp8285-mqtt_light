/*                    ---------------------
*                    |     Sonoff S20      |
*                    |       ESP8285       |
*                    |     MQTT thing      |
*                     ---------------------
* $Id$
*
* Publishes and subscribes to MQTT broker.  
*
* Modeled after https://github.com/knolleary/pubsubclient/tree/master/examples/mqtt_esp8266
*
* GNU GENERAL PUBLIC LICENSE Version 3, check the file LICENSE.md for more information
* (c) Copyright 2018, Coert Vonk
* All text above must be included in any redistribution
*
* PARTS
* =====
*   - Sonoff S20 with ESP8285, or any compatible
*       https://www.itead.cc/wiki/S20_Smart_Socket
*
* COMPILING
* =========
* Board support:
*     ESP8285 ............ Under "File -> Preferences -> Additional Boards Manager URLs", add
*                            http://arduino.esp8266.com/stable/package_esp8266com_index.json
*                          Under "Tools -> Board -> Board Manager", add 
*                            contributed board manager "esp8266"
*                          Under "tools -> Board", select
*                            Generic ESP8285
*                          Under "Tools > Flash Size" select
*                             1M (64k SPIFFS)
*/


#include "statusled.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>        /* http://arduino.esp8266.com/stable/package_esp8266com_index.json (board manager) */
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>        /* https://github.com/tzapu/WiFiManager */
#include <PubSubClient.h>       /* https://github.com/knolleary/pubsubclient */
#include <time.h>
#include <Fatal.h>              /* https://github.com/cvonk/ESP8266_Fatal */
#include "wificonnect.h"
#include "statusled.h"

/*
 * Vars
 */

struct {
	WiFiClient client;  // WiFi client
} _wifi = {
	WiFiClient()
};

struct {
	PubSubClient client;  // MQTT publish/subscribe client
	char server[40];
	char port[6];
	char user[20];
	char passwd[20];
	char topic[40];
} _mqtt = {
	PubSubClient(_wifi.client),
	"mqtt.vonk", "1883", "", "", "room/lamp"  // default values
};

static void _showStatus(void);
static void _restart(void);
static void _reset(void);
	

/*
 * GPIO
 */

uint8_t const BUTTON_PIN = 0;
uint8_t const GREENLED_PIN = 13;  /* on when 0 */
uint8_t const REDLED_AND_RELAY_PIN = 12;

static boolean
_getRelay(void) {
	return digitalRead(REDLED_AND_RELAY_PIN);
}

static void 
_setRelay(boolean const value) {
	digitalWrite(REDLED_AND_RELAY_PIN, value);
}

static void 
_toggleRelay(void) {
	Serial.println("toggle relay");
	boolean const value = !_getRelay();
	_setRelay(value);
	_mqtt.client.publish(_mqtt.topic, value ? "on" : "off");
	_showStatus();
}

static boolean volatile _buttonChanged; 

static void _buttonChangedISR(void) {
	_buttonChanged = true;
}

static void 
_handleButtonChanged(void)
{
	static int lastButtonState = HIGH;
	int currentState = digitalRead(BUTTON_PIN);
	static long startPress;
	if (currentState != lastButtonState) {
		if (currentState == LOW) {
			startPress = millis();
		}
		else if (currentState == HIGH) {
			long const duration = millis() - startPress;
			if (duration < 1000) {
				_toggleRelay();
				_buttonChanged = false;
			} else if (duration < 5000) {
				_restart();
			} else {
				_reset();
			}
		}
		lastButtonState = currentState;
	}
	else if (currentState == HIGH) {
		long const duration = millis() - startPress;
		static int mode = 0;
		if (duration >= 1000 && mode == 0) {
			mode = 1;
			setStatusLED(STATUS_RESTART);
		}
		else if (duration >= 5000 && mode == 1) {
			mode = 2;
			setStatusLED(STATUS_RESET);
		}
	}
}


/*
* MQTT
*/

char const * const SPIFF_FNAME = "/config.json";

static int
_mqttReadCfg(void)
{
	Serial.print("loading config ..");

	//SPIFFS.format();  // for testing only
	if (SPIFFS.begin()) {  // 
		if (SPIFFS.exists(SPIFF_FNAME)) {
			File configFile = SPIFFS.open(SPIFF_FNAME, "r");
			if (configFile) {
				size_t size = configFile.size();
				std::unique_ptr<char[]> buf(new char[size]);
				configFile.readBytes(buf.get(), size);
				DynamicJsonBuffer jsonBuffer;
				JsonObject& json = jsonBuffer.parseObject(buf.get());
				//json.printTo(Serial);
				if (json.success()) {
					strcpy(_mqtt.server, json["server"]);
					strcpy(_mqtt.port, json["port"]);
					strcpy(_mqtt.user, json["user"]);
					strcpy(_mqtt.passwd, json["passwd"]);
					strcpy(_mqtt.topic, json["topic"]);
				} else {
					Serial.println("JSON invalid");
					return 1;
				}
			}
		} else {
			Serial.println("JSON missing");
			return 0;
		}
	} else {
		Serial.println("no SPIFFS");  // requires "Tools > Flash Size > 1M (64k SPIFFS)" in the IDE
		return 2;
	}
	Serial.println();
	return 0;
}

static WiFiManagerParameter * _params;

static void
_mqttWriteCfg(void)
{
	Serial.print("writing config .. ");
	DynamicJsonBuffer jsonBuffer;
	JsonObject& json = jsonBuffer.createObject();

	for (WiFiManagerParameter * param = _params; *(int *)param; param++) {
		Serial.printf("%s: %s\n", param->getID(), param->getValue());
		yield(); delay(500);
		json[param->getID()] = param->getValue();
	}

	strcpy(_mqtt.server, json["server"]);
	strcpy(_mqtt.port, json["port"]);
	strcpy(_mqtt.user, json["user"]);
	strcpy(_mqtt.passwd, json["passwd"]);
	strcpy(_mqtt.topic, json["topic"]);

	File configFile = SPIFFS.open(SPIFF_FNAME, "w");
	if (!configFile) {
		Serial.println("can't write JSON");
	}	
	json.printTo(configFile);  //json.printTo(Serial);
	configFile.close();
	Serial.println();
}

static void
_mqtt_reconnect(void)  // an async alternative is mqtt_reconnect_nonblocking
{
	setStatusLED(STATUS_NO_MQTT);

	while (!_mqtt.client.connected()) {
		_showStatus();
		String clientid = "ESP" + String(ESP.getChipId());
		if (_mqtt.client.connect(clientid.c_str())) {
			_mqtt.client.subscribe(_mqtt.topic);
			setStatusLED(STATUS_OK);
		} else {
			delay(2000);
		}
	}
	_showStatus();
}

static void
_mqtt_callback(char * topic, byte * payload, unsigned int length)
{
	_setRelay(strncasecmp((char *)payload, "on", length) == 0);
	_showStatus();
}


/*
 * Misc
 */

static void
_showStatus(void)
{
	boolean const wifiConnected = WiFi.status() == WL_CONNECTED;
	boolean const mqttConnected = _mqtt.client.connected();
	char const * const connecting = " (connecting)";
	time_t now = time(nullptr);
	Serial.printf("%.19s, ", ctime(&now));
	Serial.printf("%s%s, ", WiFi.SSID().c_str(), wifiConnected ? "" : connecting);
	Serial.printf("%s:%s%s, ", _mqtt.server, _mqtt.port, mqttConnected ?  "" : connecting);
	Serial.printf("%s: %i\n", _mqtt.topic, _getRelay());
}

static void
_restart(void)
{
	Serial.println("RESTART");
	_setRelay(false);
	ESP.reset();
	delay(1000);
}

static void
_reset(void)
{
	Serial.println("RESET");
	_setRelay(false);
	//SPIFFS.format();    // wipe MQTT settings
	system_restore();
	WiFi.disconnect(true);
	yield(); delay(1000);
	ESP.reset();
	delay(1000);
}

static void
_initTime(void)
{
	int const TZ = -8;  // US Pacific (non-DST)
	configTime(TZ * 3600, 0, "pool.ntp.org", "time.nist.gov");
	while (!time(nullptr)) {
		Serial.print(".");
		delay(1000);
	}
}

/*
 * Public entry points
 */

MDNSResponder g_mdns;

void
setup(void)
{
	pinMode(GREENLED_PIN, OUTPUT);
	pinMode(REDLED_AND_RELAY_PIN, OUTPUT);
	pinMode(BUTTON_PIN, INPUT);
	setLEDpin(GREENLED_PIN, true);

	Serial.begin(115200); //74880);
	delay(100);
	{
		_mqttReadCfg();

		WiFiManagerParameter params[] =
		{
			WiFiManagerParameter("server", "MQTT server", _mqtt.server, sizeof(_mqtt.server)),
			WiFiManagerParameter("port", "MQTT port", _mqtt.port, sizeof(_mqtt.port)),
			WiFiManagerParameter("user", "MQTT user", _mqtt.user, sizeof(_mqtt.user)),
			WiFiManagerParameter("passwd", "MQTT passwd", _mqtt.passwd, sizeof(_mqtt.passwd)),
			WiFiManagerParameter("topic", "MQTT topic", _mqtt.topic, sizeof(_mqtt.topic)),
			NULL
		};
		//uint8_t const PARAMS_COUNT = sizeof(params) / sizeof(params[0]);

		Serial.println("Connecting to wifi and config ...");

		_params = params;  // can't pass a parameter for callback function
		WiFiConnect::begin(params, &_mqttWriteCfg);
		setStatusLED(STATUS_NO_MQTT);
		_params = NULL;
	}

	Serial.print("Start Time ...");
	{
		_initTime();
	}

	Serial.print("\nStart server for http://"); Serial.print(WiFi.localIP()); Serial.println("/fatal");
	{
		WiFiConnect::server.begin();
	}

	// setup MQTT
	Serial.printf("Connect to mqtt://%s:%s ...\n", _mqtt.server, _mqtt.port);
	{
		_mqtt.client.setServer(_mqtt.server, atoi(_mqtt.port));
		_mqtt.client.setCallback(_mqtt_callback);
	}

	attachInterrupt(BUTTON_PIN, _buttonChangedISR, CHANGE);
	_showStatus();
}

void
loop(void) 
{
	WiFiConnect::handle();

	if (!_mqtt.client.connected()) {
		_mqtt_reconnect();
	}
	_mqtt.client.loop();

	if (_buttonChanged) {
		_handleButtonChanged();
	}
}
