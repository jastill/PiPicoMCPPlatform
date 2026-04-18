#pragma once
#include "platform_config.h"

// ─────────────────────────────────────────────────────────────────────────────
// Sensor types — extend as needed
// ─────────────────────────────────────────────────────────────────────────────
enum class SensorType : uint8_t {
    TEMPERATURE,
    HUMIDITY,
    PRESSURE,
    LIGHT,
    DISTANCE,
    VOLTAGE,
    CURRENT,
    DIGITAL,
    ANALOG
};

// Returns a short ASCII string for a SensorType (e.g. "temperature")
const char* sensor_type_str(SensorType t);

// ─────────────────────────────────────────────────────────────────────────────
// Static metadata for a sensor — populated once in the constructor
// ─────────────────────────────────────────────────────────────────────────────
struct SensorInfo {
    char       name[MAX_NAME_LEN];   // machine-friendly identifier, e.g. "internal_temp"
    SensorType type;
    char       unit[MAX_UNIT_LEN];   // SI unit string, e.g. "°C", "lux", "V", ""
    char       description[MAX_DESC_LEN];
};

// ─────────────────────────────────────────────────────────────────────────────
// A single reading from a sensor
// ─────────────────────────────────────────────────────────────────────────────
struct SensorReading {
    float value;  // engineering-unit value (after any scale/offset)
    bool  valid;  // false if the read failed or the sensor is not ready
};

// ─────────────────────────────────────────────────────────────────────────────
// ISensor — pure interface implemented by every concrete sensor class
// ─────────────────────────────────────────────────────────────────────────────
class ISensor {
public:
    virtual ~ISensor() = default;

    // Returns static metadata — must not block
    virtual const SensorInfo& info() const = 0;

    // Performs a synchronous measurement and returns the result
    virtual SensorReading read() = 0;
};
