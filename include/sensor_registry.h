#pragma once
#include "sensor.h"

// Fixed-size registry of ISensor pointers.
// All sensor objects must be statically allocated and outlive the registry.
class SensorRegistry {
public:
    SensorRegistry();

    // Register a sensor. Returns false if the registry is full.
    bool add(ISensor* sensor);

    size_t   count() const;
    ISensor* get(size_t index) const;   // nullptr if index out of range

    // Linear scan by name. Returns nullptr if not found.
    ISensor* find(const char* name) const;

private:
    ISensor* sensors_[MAX_SENSORS];
    size_t   count_;
};
