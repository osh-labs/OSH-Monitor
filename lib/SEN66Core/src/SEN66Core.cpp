/**
 * @file SEN66Core.cpp
 * @brief Implementation of SEN66 sensor hardware abstraction library
 * 
 * @author Christopher Lee
 * @license GPL-3.0
 * @version 1.0.0
 */

#include "SEN66Core.h"
#include <math.h>

SEN66Core::SEN66Core(TwoWire &wire)
    : _wire(wire), _initialized(false), _lastError("") {
}

bool SEN66Core::begin(int sdaPin, int sclPin, uint32_t i2cFreq) {
    // Initialize I2C
    _wire.begin(sdaPin, sclPin);
    _wire.setClock(i2cFreq);
    
    // Initialize Sensirion SEN66 sensor library
    _sensor.begin(_wire, SEN66_I2C_ADDR_6B);
    
    // Reset the device
    int16_t error = _sensor.deviceReset();
    if (error != 0) {
        setError("Device reset failed with error: " + String(error));
        return false;
    }
    
    // Wait after reset
    delay(1200);
    
    // Get serial number for verification
    int8_t serialNumber[32] = {0};
    error = _sensor.getSerialNumber(serialNumber, 32);
    if (error != 0) {
        setError("Failed to read serial number");
        // Non-fatal, continue initialization
    }
    
    // Start measurement
    if (!startMeasurement()) {
        setError("Failed to start measurement");
        return false;
    }
    
    // Wait for sensor to stabilize
    delay(2000);
    
    _initialized = true;
    _lastError = "";
    return true;
}

bool SEN66Core::startMeasurement() {
    int16_t error = _sensor.startContinuousMeasurement();
    if (error != 0) {
        setError("Start measurement failed with error: " + String(error));
        return false;
    }
    return true;
}

bool SEN66Core::stopMeasurement() {
    int16_t error = _sensor.stopMeasurement();
    if (error != 0) {
        setError("Stop measurement failed with error: " + String(error));
        return false;
    }
    return true;
}

bool SEN66Core::deviceReset() {
    int16_t error = _sensor.deviceReset();
    if (error != 0) {
        setError("Device reset failed with error: " + String(error));
        return false;
    }
    delay(1200);  // Wait for reset to complete
    return true;
}

String SEN66Core::getSerialNumber() {
    int8_t serialNumber[32] = {0};
    int16_t error = _sensor.getSerialNumber(serialNumber, 32);
    if (error != 0) {
        setError("Failed to read serial number with error: " + String(error));
        return "";
    }
    return String((const char*)serialNumber);
}

bool SEN66Core::readRawData(SEN66RawData &data) {
    if (!_initialized) {
        setError("Sensor not initialized");
        return false;
    }
    
    // Use the official Sensirion library to read all values
    // This handles all I2C communication and CRC validation automatically
    float pm1_0, pm2_5, pm4_0, pm10;
    float humidity, temperature, vocIndex, noxIndex;
    uint16_t co2;
    
    int16_t error = _sensor.readMeasuredValues(
        pm1_0, pm2_5, pm4_0, pm10,
        humidity, temperature, vocIndex, noxIndex, co2
    );
    
    if (error != 0) {
        setError("Failed to read sensor values, error code: " + String(error));
        return false;
    }
    
    // Store values in data structure
    data.pm1_0 = pm1_0;
    data.pm2_5 = pm2_5;
    data.pm4_0 = pm4_0;
    data.pm10 = pm10;
    data.humidity = humidity;
    data.temperature = temperature;
    data.vocIndex = vocIndex;
    data.noxIndex = noxIndex;
    data.co2 = (float)co2;
    
    _lastError = "";
    return true;
}

bool SEN66Core::readFullData(SEN66FullData &data) {
    data.valid = false;
    
    // Read raw sensor data
    if (!readRawData(data.raw)) {
        return false;
    }
    
    // Calculate derived metrics
    computeDerivedMetrics(data.raw, data.derived);
    
    data.valid = true;
    return true;
}

float SEN66Core::calculateDewPoint(float temp, float humidity) {
    // Magnus formula for dew point calculation
    const float a = 17.27;
    const float b = 237.7;
    
    float alpha = ((a * temp) / (b + temp)) + log(humidity / 100.0);
    float dewPoint = (b * alpha) / (a - alpha);
    
    return dewPoint;
}

float SEN66Core::calculateHeatIndex(float temp, float humidity) {
    // Convert to Fahrenheit for NOAA formula
    float T = temp * 9.0 / 5.0 + 32.0;
    float RH = humidity;
    
    // Step 1: Compute simple formula (Steadman)
    float simpleHI = 0.5 * (T + 61.0 + ((T - 68.0) * 1.2) + (RH * 0.094));
    
    // Step 2: Average simple formula with temperature
    float avgHI = (simpleHI + T) / 2.0;
    
    // Step 3: If average is < 80°F, use simple formula result
    if (avgHI < 80.0) {
        return (simpleHI - 32.0) * 5.0 / 9.0;  // Convert to Celsius
    }
    
    // Step 4: Use full Rothfusz regression for heat index >= 80°F
    float HI = -42.379 + 2.04901523 * T + 10.14333127 * RH
               - 0.22475541 * T * RH - 0.00683783 * T * T
               - 0.05481717 * RH * RH + 0.00122874 * T * T * RH
               + 0.00085282 * T * RH * RH - 0.00000199 * T * T * RH * RH;
    
    // Step 5: Apply adjustments for extreme conditions
    
    // Low humidity adjustment (RH < 13% and 80°F < T < 112°F)
    if (RH < 13.0 && T >= 80.0 && T <= 112.0) {
        float adjustment = ((13.0 - RH) / 4.0) * sqrt((17.0 - abs(T - 95.0)) / 17.0);
        HI -= adjustment;
    }
    
    // High humidity adjustment (RH > 85% and 80°F < T < 87°F)
    if (RH > 85.0 && T >= 80.0 && T <= 87.0) {
        float adjustment = ((RH - 85.0) / 10.0) * ((87.0 - T) / 5.0);
        HI += adjustment;
    }
    
    // Convert back to Celsius
    return (HI - 32.0) * 5.0 / 9.0;
}

float SEN66Core::calculateAbsoluteHumidity(float temp, float humidity) {
    // Absolute humidity in g/m³
    // Using Magnus-Tetens approximation
    const float molarMass = 18.01528;  // g/mol for water
    const float gasConstant = 8.31446;  // J/(mol·K)
    
    float tempK = temp + 273.15;
    float saturationVaporPressure = 6.112 * exp((17.67 * temp) / (temp + 243.5)) * 100;  // Pa
    float vaporPressure = (humidity / 100.0) * saturationVaporPressure;
    
    float absoluteHumidity = (vaporPressure * molarMass) / (gasConstant * tempK);
    
    return absoluteHumidity;
}

bool SEN66Core::isReady() const {
    return _initialized;
}

String SEN66Core::getLastError() const {
    return _lastError;
}

void SEN66Core::computeDerivedMetrics(const SEN66RawData &raw, SEN66DerivedData &derived) {
    derived.dewPoint = calculateDewPoint(raw.temperature, raw.humidity);
    derived.heatIndex = calculateHeatIndex(raw.temperature, raw.humidity);
    derived.absoluteHumidity = calculateAbsoluteHumidity(raw.temperature, raw.humidity);
}

void SEN66Core::setError(const String &error) {
    _lastError = error;
}
