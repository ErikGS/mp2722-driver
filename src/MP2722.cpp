#include "MP2722.h"

#include <cstdarg>
#include <cstdio>
#include <algorithm>

MP2722::MP2722(const MP2722_I2C &i2c, uint8_t address)
    : _i2c(i2c), _address(address)
{
    if (!_i2c.write || !_i2c.read)
    {
        // Try to get a preset I2C if not provided or invalid
        const MP2722_I2C *platform_i2c = mp2722_get_platform_i2c();
        if (platform_i2c)
            _i2c = *platform_i2c;
    }
}

void MP2722::setLogCallback(MP2722_LogLevel level, MP2722_LogCallback callback)
{
    if (_logLevel == MP2722_LogLevel::NONE)
    {
        _logCallback = nullptr;
        return;
    }

    _logLevel = level;

    if (!_logCallback)
    {
        // Try to get a preset logger if not provided
        const MP2722_LogCallback platform_log = mp2722_get_platform_log();
        if (platform_log)
            _logCallback = platform_log;
        return;
    }

    _logCallback = callback;
}

void MP2722::log(MP2722_LogLevel level, const char *fmt, ...)
{
    if (!_logCallback || level > _logLevel || level == MP2722_LogLevel::NONE)
        return;

    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    _logCallback(level, buf);
}

MP2722_Result MP2722::writeReg(uint8_t reg, uint8_t val)
{
    if (!_i2c.write)
        return MP2722_Result::INVALID_STATE;

    int ret = _i2c.write(_address, reg, &val, 1);
    return (ret == 0) ? MP2722_Result::OK : MP2722_Result::FAIL;
}

MP2722_Result MP2722::readRegs(uint8_t start_reg, uint8_t *buf, size_t len)
{
    if (!_i2c.read)
        return MP2722_Result::INVALID_STATE;

    int ret = _i2c.read(_address, start_reg, buf, len);
    return (ret == 0) ? MP2722_Result::OK : MP2722_Result::FAIL;
}

MP2722_Result MP2722::readReg(uint8_t reg, uint8_t &val)
{
    return readRegs(reg, &val, 1);
}

MP2722_Result MP2722::updateReg(uint8_t reg, uint8_t mask, uint8_t val)
{
    uint8_t old_val;
    MP2722_Result ret = readReg(reg, old_val);
    if (ret != MP2722_Result::OK)
        return ret;

    uint8_t new_val = (old_val & ~mask) | (val & mask);
    if (new_val != old_val)
        return writeReg(reg, new_val);

    return MP2722_Result::OK;
}

