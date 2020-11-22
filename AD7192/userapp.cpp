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
}

void loop() {

    delay(1000);
    uint32_t registervalue[8] = {0};
    registervalue[0] = ad7192ReadRegisterValue(AD7192_REG_STAT, 1);
    registervalue[1] = ad7192ReadRegisterValue(AD7192_REG_MODE, 3);
    registervalue[2] = ad7192ReadRegisterValue(AD7192_REG_CONF, 3);
    registervalue[3] = ad7192ReadRegisterValue(AD7192_REG_DATA, 3);
    registervalue[4] = ad7192ReadRegisterValue(AD7192_REG_ID, 1);
    registervalue[5] = ad7192ReadRegisterValue(AD7192_REG_GPOCON, 1);
    registervalue[6] = ad7192ReadRegisterValue(AD7192_REG_OFFSET, 3);
    registervalue[7] = ad7192ReadRegisterValue(AD7192_REG_FULLSCALE, 3);

    Log.printf("Register 0 value: 0x%lx \r\n", registervalue[0]);
    Log.printf("Register 1 value: 0x%lx \r\n", registervalue[1]);
    Log.printf("Register 2 value: 0x%lx \r\n", registervalue[2]);
    Log.printf("Register 3 value: 0x%lx \r\n", registervalue[3]);
    Log.printf("Register 4 value: 0x%lx \r\n", registervalue[4]);
    Log.printf("Register 5 value: 0x%lx \r\n", registervalue[5]);
    Log.printf("Register 6 value: 0x%lx \r\n", registervalue[6]);
    Log.printf("Register 7 value: 0x%lx \r\n", registervalue[7]);
    Log.print("-------------------------------------------\r\n");
}

