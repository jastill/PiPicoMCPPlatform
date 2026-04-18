#pragma once
#include "actuator.h"

// ─────────────────────────────────────────────────────────────────────────────
// GPIOActuator — drives a single GPIO output as a binary actuator.
//
// Suitable for: relay modules, indicator LEDs, transistor-driven loads,
//               solid-state relays.
//
// active_low: when true, the GPIO is driven LOW to activate and HIGH to
//             deactivate (common for relay boards with optocoupler inputs).
// ─────────────────────────────────────────────────────────────────────────────
class GPIOActuator : public IActuator {
public:
    GPIOActuator(const char*  name,
                 ActuatorType type,          // RELAY or LED
                 const char*  description,
                 uint8_t      gpio_pin,
                 bool         active_low = false);

    const ActuatorInfo& info() const override;

    // value == 0.0f → deactivate; anything else → activate.
    bool          set(float value) override;

    ActuatorState get() const     override;

private:
    ActuatorInfo info_;
    uint8_t      gpio_pin_;
    bool         active_low_;
    bool         state_;     // cached logical state (true = activated)
};
