#include "actuator_registry.h"
#include <cstring>

const char* actuator_type_str(ActuatorType t) {
    switch (t) {
        case ActuatorType::RELAY: return "relay";
        case ActuatorType::LED:   return "led";
        case ActuatorType::PWM:   return "pwm";
        default:                  return "unknown";
    }
}

ActuatorRegistry::ActuatorRegistry() : count_(0) {
    for (size_t i = 0; i < MAX_ACTUATORS; ++i) {
        actuators_[i] = nullptr;
    }
}

bool ActuatorRegistry::add(IActuator* actuator) {
    if (count_ >= MAX_ACTUATORS || actuator == nullptr) {
        return false;
    }
    actuators_[count_++] = actuator;
    return true;
}

size_t ActuatorRegistry::count() const {
    return count_;
}

IActuator* ActuatorRegistry::get(size_t index) const {
    if (index >= count_) {
        return nullptr;
    }
    return actuators_[index];
}

IActuator* ActuatorRegistry::find(const char* name) const {
    if (name == nullptr) return nullptr;
    for (size_t i = 0; i < count_; ++i) {
        if (strncmp(actuators_[i]->info().name, name, MAX_NAME_LEN) == 0) {
            return actuators_[i];
        }
    }
    return nullptr;
}
