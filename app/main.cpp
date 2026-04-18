// ─────────────────────────────────────────────────────────────────────────────
// PiPicoMCPPlatform — main.cpp
//
// Example application: configure sensors and actuators then start the MCP
// server on USB CDC (USB serial).
//
// To add a new sensor, create a static sensor object below and call
// sensors.add(&your_sensor).  To add a new actuator, do the same with
// actuators.  No other changes are needed — the MCP server picks them up
// automatically from the registries.
//
// Flash the compiled .uf2 to the Pico, then connect with any serial terminal
// or MCP-compatible client at any baud rate (USB CDC is baud-rate agnostic).
// ─────────────────────────────────────────────────────────────────────────────

#include "pico/stdlib.h"

#include "platform_config.h"
#include "usb_cdc_transport.h"
#include "mcp_server.h"
#include "sensor_registry.h"
#include "actuator_registry.h"

// Sensor drivers
#include "internal_temp_sensor.h"
#include "adc_sensor.h"
#include "gpio_sensor.h"

// Actuator drivers
#include "gpio_actuator.h"
#include "pwm_actuator.h"

// ─────────────────────────────────────────────────────────────────────────────
// Sensor instances — statically allocated, no heap usage
// ─────────────────────────────────────────────────────────────────────────────

// RP2040 on-chip temperature sensor (ADC4)
static InternalTempSensor temp_chip("chip_temp",
                                    "RP2040 on-chip temperature");

// Example: LDR (light-dependent resistor) on ADC0 / GPIO26
//   Voltage divider: 3V3 – 10 kΩ – GPIO26 – LDR – GND
//   scale = 1.0 (raw voltage) ; adjust scale/offset for your divider
static ADCSensor light_sensor("light",
                              SensorType::LIGHT,
                              "V",
                              "LDR ambient light (raw ADC voltage)",
                              0,       // ADC0 = GPIO26
                              1.0f,    // scale
                              0.0f);   // offset

// Example: door/window reed switch on GPIO16, pull-up, contact to GND when open
static GPIOSensor door_sensor("door",
                              "Door reed switch (0=closed, 1=open)",
                              16,
                              true);  // internal pull-up

// ─────────────────────────────────────────────────────────────────────────────
// Actuator instances
// ─────────────────────────────────────────────────────────────────────────────

// Onboard LED (GPIO25 on standard Pico)
static GPIOActuator onboard_led("led",
                                ActuatorType::LED,
                                "Onboard LED",
                                25,
                                false);  // active-high

// Example: relay module on GPIO15 (most relay boards are active-low)
static GPIOActuator relay1("relay1",
                           ActuatorType::RELAY,
                           "Relay channel 1",
                           15,
                           true);  // active-low (typical relay board)

// Example: PWM fan or LED strip on GPIO14 at 25 kHz (PC fan frequency)
static PWMActuator fan("fan",
                       "Cooling fan / PWM channel 1",
                       14,
                       25000);  // 25 kHz

// ─────────────────────────────────────────────────────────────────────────────
// Platform objects
// ─────────────────────────────────────────────────────────────────────────────

static SensorRegistry   sensors;
static ActuatorRegistry actuators;
static UsbCdcTransport  transport;
static McpServer        server(sensors, actuators);

// Line buffers — statically allocated
static char rx_buf[RX_BUFFER_SIZE];
static char tx_buf[TX_BUFFER_SIZE];

// ─────────────────────────────────────────────────────────────────────────────
// Entry point
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    // Register sensors
    sensors.add(&temp_chip);
    sensors.add(&light_sensor);
    sensors.add(&door_sensor);

    // Register actuators
    actuators.add(&onboard_led);
    actuators.add(&relay1);
    actuators.add(&fan);

    // Initialise USB CDC transport (calls stdio_init_all)
    transport.init();

    // Wait for a USB host to connect and open the virtual COM port.
    // tight_loop_contents() is a Pico SDK no-op that prevents the compiler
    // from optimising away the loop while keeping USB IRQs serviced.
    while (!transport.connected()) {
        tight_loop_contents();
    }

    server.init();

    size_t rx_pos = 0;

    while (true) {
        int c = transport.read_byte();

        if (c < 0) {
            // No byte available — yield CPU briefly
            tight_loop_contents();
            continue;
        }

        if (c == '\n' || c == '\r') {
            if (rx_pos > 0) {
                rx_buf[rx_pos] = '\0';

                size_t out_len = server.process_line(rx_buf, rx_pos,
                                                     tx_buf, TX_BUFFER_SIZE);
                if (out_len > 0) {
                    transport.write(tx_buf, out_len);
                }

                rx_pos = 0;
            }
            // Ignore empty lines (bare \r\n line endings produce a lone \n
            // after the \r was already processed as a terminator).
        } else {
            if (rx_pos < RX_BUFFER_SIZE - 1) {
                rx_buf[rx_pos++] = static_cast<char>(c);
            }
            // On overflow: drop bytes silently.  When \n arrives, the partial
            // line produces a parse error response — the client will retry.
        }
    }

    // Unreachable on Pico, but satisfies the compiler.
    return 0;
}
