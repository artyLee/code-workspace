#include "ad7192.h"
#include "ad7192_defs.h"

#define CS_PIN                  D5 // Chip select pin
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

