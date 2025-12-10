/**
 * SEN66Dosimetry.h
 * 
 * Open-Source Arduino/PlatformIO Library for ESP32-S3
 * Advanced Air-Quality Acquisition, Dosimetry, and CSV Logging for the Sensirion SEN66
 * 
 * MIT License
 */

#ifndef SEN66_DOSIMETRY_H
#define SEN66_DOSIMETRY_H

#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <SensirionI2cSen66.h>
#include <Preferences.h>
#include <vector>

// Default sampling interval (seconds) for TWA calculations
#define DEFAULT_SAMPLING_INTERVAL 60
#define DEFAULT_MEASUREMENT_INTERVAL 20  // seconds
#define DEFAULT_LOGGING_INTERVAL 20  // seconds
#define TWA_WINDOW_SECONDS (8 * 3600)  // 8 hours in seconds

/**
 * @brief Configuration structure for sensor operation
 */
struct SensorConfig {
    uint16_t measurementInterval;  // Seconds between measurements
    uint16_t loggingInterval;      // Seconds between log entries (0 = log every measurement)
    uint16_t samplingInterval;     // Seconds for TWA calculations
};

/**
 * @brief Unified data structure containing all raw measurements, 
 *        derived metrics, and 8-hour TWA values
 */
struct SensorData {
    // Raw measurements from SEN66
    float temperature;         // °C
    float humidity;            // %RH
    float vocIndex;            // VOC Index
    float noxIndex;            // NOx Index
    float pm1_0;               // µg/m³
    float pm2_5;               // µg/m³
    float pm4_0;               // µg/m³
    float pm10;                // µg/m³
    float co2;                 // ppm

    // Derived environmental metrics
    float dewPoint;            // °C
    float heatIndex;           // °C
    float absoluteHumidity;    // g/m³

    // 8-hour Time-Weighted Averages
    float twa_pm1_0;           // µg/m³
    float twa_pm2_5;           // µg/m³
    float twa_pm4_0;           // µg/m³
    float twa_pm10;            // µg/m³

    uint32_t timestamp;        // Unix timestamp or uptime in seconds
};

/**
 * @brief Main class for SEN66 sensor acquisition, dosimetry, and logging
 */
class SEN66Dosimetry {
public:
    /**
     * @brief Constructor
     * @param wire I2C bus reference (default Wire)
     * @param samplingInterval Sampling interval in seconds for TWA calculations
     */
    SEN66Dosimetry(TwoWire &wire = Wire, uint16_t samplingInterval = DEFAULT_SAMPLING_INTERVAL);

    /**
     * @brief Initialize the library, sensor, and filesystem
     * @param sdaPin I2C SDA pin (default 21)
     * @param sclPin I2C SCL pin (default 22)
     * @param i2cFreq I2C frequency in Hz (default 100000)
     * @return true if initialization successful, false otherwise
     */
    bool begin(int sdaPin = 21, int sclPin = 22, uint32_t i2cFreq = 100000);

    /**
     * @brief Read all measurements from the SEN66 sensor
     * @return true if read successful and CRC valid, false otherwise
     */
    bool readSensor();

    /**
     * @brief Get the current sensor data structure
     * @return SensorData struct with all measurements and calculated values
     */
    SensorData getData() const;

    /**
     * @brief Update 8-hour TWA calculations with new sensor data
     * @param data Reference to SensorData to update with TWA values
     */
    void updateTWA(SensorData &data);

    /**
     * @brief Log a sensor data entry to CSV file
     * @param data SensorData structure to log
     * @return true if logging successful, false otherwise
     */
    bool logEntry(const SensorData &data);

    /**
     * @brief Erase all log files from filesystem
     * @return true if erasure successful, false otherwise
     */
    bool eraseLogs();

    /**
     * @brief Read a specific line from the log file
     * @param index Line index (0 = header)
     * @param line String reference to store the line
     * @return true if line read successfully, false otherwise
     */
    bool readLogLine(size_t index, String &line);

    /**
     * @brief Get the total number of lines in the log file
     * @return Number of lines (including header)
     */
    size_t getLogLineCount();

    /**
     * @brief Set the log file path
     * @param path File path (default "/sensor_log.csv")
     */
    void setLogFilePath(const String &path);

    /**
     * @brief Start sensor measurements
     * @return true if command successful, false otherwise
     */
    bool startMeasurement();

    /**
     * @brief Stop sensor measurements
     * @return true if command successful, false otherwise
     */
    bool stopMeasurement();

    /**
     * @brief Set the current Unix timestamp (for time synchronization)
     * @param unixTime Unix timestamp in seconds since epoch (1970-01-01)
     */
    void setUnixTime(uint32_t unixTime);

    /**
     * @brief Get the current Unix timestamp
     * @return Unix timestamp in seconds, or 0 if not synchronized
     */
    uint32_t getUnixTime() const;

    /**
     * @brief Check if the system time has been synchronized
     * @return true if time is synchronized, false if using uptime
     */
    bool isTimeSynchronized() const;

    /**
     * @brief Load configuration from NVS storage
     */
    void loadConfig();

    /**
     * @brief Save configuration to NVS storage
     */
    void saveConfig();

    /**
     * @brief Get current configuration
     * @return SensorConfig structure with current settings
     */
    SensorConfig getConfig() const;

    /**
     * @brief Set measurement interval
     * @param seconds Interval in seconds between measurements
     */
    void setMeasurementInterval(uint16_t seconds);

    /**
     * @brief Set logging interval
     * @param seconds Interval in seconds between log entries (0 = every measurement)
     */
    void setLoggingInterval(uint16_t seconds);

    /**
     * @brief Get measurement interval
     * @return Current measurement interval in seconds
     */
    uint16_t getMeasurementInterval() const;

    /**
     * @brief Get logging interval
     * @return Current logging interval in seconds
     */
    uint16_t getLoggingInterval() const;

private:
    TwoWire &_wire;
    uint16_t _samplingInterval;
    String _logFilePath;
    SensorData _currentData;
    
    // Configuration
    SensorConfig _config;
    Preferences _preferences;
    
    // Time synchronization
    uint32_t _timeOffset;      // Offset to convert millis() to Unix time
    bool _timeSynced;          // Whether time has been synchronized
    uint32_t _syncMillis;      // millis() value when time was synced
    
    // Circular buffers for TWA calculations
    std::vector<float> _pm1_buffer;
    std::vector<float> _pm2_5_buffer;
    std::vector<float> _pm4_buffer;
    std::vector<float> _pm10_buffer;
    size_t _bufferSize;
    size_t _bufferIndex;
    bool _bufferFull;
    SensirionI2cSen66 _sensor;

    // Internal helper methods
    bool readMeasuredValues();
    
    // Environmental calculation methods
    float calculateDewPoint(float temp, float humidity);
    float calculateHeatIndex(float temp, float humidity);
    float calculateAbsoluteHumidity(float temp, float humidity);
    
    // TWA calculation helpers
    void initializeTWABuffers();
    void addToTWABuffer(std::vector<float> &buffer, float value);
    float calculateTWAFromBuffer(const std::vector<float> &buffer);
    
    // CSV logging helpers
    bool ensureLogFileExists();
    bool appendToLogFile(const String &line);
    String sensorDataToCSV(const SensorData &data);
};

#endif // SEN66_DOSIMETRY_H
