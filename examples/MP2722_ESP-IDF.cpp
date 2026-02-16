#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#include "MP2722.h"

static const char *TAG = "MAIN";

static constexpr gpio_num_t I2C_SCL_IO = GPIO_NUM_4;
static constexpr gpio_num_t I2C_SDA_IO = GPIO_NUM_5;
static constexpr i2c_port_t I2C_NUM = I2C_NUM_0;
static constexpr uint32_t I2C_FREQ_HZ = 400000;

MP2722 *pmic = nullptr; // Declare a global pointer for the driver instance to be initialized after setting I2C handle

extern "C" void app_main(void)
{
    /*
     * --- Usual platform setup (Serial, Analog/Digital I/O, I2C, etc.) ---
     */

    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {.enable_internal_pullup = true, .allow_pd = false},
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x63,
        .scl_speed_hz = I2C_FREQ_HZ,
        .scl_wait_us = 0,
        .flags = {.disable_ack_check = false},
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
    ESP_LOGI(TAG, "I2C initialized successfully");

    /*
     * --- Driver setup ---
     */

    // Call after usual platform setup (Serial, Analog/Digital I/O, I2C, etc.)
    mp2722_platform_set_i2c_handle(dev_handle); // Only if your platform uses I2C handle

    // Now that the platform I2C/UART handle/bus is set, create the driver instance
    pmic = new MP2722();

    // Enable DEBUG driver logging for debugging
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
        pmic->getStatus(status); // Call from main app loop at some interval to continuously monitor status/faults, etc.

        // Do whatever with the status (log, update a LED or display, etc.)
        switch (status.charger_status)
        {
        case ChargerStatus::NOT_CHARGING:
            // Handle not charging state
            ESP_LOGI(TAG, "Charger Status: Not Charging.");
            break;
        case ChargerStatus::CHARGE_DONE:
            // Handle charge done state
            ESP_LOGW(TAG, "Charger Status: Charge Done!");
            break;
        default:
            // Handle charging state (pre-charge, fast charge, etc.)
            ESP_LOGI(TAG, "Charger Status: Charging...");
            break;
        }

        // Kick the watchdog to prevent it from resetting the device (if enabled, which it is by default)
        pmic.watchdogKick(); // Watchdog is a heartbeat to let the PMIC know the system is still alive.

        // Delay for 1 second interval
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
