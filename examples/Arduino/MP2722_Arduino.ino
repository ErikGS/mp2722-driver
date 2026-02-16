#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <Wire.h>

#include "MP2722.h"

// Declare the driver instance globally so it can be used in setup(), loop() and other functions if needed
// Note: For Arduino, we can create the instance directly as a global variable since we don't need dynamic allocation.
//      This is possible because I2C handle is not used on this platform.
MP2722 pmic;

// Declare a PowerStatus struct to hold the readout
PowerStatus status{};

void setup()
{
    /*
     * --- Usual platform setup (Serial, Analog/Digital I/O, I2C, etc.) ---
     */
    Serial.begin(115200);
    Wire.begin();

    /*
     * --- Driver setup ---
     */

    // Enable DEBUG driver logging for debugging
    pmic.setLogCallback(MP2722_LogLevel::DEBUG);

    /*
     * --- Driver usage ---
     */

    // All functions returns MP2722_Result::OK on success and other MP2722_Result on failure
    pmic.init();                 // In a real application you should only proceed if this returns OK
    pmic.setChargeVoltage(4200); // mV = 4.2V @ CV (Constant Voltage) Phase - Basic config
    pmic.setChargeCurrent(1000); // mA = 1A @ CC (Constant Current) Phase - Basic config
    pmic.setCharging(true);      // Enable charger (off by default as basic config is required)
}

void loop()
{
    // Call from main app loop at some interval to continuously monitor status/faults, etc.
    // Returns MP2722_Result::OK on success and other MP2722_Result on failure
    pmic.getStatus(status);

    // Do whatever with the status (log, update a LED or display, etc.)
    switch (status.charger_status)
    {
    case ChargerStatus::NOT_CHARGING:
        // Handle not charging state
        Serial.println("Charger Status: Not Charging.");
        break;
    case ChargerStatus::CHARGE_DONE:
        // Handle charge done state
        Serial.println("Charger Status: Charge Done!");
        break;
    default:
        // Handle charging state (pre-charge, fast charge, etc.)
        Serial.println("Charger Status: Charging...");
        break;
    }

    // Kick the watchdog to prevent it from resetting the device (if enabled, which it is by default)
    pmic.watchdogKick(); // Watchdog is a heartbeat to let the PMIC know the system is still alive.

    // delay for 1 second interval
    delay(1000);
}
