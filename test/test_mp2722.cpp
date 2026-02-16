#include <catch2/catch_test_macros.hpp>
#include "MP2722.h"
#include <cstring>
#include <vector>

// Mock register file
static uint8_t mock_regs[256] = {};
static std::vector<std::pair<uint8_t, uint8_t>> write_log;

int mock_write(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        mock_regs[reg + i] = data[i];
        write_log.push_back({(uint8_t)(reg + i), data[i]});
    }
    return 0;
}

int mock_read(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
        data[i] = mock_regs[reg + i];
    return 0;
}

static MP2722_I2C mock_i2c = {mock_write, mock_read};

TEST_CASE("Init succeeds with valid I2C")
{
    memset(mock_regs, 0, sizeof(mock_regs));
    write_log.clear();

    MP2722 pmic(mock_i2c);
    REQUIRE(pmic.init() == MP2722_Result::OK);
}

TEST_CASE("Charging requires voltage and current to be set first")
{
    memset(mock_regs, 0, sizeof(mock_regs));
    write_log.clear();

    MP2722 pmic(mock_i2c);
    pmic.init();

    REQUIRE(pmic.setCharging(true) == MP2722_Result::INVALID_STATE);

    pmic.setChargeVoltage(4200);
    REQUIRE(pmic.setCharging(true) == MP2722_Result::INVALID_STATE);

    pmic.setChargeCurrent(1000);
    REQUIRE(pmic.setCharging(true) == MP2722_Result::OK);
}

TEST_CASE("Charge current is clamped")
{
    memset(mock_regs, 0, sizeof(mock_regs));
    MP2722 pmic(mock_i2c);
    pmic.init();

    // Below minimum
    REQUIRE(pmic.setChargeCurrent(10) == MP2722_Result::OK);
    // Above maximum
    REQUIRE(pmic.setChargeCurrent(9999) == MP2722_Result::OK);
}
