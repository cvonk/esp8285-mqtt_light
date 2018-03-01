/*                    ---------------------
*                    |     Sonoff S20      |
*                    |       ESP8285       |
*                    |     MQTT thing      |
*                     ---------------------
* $Id$
*
* Publishes and subscribes to and for MQTT broker.  The value from analog input
* A0 is published as "sensor" with its values mapped to 0..100.  It subscribes to
* "led" to drive the on-board LED output.
*
* Modeled after https://github.com/knolleary/pubsubclient/tree/master/examples/mqtt_esp8266
*
* (c) Copyright 2018 by Coert Vonk
* MIT license, all text above must be included in any redistribution
*
* PARTS
* =====
*
*   - Sonoff S20 with ESP8285, or any compatible
*       https://www.itead.cc/wiki/S20_Smart_Socket
*
* COMPILING
* =========
*
* Board support:
*     ESP8285 ............ Under "File -> Preferences -> Additional Boards Manager URLs", add
*                            http://arduino.esp8266.com/stable/package_esp8266com_index.json
*                          Under "Tools -> Board -> Board Manager", add 
*                            contributed board manager "esp8266"
*                          Under "tools -> Board", select
*                            Generic ESP8285
* Libraries sources:
*     pubsubclient ........ https://github.com/knolleary/pubsubclient,
*                           Nick O'Leary, January 2018, MIT license
*/

#include <ESP8266WiFi.h>        // http://arduino.esp8266.com/stable/package_esp8266com_index.json (board manager)
#include <PubSubClient.h>       // https://github.com/knolleary/pubsubclient 

// WiFi credentials

struct {
	char const * const ssid;
	char const * const passwd;
	WiFiClient client;
} _wifi = {
	"Farm Field",
	"Bergen ^ Z00m, @lameda & Pdx"
};

// MQTT broker and topics

struct {
	char const * const broker;     // MQTT broker address, such as "test.mosquitto.org"
	char const * const clientid;   // unique client id, such as  __DATE__ __TIME__;
	PubSubClient client;           // MQTT publish/subscribe client
	struct {
		char const * const topic;  // subscribe topic name
		uint16_t value;            // subscribe last received value
	} sub;
	struct {
		char const * const topic;  // publish topic name
		uint16_t value;            // publish last published value
		uint16_t interval;         // publish interval [msec] 
	} pub;
} _mqtt = {
	"rpi3.vonk",
	__TIMESTAMP__,
	PubSubClient(_wifi.client),
    { "livingroom/lamp", 0 },
    { "livingroom/lamp-button", 0, 1000 }
};


uint8_t const BUTTON_PIN = 0;
uint8_t const GREENLED_PIN = 13;
uint8_t const REDLED_AND_RELAY_PIN = 12;

void _mqtt_callback(char * topic, byte * payload, unsigned int length)
{
	digitalWrite(BUILTIN_LED, (char)payload[0] == '1' ? LOW : HIGH);
	_mqtt.sub.value = atoi((const char *)payload);
	_displayStatus();
}

void _displayStatus() {
	bool const wifiConnected = WiFi.status() == WL_CONNECTED;
	bool const mqttConnected = _mqtt.client.connected();

	Serial.print("WiFi "); Serial.println(wifiConnected ? _wifi.ssid : "connecting ...");
	Serial.print("MQTT "); Serial.println(mqttConnected ? _mqtt.broker : "connecting ...");
	Serial.print("pub "); Serial.print(_mqtt.pub.topic); Serial.print("="); Serial.println(_mqtt.pub.value);
	Serial.print("sub "); Serial.print(_mqtt.sub.topic); Serial.print("="); Serial.println(_mqtt.sub.value);
}

void setup() {
	Serial.begin(74880);
	delay(100);

	pinMode(BUTTON_PIN, INPUT);
	pinMode(GREENLED_PIN, OUTPUT);
	pinMode(REDLED_AND_RELAY_PIN, OUTPUT);

	// connect WiFi
	_displayStatus();
	WiFi.begin(_wifi.ssid, _wifi.passwd);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}
	_displayStatus();

	// setup MQTT
	_mqtt.client.setServer(_mqtt.broker, 1883);
	_mqtt.client.setCallback(_mqtt_callback);
}

void reconnect_mqtt()  // an async alternative is mqtt_reconnect_nonblocking
{
	while (!_mqtt.client.connected()) {
		_displayStatus();
		if (_mqtt.client.connect(_mqtt.clientid)) {
			Serial.println("OK");
			_mqtt.client.subscribe(_mqtt.sub.topic);
		} else {
			Serial.println(_mqtt.client.state());  // see http://pubsubclient.knolleary.net/api.html#connect3
			delay(1000);
		}
	}
	_displayStatus();
}

void loop() 
{
	if (!_mqtt.client.connected()) {
		reconnect_mqtt();
	}
	_mqtt.client.loop();

	static int buttonPushCounter = 0;
	static int lastButtonState = 0;

	int buttonState = digitalRead(BUTTON_PIN);
	if (buttonState != lastButtonState && buttonState == LOW) {
		buttonPushCounter++;  // 2BD should debounce
		_mqtt.pub.value = buttonPushCounter;
		char s0[6];
		itoa(_mqtt.pub.value, s0, 10);
		_mqtt.client.publish(_mqtt.pub.topic, s0);
		_displayStatus();
	}
	lastButtonState = buttonState;
}
