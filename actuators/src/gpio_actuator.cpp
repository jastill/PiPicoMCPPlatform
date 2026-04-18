#include "gpio_actuator.h"
#include "hardware/gpio.h"
#include <cstring>

GPIOActuator::GPIOActuator(const char* name, ActuatorType type,
                           const char* description, uint8_t gpio_pin,
                           bool active_low)
    : gpio_pin_(gpio_pin), active_low_(active_low), state_(false) {
    strncpy(info_.name,        name,        MAX_NAME_LEN - 1);
    strncpy(info_.description, description, MAX_DESC_LEN - 1);
    info_.name[MAX_NAME_LEN - 1]        = '\0';
    info_.description[MAX_DESC_LEN - 1] = '\0';
    info_.type = type;

    gpio_init(gpio_pin_);
    gpio_set_dir(gpio_pin_, GPIO_OUT);
    // Start deactivated
    gpio_put(gpio_pin_, active_low_ ? 1 : 0);
}

const ActuatorInfo& GPIOActuator::info() const {
    return info_;
}

bool GPIOActuator::set(float value) {
    state_ = (value != 0.0f);
    // Apply active-low inversion
    gpio_put(gpio_pin_, active_low_ ? !state_ : state_);
    return true;
}

ActuatorState GPIOActuator::get() const {
    return {state_ ? 1.0f : 0.0f, true};
}
