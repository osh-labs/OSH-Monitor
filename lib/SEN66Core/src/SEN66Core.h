/**
 * @file SEN66Core.h
 * @brief Hardware abstraction library for Sensirion SEN66 air quality sensor
 * 
 * Provides I2C communication, raw data acquisition, CRC validation,
 * and environmental metric calculations for the SEN66 sensor.
 * 
 * @author Christopher Lee
 * @license GPL-3.0
 * @version 1.1.0
 */

#ifndef SEN66_CORE_H
#define SEN66_CORE_H

#include <Arduino.h>
#include <Wire.h>
#include <SensirionI2cSen66.h>

/**
 * @brief Sensor lifecycle states
 */
enum class SensorState {
    UNINITIALIZED,  // Sensor not initialized
    INITIALIZING,   // Sensor initialization in progress
    IDLE,           // Sensor initialized but not measuring
    MEASURING,      // Sensor actively taking measurements
    ERROR           // Sensor in error state
};

/**
 * @brief Raw measurements from SEN66 sensor
 */
struct SEN66RawData {
    float temperature;         // °C
    float humidity;            // %RH
    float vocIndex;            // VOC Index (1-500)
    float noxIndex;            // NOx Index (1-500)
    float pm1_0;               // µg/m³
    float pm2_5;               // µg/m³
    float pm4_0;               // µg/m³
    float pm10;                // µg/m³
    float co2;                 // ppm
};

/**
 * @brief Derived environmental metrics calculated from raw data
 */
struct SEN66DerivedData {
    float dewPoint;            // °C - Magnus formula
    float heatIndex;           // °C - NOAA/Steadman approximation
    float absoluteHumidity;    // g/m³
};

/**
 * @brief Complete sensor data package with raw and derived values
 */
struct SEN66FullData {
    SEN66RawData raw;
    SEN66DerivedData derived;
    bool valid;                // true if read successful and CRC valid
};

/**
 * @brief Core hardware abstraction for SEN66 sensor
 * 
 * Handles I2C communication, sensor initialization, measurement control,
 * and environmental calculations. Sensor-agnostic applications should use
 * this library through a platform orchestrator (e.g., OSHMonitor).
 */
class SEN66Core {
public:
    /**
     * @brief Constructor
     * @param wire I2C bus reference (default Wire)
     */
    SEN66Core(TwoWire &wire = Wire);
    
    /**
     * @brief Initialize sensor and I2C communication
     * @param sdaPin I2C SDA pin (default 21)
     * @param sclPin I2C SCL pin (default 22)
     * @param i2cFreq I2C frequency in Hz (default 100000)
     * @return true if initialization successful, false otherwise
     */
    bool begin(int sdaPin = 21, int sclPin = 22, uint32_t i2cFreq = 100000);
    
    /**
     * @brief Start continuous measurement mode
     * @return true if command successful, false otherwise
     */
    bool startMeasurement();
    
    /**
     * @brief Stop measurement mode
     * @return true if command successful, false otherwise
     */
    bool stopMeasurement();
    
    /**
     * @brief Reset sensor device
     * @return true if reset successful, false otherwise
     */
    bool deviceReset();
    
    /**
     * @brief Get sensor serial number
     * @return Serial number string, or empty string on error
     */
    String getSerialNumber();
    
    /**
     * @brief Read raw sensor measurements
     * @param data Reference to SEN66RawData structure to populate
     * @return true if read successful and CRC valid, false otherwise
     */
    bool readRawData(SEN66RawData &data);
    
    /**
     * @brief Read full sensor data (raw + derived metrics)
     * @param data Reference to SEN66FullData structure to populate
     * @return true if read successful and CRC valid, false otherwise
     */
    bool readFullData(SEN66FullData &data);
    
    /**
     * @brief Calculate dew point from temperature and humidity
     * @param temp Temperature in °C
     * @param humidity Relative humidity in %
     * @return Dew point in °C (Magnus-type formula)
     */
    static float calculateDewPoint(float temp, float humidity);
    
    /**
     * @brief Calculate heat index from temperature and humidity
     * @param temp Temperature in °C
     * @param humidity Relative humidity in %
     * @return Heat index in °C (NOAA/Steadman approximation)
     */
    static float calculateHeatIndex(float temp, float humidity);
    
    /**
     * @brief Calculate absolute humidity from temperature and humidity
     * @param temp Temperature in °C
     * @param humidity Relative humidity in %
     * @return Absolute humidity in g/m³
     */
    static float calculateAbsoluteHumidity(float temp, float humidity);
    
    /**
     * @brief Check if sensor is initialized and ready
     * @return true if ready for measurements, false otherwise
     */
    bool isReady() const;
    
    /**
     * @brief Get current sensor state
     * @return Current SensorState
     */
    SensorState getState() const;
    
    /**
     * @brief Check if sensor is actively measuring
     * @return true if in MEASURING state, false otherwise
     */
    bool isMeasuring() const;
    
    /**
     * @brief Get last error message
     * @return Error description string
     */
    String getLastError() const;
    
private:
    TwoWire &_wire;
    SensirionI2cSen66 _sensor;
    SensorState _state;
    String _lastError;
    
    /**
     * @brief Compute derived metrics from raw data
     * @param raw Raw sensor data
     * @param derived Output derived metrics
     */
    void computeDerivedMetrics(const SEN66RawData &raw, SEN66DerivedData &derived);
    
    /**
     * @brief Set last error message
     * @param error Error description
     */
    void setError(const String &error);
};

#endif // SEN66_CORE_H
