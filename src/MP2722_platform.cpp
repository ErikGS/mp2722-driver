#include "MP2722_platform.h"

// ============================================================================
// Arduino - I2C via Wire + Serial logging
// ============================================================================
#if defined(ARDUINO)

#include <Arduino.h>
#include <Wire.h>

static int arduino_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    for (size_t i = 0; i < len; i++)
        Wire.write(data[i]);
    return (Wire.endTransmission() == 0) ? 0 : -1;
}

static int arduino_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
        return -1;
    if (Wire.requestFrom(addr, (uint8_t)len) != (uint8_t)len)
        return -1;
    for (size_t i = 0; i < len; i++)
        data[i] = Wire.read();
    return 0;
}

static void arduino_log(MP2722_LogLevel level, const char *msg)
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
    Serial.print(prefix);
    Serial.print("MP2722: ");
    Serial.println(msg);
}

static const MP2722_I2C _platform_i2c = {arduino_i2c_write, arduino_i2c_read};

const MP2722_I2C *mp2722_get_platform_i2c()
{
    return &_platform_i2c;
}

MP2722_LogCallback mp2722_get_platform_log()
{
    return arduino_log;
}

// ============================================================================
// ESP-IDF native, not Arduino - I2C Master + ESP_LOG
// ============================================================================
#elif defined(ESP_PLATFORM)

#include "esp_log.h"
#include <string.h>

static const char *TAG = "MP2722";
static i2c_master_dev_handle_t _dev_handle = nullptr;

void mp2722_platform_set_i2c_handle(i2c_master_dev_handle_t handle)
{
    _dev_handle = handle;
}

static int espidf_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    if (!_dev_handle)
        return -1;
    uint8_t buf[len + 1];
    buf[0] = reg;
    memcpy(&buf[1], data, len);
    return (i2c_master_transmit(_dev_handle, buf, len + 1, -1) == ESP_OK) ? 0 : -1;
}

static int espidf_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (!_dev_handle)
        return -1;
    return (i2c_master_transmit_receive(_dev_handle, &reg, 1, data, len, -1) == ESP_OK) ? 0 : -1;
}

static void espidf_log(MP2722_LogLevel level, const char *msg)
{
    switch (level)
    {
    case MP2722_LogLevel::ERROR:
        ESP_LOGE(TAG, "%s", msg);
        break;
    case MP2722_LogLevel::WARN:
        ESP_LOGW(TAG, "%s", msg);
        break;
    case MP2722_LogLevel::INFO:
        ESP_LOGI(TAG, "%s", msg);
        break;
    case MP2722_LogLevel::DEBUG:
        ESP_LOGD(TAG, "%s", msg);
        break;
    default:
        break;
    }
}

static const MP2722_I2C _platform_i2c = {espidf_i2c_write, espidf_i2c_read};

const MP2722_I2C *mp2722_get_platform_i2c()
{
    return &_platform_i2c;
}

MP2722_LogCallback mp2722_get_platform_log()
{
    return espidf_log;
}

// ============================================================================
// STM32 HAL - I2C HAL + UART HAL logging
// Detect STM32 by checking for STM32 family defines set by the toolchain
// ============================================================================
#elif defined(HAL_I2C_MODULE_ENABLED) ||                                            \
    defined(STM32F0) || defined(STM32F1) || defined(STM32F2) || defined(STM32F3) || \
    defined(STM32F4) || defined(STM32F7) || defined(STM32G0) || defined(STM32G4) || \
    defined(STM32H7) || defined(STM32L0) || defined(STM32L1) || defined(STM32L4) || \
    defined(STM32L5) || defined(STM32U5) || defined(STM32WB) || defined(STM32WL) || \
    defined(STM32F401xC) || defined(STM32F401xE) || defined(STM32F405xx) ||         \
    defined(STM32F407xx) || defined(STM32F411xE) || defined(STM32F446xx) ||         \
    defined(STM32F103xB) || defined(STM32F103x8)

#if __has_include("main.h")
#include "main.h" // CubeMX projects usually include the correct stm32xxxx_hal.h here
#elif __has_include("stm32f4xx_hal.h")
#include "stm32f4xx_hal.h"
#elif __has_include("stm32f1xx_hal.h")
#include "stm32f1xx_hal.h"
#elif __has_include("stm32f0xx_hal.h")
#include "stm32f0xx_hal.h"
#elif __has_include("stm32g0xx_hal.h")
#include "stm32g0xx_hal.h"
#elif __has_include("stm32g4xx_hal.h")
#include "stm32g4xx_hal.h"
#elif __has_include("stm32h7xx_hal.h")
#include "stm32h7xx_hal.h"
#elif __has_include("stm32l0xx_hal.h")
#include "stm32l0xx_hal.h"
#elif __has_include("stm32l4xx_hal.h")
#include "stm32l4xx_hal.h"
#else
#error "STM32 HAL header not found. Define MP2722_STM32_HAL_HEADER, e.g. -DMP2722_STM32_HAL_HEADER=\"stm32f4xx_hal.h\""
#endif

#include <stdio.h>
#include <string.h>

