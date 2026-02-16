#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "stm32_hal_conf.h"

#include "MP2722.h"

extern I2C_HandleTypeDef hi2c1;   // defined by CubeMX in main.c
extern UART_HandleTypeDef huart2; // for logging via UART

/// --- Custom I2C ---

int stm32_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    // HAL expects 8-bit (left-shifted) address
    uint16_t dev_addr = (uint16_t)addr << 1;
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Write(
        &hi2c1, dev_addr, reg, I2C_MEMADD_SIZE_8BIT,
        (uint8_t *)data, len, HAL_MAX_DELAY);
    return (ret == HAL_OK) ? 0 : -1;
}

int stm32_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    uint16_t dev_addr = (uint16_t)addr << 1;
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Read(
        &hi2c1, dev_addr, reg, I2C_MEMADD_SIZE_8BIT,
        data, len, HAL_MAX_DELAY);
    return (ret == HAL_OK) ? 0 : -1;
}

/// --- Custom logger ---

void stm32_log(MP2722_LogLevel level, const char *msg)
{
    const char *prefix;
    switch (level)
    {
    case MP2722_LogLevel::ERROR:
        prefix = "[E] ";
        break;
    case MP2722_LogLevel::WARN:
        prefix = "[W] ";
        break;
    case MP2722_LogLevel::INFO:
        prefix = "[I] ";
        break;
    case MP2722_LogLevel::DEBUG:
        prefix = "[D] ";
        break;
    default:
        prefix = "";
        break;
    }

    char buf[160];
    int n = snprintf(buf, sizeof(buf), "%sMP2722: %s\r\n", prefix, msg);
    HAL_UART_Transmit(&huart2, (uint8_t *)buf, n, HAL_MAX_DELAY);
}

// --- Driver ---

// Pass the custom I2C to the driver constructor
// It can now be created globally because we already gave the custom I2C functions access to the handles
// But you can still use a pointer if you need or prefer
MP2722 pmic({stm32_i2c_write, stm32_i2c_read});

// Pass the custom logger to the driver log callback
pmic.setLogCallback(MP2722_LogLevel::DEBUG, {stm32_log});

/// --- Actual driver usage is unchanged so the rest is the same as standard examples ---

// pmic.init(); etc...
