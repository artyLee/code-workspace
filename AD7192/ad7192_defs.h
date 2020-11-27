#pragma once

/* AD7192 Register Map */
#define AD7192_REG_COMM         0 // Communications Register (WO, 8-bit) 
#define AD7192_REG_STAT         0 // Status Register         (RO, 8-bit) 
#define AD7192_REG_MODE         1 // Mode Register           (RW, 24-bit 
#define AD7192_REG_CONF         2 // Configuration Register  (RW, 24-bit)
#define AD7192_REG_DATA         3 // Data Register           (RO, 24/32-bit) 
#define AD7192_REG_ID           4 // ID Register             (RO, 8-bit) 
#define AD7192_REG_GPOCON       5 // GPOCON Register         (RW, 8-bit) 
#define AD7192_REG_OFFSET       6 // Offset Register         (RW, 24-bit 
#define AD7192_REG_FULLSCALE    7 // Full-Scale Register     (RW, 24-bit)

/* Communications Register Bit Designations (AD7192_REG_COMM) */
#define AD7192_COMM_WEN         (1 << 7)           // Write Enable. 
#define AD7192_COMM_WRITE       (0 << 6)           // Write Operation.
#define AD7192_COMM_READ        (1 << 6)           // Read Operation. 
#define AD7192_COMM_ADDR(x)     (((x) & 0x7) << 3) // Register Address. 
#define AD7192_COMM_CREAD       (1 << 2)           // Continuous Read of Data Register.

/* Configuration Register Bit Designations (AD7192_REG_CONF) */
#define AD7192_CH_AIN1P_AIN2M   0 // AIN1(+) - AIN2(-)
#define AD7192_CH_AIN3P_AIN4M   1 // AIN3(+) - AIN4(-)
#define AD7192_CH_TEMP          2 // Temp Sensor
#define AD7192_CH_AIN2P_AIN2M   3 // AIN2(+) - AIN2(-)
#define AD7192_CH_AIN1          4 // AIN1 - AINCOM
#define AD7192_CH_AIN2          5 // AIN2 - AINCOM
#define AD7192_CH_AIN3          6 // AIN3 - AINCOM
#define AD7192_CH_AIN4          7 // AIN4 - AINCOM

