#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Library error codes (platform-agnostic)
 */
enum class MP2722_Result : int8_t
{
    OK = 0,
    FAIL = -1,
    INVALID_ARG = -2,
    INVALID_STATE = -3,
    TIMEOUT = -4,
    NOT_FOUND = -5,
};

/**
 * @brief Log severity levels
 */
enum class MP2722_LogLevel : uint8_t
{
    NONE = 0,
    ERROR,
    WARN,
    INFO,
    DEBUG,
};

/**
 * @brief User-provided logging callback signature
 *
 * @param level   Severity level
 * @param message Null-terminated log string
 */
typedef void (*MP2722_LogCallback)(MP2722_LogLevel level, const char *message);

/**
 * @brief User-provided I2C read/write interface
 *
 * Users implement these two functions for their platform (Arduino Wire, ESP-IDF, STM32 HAL, etc.)
 */
struct MP2722_I2C
{
    /**
     * @brief Write bytes to a register
     * @param address  7-bit I2C device address
     * @param reg      Register address
     * @param data     Pointer to data to write
     * @param len      Number of bytes to write
     * @return 0 on success, non-zero on failure
     */
    int (*write)(uint8_t address, uint8_t reg, const uint8_t *data, size_t len);

    /**
     * @brief Read bytes from a register
     * @param address  7-bit I2C device address
     * @param reg      Start register address
     * @param data     Buffer to read into
     * @param len      Number of bytes to read
     * @return 0 on success, non-zero on failure
     */
    int (*read)(uint8_t address, uint8_t reg, uint8_t *data, size_t len);
};

// ============================================================================
// Status / Fault enums
// ============================================================================

enum class LegacyInputSrcType : uint8_t
{
    UNDEFINED = 0b0000,    // 500mA
    UNKNOWN = 0b1000,      // 500mA
    USB_SDP = 0b0001,      // 500mA
    USB_DCP = 0b0010,      // 2A
    USB_CDP = 0b0011,      // 1.5A
    DIVIDER_1 = 0b0100,    // 1A
    DIVIDER_2 = 0b0101,    // 2.1A
    DIVIDER_3 = 0b0110,    // 2.4A
    DIVIDER_4 = 0b0111,    // 2A
    DIVIDER_5 = 0b1110,    // 3A
    HIGH_VOLTAGE = 0b1001, // 2A
};

enum class ChargerStatus : uint8_t
{
    NOT_CHARGING = 0b000,   // Can be either due to charge terminated, or faults
    TRICKLE_CHARGE = 0b001, // First charging phase when battery voltage is very low. Charger applies a small current (typically around 10% of the fast charge current) to safely raise the battery voltage.
    PRE_CHARGE = 0b010,     // Second charging phase. Charger applies a medium current (typically around 50% of the fast charge current) to further raise the battery voltage until it reaches the fast charge threshold.
    FAST_CHARGE = 0b011,    // Third charging phase. Charger applies the full configured fast charge current until the battery voltage reaches the constant voltage threshold.
    CONST_VOLTAGE = 0b100,  // Last charging phase. Charger holds the battery at the constant voltage threshold and gradually reduces current as the battery approaches full charge. Charging is considered done when current drops below the termination threshold.
    CHARGE_DONE = 0b101,    // Charging is done. Charger has terminated charging because the battery is full, but a valid input source is still present. Charger will resume charging if battery voltage drops below the recharge threshold.
};

enum class ChargerFault : uint8_t
{
    NONE = 0b00,           // Normal operation, no fault
    INPUT_OVERVOLT = 0b01, // Latch-off (needs reset). Input Overvoltage-protection triggered: Input voltage is above the OVP threshold. This can be caused by a faulty charger or adapter, or a transient voltage spike.
    TIMEOUT = 0b10,        // Latch-off (needs reset). Timeout-protection triggered: The charging timer has expired.
    BATT_OVERVOLT = 0b11,  // Latch-off (needs reset). Battery Overvoltage-protection triggered: Battery voltage is above the OVP threshold.
};

enum class BoostFault : uint8_t
{
    NONE = 0b000,     // Normal operation, no fault
    OVERLOAD = 0b001, // Latch-off (needs reset). Overload-protection triggered: Too much current was drawn (possibly a short?).
    OVERVOLT = 0b010, // Not latch. Overvoltage-protection triggered: It will auto-recover (possibly a inductive kickback or similar transient condition? Which is nothing unusual). If however this just keeps happening, it then probably indicates an issue with the load, battery or the system. Further electrical debugging is then advised.
    OVERTEMP = 0b011, // Latch-off (needs reset). IC Overtemperature-protection triggered: Not unusual at higher ambient temperatures or similar transient condition. If however this happens consistently, it's often due to insufficient cooling or poor thermal design, either for the amount of current needed from the system, or the environment it's operating in.
    BATT_LOW = 0b100, // Latch-off (needs reset). Overdischarge-protection triggered: Battery level is below boost cutoff threshold.
};

