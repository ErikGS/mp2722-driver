#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "stm32f4xx_hal.h"

#include "MP2722.h"

// Declare a global pointer for the driver instance to be initialized after setting I2C handle
MP2722 *pmic = nullptr;

static I2C_HandleTypeDef hi2c1;
static UART_HandleTypeDef huart2;

// --------------------------------------------------------------------------
// CI stub peripheral-init functions
// In a real CubeMX project these are auto-generated in main.c.
// Here they are minimal stubs so the example compiles stand-alone.
static void SystemClock_Config(void) { /* Configure clocks for your board */ }
static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
}
static void MX_I2C1_Init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}
static void MX_USART2_UART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}
// --------------------------------------------------------------------------

void app_log(const char *msg)
{
    char formatted_msg[128];
    snprintf(formatted_msg, sizeof(formatted_msg), "[App] %s\r\n", msg);
    HAL_UART_Transmit(&huart2, (uint8_t *)formatted_msg, strlen(formatted_msg), HAL_MAX_DELAY);
}

int main(void)
{
    /*
     * --- Usual platform setup (Serial, Analog/Digital I/O, I2C, etc.) ---
     */

    // Standard STM32 setup: initialize HAL, clocks, and peripherals before everything else.
    HAL_Init();
    // SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART2_UART_Init();

    /*
     * --- Driver setup ---
     */

    // 1. Pass the STM32 HAL handles to the platform layer
    // This MUST be done before pmic.init() is called
    mp2722_platform_set_i2c_handle(&hi2c1);
    mp2722_platform_set_uart_handle(&huart2);

    // Now that the platform I2C/UART handle/bus is set, create the driver instance
    pmic = new MP2722();

    // 2. Enable driver logging using the built-in platform logger (wraps HAL_UART_Transmit)
    pmic->setLogCallback(MP2722_LogLevel::DEBUG);

    /*
     * --- Driver usage ---
     */

    // All functions returns MP2722_Result::OK on success and other MP2722_Result on failure
    pmic->init();                 // In a real application you should only proceed if this returns OK
    pmic->setChargeVoltage(4200); // mV = 4.2V @ CV (Constant Voltage) Phase - Basic config
    pmic->setChargeCurrent(1000); // mA = 1A @ CC (Constant Current) Phase - Basic config
    pmic->setCharging(true);      // Enable charger (off by default as basic config is required)

    PowerStatus status{}; // Declare a PowerStatus struct to hold the readout
    while (1)
    {
        // Call from main app loop at some interval
        pmic->getStatus(status);

        // Do whatever with the status (log, update a LED or display, etc.)
        switch (status.charger_status)
        {
        case ChargerStatus::NOT_CHARGING:
            // Handle not charging state
            app_log("Charger Status: Not Charging.");
            break;
        case ChargerStatus::CHARGE_DONE:
            // Handle charge done state
            app_log("Charger Status: Charge Done!");
            break;
        default:
            // Handle charging state (pre-charge, fast charge, etc.)
            app_log("Charger Status: Charging...");
            break;
        }

        // Kick the watchdog to prevent it from resetting the device (if enabled, which it is by default)
        pmic->watchdogKick(); // Watchdog is a heartbeat to let the PMIC know the system is still alive

        // Delay for 1 second interval
        HAL_Delay(1000);
    }
}
