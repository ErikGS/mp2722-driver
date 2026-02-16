#include "MP2722.h"
#include "MP2722_platform.h"
#include <cstdio>

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

// Set the STM32 HAL handles before creating the driver
// Because this platform uses handles instead of direct function pointers for I2C operations,
// we have to pass the initialized handles to the driver before init().
mp2722_platform_set_i2c_handle(&hi2c1);
mp2722_platform_set_uart_handle(&huart2); // For STM32 UART logging

// Declare the driver instance globally so it can be used in main() and other functions if needed
MP2722 pmic;

int main(void)
{
    // Standard STM32 setup: initialize HAL, clocks, and peripherals before everything else.
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    // Enable driver debug logging
    pmic.setLogCallback(MP2722_LogLevel::DEBUG);

    // Initialize the driver and check the return status
    if (pmic.init() != MP2722_Result::OK)
    {
        char msg[] = "[E] MP2722: PMIC init failed!\r\n";
        HAL_UART_Transmit(&huart2, (uint8_t *)msg, sizeof(msg) - 1, HAL_MAX_DELAY);
        Error_Handler(); // halt
    }

    // Configure for a typical 1S Li-Po (4.2V, 1A charge)
    pmic.setChargeVoltage(4200); // mV
    pmic.setChargeCurrent(1000); // mA

    // This limits current passing through USB, so it must be >= charge current. Recommended is
    // higher in order to allow for some headroom, as the device itself also consumes current.
    pmic.setInputCurrentLimit(1500); // mA

    // Start charging (make sure to have set voltage and current first, or it will return an error)
    pmic.setCharging(true);

    while (1)
    {
        // PowerStatus struct needed to hold the data read from the PMIC
        MP2722::PowerStatus status;

        // Note that because debug logging is enabled, the driver will also be printing RAW status data on getStatus().
        if (pmic.getStatus(status) == MP2722_Result::OK)
        {
            // For fully named and decoded fields instead of RAW bits, the PowerStatus struct declared above is used.
            // See MP2722::PowerStatus struct for details on the various fields you can read.
            // For example, to check if the battery is hot, you can check the 'NTCState::HOT' like:
            bool is_hot = status.ntc1_state == MP2722::NTCState::HOT;

            char buf[64];
            snprintf(buf, sizeof(buf), "[I] MP2722: Battery is %s\r\n", is_hot ? "HOT" : "NOT HOT");
            HAL_UART_Transmit(&huart2, (uint8_t *)buf, strlen(buf), HAL_MAX_DELAY);

            // Or to check if charging is done:
            if (status.charger_status == MP2722::ChargerStatus::CHARGE_DONE)
            {
                char done_msg[] = "[I] MP2722: Battery fully charged\r\n";
                HAL_UART_Transmit(&huart2, (uint8_t *)done_msg, sizeof(done_msg) - 1, HAL_MAX_DELAY);
            }
        }

        // Kick the watchdog to prevent it from resetting the device (if enabled, which it is by default)
        pmic.watchdogKick(); // Watchdog is a heartbeat to let the PMIC know the system is still alive.

        // Wait 1s before the next status check
        HAL_Delay(1000);
    }
}