MP2722_Result MP2722::init()
{
    // Check I2C is available before any hardware access
    if (!_i2c.write || !_i2c.read)
    {
        log(MP2722_LogLevel::ERROR, "No built-in platform preset nor custom interface was provided. "
                                    "If this is an unsupported platform, you need to provide your own "
                                    "I2C read/write function wrappers in the constructor, and if the platform"
                                    "uses I2C handle, set it up with mp2722_platform_set_i2c_handle(). "
                                    "See documentation and examples for details.");
        return MP2722_Result::FAIL;
    }

    // Probe registers to verify connection
    uint8_t val;
    MP2722_Result ret = readReg(MP2722_REG_CONFIG0, val);
    if (ret != MP2722_Result::OK)
    {
        log(MP2722_LogLevel::ERROR, "Failed to communicate with MP2722");
        return ret;
    }

    _initialized = true;

    // If these bits are not 000, PMIC will set a fixed limit and ignore values defined from setInputCurrentLimit()
    // or from input source detection. So we ensure it is set to 000 by default.
    // CONFIG1 bits [7:5] - set IIN_MODE to 000 (Follow IIN_LIM)
    uint8_t mode = 0 << MP2722_IIN_MODE_SHIFT;
    ret = updateReg(MP2722_REG_CONFIG1, MP2722_IIN_MODE_MASK, mode);
    if (ret != MP2722_Result::OK)
    {
        _initialized = false;
        log(MP2722_LogLevel::ERROR, "Failed to set IIN_MODE to Follow IIN_LIM");
        return ret;
    }
    // SAFETY CRITICAL:
    // Driver initial state DISABLES Charging by default, as charge parameters must be explicitly adjusted to any
    // specific battery first. Higher current and voltage limits than what the battery can handle will likely
    // damage it, possibly leading to fires or explosions.
    //
    // Also, depending on application or if the battery is removable, it might happen that the system is powered
    // via VBUS with no battery connected, so by not starting charging by default, we are ensuring that the power
    // path control logic has to be explicitly handled according to the specific needs of the application.
    ret = setCharging(false);
    if (ret != MP2722_Result::OK)
    {
        _initialized = false;
        log(MP2722_LogLevel::ERROR, "Failed to disable charging");
        return ret;
    }

    ret = setAutoDpDmDetection(true);
    if (ret != MP2722_Result::OK)
    {
        _initialized = false;
        log(MP2722_LogLevel::ERROR, "Failed to enable Auto D+/D- Detection");
        return ret;
    }

    ret = setBuck(true);
    if (ret != MP2722_Result::OK)
    {
        _initialized = false;
        log(MP2722_LogLevel::ERROR, "Failed to enable Buck Converter");
        return ret;
    }

    ret = setAutoOTG(true);
    if (ret != MP2722_Result::OK)
    {
        _initialized = false;
        log(MP2722_LogLevel::ERROR, "Failed to enable Auto OTG");
        return ret;
    }

    ret = setBoostStopOnBattLow(true);
    if (ret != MP2722_Result::OK)
    {
        _initialized = false;
        log(MP2722_LogLevel::ERROR, "Failed to enable Boost Stop on Battery Low");
        return ret;
    }

    log(MP2722_LogLevel::INFO, "MP2722 Initialized. CONFIG0=0x%02X", val);
    return MP2722_Result::OK;
}

MP2722_Result MP2722::reset()
{
    return updateReg(MP2722_REG_CONFIG0, MP2722_REG_RST_MASK, MP2722_REG_RST_MASK);
}

MP2722_Result MP2722::setChargeCurrent(uint16_t current_ma)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    if (current_ma < 80)
        current_ma = 80;
    if (current_ma > 5000)
        current_ma = 5000;

    uint8_t steps = current_ma / MP2722_ICC_STEP;
    if (steps > 0x3F)
        steps = 0x3F;

    MP2722_Result ret = updateReg(MP2722_REG_CONFIG2, MP2722_ICC_MASK, steps);
    if (ret != MP2722_Result::OK)
    {
        _isChargeCurrentSet = false;
        return ret;
    }

    _isChargeCurrentSet = true;
    log(MP2722_LogLevel::DEBUG, "Set Charge Current: %dmA (0x%02X)", current_ma, steps);
    return ret;
}

MP2722_Result MP2722::setChargeVoltage(uint16_t voltage_mv)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    if (voltage_mv < 3600)
        voltage_mv = 3600;
    if (voltage_mv > 4600)
        voltage_mv = 4600;

    uint8_t steps = (voltage_mv - MP2722_VBATT_REG_BASE) / MP2722_VBATT_REG_STEP;
    if (steps > 0x3F)
        steps = 0x3F;

    uint8_t reg_val = steps << MP2722_VBATT_REG_SHIFT;

    MP2722_Result ret = updateReg(MP2722_REG_CONFIG5, MP2722_VBATT_REG_MASK, reg_val);
    if (ret != MP2722_Result::OK)
    {
        _isChargeVoltageSet = false;
        return ret;
    }

    _isChargeVoltageSet = true;
    log(MP2722_LogLevel::DEBUG, "Set Charge Voltage: %dmV (0x%02X)", voltage_mv, steps);
    return ret;
}

MP2722_Result MP2722::setInputCurrentLimit(uint16_t current_ma)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    if (current_ma < 100)
        current_ma = 100;
    if (current_ma > 3200)
        current_ma = 3200;

    uint8_t steps = (current_ma - MP2722_IIN_LIM_BASE) / MP2722_IIN_LIM_STEP;
    if (steps > 0x1F)
        steps = 0x1F;

    log(MP2722_LogLevel::DEBUG, "Set Input Limit: %dmA (0x%02X)", current_ma, steps);
    return updateReg(MP2722_REG_CONFIG1, MP2722_IIN_LIM_MASK, steps);
}

