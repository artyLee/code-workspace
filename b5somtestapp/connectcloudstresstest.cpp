/*
 * Project connect to cloud stress-test
 * Description:
 * Author:
 * Date:
 */
#include "application.h"
#include "ifapi.h"
#include "ssd1306.h"
#include "socket_hal.h"

#define THREAD_DEFAULT_CHANNEL      15
#define THREAD_PANID                0xface
#define SETUP_BTN1                  BTN //A1

// #define CARRIBOARD                  1
#if PLATFORM_ID == PLATFORM_BSOM || PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_TRACKER
#if CARRIBOARD
#define CONTROL_LED                 A2
#define TEST_STATUS_LED             A3
#define NTC                         D0
#else
#define CONTROL_LED                 D7
#define TEST_STATUS_LED             A3
#endif
#define DEV_NAME_PREFIX_LEN         6
#endif
#define SOCKET_DATA_PORT            10005
#if CARRIBOARD
// #define SERIAL                      Serial
#define SERIAL                      Serial1
#else
#define SERIAL                      Serial
#endif
#define SERIAL_NUMBER_LENGTH        15
#define MOBILE_SECRET_LENGTH        15
#define NORDIC_DEVICE_PREFIX        0x68ce0fe0
#define OTP_SN_ADDR                 (0x00)
#define RANDOM_MAC_ADDRESS_LOC      (0x100000A4)
#define DEV_NAME_SUFFIX_LEN         6
#define SN_SETUP_CODE_OFFSET        9 // The offset of the device setup code in the serial number string.
#define VERSION                     "0.0.2-rc1"
#define LEN_NAME                    20
#define MAX_ITEM                    10
#define SIZE_NODE_DATA              24
#define MAX_CNT_CCID                500
#define SIZE_ARRAY                  (SIZE_NODE_DATA*MAX_ITEM)

constexpr uint16_t TEST_LIMIT_RESET = 1;
constexpr uint16_t TEST_LIMIT_CCID = 5;
constexpr uint16_t TEST_LIMIT_DISCONNECT = 5;
constexpr uint16_t TEST_LIMIT_RTC = 5;

#if CARRIBOARD
SerialLogHandler logHandler(115200, LOG_LEVEL_ALL, {
                                {"ncp",     LOG_LEVEL_ALL},
                                {"app",     LOG_LEVEL_ALL},
                                {"lwip",    LOG_LEVEL_ALL},
                                {"sys",     LOG_LEVEL_ALL},
                                {"system",  LOG_LEVEL_ALL}
                            });

// Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {{"app", LOG_LEVEL_ALL}});
#else
Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
                                {"ncp",     LOG_LEVEL_ALL},
                                {"app",     LOG_LEVEL_ALL},
                                {"lwip",    LOG_LEVEL_ALL},
                                {"sys",     LOG_LEVEL_ALL},
                                {"system",  LOG_LEVEL_ALL}
                            });
#endif // CARRIBOARD

//static int      addr0 = 1;
static int      addr1 = 4;
static int      addr2 = 8;
static int      rtc_addr = 10;
static int      rtc_time_addr = 12;
static int      disconnect_cloud_cnt_addr = 16;

static int OLED_Flag = 1; // if OLED_Flag == 1 then do not init OLED
uint32_t   timer_old  = 0;
uint32_t   timer_esim = 0;
uint32_t   timer_rtc_wait = 0;
uint32_t   rtc_last_time_read = 0;
static int erase_flag = 0;
static   uint16_t count_ccid = 0;
static   uint16_t count_rtc = 0;
static   uint16_t disconnectCloudCount = 0;

static const unsigned device_id_len = 12;
static char     serialNumber[SERIAL_NUMBER_LENGTH+1] = {'\0'};
static char     deviceName[20] = {'\0'};
static char     macAddr[20] = {'\0'};
static char     deviceIDStr[25]; // Device ID in string
static char     flashIDStr[7] = {'\0'};

FuelGauge fuelGauge;

// SYSTEM_THREAD(ENABLED);

static void leds_init(void) {
    pinMode(CONTROL_LED,OUTPUT);
    digitalWrite(CONTROL_LED,LOW);

    pinMode(TEST_STATUS_LED, OUTPUT);
    digitalWrite(TEST_STATUS_LED, LOW);
}

