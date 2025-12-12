/**
 * ===============================================================================
 * SEN66Dosimetry.h
 * 
 * Open-Source Arduino/PlatformIO Library for ESP32-S3
 * Advanced Air-Quality Acquisition, Dosimetry, and CSV Logging for the Sensirion SEN66
 * 
 * Project: SEN66-Dosimetry
 * Creator: Christopher Lee
 * License: GNU General Public License v3.0 (GPLv3)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * ===============================================================================
 */

#ifndef SEN66_DOSIMETRY_H
#define SEN66_DOSIMETRY_H

#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>
#include <SensirionI2cSen66.h>
#include <Preferences.h>
#include <vector>
#include <map>
#include <time.h>
#include <sys/time.h>
#include <TWACore.h>

// Default sampling interval (seconds) for TWA calculations
#define DEFAULT_SAMPLING_INTERVAL 60
#define DEFAULT_MEASUREMENT_INTERVAL 20  // seconds
#define DEFAULT_LOGGING_INTERVAL 20  // seconds
#define TWA_WINDOW_SECONDS (8 * 3600)  // 8 hours in seconds
#define MIN_OSHA_HOURS 8.0              // OSHA minimum data requirement

// RTC Configuration
#define RTC_SYNC_INTERVAL_HOURS 24      // Re-sync every 24 hours
#define RTC_DRIFT_WARNING_PPM 100       // Warn if drift >100 ppm

/**
 * @brief Time source enumeration for timestamp generation
 */
enum TimeSource {
    TIME_SOURCE_RTC,           // ESP32 RTC (preferred)
    TIME_SOURCE_SYNCED,        // Legacy synced time (millis + offset)
    TIME_SOURCE_UPTIME,        // Fallback uptime (millis/1000)
    TIME_SOURCE_INVALID        // No valid time source
};
// TWAExportResult and TWA classes now provided by TWACore library

/**
 * @brief Configuration structure for sensor operation
 */
struct SensorConfig {
    uint16_t measurementInterval;  // Seconds between measurements
    uint16_t loggingInterval;      // Seconds between log entries (0 = log every measurement)
    uint16_t samplingInterval;     // Seconds for TWA calculations
    int16_t utcOffset;             // UTC offset in hours (-12 to +14)
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
     * @brief Destructor - cleanup FastTWA instances
     */
    ~SEN66Dosimetry();

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
     * @brief Update 8-hour TWA calculations with new sensor data (real-time estimate)
     * @param data Reference to SensorData to update with TWA values
     */
    void updateTWA(SensorData &data);

    /**
     * @brief Export CSV with OSHA-compliant 8-hour TWA calculations
     * @param filename Output filename for export
     * @return true if export successful, false otherwise
     */
    bool exportCSVWithTWA(const String &filename);

    /**
     * @brief Get the result of the last TWA export operation
     * @return TWAExportResult structure with calculation details
     */
    TWAExportResult getLastTWAExport() const;

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

    /**
     * @brief Set UTC offset for localized timestamps
     * @param offset UTC offset in hours (-12 to +14)
     */
    void setUtcOffset(int16_t offset);

    /**
     * @brief Get UTC offset
     * @return Current UTC offset in hours
     */
    int16_t getUtcOffset() const;

    /**
     * @brief Convert Unix timestamp to localized human-readable format
     * @param unixTime Unix timestamp
     * @return Formatted string in yyyy-mm-dd_hh:mm:ss format
     */
    String formatLocalTime(uint32_t unixTime) const;

    // RTC Management Functions
    /**
     * @brief Initialize ESP32 RTC with system time
     * @return True if RTC initialized successfully
     */
    void initializeRTC();

    /**
     * @brief Set RTC time from Unix timestamp
     * @param unixTime Unix timestamp to set
     * @return True if time set successfully
     */
    bool setRTCTime(unsigned long unixTime);

    /**
     * @brief Get current Unix timestamp from RTC
     * @return Current Unix timestamp, 0 if RTC not available
     */
    unsigned long getRTCTime();

