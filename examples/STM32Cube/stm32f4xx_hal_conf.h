#pragma once

/* -----------------------------------------------------------------------
 * Minimal HAL configuration for CI / example builds.
 * In a real CubeMX project this file is auto-generated.
 * ----------------------------------------------------------------------- */

#define HAL_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED

#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_cortex.h"
#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_pwr.h"

#define HSE_VALUE ((uint32_t)8000000)
#define HSI_VALUE ((uint32_t)16000000)
#define LSI_VALUE ((uint32_t)32000)
#define LSE_VALUE ((uint32_t)32768)
#define VDD_VALUE ((uint32_t)3300)
#define TICK_INT_PRIORITY ((uint32_t)0)
#define USE_RTOS 0
#define PREFETCH_ENABLE 1
#define INSTRUCTION_CACHE_ENABLE 1
#define DATA_CACHE_ENABLE 1

#define assert_param(expr) ((void)0)