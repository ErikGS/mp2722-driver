# MP2722 Driver Library

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Platform-agnostic I2C driver for the **MPS MP2722** USB Type-C battery charger/power management IC.

## Features

- **Portable** — built-in I2C and logging for Arduino, ESP-IDF, STM32 HAL and Linux. Or you can simply provide your own I2C wrapper and log callback on any platform.
- **Native Linux support** — out-of-the-box compatibility with any Linux SBC, such as Raspberry Pi, BeagleBone and even Desktop, using the standard Linux I2C interface (`/dev/i2c-X`).
- **Optional logging** — Built-in logging with configurable log levels (DEBUG, INFO, WARN, ERROR or NONE) with support for custom log callbacks.
- **Full status/fault readout** — USB Type-C CC detection, legacy/non-standard USB detection, JEITA NTC, charger state, fault state, etc. Refer to [register map](docs/register_map.md) for complete list of available status and fault registers.

## Quick Start

### Supported platforms and setup requirements

| Platform          | I2C             | Logging             | Setup required                                           |
| ----------------- | --------------- | ------------------- | -------------------------------------------------------- |
| **Arduino**       | Wire            | Serial              | `Serial.begin()` + `Wire.begin()`                        |
| **ESP-IDF**       | `i2c_master`    | `ESP_LOGx`          | `mp2722_platform_set_i2c_handle()`                       |
| **STM32 HAL**     | `HAL_I2C_Mem_*` | `HAL_UART_Transmit` | `mp2722_platform_set_i2c_handle()` + `set_uart_handle()` |
| **Linux**         | `/dev/i2c-1`    | `stderr`            | `mp2722_platform_set_i2c_bus("BUS_ADDRESS")`             |
| **Windows/macOS** | —               | `stderr`            | Provide your own wrappers                                |
| **Other**         | —               | —                   | Provide your own wrappers                                |

### Using built-in I2C supported platforms (Arduino, ESP-IDF, STM32CubeMX, Linux host)

For platforms that don't use I2C/UART handle or bus address (e.g., Arduino Wire):

```cpp
#include "MP2722.h"

MP2722 pmic; // Declare the driver instance globally on supported platform

void pmic_init() // Call this after usual platform setup (Serial, Analog/Digital I/O, I2C, etc.)
{
   // (Optional) Enable built-in driver logging on supported platforms
   pmic.setLogCallback(MP2722_LogLevel::DEBUG);

   // All functions returns MP2722_Result::OK on success or other MP2722_Result on failure
   pmic.init(); // In a real application you should only proceed if this returns OK

   // Standard 3.7-4.2V 2100mAh Li-ion/Li-Po battery in ~0.5C charge rate.
   pmic.setChargeVoltage(4200); // mV = 4.2V @ CV (Constant Voltage) Phase - Basic config
   pmic.setChargeCurrent(1000); // mA = 1A @ CC (Constant Current) Phase - Basic config
   pmic.setCharging(true); // Enable charger (off by default as basic config is required)
}
```

For platforms that do use I2C/UART handle or bus address (e.g., ESP-IDF I2C Master, STM32 HAL, Linux host):

```cpp
#include "MP2722.h"

MP2722 *pmic = nullptr; // Declare a pointer for the driver instance globally

void pmic_init() // Call after usual platform setup (Serial, Analog/Digital I/O, I2C, etc.)
{
   // Set the appropriate handles or bus address for your platform
   mp2722_platform_set_i2c_handle(i2c_device_handle); // Only if your platform uses I2C handle
   mp2722_platform_set_uart_handle(uart_device_handle); // Only if your platform uses UART handle
   mp2722_platform_set_i2c_bus("/dev/i2c-1"); // Only if you're on Linux, replace with your correct bus address

   // Now that the platform I2C/UART handle/bus is set, create the driver instance
   pmic = new MP2722();

   // (Optional) Enable built-in driver logging on supported platforms
   pmic->setLogCallback(MP2722_LogLevel::DEBUG);

   // Actual driver usage remains the same, besides now dealing with a pointer (-> instead of .)
   pmic->init(); // In a real application you should only proceed if this returns OK
   pmic->setChargeVoltage(4200); // mV = 4.2V @ CV (Constant Voltage) Phase - Basic config
   pmic->setChargeCurrent(1000); // mA = 1A @ CC (Constant Current) Phase - Basic config
   pmic->setCharging(true); // Enable charger (off by default as basic config is required)
}
```

### Basic Configuration notes

