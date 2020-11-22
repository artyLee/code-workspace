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

}

