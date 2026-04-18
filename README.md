# PiPicoMCPPlatform

A C++ MCP (Model Context Protocol) server for the Raspberry Pi Pico (RP2040).

Connect Claude or any MCP-compatible client to the Pico over USB serial and
interact with sensors and actuators through natural language.

## Features

- **MCP over USB CDC** — plug in the Pico; it appears as a virtual COM port
- **Zero heap allocation** — all buffers are statically sized (safe for bare-metal)
- **Configurable sensor/actuator registry** — add up to 16 of each in `main.cpp`
- **Built-in sensor types**: internal temperature, generic ADC, GPIO digital input
- **Built-in actuator types**: GPIO relay/LED, PWM (fan, motor, LED dimmer)

## MCP Tools

| Tool | Description |
|------|-------------|
| `list_sensors` | List all registered sensors with name, type, unit, description |
| `get_sensor_readings` | Get current readings from every sensor |
| `get_sensor_reading` | Get a single sensor reading by name |
| `list_outputs` | List all registered outputs with current state |
| `set_output` | Set an output: `0`/`1` for relay/LED; `0`-`100` for PWM |
| `get_output_state` | Get the current state of one output by name |

## Directory Structure

```
PiPicoMCPPlatform/
├── CMakeLists.txt
├── pico_sdk_import.cmake
├── include/                    # Core platform headers
│   ├── platform_config.h       # All compile-time limits (edit here to resize buffers)
│   ├── sensor.h                # ISensor interface, SensorType, SensorReading
│   ├── actuator.h              # IActuator interface, ActuatorType, ActuatorState
│   ├── sensor_registry.h
│   ├── actuator_registry.h
│   ├── json_builder.h          # Zero-allocation JSON serialiser
│   ├── json_scanner.h          # MCP request field extractor
│   ├── mcp_request.h           # Parsed request POD struct
│   ├── mcp_server.h            # JSON-RPC dispatcher
│   ├── transport.h             # ITransport interface
│   └── usb_cdc_transport.h     # USB serial transport
├── src/                        # Core platform implementations
├── sensors/
│   ├── include/
│   │   ├── internal_temp_sensor.h   # RP2040 on-chip temp (ADC4)
│   │   ├── adc_sensor.h             # Generic ADC (GPIO26-29)
│   │   └── gpio_sensor.h            # Digital input
│   └── src/
├── actuators/
│   ├── include/
│   │   ├── gpio_actuator.h          # Relay / LED
│   │   └── pwm_actuator.h           # PWM output
│   └── src/
└── app/
    └── main.cpp                # Configure sensors/actuators here
```

## Build

### Prerequisites

- [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (v1.5+)
- CMake >= 3.13
- `arm-none-eabi-gcc` toolchain

### Steps

```bash
export PICO_SDK_PATH=/path/to/pico-sdk

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

This produces `build/mcp_server.uf2`.

### Flash

1. Hold the **BOOTSEL** button and plug in the Pico
2. It mounts as a USB mass storage device
3. Copy `mcp_server.uf2` to it — it reboots automatically

## Customising

Edit `app/main.cpp` to add or remove sensors and actuators.

### Add an ADC sensor (e.g. voltage divider or NTC thermistor)

```cpp
// GPIO27 = ADC1; scale converts 0-3.3 V to 0-10 V (voltage divider ratio)
static ADCSensor supply_voltage("supply_v",
                                SensorType::VOLTAGE,
                                "V",
                                "12V supply rail (via divider)",
                                1,             // ADC1 = GPIO27
                                10.0f/3.3f,    // scale
                                0.0f);         // offset
sensors.add(&supply_voltage);
```

### Add a relay

```cpp
static GPIOActuator heater("heater",
                           ActuatorType::RELAY,
                           "Heater relay",
                           3,       // GPIO3
                           true);   // active-low (typical relay board)
actuators.add(&heater);
```

### Add a PWM actuator

```cpp
static PWMActuator pump("pump",
                        "Water pump speed",
                        6,       // GPIO6
                        20000);  // 20 kHz
actuators.add(&pump);
```

## MCP Protocol Transport

Messages are **newline-delimited JSON-RPC 2.0** over USB serial.
The server responds to each request with a single JSON line.

Example session (test manually with `minicom`, `screen`, or any serial terminal):

```
-> {"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"test","version":"1.0"}}}
<- {"jsonrpc":"2.0","id":1,"result":{"protocolVersion":"2024-11-05","capabilities":{"tools":{}},"serverInfo":{"name":"PiPicoMCPServer","version":"1.0.0"}}}

-> {"jsonrpc":"2.0","method":"notifications/initialized"}
(no response -- notification)

-> {"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"get_sensor_readings","arguments":{}}}
<- {"jsonrpc":"2.0","id":2,"result":{"content":[{"type":"text","text":"{\"readings\":[{\"name\":\"chip_temp\",\"type\":\"temperature\",\"unit\":\"C\",\"value\":28.450,\"valid\":true},...],\"count\":3}"}],"isError":false}}
```

## Configuration

All limits are in `include/platform_config.h`:

| Constant | Default | Description |
|----------|---------|-------------|
| `MAX_SENSORS` | 16 | Maximum registered sensors |
| `MAX_ACTUATORS` | 16 | Maximum registered actuators |
| `RX_BUFFER_SIZE` | 512 | Inbound line buffer (bytes) |
| `TX_BUFFER_SIZE` | 4096 | Outbound response buffer (bytes) |
| `INNER_BUF_SIZE` | 2048 | Tool result payload buffer (bytes) |

Total static RAM usage is under 8 KB -- well within the RP2040's 264 KB.

## Implementing a Custom Sensor

```cpp
#include "sensor.h"

class MyCustomSensor : public ISensor {
public:
    MyCustomSensor() {
        strncpy(info_.name,        "my_sensor",        MAX_NAME_LEN - 1);
        strncpy(info_.unit,        "units",             MAX_UNIT_LEN - 1);
        strncpy(info_.description, "My custom sensor",  MAX_DESC_LEN - 1);
        info_.name[MAX_NAME_LEN - 1]        = '\0';
        info_.unit[MAX_UNIT_LEN - 1]        = '\0';
        info_.description[MAX_DESC_LEN - 1] = '\0';
        info_.type = SensorType::ANALOG;
    }
    const SensorInfo& info() const override { return info_; }
    SensorReading read() override {
        float value = /* read your hardware */ 0.0f;
        return {value, true};
    }
private:
    SensorInfo info_;
};
```

Register in `main.cpp`:

```cpp
static MyCustomSensor my_sensor;
// ... in main():
sensors.add(&my_sensor);
```

## License

MIT
