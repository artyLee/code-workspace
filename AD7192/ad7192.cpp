#include "ad7192.h"
#include "ad7192_defs.h"

#define CS_PIN                  D5 // Chip select pin
/* AD7192 register 0~7 default(Power-On/Reset) value:
    0x00,           // 0 - status register
    0x080060,       // 1 - mode register
    0x000117,       // 2 - configuration register
    0x000000,       // 3 - data register
    0xa0,           // 4 - ID register
    0x00,           // 5 - GPOCON register
    0x800000,       // 6 - offset register
    0x553f60        // 7 - full-scale register
*/
uint32_t registerMap[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t registerSize[8] = {1, 3, 3, 3, 1, 1, 3, 3};

void ad7192Init(void) {
    pinMode(CS_PIN, OUTPUT); // CS pin initialization
    digitalWrite(CS_PIN, HIGH);
    SPI.begin();             // SPI initialization
    delay(100);
    ad7192SoftwareReset();
}

void ad7192SoftwareReset(void) {
    SPI.beginTransaction(SPISettings(4*MHZ, MSBFIRST, SPI_MODE3));
    digitalWrite(CS_PIN, LOW);
    for(char i = 0; i < 6; i++) { // If a Logic 1 is written to the AD7192 DIN line for at least 40 serial clock cycles, the serial interface is reset
        SPI.transfer(0xFF);
    }
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
    delay(1);                   // Following a reset, the user should wait a period of 500 μs before addressing the serial interface
}

void ad7192SetPGAGain(uint8_t gain) {
    uint32_t gainSetting = 0;
    uint8_t regAddress = AD7192_REG_CONF;
    switch (gain) { // Valid Gain settings are 1, 8, 16, 32, 64, 128
        case 1:
            gainSetting = 0x0; // G2 G1 GO: 000
            break;
        case 8:
            gainSetting = 0x3; // G2 G1 GO: 011
            break;
        case 16:
            gainSetting = 0x4; // G2 G1 GO: 100
            break;
        case 32:
            gainSetting = 0x5; // G2 G1 GO: 101
            break;
        case 64:
            gainSetting = 0x6; // G2 G1 GO: 110
            break;
        case 128:
            gainSetting = 0x7; // G2 G1 GO: 111
            break;
        default:
            Log.error("ERROR - Invalid Gain Setting. Valid Gain settings are 1, 8, 16, 32, 64, 128");
            return;
    }
    registerMap[regAddress] = ad7192ReadRegisterValue(regAddress, registerSize[regAddress]); // Read before setting configuration register value
    registerMap[regAddress] &= 0xFFFFF8; // keep all bit values except gain bits
    registerMap[regAddress] |= gainSetting; // setting gain bits values (G2 G1 G0)
    ad7192WriteRegisterValue(regAddress, registerMap[regAddress], registerSize[regAddress]); // After write setting value to configuration register
}

void ad7192InternalZeroScaleCalibration(void) {
    // 1.read--->2.edit--->3.write
    uint8_t regAddress = AD7192_REG_MODE;
    registerMap[regAddress] = ad7192ReadRegisterValue(regAddress, registerSize[regAddress]); // Read before setting mode register value
    registerMap[regAddress] &= 0x1FFFFF; // keep all bit values except mode select bits(MD2 MD1 MD0)
    registerMap[regAddress] |= 0x800000; // internal zero scale calibration(MD2 MD1 MD0 = 1, 0, 0)
    ad7192WriteRegisterValue(regAddress, registerMap[regAddress], registerSize[regAddress]); // After overwriting setting value to mode register
}

uint32_t ad7192ReadRegisterValue(uint8_t registerAddress, uint8_t bytesSize) {
    uint8_t receiveBuffer = 0;
    uint8_t byteIndex = 0;
    uint32_t readRegisterValue = 0;
    uint8_t writeCommandByte = AD7192_COMM_READ | AD7192_COMM_ADDR(registerAddress);
    SPI.beginTransaction(SPISettings(4*MHZ, MSBFIRST, SPI_MODE3));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(writeCommandByte);
    while(byteIndex < bytesSize) {
        receiveBuffer = SPI.transfer(0);
        readRegisterValue = (readRegisterValue << 8) + receiveBuffer;
        byteIndex++;
    }
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();

    return(readRegisterValue);
}

void ad7192WriteRegisterValue(uint8_t registerAddress, uint32_t registerValue, uint8_t bytesSize) {
    uint8_t txBuffer[4] = {0, 0, 0, 0};
    uint8_t writeCommandByte = AD7192_COMM_WRITE | AD7192_COMM_ADDR(registerAddress);
    txBuffer[0] = (registerValue >> 0)  & 0x000000FF;
    txBuffer[1] = (registerValue >> 8)  & 0x000000FF;
    txBuffer[2] = (registerValue >> 16) & 0x000000FF;
    txBuffer[3] = (registerValue >> 24) & 0x000000FF;
    SPI.beginTransaction(SPISettings(4*MHZ, MSBFIRST, SPI_MODE3));
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(writeCommandByte);
    while(bytesSize > 0) {
        SPI.transfer(txBuffer[bytesSize - 1]);
        bytesSize--;
    }
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
}

