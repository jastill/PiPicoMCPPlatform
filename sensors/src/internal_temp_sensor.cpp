#include "internal_temp_sensor.h"
#include "hardware/adc.h"
#include <cstring>

InternalTempSensor::InternalTempSensor(const char* name, const char* description) {
    strncpy(info_.name,        name,        MAX_NAME_LEN - 1);
    strncpy(info_.unit,        "\xc2\xb0""C", MAX_UNIT_LEN - 1);  // °C in UTF-8
    strncpy(info_.description, description, MAX_DESC_LEN - 1);
    info_.name[MAX_NAME_LEN - 1]        = '\0';
    info_.unit[MAX_UNIT_LEN - 1]        = '\0';
    info_.description[MAX_DESC_LEN - 1] = '\0';
    info_.type = SensorType::TEMPERATURE;

    // Initialise the ADC subsystem.  Calling adc_init() multiple times is safe
    // (the SDK guards it), so we do it here even if main.cpp also calls it.
    adc_init();
    adc_set_temp_sensor_enabled(true);
}

const SensorInfo& InternalTempSensor::info() const {
    return info_;
}

SensorReading InternalTempSensor::read() {
    adc_select_input(4);   // ADC4 = internal temperature sensor
    uint16_t raw = adc_read();

    // Convert 12-bit ADC reading to voltage (Vref = 3.3 V)
    float voltage = raw * (3.3f / 4096.0f);

    // RP2040 datasheet formula: T = 27 - (V - 0.706) / 0.001721
    float temperature = 27.0f - (voltage - 0.706f) / 0.001721f;

    return {temperature, true};
}