MP2722_Result MP2722::forceDpDmDetection()
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    return updateReg(MP2722_REG_CONFIGA, MP2722_FORCEDPDM_MASK, MP2722_FORCEDPDM_MASK);
}

MP2722_Result MP2722::setAutoDpDmDetection(bool enable)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    uint8_t val = enable ? MP2722_AUTODPDM_MASK : 0;
    return updateReg(MP2722_REG_CONFIGA, MP2722_AUTODPDM_MASK, val);
}

MP2722_Result MP2722::setCharging(bool enable)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    if (enable && (!_isChargeVoltageSet || !_isChargeCurrentSet))
    {
        log(MP2722_LogLevel::ERROR, "Charge FAULT: Voltage and Current must be adjusted first!");
        return MP2722_Result::INVALID_STATE;
    }

    uint8_t val = enable ? MP2722_EN_CHG_MASK : 0;
    return updateReg(MP2722_REG_CONFIG9, MP2722_EN_CHG_MASK, val);
}

MP2722_Result MP2722::setBuck(bool enable)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    uint8_t val = enable ? MP2722_EN_BUCK_MASK : 0;
    return updateReg(MP2722_REG_CONFIG9, MP2722_EN_BUCK_MASK, val);
}

MP2722_Result MP2722::setBoost(bool enable)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    uint8_t val = enable ? MP2722_EN_BOOST_MASK : 0;
    return updateReg(MP2722_REG_CONFIG9, MP2722_EN_BOOST_MASK, val);
}

MP2722_Result MP2722::setBoostStopOnBattLow(bool enable)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    uint8_t val = enable ? MP2722_BOOST_STP_EN_MASK : 0;
    return updateReg(MP2722_REG_CONFIGC, MP2722_BOOST_STP_EN_MASK, val);
}

MP2722_Result MP2722::setAutoOTG(bool enable)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    uint8_t val = enable ? MP2722_AUTOOTG_MASK : 0;
    return updateReg(MP2722_REG_CONFIG9, MP2722_AUTOOTG_MASK, val);
}

MP2722_Result MP2722::setStatAsAnalogIB(bool enable, bool charging_only)
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    uint8_t val = enable ? MP2722_EN_STAT_IB_MASK : 0;
    MP2722_Result ret = updateReg(MP2722_REG_CONFIG0, MP2722_EN_STAT_IB_MASK, val);
    if (ret != MP2722_Result::OK)
        return ret;

    val = charging_only ? 0 : MP2722_IB_EN_MASK;
    return updateReg(MP2722_REG_CONFIG7, MP2722_IB_EN_MASK, val);
}

MP2722_Result MP2722::enterShippingMode()
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    log(MP2722_LogLevel::WARN, "Entering Shipping Mode (BATFET Off)");
    return updateReg(MP2722_REG_CONFIG8, MP2722_BATTFET_DIS_MASK, MP2722_BATTFET_DIS_MASK);
}

MP2722_Result MP2722::watchdogKick()
{
    if (!_initialized)
    {
        log(MP2722_LogLevel::ERROR, "init() must be called first");
        return MP2722_Result::INVALID_STATE;
    }

    return updateReg(MP2722_REG_CONFIG7, MP2722_WATCHDOG_RST_MASK, MP2722_WATCHDOG_RST_MASK);
}