Trickle charge, pre-charge, etc. Are all handled by the IC, so you don't need to worry about them. Just set the fast charge current and the IC will automatically step through the appropriate charging phases based on battery voltage and other conditions. You can read the charger status (pmic.getChargerStatus()) to see which phase it's in. Refer to the [MP2722 datasheet](https://www.monolithicpower.com/en/mp2722.html) for more details on the charging algorithm and how the IC manages the different phases.

OTG boost mode (port can power devices) is enabled by default based on USB detection, automatically deactivating on low battery. But you can control both things manually if needed (see the [API Reference](docs/api_reference.md) for details).

### Watchdog Timer (heartbeat)

The MP2722 has a built-in watchdog timer that is enabled by default and has an expiration time of 40 seconds, requiring periodic "kicks" to prevent it from resetting the device. It is recommended `watchdogKick()` method should be called at least once every 30 seconds or less to keep the watchdog from expiring. You can call this in your main loop or set up a timer to call it at regular intervals. You can also disable the watchdog if you don't need it, but it's generally recommended to keep it enabled for safety in case of software crashes or unresponsive states.

```cpp
// From your main application loop at a set interval of 30 seconds or less
pmic.watchdogKick(); // Reset the watchdog timer to prevent PMIC reset. You can also disable this.
```

### Reading data

To read charger status, faults, etc. you can call `getStatus()` at any time:

```cpp
// Declare a PowerStatus struct to hold the readout
PowerStatus status{};

// Call from your main app loop at some interval to monitor status/faults, etc.
pmic.getStatus(status); // or pmic->getStatus(status) if using pointer instance

// Do whatever with the status (log, update a LED or display, etc.)
switch (status.charger_status)
{
   case ChargerStatus::NOT_CHARGING:
      // Handle not charging state
      break;
   case ChargerStatus::CHARGE_DONE:
      // Handle charge done state
      break;
   default:
      // Handle charging state (pre-charge, fast charge, etc.)
      break;
}
```

### Usage on other platforms or if you want your own custom I2C and loggging wrappers

```cpp
#include "MP2722.h"

// 1. Declare the driver instance globally providing custom wrappers for your platform's I2C read/write functions
MP2722 pmic({ your_i2c_write_function, your_i2c_read_function });

// 2. (Optional) Enable driver logging providing a custom wrapper for your platform's logging function
pmic.setLogCallback(MP2722_LogLevel::DEBUG, your_log_function);

/// --- Actual driver usage remains the same ---
```

**Note:** the I2C function pointers should match `MP2722_I2C` and the log `MP2722_LogCallback` signatures. For the I2C read/write both functions should return `0` on success and non-zero on failure. See the [examples](examples) folder for complete implementation examples.

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps =
    https://github.com/erikgs/MP2722-driver.git#main
```

PlatformIO auto-detects the library in the `MP2722/` subfolder.

### Arduino

Clone into your Arduino libraries folder:

```bash
cd ~/Arduino/libraries
git clone https://github.com/erikgs/mp2722-driver.git
```

Or in the Arduino IDE: **Sketch → Include Library → Manage Libraries…** → search "MP2722".

### ESP-IDF Component

Move/copy or symlink `mp2722/` inside `components/` in your project, manually or using git submodule:

```bash
git submodule add https://github.com/erikgs/mp2722-driver.git components/driver_mp2722
```

### STM32 or Other (CubeIDE / CMake)

Move/copy or symlink `mp2722/` inside `libs/` in your project, manually or using git submodule:

```bash
git submodule add https://github.com/erikgs/mp2722-driver.git libs/driver_mp2722
```

- Add `MP2722.cpp` and `MP2722_platform.cpp` to your source list
- Add the directory containing these files (`libs/driver_mp2722/src/`) to your compiler's include path

## API Reference

| Method                                           | Description                                                                                                                   |
| ------------------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------- |
| `setLogCallback(level, callback)`                | Set logging callback and max level (`nullptr` to disable)                                                                     |
| `init()`                                         | Probe device, apply safe defaults (charging off, IIN_MODE=follow-limit), enable buck, auto-OTG, auto-D+/D−, boost-stop-on-low |
| `reset()`                                        | Reset all registers to defaults                                                                                               |
| `setChargeVoltage(mv)`                           | Set battery regulation voltage (3600–4600 mV, step 25 mV). **Note:** Values rounded down to nearest step.                     |
| `setChargeCurrent(ma)`                           | Set fast-charge current (80–5000 mA, step 80 mA). **Note:** Values rounded down to nearest step.                              |
| `setInputCurrentLimit(ma)`                       | Override input current limit (100–3200 mA, step 100 mA). **Note:** Values rounded down to nearest step.                       |
| `setCharging(enable)`                            | Enable/disable charging (requires voltage & current set first)                                                                |
| `setBuck(enable)`                                | Enable/disable buck converter                                                                                                 |
| `setBoost(enable)`                               | Force enable/disable OTG boost                                                                                                |
| `setAutoOTG(enable)`                             | Enable/disable automatic OTG based on USB detection                                                                           |
| `setBoostStopOnBattLow(enable)`                  | Latch-off boost on low battery vs. interrupt only                                                                             |
| `forceDpDmDetection()`                           | Trigger immediate D+/D− source detection                                                                                      |
| `setAutoDpDmDetection(enable)`                   | Enable/disable automatic D+/D− detection                                                                                      |
| `setStatAsAnalogIB(enable, charging_only=false)` | Configure STAT pin as analog IB or digital LED                                                                                |
| `getStatus(status)`                              | Read all status/fault registers into `PowerStatus` struct                                                                     |
| `watchdogKick()`                                 | Reset the hardware watchdog timer                                                                                             |
| `enterShippingMode()`                            | Disconnect battery (deep power off)                                                                                           |

## PowerStatus Structure

The `getStatus(PowerStatus &status)` method populates the following fields:

| Field                           | Type                                                                                                                                        | Description                                                                 |
| ------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------- |
| `legacy_src_type`               | [LegacyInputSrcType](https://github.com/ErikGS/mp2722-driver/blob/dd3834e20c1b6c372e6c757a85054296a46d38d6/src/MP2722_defs.h#L71C12-L71C30) | Detected input source (USB SDP, DCP, CDP, etc.)                             |
| `vin_good`                      | `bool`                                                                                                                                      | Input source is valid                                                       |
| `charger_status`                | [ChargerStatus](https://github.com/ErikGS/mp2722-driver/blob/dd3834e20c1b6c372e6c757a85054296a46d38d6/src/MP2722_defs.h#L86)                | Charging state (Not Charging, Pre-charge, Fast Charge, Done, etc.)          |
| `charger_fault`                 | [ChargerFault](https://github.com/ErikGS/mp2722-driver/blob/dd3834e20c1b6c372e6c757a85054296a46d38d6/src/MP2722_defs.h#L96)                 | Fault type (None, Input OVP, Thermal Shutdown, Timer Expired, Battery OVP)  |
| `boost_fault`                   | [BoostFault](https://github.com/ErikGS/mp2722-driver/blob/dd3834e20c1b6c372e6c757a85054296a46d38d6/src/MP2722_defs.h#L104)                  | Boost mode fault (Overload, OVP, OTP, Battery Low)                          |
| `fault_battery`                 | `bool`                                                                                                                                      | Battery missing or connection fault                                         |
| `fault_ntc`                     | `bool`                                                                                                                                      | NTC thermistor missing or fault                                             |
| `ntc1_state`                    | [NTCState](https://github.com/ErikGS/mp2722-driver/blob/dd3834e20c1b6c372e6c757a85054296a46d38d6/src/MP2722_defs.h#L113)                    | JEITA state for NTC1 (Normal, Warm, Cool, Cold, Hot)                        |
| `cc1_snk_stat` / `cc2_snk_stat` | [CCSinkStatus](https://github.com/ErikGS/mp2722-driver/blob/dd3834e20c1b6c372e6c757a85054296a46d38d6/src/MP2722_defs.h#L122)                | USB-C CC1/CC2 sink detection (vRa, vRd-USB, vRd-1.5A, vRd-3.0A)             |
| `cc1_src_stat` / `cc2_src_stat` | [CCSourceStatus](https://github.com/ErikGS/mp2722-driver/blob/dd3834e20c1b6c372e6c757a85054296a46d38d6/src/MP2722_defs.h#L130C12-L130C26)   | USB-C CC1/CC2 source detection (vOpen, vRd, vRa)                            |
| `batt_low_stat`                 | `bool`                                                                                                                                      | Battery voltage is below low threshold                                      |
| `thermal_regulation`            | `bool`                                                                                                                                      | IC is throttling charge current due to heat                                 |
| `input_dpm_regulation`          | `bool`                                                                                                                                      | IC is throttling charge current due to input voltage/current limit (VINDPM) |

## Error Handling

All methods return `MP2722_Result`:

| Code            | Meaning                                                              |
| --------------- | -------------------------------------------------------------------- |
| `OK`            | Success                                                              |
| `FAIL`          | I2C communication error                                              |
| `INVALID_ARG`   | Parameter out of range                                               |
| `INVALID_STATE` | Operation not allowed (e.g., charging before configuring parameters) |
| `TIMEOUT`       | I2C timeout                                                          |
| `NOT_FOUND`     | Device not responding                                                |

## License

   [Apache-2.0 license](LICENSE) Copyright © 2026 Erik Gimenes dos Santos
