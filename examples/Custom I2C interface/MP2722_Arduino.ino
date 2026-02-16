// Arduino user code
#include <Wire.h>
#include "MP2722.h"

int arduino_i2c_write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(data, len);
    return Wire.endTransmission() == 0 ? 0 : -1;
}

int arduino_i2c_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
        return -1;
    if (Wire.requestFrom(addr, (uint8_t)len) != len)
        return -1;
    for (size_t i = 0; i < len; i++)
        data[i] = Wire.read();
    return 0;
}

void arduino_log(MP2722_LogLevel level, const char *msg)
{
    Serial.print("[MP2722] ");
    Serial.println(msg);
}

MP2722_I2C i2c = {arduino_i2c_write, arduino_i2c_read};
MP2722 pmic(i2c);

void setup()
{
    Wire.begin();
    pmic.setLogCallback(arduino_log, MP2722_LogLevel::DEBUG);
    pmic.init();
}