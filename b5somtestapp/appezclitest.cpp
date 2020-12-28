
#include "application.h"
#include "ezcli.h"

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);
#define SERIAL                      Serial
using namespace wos::ezcli; // using name space for ezcli
Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
                                {"ncp",     LOG_LEVEL_ALL},
                                {"app",     LOG_LEVEL_ALL},
                                {"lwip",    LOG_LEVEL_ALL},
                                {"ot",      LOG_LEVEL_ALL},
                                {"sys",     LOG_LEVEL_ALL},
                                {"net.pppncp", LOG_LEVEL_ALL},
                                {"system",  LOG_LEVEL_ALL}
                            });

CLI_CMD_REG(touch, [](int argc, const char* argv[])->void {
    int x = atoi(argv[0]);
    int y = atoi(argv[1]);
    Log.info("touch, x: %d, y: %d", x, y);
    Log.info("touch, argv[0]: %s, argv[1]: %s", argv[0], argv[1]);
    if (strcmp(argv[0], "read") == 0) {
        Log.info("Input string is: \"read\"");
    } else if(strcmp(argv[0], "write") == 0) {
        Log.info("Input string is: \"write\"");
    } else if(strcmp(argv[0], "delete") == 0) {
        Log.info("Input string is: \"delete\"");
    } else if(strcmp(argv[0], "remove") == 0) {
        Log.info("Input string is: \"remove\"");
    } else {
        Log.info("Input string do not match, please try agian");
    }

}, "Format: touch x y\r\nExample: touch 25 25\r\n");

void canEnablePowerOn() {
    pinMode(CAN_RST, OUTPUT);
    digitalWrite(CAN_RST, HIGH);
    pinMode(CAN_INT, INPUT_PULLUP);
    pinMode(CAN_CS, OUTPUT);
    digitalWrite(CAN_CS, HIGH);
    pinMode(CAN_STBY, OUTPUT);
    digitalWrite(CAN_STBY, HIGH); // Normal mode
    pinMode(CAN_PWR, OUTPUT);
    digitalWrite(CAN_PWR, HIGH);
}
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    canEnablePowerOn(); // When useing the latest TrackerONE should Enable CAN, then serial1 can work
}

void loop() {
    ssize_t size =  SERIAL.available();
    if (size > 0) {
        std::unique_ptr<char> dataBuf(new(std::nothrow) char[size]);
        char* p = dataBuf.get();
        for (ssize_t i = 0; i < size; i++ ) {
            p[i] = SERIAL.read();
        }
        EzcliParser.parse(p, size);
        Log.printf("RX: ");
        Log.dump(p, size);
        Log.printf("\r\n");
    }
}

