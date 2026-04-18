#include "pwm_actuator.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include <cstring>

PWMActuator::PWMActuator(const char* name, const char* description,
                         uint8_t gpio_pin, uint32_t freq_hz)
    : gpio_pin_(gpio_pin), duty_(0.0f) {
    strncpy(info_.name,        name,        MAX_NAME_LEN - 1);
    strncpy(info_.description, description, MAX_DESC_LEN - 1);
    info_.name[MAX_NAME_LEN - 1]        = '\0';
    info_.description[MAX_DESC_LEN - 1] = '\0';
    info_.type = ActuatorType::PWM;

    // Assign the GPIO to the PWM function
    gpio_set_function(gpio_pin_, GPIO_FUNC_PWM);

    pwm_slice_   = pwm_gpio_to_slice_num(gpio_pin_);
    pwm_channel_ = pwm_gpio_to_channel(gpio_pin_);

    // Compute divider and wrap to achieve the requested frequency.
    // f_pwm = f_sys / (divider * (wrap + 1))
    // We want wrap to be as large as possible (for resolution) while fitting
    // in a uint16_t (max 65535).
    uint32_t sys_hz = clock_get_hz(clk_sys);
    uint32_t divider = 1;
    uint32_t wrap_val;

    if (freq_hz == 0) freq_hz = 1;   // guard against divide-by-zero

    while (true) {
        wrap_val = sys_hz / (divider * freq_hz);
        if (wrap_val <= 65535 || divider >= 255) break;
        divider++;
    }
    if (wrap_val > 65535) wrap_val = 65535;
    if (wrap_val == 0)    wrap_val = 1;

    wrap_ = (uint16_t)(wrap_val - 1);  // PWM counts from 0 to wrap inclusive

    // Configure the PWM slice
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&cfg, (uint8_t)divider);
    pwm_config_set_wrap(&cfg, wrap_);
    pwm_init(pwm_slice_, &cfg, true);

    // Start at 0 % duty cycle
    pwm_set_chan_level(pwm_slice_, pwm_channel_, 0);
}

const ActuatorInfo& PWMActuator::info() const {
    return info_;
}

bool PWMActuator::set(float value) {
    // Clamp to [0, 100]
    if (value < 0.0f)   value = 0.0f;
    if (value > 100.0f) value = 100.0f;
    duty_ = value;

    uint16_t level = (uint16_t)(value / 100.0f * (float)(wrap_ + 1));
    if (level > wrap_ + 1) level = wrap_ + 1;

    pwm_set_chan_level(pwm_slice_, pwm_channel_, level);
    return true;
}

ActuatorState PWMActuator::get() const {
    return {duty_, true};
}