MP2722_Result MP2722::getStatus(PowerStatus &status)
{
    uint8_t buf[6];
    uint8_t start_reg = MP2722_REG_STATUS11;

    MP2722_Result ret = readRegs(start_reg, buf, 6);
    if (ret != MP2722_Result::OK)
        return ret;

    uint8_t reg11 = buf[0]; // DPDM / DPM
    uint8_t reg12 = buf[1]; // Power / Therm / Watchdog
    uint8_t reg13 = buf[2]; // Charger / Boost
    uint8_t reg14 = buf[3]; // Physical Faults & NTC JEITA status
    uint8_t reg15 = buf[4]; // CC1/CC2 vRa/vRd detection status (USB Type-C)
    uint8_t reg16 = buf[5]; // Reserved for future status

    log(MP2722_LogLevel::INFO, "STATUS: R11=0x%02X R12=0x%02X R13=0x%02X R14=0x%02X R15=0x%02X R16=0x%02X",
        reg11, reg12, reg13, reg14, reg15, reg16);

    // --- Register 11 ---
    status.legacy_src_type = static_cast<LegacyInputSrcType>((reg11 & MP2722_DPDM_STAT_MASK) >> MP2722_DPDM_STAT_SHIFT);
    status.input_dpm_regulation = (reg11 & (MP2722_VINDPM_STAT_MASK | MP2722_IINDPM_STAT_MASK)) != 0;

    // --- Register 12 ---
    status.vin_good = (reg12 & MP2722_VIN_GD_MASK) != 0;
    status.vin_ready = (reg12 & MP2722_VIN_RDY_MASK) != 0;
    status.charger_ready = status.vin_good && status.vin_ready;
    status.vsys_regulation = (reg12 & MP2722_VSYS_STAT_MASK) != 0;
    status.thermal_regulation = (reg12 & MP2722_THERM_STAT_MASK) != 0;
    status.legacy_cable = (reg12 & MP2722_LEGACYCABLE_MASK) != 0;
    status.fault_watchdog = (reg12 & MP2722_WATCHDOG_FAULT_MASK) != 0;

    // --- Register 13 ---
    status.charger_status = static_cast<ChargerStatus>((reg13 & MP2722_CHG_STAT_MASK) >> MP2722_CHG_STAT_SHIFT);
    status.charger_fault = static_cast<ChargerFault>(reg13 & MP2722_CHG_FAULT_MASK);
    status.boost_fault = static_cast<BoostFault>((reg13 & MP2722_BOOST_FAULT_MASK) >> MP2722_BOOST_FAULT_SHIFT);

    // --- Register 14 ---
    status.fault_battery = (reg14 & MP2722_BATT_MISSING_MASK) != 0;
    status.fault_ntc = (reg14 & MP2722_NTC_MISSING_MASK) != 0;
    status.ntc1_state = static_cast<NTCState>((reg14 & MP2722_NTC1_FAULT_MASK) >> MP2722_NTC1_FAULT_SHIFT);
    status.ntc2_state = static_cast<NTCState>((reg14 & MP2722_NTC2_FAULT_MASK) >> MP2722_NTC2_FAULT_SHIFT);

    // --- Register 15 ---
    status.cc1_snk_stat = static_cast<CCSinkStatus>((reg15 & MP2722_CC1_SNK_STAT_MASK) >> MP2722_CC1_SNK_STAT_SHIFT);
    status.cc2_snk_stat = static_cast<CCSinkStatus>((reg15 & MP2722_CC2_SNK_STAT_MASK) >> MP2722_CC2_SNK_STAT_SHIFT);
    status.cc1_src_stat = static_cast<CCSourceStatus>((reg15 & MP2722_CC1_SRC_STAT_MASK) >> MP2722_CC1_SRC_STAT_SHIFT);
    status.cc2_src_stat = static_cast<CCSourceStatus>((reg15 & MP2722_CC2_SRC_STAT_MASK) >> MP2722_CC2_SRC_STAT_SHIFT);

    // --- Register 16 ---
    status.topoff_active = (reg16 & MP2722_TOPOFF_ACTIVE_MASK) != 0;
    status.bfet_stat = (reg16 & MP2722_BFET_STAT_MASK) != 0;
    status.batt_low_stat = (reg16 & MP2722_BATT_LOW_STAT_MASK) != 0;
    status.otg_need = (reg16 & MP2722_OTG_NEED_MASK) != 0;
    status.vin_test_high = (reg16 & MP2722_VIN_TEST_HIGH_MASK) != 0;
    status.debug_acc = (reg16 & MP2722_DEBUGACC_MASK) != 0;
    status.audio_acc = (reg16 & MP2722_AUDIOACC_MASK) != 0;

    return MP2722_Result::OK;
}
