#pragma once
#include "sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
// ADCSensor — generic single-channel ADC sensor.
//
// Reads one of the four user-accessible ADC inputs (ADC0–ADC3, on GPIO 26–29)
// and converts the raw 12-bit value to an engineering unit using:
//
//   value = (raw_voltage * scale) + offset
//
// where raw_voltage = adc_raw * (3.3 / 4096).
//
// Example uses:
//   Voltage divider to measure 0–10 V supply:
//     adc_input=0, scale=10.0f/3.3f, offset=0.0f, type=VOLTAGE, unit="V"
//
//   NTC thermistor with look-up table (future):
//     subclass and override read() to apply the Steinhart-Hart equation.
//
//   Light-dependent resistor (LDR) as a light sensor:
//     adc_input=1, scale=330.0f, offset=0.0f, type=LIGHT, unit="lux" (approx.)
// ─────────────────────────────────────────────────────────────────────────────
class ADCSensor : public ISensor {
public:
    // adc_input: 0–3  (maps to GPIO 26–29)
    // scale:     multiplied by the ADC voltage to produce an engineering value
    // offset:    added after scaling
    ADCSensor(const char* name,
              SensorType  type,
              const char* unit,
              const char* description,
              uint8_t     adc_input,
              float       scale  = 1.0f,
              float       offset = 0.0f);

    const SensorInfo& info() const override;
    SensorReading     read()        override;

private:
    SensorInfo info_;
    uint8_t    adc_input_;
    float      scale_;
    float      offset_;
};
