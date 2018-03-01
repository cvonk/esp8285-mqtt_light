# Rotary Encoder for Arduino101  [![Build Status](https://travis-ci.org/cvonk/esp8285-MQTT_light.svg?branch=master)](https://travis-ci.org/cvonk/esp8285-MQTT_light.svg/)

Features:

* initial configuration using WiFi AP mode (WiFiManager)
* resillent to WiFi and MQTT broker outages
* clear status codes using green LED

## Control your $10 Sonoff S20 using MQTT

This firmware lets you control the Sonoff S20 using the MQTT protocol.  Indirectly, this allows Google Assistant to change the status of your S20 power socket.

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
* using the web page, configure WiFi SSID and password, and MQTT parameters.  Let's assume the topic is "room/lamp".
* this should connect the ESP to your usual WiFi.  The green LED will first blink 2 times (every second) to indicate that it is connecting to the your WiFi router. 
* once connected to WiFi, the green LED will change to 1 blink (every second) to indicate it is connecting to the MQTT broker.
* run Eclipse "ponte" on e.g. raspberry pi (rpi).  "ponte" is a MQTT broker, but can also be accessed using HTTP
* once connected to the MQTT broker, the green LED will go off.

Testing :

* MQTT subscribe
   * Use Eclipse `mosquitto_sub -t room/lamp` on a rpi to subscribe to the MQTT topic. 
   * Press the button on the S20, and the red LED and relay should go on and the `mosquitto_sub` should show the value "on".
   * Press the button again to power it off.
* MQTT publish
   * Use Eclipse `mosquitto_pub -t room/lamp -m on` to turn the red LED and relay on.
   * Use Eclipse `mosquitto_pub -t room/lamp -m off` to turn them off.
* HTTP GET
   * On the rpi, use `curl http://localhost:3000/resources/room/lamp` to read the value of the red LED and relay.
* HTTP PUT   
   * On the rpi, use `curl -X PUT -d on http://localhost:3000/resources/room/lamp` to set the value of the red LED and relay.

## Making it available to the internet

Configuration

* Create an A record on a DNS server to point to your router, or use a DDNS name.  E.g. "house.domain.com".
* On your router use the remote proxy pound with e.g. a LetsEncrypt certificate, to translate between HTTPS and HTTP, and route the traffic URLs "//resources.*" to port 3000 on the rpi.  [pound and letsencrypt](https://coertvonk.com/sw/networking/dd-wrt-reverse-proxy-https-asus-rt-ac68-pound-letsencrypt-23660).  Add "xHTTP 1" inside the "ListenHTTPS" in the `pound.pt1` configuration file to allow for HTTP GET.

Test

* HTTP GET
  * On the rpi, use `curl https://house.domain.com/resources/room/lamp` to read the value of the red LED and relay.
* HTTP PUT   
   * On the rpi, use `curl -X PUT -d on https://house.domain.com/resources/room/lamp` to set the value of the red LED and relay.

## Connecting it to Google Assistent

Configuration

* use 


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
