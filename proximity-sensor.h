#pragma once
#include <Arduino.h>                    // To add IntelliSense for platform constants.
#include <Wire.h>                       // To read the gesture sensor via I2C.
#undef NA_STATE             // ⚠️ Löst den Konflikt mit ESP8266 WiFi-Core!
#include "SparkFun_APDS9960.h"  // Third-party library with gesture sensor utilities.

#ifndef proximity_sensor_h
#define proximity_sensor_h

class ProximitySensor {
  public:
    void attach(uint8_t sdaPin, uint8_t sclPin, uint8_t rangeThreshold);
    uint8_t getProximity();
    bool isInRange();

  private:
    SparkFun_APDS9960 _sensor;
    uint8_t _lastProximityValue;
    uint8_t _rangeThreshold;
};

#endif /* proximity_sensor_h */
