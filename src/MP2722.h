#pragma once

#include <stdint.h>
#include <stddef.h>
#include <cstdio>

#include "MP2722_defs.h"
#include "MP2722_regs.h"
#include "MP2722_platform.h"

/**
 * @brief Driver for MPS MP2722 Battery Charger
 */
class MP2722
{
public:
    /**
     * @brief Construct a new MP2722 object
     *
     * @param i2c      Platform-specific I2C interface (user-provided read/write functions)
     * @param address  I2C address (default 0x3F)
     */
    MP2722(const MP2722_I2C &i2c = {}, uint8_t address = MP2722_I2C_ADDRESS);

    ~MP2722() = default;

    /**
     * @brief Set a logging callback. Pass nullptr to disable logging.
     *
     * @param callback  Function pointer matching MP2722_LogCallback signature
     * @param level     Maximum log level to emit (default: INFO)
     */
    void setLogCallback(MP2722_LogCallback callback = {}, MP2722_LogLevel level = MP2722_LogLevel::INFO);

    /**
     * @brief Initialize the driver and check device presence
     */
    MP2722_Result init();

    /**
     * @brief Reset all registers to default
     */
    MP2722_Result reset();

    /**
     * @brief Imediately perform D+/D- detection for USB input source type detection.
     *
     * D+/D- detection includes the USB Battery Charging Specification 1.2 (BC1.2), nonstandard adapter applications,
     * and adjustable high-voltage adapter handshake. BC1.2 detection begins with data contact detection (DCD).
     * If DCD detection is successful, the standard downstream port (SDP), dedicated charging port (DCP), and charging
     * downstream port (CDP) are distinguished by primary and secondary detection. If the DCD timer expires, then
     * non-standard adapter detection is initiated.
     *
     * @note - D+/D- detection is performed automatically by default, this can be changed via `setAutoDpDmDetection()`.
     */
    MP2722_Result forceDpDmDetection();

    /**
     * @brief Enable or Disable automatic D+/D- detection for USB input source type detection, which can still have its
     * state overridden via `forceDpDmDetection()`.
     *
     * D+/D- detection includes the USB Battery Charging Specification 1.2 (BC1.2), nonstandard adapter applications,
     * and adjustable high-voltage adapter handshake. BC1.2 detection begins with data contact detection (DCD).
     * If DCD detection is successful, the standard downstream port (SDP), dedicated charging port (DCP), and charging
     * downstream port (CDP) are distinguished by primary and secondary detection. If the DCD timer expires, then
     * non-standard adapter detection is initiated.
     *
     * @note - Enabled by default.
     * @note - Disable if you want to have full control over the feature, then use `forceDpDmDetection()` as needed.
     */
    MP2722_Result setAutoDpDmDetection(bool enable);

    /**
     * @brief Set Charge Current (ICC)
     *
     * @param current_ma Charge current in mA (Range: 80-5000mA, Step: 80mA approx)
     */
    MP2722_Result setChargeCurrent(uint16_t current_ma);

    /**
     * @brief Set Charge Voltage (VBATT_REG)
     *
     * @param voltage_mv Charge voltage in mV (Range: 3600-4600mV)
     */
    MP2722_Result setChargeVoltage(uint16_t voltage_mv);

    /**
     * @brief Set Input Current Limit (IIN_LIM)
     *
     * @note - IIN_LIM is automatically updated after input source type detection.
     * This is used to overwrite the IIN_LIM value.
     *
     * @param current_ma Input current limit in mA (Range: 100-3200mA)
     */
    MP2722_Result setInputCurrentLimit(uint16_t current_ma);

    /**
     * @brief Enable or Disable Charging
     *
     * @warning - Requires having charge parameters explicitly set before enabling.
     */
    MP2722_Result setCharging(bool enable);

    /**
     * @brief Enable or Disable the Buck Converter (Switching Regulator)
     *
     * @warning - Must be ON for the system to receive power from USB efficiently.
     * @warning - Enabled by default. Refer to MP2722 datasheet before deciding if you need to disable.
     */
    MP2722_Result setBuck(bool enable);

    /**
     * @brief Enable or Disable outputing power on the USB port.
     *
     * Force enabling or disabling the OTG Boost Converter (Switching Regulator for OTG mode) regardless of automatic control.
     *
     * @note - By default this is automatically controlled via USB detection. See `setAutoOTG()` for details.
     */
    MP2722_Result setBoost(bool enable);

    /**
     * @brief Enable or Disable Boost Stop on Battery Low.
     *
     * @param enable true: The BATT_LOW comparator turns off boost operation and latches it off.
     *             - false: The BATT_LOW comparator only generates an interrupt (INT).
     */
    MP2722_Result setBoostStopOnBattLow(bool enable);

    /**
     * @brief Enable or Disable automatically acting as a USB power source via USB detection.
     *
     * Toggle automatic OTG Boost Converter (Switching Regulator for OTG mode), which can still have its
     * state overridden via `setBoost()`.
     *
     * @note - Enabled by default.
     * @note - Disable if you want to have full control over the feature, then use `setBoost()` as needed.
     */
    MP2722_Result setAutoOTG(bool enable);

    /**
     * @brief Configure the STAT/IB pin function.
     *
     * @param enable If true, pin outputs Analog Current (IB) for ADC read. If false, outputs Digital Hi/Lo for LED (STAT).
     * @param charging_only If true, IB pin only sources voltage Charging. If false (default), it always sources voltage (Charging/Discharging).
     */
    MP2722_Result setStatAsAnalogIB(bool enable, bool charging_only = false);

    /**
     * @brief Read all PMIC status registers.
     *
     * @param status Reference to PowerStatus struct to fill with current PMIC status
     */
    MP2722_Result getStatus(PowerStatus &status);

    /**
     * @brief Kick PMIC Watchdog to prevent it from resetting registers to default.
     *
     * @note - MP2722 watchdog period defaults to 40s. Kicking it every few seconds is fine.
     */
    MP2722_Result watchdogKick();

    /**
     * @brief Enter shipping mode by setting BATTFET_DIS (Bit 5 in CONFIG8), effectively disconnecting the battery.
     *
     * @note - System will be completely powered off.
     * @note - Can only wake up by plugging in USB or pulling RST pin to logic low (RESET button) for 1.1 sec.
     */
    MP2722_Result enterShippingMode();

private:
    MP2722_I2C _i2c = {};
    uint8_t _address = MP2722_I2C_ADDRESS;

    MP2722_LogCallback _logCallback = {};
    MP2722_LogLevel _logLevel = MP2722_LogLevel::INFO;

    bool isChargeCurrentSet = false;
    bool isChargeVoltageSet = false;

    /**
     * @brief Helper to check the if safety-critical charge parameters have been set (charge current and voltage)
     *
     * @return true if both charge current and voltage have been set, false otherwise
     */
    bool getIsSafeToCharge() const { return isChargeCurrentSet && isChargeVoltageSet; }

    MP2722_Result writeReg(uint8_t reg, uint8_t val);
    MP2722_Result readRegs(uint8_t start_reg, uint8_t *buf, size_t len);
    MP2722_Result readReg(uint8_t reg, uint8_t &val);
    MP2722_Result updateReg(uint8_t reg, uint8_t mask, uint8_t val);

    void log(MP2722_LogLevel level, const char *fmt, ...);
};
