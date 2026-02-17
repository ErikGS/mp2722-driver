/**
 * @brief Minimal CI stub for STM32Cube build verification.
 *
 * A real STM32 project requires CubeMX-generated code (SystemClock_Config,
 * MX_*_Init, hi2c1, huart2, etc.) which cannot exist in a CI environment.
 * This stub only verifies that the driver library compiles against the
 * STM32 HAL headers.
 */
#include <string.h>
#include "stm32f4xx_hal.h"
#include "MP2722.h"

// Verify the driver types and platform API are visible
static MP2722 *pmic = nullptr;
static I2C_HandleTypeDef hi2c1;
static UART_HandleTypeDef huart2;

int main(void)
{
    HAL_Init();

    mp2722_platform_set_i2c_handle(&hi2c1);
    mp2722_platform_set_uart_handle(&huart2);

    pmic = new MP2722();
    pmic->setLogCallback(MP2722_LogLevel::DEBUG);
    pmic->init();
    pmic->setChargeVoltage(4200);
    pmic->setChargeCurrent(1000);
    pmic->setCharging(true);

    PowerStatus status{};
    pmic->getStatus(status);
    pmic->watchdogKick();

    delete pmic;
    while (1)
    {
    }
}