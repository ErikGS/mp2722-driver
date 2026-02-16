#include <Wire.h>
#include "MP2722.h"

// Declare the driver instance globally so it can be used in both setup() and loop()
MP2722 pmic;

void setup()
{
    // Standard Arduino setup: initialize Serial and Wire before everything else.
    Serial.begin(115200);
    Wire.begin();

    // Enable driver logging with a specific DEBUG log level (optional, default is INFO)
    pmic.setLogCallback(MP2722_LogLevel::DEBUG);

    // Initialize the driver and check the return status
    if (pmic.init() != MP2722_Result::OK)
    {
        Serial.println("[E] MP2722: PMIC init failed!");
        while (1)
            ; // halt
    }

    // Configure for a typical 1S Li-Po (4.2V, 1A charge)
    pmic.setChargeVoltage(4200); // mV
    pmic.setChargeCurrent(1000); // mA

    // This limits currrent passing through USB, so it must be >= charge current. Recommended is
    // higher in order to allow for some headroom, as the device itself also consumes current.
    pmic.setInputCurrentLimit(1500); // mA

    // Start charging (make sure to have set voltage and current first, or it will return an error)
    pmic.setCharging(true);
}

void loop()
{
    // PowerStatus struct needed to hold the data read from the PMIC
    MP2722::PowerStatus status;

    // Note that because debug logging is enabled, the driver will also be printing RAW status data on getStatus().
    if (pmic.getStatus(status) == MP2722_Result::OK)
    {
        // For fully named and decoded fields instead of RAW bits, the PowerStatus struct declared above is used.
        // See MP2722::PowerStatus struct for details on the various fields you can read.
        // For example, to check if the battery is hot, you can check the 'NTCState::HOT' like:
        bool is_hot = status.ntc1_state == MP2722::NTCState::HOT;

        Serial.println("[I] MP2722: Battery is " + String(is_hot ? "HOT" : "NOT HOT"));

        // Or to check if charging is done:
        if (status.charger_status == MP2722::ChargerStatus::CHARGE_DONE)
        {
            Serial.println("[I] MP2722: Battery fully charged");
        }
    }

    // Kick the watchdog to prevent it from resetting the device (if enabled, which it is by default)
    pmic.watchdogKick(); // Watchdog is a heartbeat to let the PMIC know the system is still alive.

    // Wait 1s before the next status check
    delay(1000);
}
