#pragma once

#include "MP2722_defs.h"

/**
 * @brief Get a pre-configured MP2722_I2C for the current platform.
 *
 * Automatically selects the correct implementation based on build environment.
 * Returns nullptr if no built-in implementation is available for the platform,
 * in which case the user must provide their own MP2722_I2C.
 *
 * @note Arduino: Uses Wire (must call Wire.begin() before use)
 * @note ESP-IDF: User must call mp2722_platform_set_i2c_handle() first
 * @note STM32 HAL: User must call mp2722_platform_set_i2c_handle() first
 */
const MP2722_I2C *mp2722_get_platform_i2c();

/**
 * @brief Get a pre-configured log callback for the current platform.
 *
 * Returns nullptr if no built-in implementation is available,
 * in which case the user can provide their own via setLogCallback().
 *
 * @note Arduino: Logs to Serial
 * @note ESP-IDF: Logs via ESP_LOGx
 * @note STM32 HAL: User must call mp2722_platform_set_uart_handle() first
 */
MP2722_LogCallback mp2722_get_platform_log();

#if defined(ESP_PLATFORM) && !defined(ARDUINO)
#include "driver/i2c_master.h"
/**
 * @brief Set the ESP-IDF I2C device handle
 * Must be called before mp2722_get_platform_i2c()
 */
void mp2722_platform_set_i2c_handle(i2c_master_dev_handle_t handle);

#elif defined(HAL_I2C_MODULE_ENABLED)

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

/**
 * @brief Set the STM32 HAL I2C handle
 * Must be called before mp2722_get_platform_i2c()
 */
void mp2722_platform_set_i2c_handle(I2C_HandleTypeDef *handle);

/**
 * @brief Set the STM32 HAL UART handle for logging (optional)
 */
void mp2722_platform_set_uart_handle(UART_HandleTypeDef *handle);
#elif defined(__linux__)
/**
 * @brief Set the Linux I2C bus device (e.g., "/dev/i2c-1")
 * Must be called before mp2722_get_platform_i2c()
 */
void mp2722_platform_set_i2c_bus(const char *device);

/**
 * @brief Set an already-opened Linux I2C file descriptor
 * Alternative to mp2722_platform_set_i2c_bus()
 */
void mp2722_platform_set_i2c_fd(int fd);
#endif