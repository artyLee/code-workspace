/*
 * Project AD7192
 * Description:
 * Author:
 * Date:
 */
#include "ad7192.h"
#include "ad7192_defs.h"
#include "application.h"

Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {{"app",    LOG_LEVEL_ALL}});
SYSTEM_MODE(MANUAL);
void setup() {
    ad7192Init();
    ad7192SetPGAGain(1);
    ad7192SetFilterSelectBit(100);
    ad7192InternalZeroFullScaleCalibration();
}

void loop() {
    // Test ADC CH1 raw data to convert voltage
    uint32_t rawData = ad7192ReadADCChannelData(AD7192_CH_AIN1);
    float volt = ad7192RawDataToVoltage(rawData);
    Log.info("CH1 data: %lx, Voltage Measurement: %fmV", rawData, volt);
    delay(1000);
}

