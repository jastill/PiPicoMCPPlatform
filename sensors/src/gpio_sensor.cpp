#include "gpio_sensor.h"
#include "hardware/gpio.h"
#include <cstring>

GPIOSensor::GPIOSensor(const char* name, const char* description,
                       uint8_t gpio_pin, bool pull_up)
    : gpio_pin_(gpio_pin) {
    strncpy(info_.name,        name,        MAX_NAME_LEN - 1);
    strncpy(info_.unit,        "",          MAX_UNIT_LEN - 1);
    strncpy(info_.description, description, MAX_DESC_LEN - 1);
    info_.name[MAX_NAME_LEN - 1]        = '\0';
    info_.unit[MAX_UNIT_LEN - 1]        = '\0';
    info_.description[MAX_DESC_LEN - 1] = '\0';
    info_.type = SensorType::DIGITAL;

    gpio_init(gpio_pin_);
    gpio_set_dir(gpio_pin_, GPIO_IN);
    if (pull_up) {
        gpio_pull_up(gpio_pin_);
    } else {
        gpio_pull_down(gpio_pin_);
    }
}

const SensorInfo& GPIOSensor::info() const {
    return info_;
}

SensorReading GPIOSensor::read() {
    return {gpio_get(gpio_pin_) ? 1.0f : 0.0f, true};
}
