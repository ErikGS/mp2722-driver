// ESP-IDF user code
#include "MP2722.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static i2c_master_dev_handle_t dev_handle;

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

MP2722_I2C i2c = {espidf_i2c_write, espidf_i2c_read};
MP2722 pmic(i2c);