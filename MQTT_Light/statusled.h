// statusled.h

#pragma once
#include "Arduino.h"
#include <Ticker.h>

typedef enum {
	STATUS_OK = 0,
	STATUS_NO_MQTT = 1,
	STATUS_NO_WIFI = 2,
	STATUS_WIFI_CFG = 3,
	STATUS_RESTART = 4,
	STATUS_RESET = 5
} statusled_t;

extern void setStatusLED(uint8_t status);
void setLEDpin(uint8 const pin, bool const inverse = false);
