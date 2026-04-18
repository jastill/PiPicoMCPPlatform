#include "adc_sensor.h"
#include "hardware/adc.h"
#include <cstring>

ADCSensor::ADCSensor(const char* name, SensorType type, const char* unit,
                     const char* description, uint8_t adc_input,
                     float scale, float offset)
    : adc_input_(adc_input), scale_(scale), offset_(offset) {
    strncpy(info_.name,        name,        MAX_NAME_LEN - 1);
    strncpy(info_.unit,        unit,        MAX_UNIT_LEN - 1);
    strncpy(info_.description, description, MAX_DESC_LEN - 1);
    info_.name[MAX_NAME_LEN - 1]        = '\0';
    info_.unit[MAX_UNIT_LEN - 1]        = '\0';
    info_.description[MAX_DESC_LEN - 1] = '\0';
    info_.type = type;

    // Map adc_input 0-3 to GPIO 26-29 and initialise the pin.
    adc_init();
    if (adc_input_ <= 3) {
        adc_gpio_init(26 + adc_input_);
    }
}

const SensorInfo& ADCSensor::info() const {
    return info_;
}

SensorReading ADCSensor::read() {
    if (adc_input_ > 3) {
        return {0.0f, false};
    }
    adc_select_input(adc_input_);
    uint16_t raw     = adc_read();
    float    voltage = raw * (3.3f / 4096.0f);
    float    value   = voltage * scale_ + offset_;
    return {value, true};
}
