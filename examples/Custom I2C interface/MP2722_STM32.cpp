// STM32 HAL user code
#include "MP2722.h"
#include "stm32f446_hal.h" // adjust for your STM32 family (f1xx, l4xx, h7xx, etc.)
#include <cstdio>

extern I2C_HandleTypeDef hi2c1;   // defined by CubeMX in main.c
extern UART_HandleTypeDef huart2; // for logging via UART

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

// --- Usage in main.cpp ---

MP2722_I2C i2c = {stm32_i2c_write, stm32_i2c_read};
MP2722 pmic(i2c);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    pmic.setLogCallback(stm32_log, MP2722_LogLevel::DEBUG);

    if (pmic.init() != MP2722_Result::OK)
    {
        stm32_log(MP2722_LogLevel::ERROR, "PMIC init failed!");
        Error_Handler();
    }

    // Configure for a typical 1S Li-Po (4.2V, 1A charge)
    pmic.setChargeVoltage(4200);
    pmic.setChargeCurrent(1000);
    pmic.setInputCurrentLimit(1500);
    pmic.setCharging(true);

    while (1)
    {
        MP2722::PowerStatus status;
        if (pmic.getStatus(status) == MP2722_Result::OK)
        {
            if (status.charger_status == MP2722::ChargerStatus::CHARGE_DONE)
            {
                stm32_log(MP2722_LogLevel::INFO, "Battery fully charged");
            }
        }

        pmic.watchdogKick();
        HAL_Delay(1000);
    }
}