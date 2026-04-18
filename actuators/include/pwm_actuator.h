#pragma once
#include "actuator.h"

// ─────────────────────────────────────────────────────────────────────────────
// PWMActuator — drives a GPIO in hardware-PWM mode with a 0–100 % duty cycle.
//
// Suitable for: dimmable LEDs, fan speed control, DC motor speed, heater
//               power (via SSR), servo position (use freq_hz = 50).
//
// The RP2040 PWM peripheral has 8 slices, each with two channels (A/B).
// Any GPIO can be assigned to a PWM channel — the slice and channel numbers
// are determined automatically from the GPIO number.
//
// PWM frequency:
//   f_pwm = f_sys / (divider * (wrap + 1))
//   At 125 MHz system clock with divider=1:
//     freq_hz = 1000  →  wrap ≈ 124999  (fine for LED dimming, fans)
//     freq_hz = 50    →  wrap ≈ 2499999 (servo — may need divider > 1)
//   For very low frequencies (< ~100 Hz) a clock divider is applied
//   automatically to keep wrap within the 16-bit register limit (65535).
// ─────────────────────────────────────────────────────────────────────────────
class PWMActuator : public IActuator {
public:
    // gpio_pin: RP2040 GPIO number (must support PWM — all GPIOs do)
    // freq_hz:  desired PWM output frequency in Hz
    PWMActuator(const char* name,
                const char* description,
                uint8_t     gpio_pin,
                uint32_t    freq_hz = 1000);

    const ActuatorInfo& info() const override;

    // value is clamped to [0, 100] and used as the duty cycle %.
    bool          set(float value) override;

    ActuatorState get() const     override;

private:
    ActuatorInfo info_;
    uint8_t      gpio_pin_;
    uint8_t      pwm_slice_;
    uint8_t      pwm_channel_;
    uint16_t     wrap_;      // PWM counter wrap value
    float        duty_;      // cached duty cycle %
};
