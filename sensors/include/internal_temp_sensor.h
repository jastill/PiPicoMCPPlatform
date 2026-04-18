#pragma once
#include "sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
// InternalTempSensor — reads the RP2040 on-chip temperature sensor via ADC4.
//
// Conversion formula (from the RP2040 datasheet):
//   T (°C) = 27 - (V_adc - 0.706) / 0.001721
//
// Accuracy: ±5 °C typical (datasheet spec) — suitable for ambient monitoring
// but not precision measurement.
// ─────────────────────────────────────────────────────────────────────────────
class InternalTempSensor : public ISensor {
public:
    // name: the identifier used in MCP tool responses (default: "internal_temp")
    explicit InternalTempSensor(const char* name        = "internal_temp",
                                const char* description = "RP2040 on-chip temperature sensor");

    const SensorInfo& info() const override;
    SensorReading     read()        override;

private:
    SensorInfo info_;
};
