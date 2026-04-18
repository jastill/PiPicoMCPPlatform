cmake_minimum_required(VERSION 3.13)

# Must be included BEFORE project() so that the Pico SDK toolchain is set up.
include(pico_sdk_import.cmake)

project(PiPicoMCPPlatform C CXX ASM)
set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# ── Source files ──────────────────────────────────────────────────────────────

set(PLATFORM_SOURCES
    # Core platform
    src/sensor_registry.cpp
    src/actuator_registry.cpp
    src/json_builder.cpp
    src/json_scanner.cpp
    src/mcp_server.cpp
    src/usb_cdc_transport.cpp

    # Sensor drivers
    sensors/src/internal_temp_sensor.cpp
    sensors/src/adc_sensor.cpp
    sensors/src/gpio_sensor.cpp

    # Actuator drivers
    actuators/src/gpio_actuator.cpp
    actuators/src/pwm_actuator.cpp

    # Application entry point
    app/main.cpp
)

add_executable(mcp_server ${PLATFORM_SOURCES})

# ── Include paths ─────────────────────────────────────────────────────────────

target_include_directories(mcp_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sensors/include
    ${CMAKE_CURRENT_SOURCE_DIR}/actuators/include
)

# ── Pico SDK libraries ────────────────────────────────────────────────────────

target_link_libraries(mcp_server
    pico_stdlib       # stdio, time, tight_loop_contents
    hardware_adc      # ADC peripheral
    hardware_gpio     # GPIO
    hardware_pwm      # PWM peripheral
    hardware_clocks   # clock_get_hz()
)

# ── USB stdio — enable USB CDC, disable UART ─────────────────────────────────

pico_enable_stdio_usb (mcp_server 1)
pico_enable_stdio_uart(mcp_server 0)

# ── Output formats (.uf2, .hex, .bin, .map) ──────────────────────────────────

pico_add_extra_outputs(mcp_server)

# ── Compiler warnings ─────────────────────────────────────────────────────────

target_compile_options(mcp_server PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)
