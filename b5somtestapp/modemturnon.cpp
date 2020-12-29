#include "application.h"

#define SERIAL Serial
SYSTEM_MODE(MANUAL);
Serial1LogHandler logHandler(115200, LOG_LEVEL_ALL, {
                                {"ncp",     LOG_LEVEL_ALL},
                                {"app",     LOG_LEVEL_ALL},
                                {"lwip",    LOG_LEVEL_ALL},
                                {"ot",      LOG_LEVEL_ALL},
                                {"sys",     LOG_LEVEL_ALL},
                                {"net.pppncp", LOG_LEVEL_ALL},
                                {"system",  LOG_LEVEL_ALL}
                            });

#if PLATFORM_ID == PLATFORM_BSOM
#define MODEM_PWR       34 // UBPWR
#define MODEM_RST       35 // UBRST
#define MODEM_BUFEN     36 // BUFEN
#define MODEM_PWR_STATE 37 // UBVINT
#elif PLATFORM_ID == PLATFORM_B5SOM
#define MODEM_PWR       32 // BGPWR
#define MODEM_RST       33 // BGRST
#define MODEM_PWR_STATE 34 // BGVINT
#define MODEM_CTS1      30 // CTS1
#define MODEM_RTS1      31 // RTS1
#elif PLATFORM_ID == PLATFORM_TRACKER
#define MODEM_PWR       26 // BGPWR
#define MODEM_RST       25 // BGRST
#define MODEM_PWR_STATE 27 // BGVINT
#define MODEM_CTS1      22 // CTS1
#define MODEM_RTS1      21 // RTS1
#endif

void setupModemOn() {
    RGB.control(true);
    RGB.brightness(255);

    // Init GPIO for the Ublox/Quectel power system
    HAL_Pin_Mode(MODEM_PWR, OUTPUT);
    HAL_Pin_Mode(MODEM_RST, OUTPUT);
    HAL_Pin_Mode(MODEM_PWR_STATE, INPUT);

#if PLATFORM_ID == PLATFORM_BSOM
    // Init GPIO for the Ublox power system
    HAL_GPIO_Write(MODEM_PWR, HIGH);
    HAL_GPIO_Write(MODEM_RST, HIGH);
#elif PLATFORM_ID == PLATFORM_B5SOM
    // Init GPIO for the Quectel power system
    HAL_GPIO_Write(MODEM_PWR, LOW);
    HAL_GPIO_Write(MODEM_RST, LOW);
#elif PLATFORM_ID == PLATFORM_TRACKER
    // Init GPIO for the Quectel power system
    digitalWrite(MODEM_PWR, LOW);
    digitalWrite(MODEM_PWR, LOW);
#endif
    HAL_Delay_Milliseconds(100);

#if PLATFORM_ID == PLATFORM_BSOM
    //Read Ublox modem status
    if (HAL_GPIO_Read(MODEM_PWR_STATE))
#elif PLATFORM_ID == PLATFORM_B5SOM
    //Read Quectel modem status
    if (!HAL_GPIO_Read(MODEM_PWR_STATE))
#elif PLATFORM_ID == PLATFORM_TRACKER
    //Read Quectel modem status
    if (!HAL_GPIO_Read(MODEM_PWR_STATE))
#endif
    {
        RGB.color(255, 0, 0);
        delay(300);
        while(1);   // If modum has been powered on, should not be cotinued runing
    }
#if PLATFORM_ID == PLATFORM_BSOM
    // Turn on Ublox modem
    HAL_GPIO_Write(MODEM_PWR, LOW); // Pull low, i.e. 0
    HAL_Delay_Milliseconds(150);
    HAL_GPIO_Write(MODEM_PWR, HIGH); // Pull high, i.e. 1
    // Reset Ublox
    HAL_GPIO_Write(MODEM_RST, LOW); // Pull low
    HAL_Delay_Milliseconds(150);
    HAL_GPIO_Write(MODEM_RST, HIGH); // Pull high
    // Turn on U15
    HAL_Pin_Mode(MODEM_BUFEN, OUTPUT);
    HAL_Delay_Milliseconds(100);
    HAL_GPIO_Write(MODEM_BUFEN, HIGH); // Pull high
    HAL_Delay_Milliseconds(100);
    HAL_GPIO_Write(MODEM_BUFEN, LOW); // Pull low
#elif PLATFORM_ID == PLATFORM_B5SOM
    // Turn on Quectel modem
    HAL_GPIO_Write(MODEM_PWR, HIGH);
    HAL_Delay_Milliseconds(500);
    HAL_GPIO_Write(MODEM_PWR, LOW);
#elif PLATFORM_ID == PLATFORM_TRACKER
    // Turn on Quectel modem
    digitalWrite(MODEM_PWR, HIGH);
    HAL_Delay_Milliseconds(500);
    digitalWrite(MODEM_PWR, LOW);
#endif

// waitting modem be ready
#if PLATFORM_ID == PLATFORM_BSOM
    while (!HAL_GPIO_Read(MODEM_PWR_STATE)) {
        /* code */
    }
#elif PLATFORM_ID == PLATFORM_B5SOM
    while (HAL_GPIO_Read(MODEM_PWR_STATE)) {
        /* code */
    }
#elif PLATFORM_ID == PLATFORM_TRACKER
    while (HAL_GPIO_Read(MODEM_PWR_STATE)) {
        /* code */
    }
#endif
    // when modem has been ready, then blink blue, green, cyan
    RGB.color(0, 0, 255);
    delay(300);
    RGB.color(0, 255, 0);
    delay(300);
    RGB.color(255, 0, 255);
    delay(300);
    RGB.color(0, 0, 0);

#if PLATFORM_ID == PLATFORM_BSOM
    SERIAL.println("B-SoM Ready!\r\n");
#elif PLATFORM_ID == PLATFORM_B5SOM
    SERIAL.println("B5-SoM Ready!\r\n");
#elif PLATFORM_ID == PLATFORM_TRACKER
    SERIAL.println("Asset Tracker Ready!\r\n");
#endif
}

void setup() {
    SERIAL.begin(115200);
    setupModemOn();
}

void loop() {
    delay(1000);
#if PLATFORM_ID == PLATFORM_BSOM
    SERIAL.println("B-SoM platform!");
#elif PLATFORM_ID == PLATFORM_B5SOM
    SERIAL.println("B5-SoM platform!");
#elif PLATFORM_ID == PLATFORM_TRACKER
    SERIAL.println("Asset Tracker platform!");
#endif
}