static void button_init(void) {
    pinMode(SETUP_BTN1, INPUT_PULLUP);
    HAL_Pin_Mode(BTN, INPUT_PULLUP);
}


void serialPrint(const char *str) {
    SERIAL.print(str);//user serial1
}

void serialPrintln(const char *str) {
    SERIAL.println(str);//user serial1
}

void printItem(const char *name, const char *value) {
    serialPrint(name);
    serialPrint(": ");
    serialPrintln(value);
}

void printInfo(void) {
    printItem("Version", VERSION);
    printItem("Board", (const char *)deviceName);
    printItem("DeviceID", deviceIDStr);
    printItem("MAC Address", macAddr);
    printItem("Flash ID", flashIDStr);
}

static void constructDeviceName(void) {
#if PLATFORM_ID == PLATFORM_B5SOM
    strcpy(deviceName, "B5SoM-");
#elif PLATFORM_ID == PLATFORM_BSOM // Compatible to old version of Device-OS
    strcpy(deviceName, "Boron-SoM-");
#elif PLATFORM_ID == PLATFORM_TRACKER
    strcpy(deviceName, "ATSoM-");
#endif

    // Get serial number.
    uint8_t temp[SERIAL_NUMBER_LENGTH+1];
    memset(temp, 0xFF, sizeof(temp));
    // flash_read_otp(OTP_SN_ADDR, serialNumber, SERIAL_NUMBER_LENGTH+1);
    // hal_exflash_read(OTP_SN_ADDR,serialNumber,SERIAL_NUMBER_LENGTH);
    if (!memcmp(temp, serialNumber, SERIAL_NUMBER_LENGTH+1)) {
        // The OTP hasn't been programmed yet.
        memset(serialNumber, 'F', SERIAL_NUMBER_LENGTH);
    }
    else {
        for (uint8_t i = 0; i < SERIAL_NUMBER_LENGTH; i++) {
            if ((serialNumber[i] >= '0' && serialNumber[i] <='9') ||
                (serialNumber[i] >= 'A' && serialNumber[i] <='Z')) {
                continue;
            }
            else {
                serialNumber[i] = '?';
            }
        }
    }
    serialNumber[SERIAL_NUMBER_LENGTH] = '\0';
    memcpy(&deviceName[DEV_NAME_PREFIX_LEN], &serialNumber[SN_SETUP_CODE_OFFSET], DEV_NAME_SUFFIX_LEN);
    deviceName[DEV_NAME_PREFIX_LEN + DEV_NAME_SUFFIX_LEN] = '\0';
}

static unsigned HAL_device_ID(uint8_t* dest, unsigned destLen) {
    uint32_t device_id[3] = {NORDIC_DEVICE_PREFIX, NRF_FICR->DEVICEID[0], NRF_FICR->DEVICEID[1]};

    if (dest != NULL && destLen > 0) {
        memcpy(dest, (char*)device_id, MIN(destLen, device_id_len));
    }
    return device_id_len;
}


void device_INFO(void) {
    // Device ID, MAC Address
    uint8_t devID[12];
    uint8_t *p = (uint8_t *)RANDOM_MAC_ADDRESS_LOC;

    // Get Device ID
    HAL_device_ID(devID, 12);
    sprintf( deviceIDStr, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        devID[0], devID[1], devID[2], devID[3], devID[4], devID[5], devID[6], devID[7], devID[8], devID[9], devID[10], devID[11] );

    // Get MAC Address
    sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", p[5] | 0xC0, p[4], p[3], p[2], p[1], p[0]);

    constructDeviceName();

    // exflash_init(flashIDStr);

    printInfo();
}


// Get the ICCID number of the inserted SIM card
int callbackICCID(int type, const char* buf, int len, char* iccid)
{
    if ((type == TYPE_PLUS) && iccid) {
        if (sscanf(buf, "\r\n+CCID: %[^\r]\r\n", iccid) == 1)
        /*nothing*/;
    }
    return WAIT;
}

