#include "MP2722.h"
#include "MP2722_platform.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "main";

// Declare the driver instance globally so it can be used in app_main() and other functions if needed
MP2722 pmic;

extern "C" void app_main(void)
{
    // Standard ESP-IDF setup: initialize I2C bus and device before everything else.
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x3F,
        .scl_speed_hz = 400000,
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // Because this platform uses handles instead of direct function pointers for I2C operations,
    // we have to pass the initialized I2C device handle to the driver before init().
    mp2722_platform_set_i2c_handle(dev_handle);

    // Enable driver debug logging
    pmic.setLogCallback(MP2722_LogLevel::DEBUG);

    // Initialize the driver and check the return status
    if (pmic.init() != MP2722_Result::OK)
    {
        ESP_LOGE(TAG, "PMIC init failed!");
        return; // halt
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

            ESP_LOGI(TAG, "Battery is %s", is_hot ? "HOT" : "NOT HOT");

            // Or to check if charging is done:
            if (status.charger_status == MP2722::ChargerStatus::CHARGE_DONE)
            {
                ESP_LOGI(TAG, "Battery fully charged");
            }
        }

        // Kick the watchdog to prevent it from resetting the device (if enabled, which it is by default)
        pmic.watchdogKick(); // Watchdog is a heartbeat to let the PMIC know the system is still alive.

        // Wait 1s before the next status check
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}