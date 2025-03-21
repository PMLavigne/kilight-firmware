/**
 * TMP1826Driver.cpp
 *
 * @author Patrick Lavigne
 */

#include "kilight/hw/TMP1826Driver.h"

#include "kilight/util/MathUtil.h"

using kilight::util::MathUtil;

namespace kilight::hw {
    TMP1826Driver::TMP1826Driver(DS2485Driver* const busDriver) :
        OneWireDevice(busDriver) {
    }

    int16_t TMP1826Driver::currentTemperature() const {
        return static_cast<int16_t>(static_cast<float>(m_scratchpad.temperature) * 100 / 128);
    }

    bool TMP1826Driver::configurationNeedsSetting() const {
        return m_scratchpad.configurationRegister != ConfigurationToSet;
    }

    bool TMP1826Driver::configurationIsValid() const {
        return m_scratchpad.configurationRegister == ConfigurationToSet &&
               m_scratchpad.shortAddress == 0 &&
               m_scratchpad.temperatureAlertLow == 0 &&
               m_scratchpad.temperatureAlertHigh == 0 &&
               m_scratchpad.temperatureOffset == 0;
    }

    bool TMP1826Driver::startReadScratchpadCommand() const {
        return driver()->startOneWireWriteBlockData(read_scratchpad_command_t{address()});
    }

    bool TMP1826Driver::completeReadScratchpadCommand() const {
        return driver()->completeOneWireWriteBlock();
    }

    bool TMP1826Driver::startReadScratchpad() const {
        return driver()->startOneWireReadBlock(sizeof(scratchpad_t));
    }

    bool TMP1826Driver::completeReadScratchpad() {
        bool const result = driver()->completeOneWireReadBlock(&m_scratchpad);
        if (result) {
            static_assert(sizeof(scratchpad_t) >= 18, "scratchpad_t is the wrong size");
            std::array<std::byte, sizeof(scratchpad_t)> buff {};
            memcpy(buff.begin(), &m_scratchpad, buff.size());
            std::span<std::byte const> const buffSpan {buff};
            uint8_t const firstCRC = MathUtil::crc8(buffSpan.subspan(0, 8));
            uint8_t const lastCRC = MathUtil::crc8(buffSpan.subspan(9, 8));

            TRACE("Calculated First CRC: {} / Provided CRC: {}", firstCRC, m_scratchpad.firstCRC);
            TRACE("Calculated Last CRC: {} / Provided CRC: {}", lastCRC, m_scratchpad.lastCRC);

            if (firstCRC != m_scratchpad.firstCRC || lastCRC != m_scratchpad.lastCRC) {
                DEBUG("Temperature CRC mismatch, retrying");
                return false;
            }

            TRACE("Temperature: {:.2f} C", static_cast<float>(m_scratchpad.temperature) / 128);

            TRACE("Scratchpad Config - conversionMode: {} / averageEightConversions: {} / "
                  "alertPinMode: {} / useLongConversionTime: {} / useSixteenBitTemperatureFormat: {}",
                  static_cast<uint8_t>(m_scratchpad.configurationRegister.conversionMode),
                  static_cast<bool>(m_scratchpad.configurationRegister.averageEightConversions),
                  static_cast<bool>(m_scratchpad.configurationRegister.alertPinMode),
                  static_cast<bool>(m_scratchpad.configurationRegister.useLongConversionTime),
                  static_cast<bool>(m_scratchpad.configurationRegister.useSixteenBitTemperatureFormat));

            TRACE("Scratchpad Config - registerProtectionLockEnabled: {} / alertHysteresis: {} / "
                  "arbitrationMode: {} / flexAddressMode: {} / overdriveEnabled: {}",
                  static_cast<bool>(m_scratchpad.configurationRegister.registerProtectionLockEnabled),
                  static_cast<uint8_t>(m_scratchpad.configurationRegister.alertHysteresis),
                  static_cast<uint8_t>(m_scratchpad.configurationRegister.arbitrationMode),
                  static_cast<uint8_t>(m_scratchpad.configurationRegister.flexAddressMode),
                  static_cast<bool>(m_scratchpad.configurationRegister.overdriveEnabled));

            TRACE("Scratchpad Config - ShortAddr: {} / tempLow: {} / tempHigh {} / tempOffset {}",
                  static_cast<uint8_t>(m_scratchpad.shortAddress),
                  static_cast<int16_t>(m_scratchpad.temperatureAlertLow),
                  static_cast<int16_t>(m_scratchpad.temperatureAlertHigh),
                  static_cast<int16_t>(m_scratchpad.temperatureOffset));

            onTemperatureReady(currentTemperature());
        }
        return result;
    }

    bool TMP1826Driver::startWriteScratchpadCommand() const {
        constexpr scratchpad_write_t scratchpadToWrite {
                .configurationRegister = ConfigurationToSet,
                .shortAddress = 0,
                .temperatureAlertLow = 0,
                .temperatureAlertHigh = 0,
                .temperatureOffset = 0
            };
        return driver()->startOneWireWriteBlockData(write_scratchpad_command_t{address(), scratchpadToWrite});
    }

    bool TMP1826Driver::completeWriteScratchpadCommand() const {
        return driver()->completeOneWireWriteBlock();
    }

    bool TMP1826Driver::startCopyScratchpadCommand() const {
        return driver()->startOneWireWriteBlockData(copy_scratchpad_command_t{address()});
    }

    bool TMP1826Driver::completeCopyScratchpadCommand() const {
        return driver()->completeOneWireWriteBlock();
    }
}