enum class NTCState : uint8_t
{
    NORMAL = 0b000, // JEITA
    WARM = 0b001,   // JEITA
    COOL = 0b010,   // JEITA
    COLD = 0b011,   // JEITA
    HOT = 0b100,    // JEITA
};

enum class CCSinkStatus : uint8_t
{
    vRa = 0b00,      // CC detects vRa
    vRd_USB = 0b01,  // CC detects vRd-USB
    vRd_1_5A = 0b10, // CC detects vRd-1.5
    vRd_3_0A = 0b11, // CC detects vRd-3.0
};

enum class CCSourceStatus : uint8_t
{
    vOPEM = 0b00, // CC is vOPEN
    vRd = 0b01,   // CC detects vRd
    vRa = 0b10,   // CC detects vRa
};

// ============================================================================
// Power status struct
// ============================================================================

struct PowerStatus
{
    // Power Status
    LegacyInputSrcType legacy_src_type; // Input source D+/D- detection (DPDM_STAT: USB SDP, DCP, CDP, etc.)
    bool legacy_cable;                  // Legacy cable is detected (not valid in DRP mode)
    bool vin_good;                      // Input source valid (VIN_GD)
    bool vin_ready;                     // Input source detection complete (VIN_RDY)
    bool charger_ready;                 // Input source valid and ready (VIN_GD & VIN_RDY)
    bool vsys_regulation;               // System is in VSYS_MIN regulation (VSYS_STAT: 0: VBATT < VSYS_MIN 1: VBATT > VSYS_MIN)
    bool thermal_regulation;            // IC Die is hot (T_REG loop active) and throttling
    bool input_dpm_regulation;          // IC is throttling due to weak input source (VINDPM or IINDPM)
    bool fault_watchdog;                // Watchdog timer expired
    ChargerStatus charger_status;       // CHG_STAT (000: Not charging 001: Trickle charge 010: Pre-charge 011: Fast charge 100: Constant-voltage charge 101: Charging is done)
    ChargerFault charger_fault;         // CHG_FAULT (0=Normal, 1=Input OVP, 2=Timer expired, 3=Batt OVP)
    BoostFault boost_fault;             // BOOST_FAULT (000: Normal 001: An IN overload or short (latch-off) has occurred 010: Boost over-voltage protection (OVP) (not latch) has occurred 011: Boost over-temperature protection (latch-off) has occurred 100: The boost has stopped due to BATT_LOW (latch-off))

    // Battery/NTC Status (Reg 0x14)
    bool fault_battery;  // Possible physical connection fault at battery input
    bool fault_ntc;      // Possible Physical connection fault at NTC thermistor input(s)
    NTCState ntc1_state; // JEITA Zone 0=Normal, 1=Warm, 2=Cool, 3=Cold, 4=Hot. Derived from NTC1, or weighted of NTC1 + (NTC2 if used) (Ref. Datasheet Table 6, pg.21. Rev. 1.0)
    NTCState ntc2_state; // JEITA Zone 0=Normal, 1=Warm, 2=Cool, 3=Cold, 4=Hot. Derived from NTC2, or weighted of (NTC1 if used) + NTC2 (Ref. Datasheet Table 6, pg.21. Rev. 1.0)

    // USB Type-C CC Detection (Reg 0x15)
    CCSinkStatus cc1_snk_stat; // 00: vRa, 01: vRd-USB, 10: vRd-1.5, 11: vRd-3.0
    CCSinkStatus cc2_snk_stat;
    CCSourceStatus cc1_src_stat; // 00: vOPEN, 01: vRd, 10: vRa
    CCSourceStatus cc2_src_stat;

    // Misc Status (Reg 0x16)
    bool topoff_active; // Top-off timer counting
    bool bfet_stat;     // 0: Charging/Disabled, 1: Discharging (Battery powering system)
    bool batt_low_stat; // 0: VBATT > BATT_LOW, 1: VBATT < BATT_LOW
    bool otg_need;      // 0: Boost disabled, 1: Boost enabled (OTG needed)
    bool vin_test_high; // VIN test threshold status
    bool debug_acc;     // Debug Accessory Detected
    bool audio_acc;     // Audio Accessory Detected
};