int read_esim_info(void)
{
    char iccid[32] = "";
    char str[20];
    sprintf(str,"CCID count is %d", count_ccid);
    if ((RESP_OK == Cellular.command(callbackICCID, iccid, 5000, "AT+CCID\r\n"))
        && (strcmp(iccid,"") != 0)) {
        SERIAL.printf("ICCID: %s\r\n", iccid);
    } else {
        Log.info("Could not read CCID");
        SERIAL.println("NCP: FAIL - CCID!");
        count_ccid++;
        EEPROM.put(addr2,count_ccid);
        SERIAL.printlnf("CCID count is: %d", count_ccid);
        Cellular.on();
        delay(15000);// wait cellular to ready status.
        if (count_ccid >= MAX_CNT_CCID) {
            RGB.control(true);
            RGB.brightness(255);
            RGB.color(255, 0, 0);
            if (OLED_Flag != 1) {
                oled.println("Read Modem error!!!");
            }
            while(1); // stop here, don't run program
        }
    }
    return 0;
}

void loop_handle_check_esim(void)
{
    if (millis() - timer_esim >= 5000) {
        timer_esim = millis();
        read_esim_info();
    }
}

void enter_shipping_mode()
{
    digitalWrite(CONTROL_LED,HIGH);
    digitalWrite(TEST_STATUS_LED, LOW);

    PMIC pmic;

    // blink RGB to signal entering shipping mode
    RGB.control(true);
    RGB.brightness(255);
    for(int i=0;
        i < (5000 / 250);
        i++)
    {
        // cycle between primary colors
        RGB.color(((uint32_t) 0xFF) << ((i % 3) * 8));
        HAL_Delay_Milliseconds(250);
    }

    WITH_LOCK(pmic)
    {
        pmic.disableWatchdog();
        pmic.disableBATFET();
    }

    RGB.brightness(0);

    // sleep forever waiting for power to be removed
    // leave network on for quicker drain of residual power
    // once main power is removed
    System.sleep(SLEEP_MODE_DEEP, 0, SLEEP_NETWORK_STANDBY);

    // shouldn't hit these lines as never coming back from sleep but out of an
    // abundance of paranoia force a reset so we don't get stuck in some weird
    // pseudo-shutdown state
    System.reset();
}

void read_from_EEPRom(void)
{
    uint16_t reset_cnt = 0;
    uint16_t ccid_cnt = 0;
    uint16_t wrtc_cnt = 0;
    uint32_t rtc_latest_time = 0;
    bool shutdown = false;

    if (SERIAL.available() > 0) {
        switch(SERIAL.read()) {
            case 'r':
                SERIAL.println("input character: r");
                EEPROM.get(addr1,reset_cnt);
                SERIAL.printlnf("System reset count:%d", reset_cnt);
                EEPROM.get(addr2,ccid_cnt);
                #if PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_TRACKER
                SERIAL.printlnf("Quectel ccid count:%d", ccid_cnt);
                #else
                SERIAL.printlnf("ublox ccid count:%d", ccid_cnt);
                #endif
                EEPROM.get(disconnect_cloud_cnt_addr, disconnectCloudCount);
                SERIAL.printlnf("Disconnect cloud count: %d", disconnectCloudCount);
                EEPROM.get(rtc_addr, wrtc_cnt);
                SERIAL.printlnf("RTC failed count:%d", wrtc_cnt);
                EEPROM.get(rtc_time_addr, rtc_latest_time);
                SERIAL.printlnf("RTC UTC time:%ld", rtc_latest_time);
                break;

            case '1': {
                SERIAL.println("input character: 1");
                disconnectCloudCount += 6;
                EEPROM.put(disconnect_cloud_cnt_addr, disconnectCloudCount);
                break;
            }

            case 'g':
                SERIAL.println("input character: g");
                shutdown = true;
            // Fall through
            case 'e':
                SERIAL.println("input character: e");
                EEPROM.clear();
                SERIAL.println("Clean EEPROM sucess:");

                if (shutdown) {
                    SERIAL.println("System entering shipping mode:");
                    delay(1000);
                    enter_shipping_mode();
                } else {
                    SERIAL.println("System reset:");
                    System.reset();
                }
                break;

           case 'x':
                SERIAL.println("input character: x");
                OLED_Flag = 1;
                SERIAL.println("Set OLED_Flag Done");
                break;

            case 'a':
                SERIAL.println("input character: all");
                for (int i = 0; i < SIZE_ARRAY; i++) {
                    uint8_t value = EEPROM.read(i);
                    SERIAL.print(value);
                    SERIAL.printlnf(" :%c",value);
                }
                break;

            case 's':
                SERIAL.println("input character: s");
                delay(1000);
                enter_shipping_mode();
                break;

            default:
            break;
        }
    }
}

