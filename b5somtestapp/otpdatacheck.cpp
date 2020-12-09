#if 1
#include "application.h"
#include "deviceid_hal.h"
#include "exflash_hal.h"
#include "platform_ncp.h"
#include "dct.h"
#include "str_util.h"
SYSTEM_MODE(MANUAL)
#undef CHECK
#define CHECK(_expr) \
        do { \
            const auto _ret = _expr; \
            if (_ret < 0) { \
                Log.error(#_expr " failed: %d", (int)_ret); \
                return _ret; \
            } \
        } while (false)
namespace {
const Serial1LogHandler logHandler(115200, LOG_LEVEL_WARN, {
    { "app", LOG_LEVEL_ALL }
});
#define CFG_SEIRAL_NUMBER "QSOMV200SN00111"
#define CFG_MOBILE_SECRET "QSOMV200DS00111" 
#define CFG_NCP_TYPE      PlatformNCPIdentifier::PLATFORM_NCP_QUECTEL_BG96
const size_t OTP_DEVICE_SERIAL_ADDRESS = 0;
const size_t OTP_DEVICE_SERIAL_SIZE = HAL_DEVICE_SERIAL_NUMBER_SIZE;
const size_t OTP_DEVICE_SECRET_ADDRESS = 16;
const size_t OTP_DEVICE_SECRET_SIZE = HAL_DEVICE_SECRET_SIZE;
const size_t OTP_NCP_ID_ADDRESS = 32;
const size_t OTP_NCP_ID_SIZE = 4;
void dump(const char* data, size_t size) {
    if (isPrintable(data, size)) {
        Log.write(data, size);
    } else {
        Log.dump(data, size);
    }
    Log.print("\r\n");
}
bool isInitialized(const char* data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if ((uint8_t)data[i] != 0xff) {
            return true;
        }
    }
    return false;
}
int eraseDeviceSecretInDct() {
    char secret[DCT_DEVICE_SECRET_SIZE] = {};
    memset(secret, 0xff, sizeof(secret));
    CHECK(dct_write_app_data(secret, DCT_DEVICE_SECRET_OFFSET, sizeof(secret)));
    return 0;
}
int eraseDeviceNameInDct() {
    char name[DCT_SSID_PREFIX_SIZE] = {};
    memset(name, 0xff, sizeof(name));
    CHECK(dct_write_app_data(name, DCT_SSID_PREFIX_OFFSET, sizeof(name)));
    return 0;
}
int eraseSetupCodeInDct() {
    char code[DCT_DEVICE_CODE_SIZE] = {};
    memset(code, 0xff, sizeof(code));
    CHECK(dct_write_app_data(code, DCT_DEVICE_CODE_OFFSET, sizeof(code)));
    return 0;
}
int eraseNcpIdInDct() {
    char id[DCT_NCP_ID_SIZE] = {};
    memset(id, 0xff, sizeof(id));
    CHECK(dct_write_app_data(id, DCT_NCP_ID_OFFSET, sizeof(id)));
    return 0;
}
int eraseDct() {
    CHECK(eraseDeviceNameInDct());
    CHECK(eraseSetupCodeInDct());
    CHECK(eraseDeviceSecretInDct());
    CHECK(eraseNcpIdInDct());
    return 0;
}
int dumpOtpFlash() {
    char serial[OTP_DEVICE_SERIAL_SIZE] = {};
    CHECK(hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, OTP_DEVICE_SERIAL_ADDRESS, (uint8_t*)serial,
            sizeof(serial)));
    Log.info("OTP: Device serial:");
    dump(serial, sizeof(serial));
    char secret[OTP_DEVICE_SECRET_SIZE] = {};
    CHECK(hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, OTP_DEVICE_SECRET_ADDRESS, (uint8_t*)secret,
            sizeof(secret)));
    Log.info("OTP: Device secret:");
    dump(secret, sizeof(secret));
    char data[15] = {};
    CHECK(hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, OTP_DEVICE_SECRET_ADDRESS + OTP_DEVICE_SECRET_SIZE + 1,
            (uint8_t*)data, sizeof(data)));
    Log.info("OTP: Trailing data:");
    dump(data, sizeof(data));
    return 0;
}
int writeDeviceSerialToOtp() {
    // First, ensure the OTP section is not initialized already
    char buf[OTP_DEVICE_SERIAL_SIZE] = {};
    CHECK(hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, OTP_DEVICE_SERIAL_ADDRESS, (uint8_t*)buf, sizeof(buf)));
    if (isInitialized(buf, sizeof(buf))) {
        Log.error("Device serial is already initialized in the OTP flash");
        return -1;
    }
    // Write the serial number
    const char serial[] = CFG_SEIRAL_NUMBER;
    const size_t size = sizeof(serial) - 1; // Not including term. null
    static_assert(size == OTP_DEVICE_SERIAL_SIZE, "Invalid size of the serial number");
    CHECK(hal_exflash_write_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, OTP_DEVICE_SERIAL_ADDRESS, (const uint8_t*)serial,
            size));
    return 0;
}
int writeDeviceSecretToOtp() {
    // First, ensure the OTP section is not initialized already
    char buf[OTP_DEVICE_SECRET_SIZE] = {};
    CHECK(hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, OTP_DEVICE_SECRET_ADDRESS, (uint8_t*)buf, sizeof(buf)));
    if (isInitialized(buf, sizeof(buf))) {
        Log.error("Device secret is already initialized in the OTP flash");
        return -1;
    }
    // Write the device secret
    const char secret[] = CFG_MOBILE_SECRET;
    const size_t size = sizeof(secret) - 1; // Not including term. null
    static_assert(size == OTP_DEVICE_SECRET_SIZE, "Invalid size of the device secret");
    CHECK(hal_exflash_write_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, OTP_DEVICE_SECRET_ADDRESS, (const uint8_t*)secret,
            size));
    return 0;
}
int writeNcpIdToOtp() {
    // First, ensure the OTP section is not initialized already
    char buf[OTP_NCP_ID_SIZE] = {};
    CHECK(hal_exflash_read_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, OTP_NCP_ID_ADDRESS, (uint8_t*)buf, sizeof(buf)));
    if (isInitialized(buf, sizeof(buf))) {
        Log.error("NCP ID is already initialized in the OTP flash");
        return -1;
    }
    // Write the NCP ID
    const uint8_t ncpId = CFG_NCP_TYPE;
    CHECK(hal_exflash_write_special(HAL_EXFLASH_SPECIAL_SECTOR_OTP, OTP_NCP_ID_ADDRESS, &ncpId, sizeof(ncpId)));
    return 0;
}
} // unnamed
void setup() {
    // eraseDct();
    // writeDeviceSerialToOtp();
    // writeDeviceSecretToOtp();
    // writeNcpIdToOtp();
    dumpOtpFlash();
}
void loop() {
}
#endif 

