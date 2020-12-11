#include "application.h"
#include <string>
#include <cstdlib>
Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {{"app", LOG_LEVEL_ALL}});
STARTUP(cellular_credentials_set("", "", "", NULL));
SYSTEM_MODE(MANUAL);
static int lines;
static int handler(int type, const char* buf, int len, int* lines) {
    Log.info("type: %d, data: %s", type, buf);
    return 0;
}
void setup(void) {
    Cellular.on();
    // Must add delay here, wait modem ready
    delay(5000);
    Cellular.command(handler, &lines, 5000, "AT+QGPS=1");
    // Must add delay here
    delay(5000);
    Cellular.command(handler, &lines, 5000, "AT+QGPSCFG=\"nmeasrc\",1");         // nmeasrc enable
    Cellular.command(handler, &lines, 5000, "AT+QGPSCFG=\"gpsnmeatype\",31");    // glonassnmeatype defaul has been configured NMEA data output 
    Cellular.command(handler, &lines, 5000, "AT+QGPSCFG=\"glonassnmeatype\",7"); // glonassnmeatype
    Cellular.command(handler, &lines, 5000, "AT+QGPSCFG=\"galileonmeatype\",1"); // galileonmeatype
    Cellular.command(handler, &lines, 5000, "AT+QGPSCFG=\"beidounmeatype\",3");  // beidounmeatype
    Cellular.command(handler, &lines, 5000, "AT+QGPSCFG=\"gsvextnmeatype\",1");  // gsvextnmeatype

    // Cellular.command(handler, &lines, 5000, "AT+QGPSCFG=\"outport\",\"uartdebug\"");    // specified GPX_TX pint to output nmea data 
    Cellular.command(handler, &lines, 5000, "AT+QGPSCFG=\"outport\",\"usbnmea\"");      // specified usb port to output nmea data

    // Enable the power of GPS antenna
    Cellular.command(1000, "at+qgpios=18,0,1,3,1\r\n"); // set the mode of the GPIO
    Cellular.command(1000, "at+qgpiow=18,1\r\n"); // set the GPIO output high
}
void loop() {
    delay(1000);
    Cellular.command(handler, &lines, 5000, "AT+QGPSLOC?");
}
