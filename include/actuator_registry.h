#pragma once
#include "actuator.h"

// Fixed-size registry of IActuator pointers.
// All actuator objects must be statically allocated and outlive the registry.
class ActuatorRegistry {
public:
    ActuatorRegistry();

    // Register an actuator. Returns false if the registry is full.
    bool add(IActuator* actuator);

    size_t     count() const;
    IActuator* get(size_t index) const;   // nullptr if index out of range

    // Linear scan by name. Returns nullptr if not found.
    IActuator* find(const char* name) const;

private:
    IActuator* actuators_[MAX_ACTUATORS];
    size_t     count_;
};
