#pragma once
#include "sensor.h"

// ─────────────────────────────────────────────────────────────────────────────
// GPIOSensor — reads a GPIO pin as a digital (boolean) sensor.
//
// Returned value:  1.0f when the pin is high, 0.0f when low.
// Type is always SensorType::DIGITAL, unit is always "".
//
// Typical uses: door/window reed switch, PIR motion sensor output,
//               float switch, push button state.
// ─────────────────────────────────────────────────────────────────────────────
class GPIOSensor : public ISensor {
public:
    // gpio_pin:   RP2040 GPIO number (0–28; avoid ADC pins if ADCSensor is used)
    // pull_up:    true → enable internal pull-up (normally-open contact to GND)
    //             false → enable internal pull-down (normally-open contact to 3V3)
    GPIOSensor(const char* name,
               const char* description,
               uint8_t     gpio_pin,
               bool        pull_up = true);

    const SensorInfo& info() const override;
    SensorReading     read()        override;

private:
    SensorInfo info_;
    uint8_t    gpio_pin_;
};
