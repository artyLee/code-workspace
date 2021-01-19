#include "ad7192.h"
#include "ad7192_defs.h"
#include "application.h"

#define CS_PIN                  D5 // Chip select pin
#define V_REF                   3300 // millivolt reference voltage

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

void ad7192InternalFullScaleCalibration(void) {
    // 1.read--->2.edit--->3.write
    uint8_t regAddress = AD7192_REG_MODE;
    registerMap[regAddress] = ad7192ReadRegisterValue(regAddress, registerSize[regAddress]); // Read before setting mode register value
    registerMap[regAddress] &= 0x1FFFFF; // keep all bit values except mode select bits(MD2 MD1 MD0)
    registerMap[regAddress] |= 0xA00000; // internal full scale calibration(MD2 MD1 MD0 = 1, 0, 1)
    ad7192WriteRegisterValue(regAddress, registerMap[regAddress], registerSize[regAddress]); // After overwriting setting value to mode register
}

void ad7192InternalZeroFullScaleCalibration(void) {
    ad7192InternalZeroScaleCalibration();
    ad7192InternalFullScaleCalibration();
}

void ad7192SetFilterSelectBit(uint16_t filterRate) {
    if (filterRate > 0x3ff) { // FS9~FS0, the max value of 10bit data = 0x3ff
        Log.info("ERROR - Invalid Filter Rate Setting - no changes made.  Filter Rate is a 10-bit value");
        return;
    }
    uint8_t regAddress = AD7192_REG_MODE;
    registerMap[regAddress] = ad7192ReadRegisterValue(regAddress, registerSize[regAddress]); // Read before setting mode register value
    registerMap[regAddress] &= 0xFFFC00; // keep all bit values except filter setting bits
    registerMap[regAddress] |= filterRate; // setting filter bits values (FS9 ~ FS0)
    ad7192WriteRegisterValue(regAddress, registerMap[regAddress], registerSize[regAddress]); // After overwriting setting value to mode register
}

void ad7192SetChannel(uint8_t channel) { // Noted: channel number should be reference 0: AIN1+ AIN2-; 1:AIN3+ AIN4-; 2: temperature sensor; 3:AIN2+ AIN2-; 4:AIN1 AINCOM; 5:AIN2 AINCOM; 6:AIN3 AINCOM; 7:AIN4 AINCOM;
    uint8_t regAddress = AD7192_REG_CONF;
    registerMap[regAddress] = ad7192ReadRegisterValue(regAddress, registerSize[regAddress]); // Read before setting Conffigration register value
    uint32_t shiftvalue = 0x00000100;
    uint32_t channelBits = shiftvalue << channel; // generate Channel settings bits for Configuration write
    registerMap[regAddress] &= 0xFF00FF;          // keep all bit values except Channel bits(CON15~CON8--->CH7~CH0)
    registerMap[regAddress] |= channelBits;       // setting Channel bits to Config register
    ad7192WriteRegisterValue(regAddress, registerMap[regAddress], registerSize[regAddress]);// write channel selected to Configuration register
}

void ad7192StartSingleConversion(void) {
    uint8_t regAddress = AD7192_REG_MODE;
    registerMap[regAddress] = ad7192ReadRegisterValue(regAddress, registerSize[regAddress]); // Read before setting Mode register value
    registerMap[regAddress] &= 0x1FFFFF; // keep all bit values except MR23-MR21 bits
    registerMap[regAddress] |= 0x200000; // setting single conversion mode bits
    ad7192WriteRegisterValue(regAddress, registerMap[regAddress], registerSize[regAddress]);// write channel conversion mode to Mode register
}

void ad7192StartContinuousConversion(void) {
    uint8_t regAddress = AD7192_REG_MODE;
    registerMap[regAddress] = ad7192ReadRegisterValue(regAddress, registerSize[regAddress]); // Read before setting Mode register value
    registerMap[regAddress] &= 0x1FFFFF; // keep all bit values except MR23-MR21 bits
    registerMap[regAddress] |= 0x000000; // setting continous conversion mode bits
    ad7192WriteRegisterValue(regAddress, registerMap[regAddress], registerSize[regAddress]);// write channel conversion mode to Mode register
}

uint32_t ad7192ReadConvertingData(void) {
    uint32_t regConvertData = 0;
    regConvertData = ad7192ReadRegisterValue(AD7192_REG_DATA, registerSize[AD7192_REG_DATA]);
    return regConvertData;
}

float ad7192RawDataToVoltage(uint32_t rawData) {
    float mvoltage = 0;
    int PGASetting = 0;
    int PGAGain = 0;
    uint8_t regAddress = AD7192_REG_CONF;
    registerMap[regAddress] = ad7192ReadRegisterValue(regAddress, registerSize[regAddress]);
    PGASetting = registerMap[regAddress] & 0x000007;  // keep only the PGA setting bits
    switch (PGASetting) { // Valid PGASetting are 0, 3, 4, 5, 6, 7
        case 0:
            PGAGain = 1; // G2 G1 GO: 000
            break;
        case 3:
            PGAGain = 8; // G2 G1 GO: 011
            break;
        case 4:
            PGAGain = 16; // G2 G1 GO: 100
            break;
        case 5:
            PGAGain = 32; // G2 G1 GO: 101
            break;
        case 6:
            PGAGain = 64; // G2 G1 GO: 110
            break;
        case 7:
            PGAGain = 128; // G2 G1 GO: 111
            break;
        default:
            Log.error("ERROR - Invalid Gain Setting. Valid Gain settings are 0, 3, 4, 5, 6, 7");
            return -1;
    }
    mvoltage = (((float)rawData / (float)8388608) - (float)1) * (V_REF / (float)PGAGain);
    return mvoltage;
}


uint32_t ad7192ReadADCChannelData(uint8_t channel)  {
    uint32_t rawConvertData = 0;
    ad7192SetChannel(channel);                  // set one channel to be selected
    ad7192StartSingleConversion();              // write command to initial conversion mode
    delay(100);                                 // TODO: hardcoded wait time for data to be ready
    rawConvertData = ad7192ReadConvertingData();// read raw convertible data from ADC
    return rawConvertData;
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
    return readRegisterValue;
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
