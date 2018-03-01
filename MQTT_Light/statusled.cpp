/*
* Blinks a status LED to signal status/error
*
* Platform: ESP8266/8285 using Arduino IDE
* Documentation: https://github.com/cvonk/ESP8266_Framework
* Tested with: Arduino IDE 1.8.5, board package esp8266 2.4.0, Adafruit huzzah (feather) esp8266 / general esp8285
*
* GNU GENERAL PUBLIC LICENSE Version 3, check the file LICENSE.md for more information
* (c) Copyright 2018, Coert Vonk
* All text above must be included in any redistribution
*/

#include "statusled.h"

static Ticker _ticker;
static uint8_t _count;
static uint8_t const LEDPIN_UNASSIGNED = 0xFF;
static uint8_t _ledPin = LEDPIN_UNASSIGNED;
static bool _inverse;
static uint8_t _status;

void
setLEDpin(uint8 const pin, bool const inverse) {
	_ledPin = pin;
	_inverse = inverse;
}

static bool
_getLED() {
	return digitalRead(_ledPin) ^ _inverse;
}

static void
_setLED(bool const value) {
	digitalWrite(_ledPin, value ^ _inverse);
}

// display a status number by blinking the LED, pause, and do it again
static void
_blinkStatusLED() {
	if (_count < (_status << 1)) {
		_setLED(!_getLED());
		_count++;
	} else if (_count < (_status << 1) + 5) {
		_count++;
	} else {
		_setLED(false);
		_count = 0;  // start all over after 1s rest
	}
}

void
setStatusLED(uint8_t status) {
	if (_ledPin != LEDPIN_UNASSIGNED) {
		_count = 0;
		_status = status;
		_setLED(false);
		if (status) {
			_ticker.attach(0.2, _blinkStatusLED);
		} else {
			_ticker.detach();
		}
	}
}

