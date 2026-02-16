#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_log.h"

#include "MP2722.h"

// Mock handle for demonstration. Replace with actual handle from your I2C initialization
static i2c_master_dev_handle_t dev_handle;

/// --- Custom I2C ---

int espidf_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    uint8_t buf[len + 1];
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    return i2c_master_transmit(dev_handle, buf, len + 1, -1) == ESP_OK ? 0 : -1;
}

int espidf_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, -1) == ESP_OK ? 0 : -1;
}

/// --- Custom logger ---

// Following MP2722_LogCallback signature
void espidf_log(MP2722_LogLevel level, const char *msg)
{
    switch (level)
    {
    case MP2722_LogLevel::ERROR:
        ESP_LOGE("MP2722", "%s", msg);
        break;
    case MP2722_LogLevel::WARN:
        ESP_LOGW("MP2722", "%s", msg);
        break;
    case MP2722_LogLevel::INFO:
        ESP_LOGI("MP2722", "%s", msg);
        break;
    case MP2722_LogLevel::DEBUG:
        ESP_LOGD("MP2722", "%s", msg);
        break;
    default:
        break;
    }
}

// --- Driver ---

// Pass the custom I2C to the driver constructor
// It can now be created globally because we already gave the custom I2C functions access to the handles
// But you can still use a pointer if you need or prefer
MP2722 pmic({espidf_i2c_write, espidf_i2c_read});

// Pass the custom logger to the driver log callback
pmic.setLogCallback(MP2722_LogLevel::DEBUG, {espidf_log});

/// --- Actual driver usage is unchanged so the rest is the same as standard examples ---

// pmic.init(); etc...
