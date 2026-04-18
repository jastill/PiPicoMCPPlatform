#pragma once
#include "platform_config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Actuator types
// ─────────────────────────────────────────────────────────────────────────────
enum class ActuatorType : uint8_t {
    RELAY,   // binary on/off (coil-driven relay or solid-state relay)
    LED,     // binary on/off or PWM-dimmable LED
    PWM      // continuous 0–100 % duty cycle (fan, motor, heater, servo, etc.)
};

// Returns a short ASCII string for an ActuatorType (e.g. "relay")
const char* actuator_type_str(ActuatorType t);

// ─────────────────────────────────────────────────────────────────────────────
// Static metadata for an actuator
// ─────────────────────────────────────────────────────────────────────────────
struct ActuatorInfo {
    char         name[MAX_NAME_LEN];
    ActuatorType type;
    char         description[MAX_DESC_LEN];
};

// ─────────────────────────────────────────────────────────────────────────────
// Current state of an actuator
// ─────────────────────────────────────────────────────────────────────────────
struct ActuatorState {
    float value;  // 0 or 1 for RELAY/LED; 0–100 for PWM
    bool  valid;
};

// ─────────────────────────────────────────────────────────────────────────────
// IActuator — pure interface implemented by every concrete actuator class
// ─────────────────────────────────────────────────────────────────────────────
class IActuator {
public:
    virtual ~IActuator() = default;

    // Returns static metadata — must not block
    virtual const ActuatorInfo& info() const = 0;

    // Set the actuator state.
    //   RELAY / LED:  value == 0.0f → off, anything else → on
    //   PWM:          value clamped to [0, 100] and used as duty cycle %
    // Returns true on success.
    virtual bool set(float value) = 0;

    // Returns the last state set via set().  Not a live hardware read.
    virtual ActuatorState get() const = 0;
};