void button_clicked(system_event_t event, int param)
{
    int times = system_button_clicks(param);
    SERIAL.printlnf("button was clicked %d times", times);
    erase_flag = 1;
}
void erase_EEPRom(void) {
    oled.println("Mode button pressed:");
    oled.println("Start clean EEPROM:");
    SERIAL.println("Start clean EEPROM:");
    EEPROM.clear();
    SERIAL.println("Clean EEPROM sucess:");
    oled.println("Clean EEPROM sucess:");
    SERIAL.println("System reset:");
    oled.println("System reset:");
    System.reset();
}
void check_erase_flag(void) {
    if (erase_flag == 1) {
        erase_EEPRom();
    }
}

void record_ublox_cnt() {
    uint16_t ublox_cnt = 0;
    char str [64];
    EEPROM.get(addr2,ublox_cnt);
    if (ublox_cnt >= 65535) {
        ublox_cnt = 0;
    }
    count_ccid = ublox_cnt;
#if PLATFORM_ID == PLATFORM_B5SOM || PLATFORM_ID == PLATFORM_TRACKER
    SERIAL.printlnf("Quectel ccid count:%d", ublox_cnt);
    snprintf(str, ARRAY_SIZE(str), "Quectel ccid count:%d", ublox_cnt);
#else
    SERIAL.printlnf("ublox ccid count:%d", ublox_cnt);
    snprintf(str, ARRAY_SIZE(str), "ublox ccid count:%d", ublox_cnt);
#endif
    EEPROM.put(addr2, ublox_cnt);
}

void record_reset_cnt() {
    uint16_t reset_cnt = 0;
    char str [64];
    EEPROM.get(addr1,reset_cnt);
    if (reset_cnt >= 65535) {
        reset_cnt = 0;
        EEPROM.put(addr1, reset_cnt);
    }
    reset_cnt++;
    SERIAL.printlnf("System reset count:%d", reset_cnt);
    if (OLED_Flag != 1) {
        snprintf(str, ARRAY_SIZE(str), "System reset count:%d", reset_cnt);
        oled.println(str);
    }
    EEPROM.put(addr1, reset_cnt);
}

void record_rtc_cnt() {
    uint16_t rtc_cnt = 0;
    uint32_t latest_time = 0;
    char str [64];
    EEPROM.get(rtc_addr, rtc_cnt);
    EEPROM.get(rtc_time_addr, latest_time);
    if (rtc_cnt >= 65535) {
        rtc_cnt = 0;
        EEPROM.put(rtc_addr, rtc_cnt);
    }
    if (latest_time >= 4294967295) {
        latest_time = Time.now();
        EEPROM.put(rtc_time_addr, latest_time);
    }
    count_rtc = rtc_cnt;
    rtc_last_time_read = latest_time;
    SERIAL.printlnf("RTC failed count:%d", rtc_cnt);
    SERIAL.printlnf("RTC UTC time:%ld", latest_time);
    if (OLED_Flag != 1) {
        snprintf(str, ARRAY_SIZE(str), "RTC failed count:%d", rtc_cnt);
        oled.println(str);
    }
}

void loop_handle_check_rtc(void){

}

void read_rtc_time() {

}

void enable_wifi(void) {

}

void enable_can(void) {

}

void enable_gnss(void) {

}

void record_disconnect_cloud_cnt() {
    uint16_t disCloudCnt = 0;
    EEPROM.get(disconnect_cloud_cnt_addr, disCloudCnt);
    if (disCloudCnt >= 65535) {
        disCloudCnt = 0;
    }
    disconnectCloudCount = disCloudCnt;
    SERIAL.printlnf("Disconnect cloud count: %d", disCloudCnt);
    EEPROM.put(disconnect_cloud_cnt_addr, disCloudCnt);
}

