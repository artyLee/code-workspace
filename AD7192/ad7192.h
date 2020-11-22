#pragma once
#include "application.h"

void ad7192Init(void);
void ad7192SoftwareReset(void);
void ad7192SetPGAGain(uint8_t gain);
void ad7192SetChannel(uint8_t channel);
void ad7192SetFilterSelectBit(uint16_t filterRate);
void ad7192SetPsuedoDifferentialInputs(void);
void ad7192AppendStatusValuetoData(void);
void ad7192InternalZeroScaleCalibration(void);
void ad7192InternalFullScaleCalibration(void);
void ad7192InternalZeroFullScaleCalibration(void);
void ad7192StartSingleConversion(uint8_t Channel);
void ad7192StartContinuousConversion(uint8_t Channel);
void ad7192WriteRegisterValue(uint8_t registerAddress, uint32_t registerValue, uint8_t bytesSize);
uint32_t ad7192ReadRegisterValue(uint8_t registerAddress, uint8_t bytesSize);
uint32_t ad7192ReadConvertingData(void);
uint32_t ad7192ReadADCChannelData(uint8_t channel);
uint8_t ad7192ReadIDInfor(void);
void ad7192ReadRegisterMapValue(void);
float ad7192RawDataToVoltage(uint32_t rawData);
float ad7192TempSensorDataToDegC(uint32_t rawData);
