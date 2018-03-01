# Rotary Encoder for Arduino101  [![Build Status](https://travis-ci.org/cvonk/esp8285-MQTT_light.svg?branch=master)](https://travis-ci.org/cvonk/esp8285-MQTT_light.svg/)

## Control your $10 Sonoff S20 using MQTT

This firmware lets you control the Sonoff S20 using the MQTT protocol.  Indirectly, this allows Google Assistant to change the status of your S20 power socket.

Features:

* initial configuration using WiFi AP mode (WiFiManager)
* recovers from WiFi and MQTT broker outages
* clear status codes using green LED

Hardware:

* connect a FTDI interface to the RX, TX and GND pins
* connect a 3.3V power supply to the VCC and GND pins
* do not connect to mains while the cover is off

Compile:

* clone or download this repository
* install ESP8266 board support using the Arduino IDE
* in the IDE, Tools > Board > Board Manager", add "esp8266"
* in the IDE, Tools > Board, select Generic ESP8285
* in the IDE, Tools > Flash Size, select 1M (64k SPIFFS)
* in the IDE, Tools > Port, select the FTDI port 
* the upload is touchy.  Power off; press upload; press the button, and power up when the IDE says uploading
* from here on, open the Serial monitor to keep an eye on things.

Configuration:

* initially, the green LED will blink 3 times every second to indicate that it is in WiFi AP mode for configuration
* use your phone or laptop to connect to the ESP* access point, this should bring you to the configuration page. (if not, browse to  192.168.4.1 by hand)
* using the web page, configure WiFi SSID and password, and MQTT parameters
* this should connect the ESP to your usual WiFi.  The green LED will first blink 2 times (every second) to indicate that it is connecting to the your WiFi router. 
* once connected to WiFi, the green LED will change to 1 blink (every second) to indicate it is connecting to the MQTT broker.
* once connected to the MQTT broker, the green LED will go off.

Testing:

* run Eclipse ponte on e.g. raspberry pi (rpi)
* use Eclipse mosquitto_sub on a rpi to subscribe to the MQTT topic. Press the button on the S20, and the red LED and relay should go on and the mosquitto_sub should show the value "on".  Press the button again to power it off.
* use Eclipse mosquitto_pub on a rpi to publish the value "on" to the MQTT topic.  The red LED and relay should go on.
* use Eclipse mosquitto_pub on a rpi to publish the value "off" to the MQTT topic.  The red LED and relay should go off.

Making it available to the internet

* on your router use the remote proxy pound with e.g. a LetsEncrypt certificate, to translate between HTTPS and HTTP, and route the traffic URLs starting with /resources to ponte.  Remember to 


	STATUS_OK = 0,
	STATUS_NO_MQTT = 1,
	STATUS_NO_WIFI = 2,
	STATUS_WIFI_CFG = 3,
	STATUS_RESTART = 4,
	STATUS_RESET = 5

Assumptions:

* ability to flash new firmware.  the lThe common pins of the rotary encoder are connected to ground, and the remaining pins to GPIO ports

![Rotary encoder](media/rotrencoder.png)

Many sketches exist for the Arduino UNO, most of them only supporting a single rotary encoder.   I borrowed from [one of them](http://www.instructables.com/id/Improved-Arduino-Rotary-Encoder-Reading/), and adapted it for Arduino/Genuino 101.

The challenge was for the interrupt service routines to invoke a C++ Class Member Function.  The solution chosen relies on virtual functions and friend classes used as described in the article [Interrupts in C++](http://www.embedded.com/design/prototyping-and-development/4023817/Interrupts-in-C-) by Alan Dorfmeyer and Pat Baird.

![Rotary encoder](schematic/schematic.png)

More projects can be found at [coertvonk.com](http://www.coertvonk.com/technology/embedded).