void loop_handle_detect_cloud() {
    static system_tick_t lastMillis = millis();
    float fvcell = fuelGauge.getVCell();
    float fpcell = fuelGauge.getSoC();
    int batteryState = System.batteryState();
    uint16_t reset_cnt = 0;
    uint16_t ccid_cnt = 0;
    uint16_t wrtc_cnt = 0;
    // Optional data, up to 622 bytes of UTF-8 encoded characters. (Limited to 255 bytes prior to Device OS 0.8.0.)
    String transferData = {"...This feature allows the device to generate an event based on a condition. For example, you could connect a motion sensor to the device and have the device generate an event whenever motion is detected.Particle.publish pushes the value out of the device at a time controlled by the device firmware. Particle.variable allows the value to be pulled from the device when requested from the cloud side."};
    EEPROM.get(addr1, reset_cnt);
    EEPROM.get(addr2, ccid_cnt);
    EEPROM.get(rtc_addr, wrtc_cnt);
    if ((millis() - lastMillis) > 30000) {
        if (Particle.connected()) {
            SERIAL.printlnf("Connected cloud succesful");
            SERIAL.printlnf("[FUEL] Battery voltage: %fV", fvcell);
            SERIAL.printlnf("[FUEL] Battery percent: %f%%", fpcell);
            SERIAL.printlnf("[PMIC] Battery charging: %d", batteryState);
            Particle.publish("B5SoM_EG91NA_Test", "Date: " + String((long)Time.now()) + \
                            " \r\nSystemResetCount: " + String(reset_cnt) + \
                            " \r\nQuectelCCIDCount: " + String(ccid_cnt) + \
                            " \r\nDisconnectCloudCount: " + String(disconnectCloudCount) + \
                            " \r\nBatteryState: " + String(batteryState) + \
                            " \r\nBatteryVoltage: " + String(fvcell) +"v" + \
                            " \r\nBatteryCapacity: " + String(fpcell) +"%" + \
                            " \r\nTransferString: " + transferData +"#", 60, PRIVATE);
        } else {
            disconnectCloudCount++;
            Log.info("Disconnected from cloud");
            SERIAL.printlnf("Disconnect cloud count: %d", disconnectCloudCount);
            EEPROM.put(disconnect_cloud_cnt_addr, disconnectCloudCount);
        }
        lastMillis = millis();

        SERIAL.println("Checking status for LED");
        if ((reset_cnt > TEST_LIMIT_RESET) ||
            (ccid_cnt > TEST_LIMIT_CCID) ||
            (disconnectCloudCount > TEST_LIMIT_DISCONNECT) ||
            (wrtc_cnt > TEST_LIMIT_RTC) ) {
            digitalWrite(TEST_STATUS_LED, HIGH);
        } else {
            digitalWrite(TEST_STATUS_LED, LOW);
        }

    }
}

int command_parser(String command) {
    if (command == "g") {
        EEPROM.clear();
        delay(1000);
        enter_shipping_mode();
        return 0;
    } else if (command == "e") {
        EEPROM.clear();
        System.reset();
        return 0;
    } else if (command == "s") {
        delay(1000);
        enter_shipping_mode();
        return 0;
    }

    return -1;
}

// setup() runs once, when the device is first turned on.
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    leds_init();
    button_init();
    if (OLED_Flag != 1) {
        oled.init();
        oled.clear();
        oled.println("Restarted");
    }
    enable_wifi();
    enable_can();
    enable_gnss();
    device_INFO();
    record_reset_cnt();
    record_ublox_cnt();
    record_rtc_cnt();

    Particle.function("command", command_parser);

    Cellular.on();
    Cellular.connect();
    Particle.connect();
    timer_old  = millis();
    timer_esim = millis();
    record_disconnect_cloud_cnt();
    System.on(button_click, button_clicked);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
    loop_handle_check_esim();
    loop_handle_check_rtc();
    read_from_EEPRom();
    check_erase_flag();
    loop_handle_detect_cloud();
}
