# Itead Sonoff S20 + MQTT  [![Build Status](https://travis-ci.org/cvonk/esp8285-MQTT_light.svg?branch=master)](https://travis-ci.org/cvonk/esp8285-MQTT_light.svg/)

This is a follow-up from [Talk to your CD player using Google Home](https://coertvonk.com/sw/embedded/google-home-ifttt-esp8266-integration-23066).  This time we use the light and elegant MQTT protocol to control a Itead Sonoff S20 power switch.

Features:

* initial configuration using WiFi AP mode (WiFiManager)
* resilient to WiFi and MQTT broker outages
* clear status codes using green LED

## Control your $10 Sonoff S20 using MQTT

This firmware lets you control the Sonoff S20 using the MQTT protocol.  Indirectly, this allows Google Assistant to change the status of your S20 power socket.

Hardware:

* do not connect to mains while the cover is off!
* connect a FTDI interface to the RX, TX and GND pins
* connect a 3.3V power supply to the VCC and GND pins
* [hardware details](https://www.itead.cc/wiki/S20_Smart_Socket)

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
* this should connect the ESP to your usual WiFi.  The green LED will first blink 2 times (every second) to indicate that it is connecting to your WiFi router. 
* once connected to WiFi, the green LED will change to 1 blink (every second) to indicate it is connecting to the MQTT broker.
* run Eclipse "ponte" on e.g. raspberry pi (rpi).  "ponte" is a MQTT broker, but can also be accessed using HTTP
* once connected to the MQTT broker, the green LED will go off.

Testing:

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

The button on the Sonoff S20: 

* short press will toggle the red LED and relay
* 2 second press will restart the Sonoff S20
* 10 second press will reset the configuration

## Making it available to the internet

Configuration:

* Create an A record on a DNS server to point to your router, or use a DDNS name.  E.g. "house.domain.com".
* On your router use the remote proxy pound with e.g. a LetsEncrypt certificate, to translate between HTTPS and HTTP, and route the traffic URLs "//resources.*" to port 3000 on the rpi.  For details, see my [Pound and LetsEncrypt](https://coertvonk.com/sw/networking/dd-wrt-reverse-proxy-https-asus-rt-ac68-pound-letsencrypt-23660).  Add "xHTTP 1" inside the "ListenHTTPS" in the `pound.pt1` configuration file to allow for HTTP GET.

Testing:

* HTTP GET
  * On the rpi, use `curl https://house.domain.com/resources/room/lamp` to read the value of the red LED and relay.
* HTTP PUT   
   * On the rpi, use `curl -X PUT -d on https://house.domain.com/resources/room/lamp` to set the value of the red LED and relay.

## Connecting it to Google Assistant

Configuration:

* On IFTTT, create a new applet (Profile Dropdown » New Applet)
  * Start by specifying a trigger by clicking the +this and selecting “Google Assistant” from the list of services.
    * Click Connect to authorize IFTTT to manage voice commands
    * Click Say a phrase with a text ingredient, and fill in the trigger fields:
    * What do you want to say? = turn light $
    * What’s another way to say it? = switch light $
    * What do you want the Assistant to say in response? = turning light $
    * Press Create Trigger
  * Specify the Action by clicking the +that.
    * Choose action service = Select “Webhooks” and press Connect
    * Choose action = Make a web request
    * Fill in the action fields
    * URL = https://house.domain.com/resources/room/lamp
    * Method = PUT
    * Content Type = text/plain
    * Message = {{TextField}}
    * Press Create Action
  * Press Finish

Testing:

* Using a phone, involve the Google Assistent and say "turn light on".  The red LED and relay should turn on.
* Using a phone, involve the Google Assistent and say "turn light off".  The red LED and relay should turn off.
* Try the same with a Google Home device.

## Diagnostic

Status codes, indicated by blinking the green LED

	STATUS_OK = 0,
	STATUS_NO_MQTT = 1,
	STATUS_NO_WIFI = 2,
	STATUS_WIFI_CFG = 3,
	STATUS_RESTART = 4,
	STATUS_RESET = 5


More projects can be found at [coertvonk.com](http://www.coertvonk.com/technology/embedded).