static I2C_HandleTypeDef *_hi2c = nullptr;
static UART_HandleTypeDef *_huart = nullptr;

void mp2722_platform_set_i2c_handle(I2C_HandleTypeDef *handle)
{
    _hi2c = handle;
}

void mp2722_platform_set_uart_handle(UART_HandleTypeDef *handle)
{
    _huart = handle;
}

static int stm32_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    if (!_hi2c)
        return -1;
    uint16_t dev_addr = (uint16_t)addr << 1;
    return (HAL_I2C_Mem_Write(_hi2c, dev_addr, reg, I2C_MEMADD_SIZE_8BIT,
                              (uint8_t *)data, len, HAL_MAX_DELAY) == HAL_OK)
               ? 0
               : -1;
}

static int stm32_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (!_hi2c)
        return -1;
    uint16_t dev_addr = (uint16_t)addr << 1;
    return (HAL_I2C_Mem_Read(_hi2c, dev_addr, reg, I2C_MEMADD_SIZE_8BIT,
                             data, len, HAL_MAX_DELAY) == HAL_OK)
               ? 0
               : -1;
}

static void stm32_log(MP2722_LogLevel level, const char *msg)
{
    if (!_huart)
        return;

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
    HAL_UART_Transmit(_huart, (uint8_t *)buf, n, HAL_MAX_DELAY);
}

static const MP2722_I2C _platform_i2c = {stm32_i2c_write, stm32_i2c_read};

const MP2722_I2C *mp2722_get_platform_i2c()
{
    return &_platform_i2c;
}

MP2722_LogCallback mp2722_get_platform_log()
{
    return (_huart) ? stm32_log : nullptr;
}

// ============================================================================
// Linux Hosted Environment - I2C via /dev/i2c-X + stderr logging
// ============================================================================
#elif defined(__linux__)

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

static int _i2c_fd = -1;

void mp2722_platform_set_i2c_bus(const char *device)
{
    if (_i2c_fd >= 0)
        close(_i2c_fd);
    _i2c_fd = open(device, O_RDWR);
}

void mp2722_platform_set_i2c_fd(int fd)
{
    _i2c_fd = fd;
}

static int linux_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    if (_i2c_fd < 0)
        return -1;
    if (ioctl(_i2c_fd, I2C_SLAVE, addr) < 0)
        return -1;

    uint8_t buf[len + 1];
    buf[0] = reg;
    for (size_t i = 0; i < len; i++)
        buf[i + 1] = data[i];

    return (write(_i2c_fd, buf, len + 1) == (ssize_t)(len + 1)) ? 0 : -1;
}

static int linux_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    if (_i2c_fd < 0)
        return -1;
    if (ioctl(_i2c_fd, I2C_SLAVE, addr) < 0)
        return -1;
    if (write(_i2c_fd, &reg, 1) != 1)
        return -1;
    return (read(_i2c_fd, data, len) == (ssize_t)len) ? 0 : -1;
}

static const MP2722_I2C _platform_i2c = {linux_i2c_write, linux_i2c_read};

static void stderr_log(MP2722_LogLevel level, const char *msg)
{
    const char *prefix;
    switch (level)
    {
    case MP2722_LogLevel::ERROR:
        prefix = "[E]";
        break;
    case MP2722_LogLevel::WARN:
        prefix = "[W]";
        break;
    case MP2722_LogLevel::INFO:
        prefix = "[I]";
        break;
    case MP2722_LogLevel::DEBUG:
        prefix = "[D]";
        break;
    default:
        prefix = "";
        break;
    }
    fprintf(stderr, "%s MP2722: %s\n", prefix, msg);
}

const MP2722_I2C *mp2722_get_platform_i2c()
{
    return (_i2c_fd >= 0) ? &_platform_i2c : nullptr;
}

MP2722_LogCallback mp2722_get_platform_log()
{
    return stderr_log;
}

// ============================================================================
// Other Hosted Environment (Windows, macOS) - no built-in I2C, stderr logging
// ============================================================================
#elif defined(_WIN32) || defined(__APPLE__)

#include <cstdio>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static void stderr_log(MP2722_LogLevel level, const char *msg)
{
    const char *prefix;
    switch (level)
    {
    case MP2722_LogLevel::ERROR:
        prefix = "[E]";
        break;
    case MP2722_LogLevel::WARN:
        prefix = "[W]";
        break;
    case MP2722_LogLevel::INFO:
        prefix = "[I]";
        break;
    case MP2722_LogLevel::DEBUG:
        prefix = "[D]";
        break;
    default:
        prefix = "";
        break;
    }
    fprintf(stderr, "%s MP2722: %s\n", prefix, msg);
}

const MP2722_I2C *mp2722_get_platform_i2c()
{
    return nullptr;
}

MP2722_LogCallback mp2722_get_platform_log()
{
    return stderr_log;
}

// ============================================================================
// Unknown / Custom platform - user must provide I2C and logging instead
// ============================================================================
#else

const MP2722_I2C *mp2722_get_platform_i2c()
{
    return nullptr;
}

MP2722_LogCallback mp2722_get_platform_log()
{
    return nullptr;
}

#endif