    /**
     * @brief Check if RTC is initialized
     * @return True if RTC is initialized and working
     */
    bool isRTCInitialized();
    
    /**
     * @brief Get current timestamp using best available source
     * @return Current Unix timestamp from RTC, legacy sync, or millis
     */
    unsigned long getCurrentTimestamp();
    
    /**
     * @brief Get RTC status as formatted string
     * @return String containing RTC status information
     */
    String getRTCStatus();
    
    /**
     * @brief Check if RTC needs synchronization
     * @return True if sync recommended (>24h since last sync)
     */
    bool needsRTCSync();

    /**
     * @brief Get current time source being used
     * @return TimeSource enumeration
     */
    TimeSource getTimeSource();

    /**
     * @brief Get time quality information string
     * @return Descriptive string of current time source and sync status
     */
    String getTimeQualityInfo();

    // Metadata management
    /**
     * @brief Set a metadata key-value pair
     * @param key Metadata key
     * @param value Metadata value
     * @param clearLog If true, will clear existing log file (changes CSV structure)
     */
    void setMetadata(const String &key, const String &value, bool clearLog = false);
    
    /**
     * @brief Get a metadata value by key
     * @param key Metadata key
     * @param defaultValue Default value if key not found
     * @return Metadata value or default
     */
    String getMetadata(const String &key, const String &defaultValue = "") const;
    
    /**
     * @brief Check if log file needs to be cleared for metadata change
     * @param key Metadata key being changed
     * @return True if log file should be cleared
     */
    bool shouldClearLogForMetadata(const String &key) const;
    
    /**
     * @brief Get all metadata keys
     * @return Vector of all metadata keys
     */
    std::vector<String> getMetadataKeys() const;
    
    /**
     * @brief Convenience method to set user metadata
     * @param user User name
     */
    void setUser(const String &user);
    
    /**
     * @brief Convenience method to set project metadata
     * @param project Project name
     */
    void setProject(const String &project);
    
    /**
     * @brief Convenience method to set location metadata
     * @param location Location description
     */
    void setLocation(const String &location);
    
    /**
     * @brief Reset metadata to default state (user, project, location with empty values)
     * Deletes all non-system metadata key-value pairs
     * @return true if successful
     */
    bool resetMetadata();

private:
    TwoWire &_wire;
    uint16_t _samplingInterval;
    String _logFilePath;
    SensorData _currentData;
    
    // Configuration
    SensorConfig _config;
    Preferences _preferences;
    
    // Metadata storage
    std::map<String, String> _metadata;
    

    
    // RTC Management
    bool _rtcInitialized;      // Whether ESP32 RTC is initialized
    uint32_t _lastSyncTime;    // Last external sync timestamp (Unix time)
    uint32_t _bootTime;        // System boot time (Unix timestamp)
    
    // Hybrid TWA calculation system
    FastTWA* _pm1_fastTWA;
    FastTWA* _pm2_5_fastTWA;
    FastTWA* _pm4_fastTWA;
    FastTWA* _pm10_fastTWA;
    TWAExportResult _lastTWAExport;
    SensirionI2cSen66 _sensor;

    // Internal helper methods
    bool readMeasuredValues();
    
    // Environmental calculation methods
    float calculateDewPoint(float temp, float humidity);
    float calculateHeatIndex(float temp, float humidity);
    float calculateAbsoluteHumidity(float temp, float humidity);
    
    // TWA calculation helpers
    void initializeFastTWA();
    void cleanupFastTWA();
    bool parseCSVLine(const String &line, SensorData &data, unsigned long &timestamp);
    float calculateTimeWeightedTWA(const std::vector<std::pair<unsigned long, float>> &dataPoints, 
                                   unsigned long periodStart, unsigned long periodEnd,
                                   unsigned long &gaps);
    bool writeExportHeader(File &file);
    
    // CSV logging helpers
    bool ensureLogFileExists();
    bool appendToLogFile(const String &line);
    String sensorDataToCSV(const SensorData &data);
    
    // Metadata persistence
    void loadMetadata();
    void saveMetadata();
};


#endif // SEN66_DOSIMETRY_H
