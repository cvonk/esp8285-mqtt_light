
#include "testfatal.h"

using namespace TestFatal;

ESP8266WebServer TestFatal::server(80);  // declared
ESP8266WebServer &server

void tripReset(void) {
	ESP.reset();  // nothing will be saved in EEPROM
}
void tripRestart(void) {
	ESP.restart();  // nothing will be saved in EEPROM
}
void tripHardwareWdt(void) {
	ESP.wdtDisable();
	while (true) {
		; // block forever
	} // nothing will be saved in EEPROM
}
void tripSoftwareWdt(void) {
	while (true) {
		; // block forever
	}
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiv-by-zero"
#pragma GCC diagnostic ignored "-Wunused-variable"
void tripDivBy0(void) {
	int zero = 0;
	int volatile result = 1 / 0;
}
void tripReadNull(void) {
	int volatile result = *(int *)NULL;
}
#pragma GCC diagnostic pop

void tripWriteNull(void) {
	*(int *)NULL = 0;
}

typedef void fnc_t(void);

typedef struct {
	char const * const name;
	char const * const description;
	fnc_t * const      fnc;
} crashcause_t;

crashcause_t crashcauses[] = {
	{ "reset",       "reset module",                    tripReset },       // no Fatal log
{ "restart",     "restart module",                  tripRestart },     // no Fatal log
{ "hardwarewdt", "hardware WDT exception",          tripHardwareWdt }, // no Fatal log
{ "softwarewdt", "software WDT exception (exc.4)",  tripSoftwareWdt },
{ "divby0",      "divide by zero (exc.0)",          tripDivBy0 },
{ "readnull",    "read from null pointer (exc.28)", tripReadNull },
{ "writenull",   "write to null pointer (exc.29)",  tripWriteNull },
{ NULL,          NULL,                              NULL }
};

void handleRoot(void)
{
	String message = "OK\n\n";
	message += "URI: "; message += wifi::server.uri();
	message += "\nMethod: "; message += (wifi::server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: "; message += wifi::server.args();
	message += "\n";
	for (uint8_t i = 0; i < wifi::server.args(); i++) {
		message += " " + wifi::server.argName(i) + ": " + wifi::server.arg(i) + "\n";
	}
	message += "\n\nUse /crash to cause a crash";
	WiFi::server.send(404, "text/plain", message);
}

struct {
	crashcause_t const * cause;
	unsigned long        time;  // [msec]
} scheduled = { NULL, 0 };

void handleCrash(void)
{
	if (wifi::server.args() >= 1 && wifi::server.argName(0) == "req") {
		for (crashcause_t const *p = crashcauses; p->name; p++) {
			if (strcmp(p->name, wifi::server.arg(0).c_str()) == 0) {
				scheduled.cause = p;
				scheduled.time = millis() + 5000;  // time to send the replay
				String message("Performing \"");
				message = message + p->description + "\"<br /><a href=\"/fatal\">Fatal history</a> will be available after the ESP reboots.";

				wifi::server.send(200, "text/html", message);
				return;
			}
		}
	}
	String message("Crash to perform (and log):\n<ul>");
	for (crashcause_t const *p = crashcauses; p->name; p++) {
		message = message + "<li><a href=\"/crash?req=" + p->name + "\">" + p->description + "</a></li>\n";
	}
	message += "</ul><br />"
		"Inspect and clear the <a href=\"/fatal\">Fatal log</a>.<br />"
		"Remember the ESP reboots from the same source it started from. "
		"If you uploaded code through the UART, it will try to boot from the UART.";
	wifi::server.send(200, "text/html", message);
}

uint8_t const
TestFatal::begin(ESP8266WebServer &server) 
{
	server = server_;
WiFi::server.on("/", handleRoot);
WiFi::server.on("/crash", handleCrash);
}


uint8_t const
TestFatal::handle(){
	if ((scheduled.cause != NULL) && (millis() > scheduled.time)) {
		(scheduled.cause->fnc)();
	}
	return 0;
}
