#include "sensor_registry.h"
#include <cstring>

const char* sensor_type_str(SensorType t) {
    switch (t) {
        case SensorType::TEMPERATURE: return "temperature";
        case SensorType::HUMIDITY:    return "humidity";
        case SensorType::PRESSURE:    return "pressure";
        case SensorType::LIGHT:       return "light";
        case SensorType::DISTANCE:    return "distance";
        case SensorType::VOLTAGE:     return "voltage";
        case SensorType::CURRENT:     return "current";
        case SensorType::DIGITAL:     return "digital";
        case SensorType::ANALOG:      return "analog";
        default:                      return "unknown";
    }
}

SensorRegistry::SensorRegistry() : count_(0) {
    for (size_t i = 0; i < MAX_SENSORS; ++i) {
        sensors_[i] = nullptr;
    }
}

bool SensorRegistry::add(ISensor* sensor) {
    if (count_ >= MAX_SENSORS || sensor == nullptr) {
        return false;
    }
    sensors_[count_++] = sensor;
    return true;
}

size_t SensorRegistry::count() const {
    return count_;
}

ISensor* SensorRegistry::get(size_t index) const {
    if (index >= count_) {
        return nullptr;
    }
    return sensors_[index];
}

ISensor* SensorRegistry::find(const char* name) const {
    if (name == nullptr) return nullptr;
    for (size_t i = 0; i < count_; ++i) {
        if (strncmp(sensors_[i]->info().name, name, MAX_NAME_LEN) == 0) {
            return sensors_[i];
        }
    }
    return nullptr;
}
