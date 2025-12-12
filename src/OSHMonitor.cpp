/**
 * ===============================================================================
 * OSHMonitor.cpp
 * 
 * Implementation of OSH-Monitor platform for environmental monitoring and dosimetry
 * 
 * Project: OSH-Monitor
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

#include "OSHMonitor.h"
#include <math.h>

OSHMonitor::OSHMonitor(TwoWire &wire, uint16_t samplingInterval)
    : _wire(wire), 
      _samplingInterval(samplingInterval),
      _logFilePath("/sensor_log.csv"),
      _rtcInitialized(false),
      _lastSyncTime(0),
      _bootTime(0),
      _sensor(nullptr),
      _pm1_fastTWA(nullptr),
      _pm2_5_fastTWA(nullptr),
      _pm4_fastTWA(nullptr),
      _pm10_fastTWA(nullptr),
      _storageWarningDisplayed(false) {
    
    memset(&_currentData, 0, sizeof(SensorData));
    memset(&_lastTWAExport, 0, sizeof(TWAExportResult));
    
    // Set default configuration
    _config.measurementInterval = DEFAULT_MEASUREMENT_INTERVAL;
    _config.loggingInterval = DEFAULT_LOGGING_INTERVAL;
    _config.samplingInterval = samplingInterval;
    
    // Initialize FastTWA instances
    initializeFastTWA();
}

OSHMonitor::~OSHMonitor() {
    delete _sensor;
    cleanupFastTWA();
}

bool OSHMonitor::begin(int sdaPin, int sclPin, uint32_t i2cFreq) {
    // Initialize SEN66 sensor via library
    _sensor = new SEN66Core(_wire);
    if (!_sensor->begin(sdaPin, sclPin, i2cFreq)) {
        Serial.println("Failed to initialize SEN66 sensor");
        Serial.println("Error: " + _sensor->getLastError());
        return false;
    }
    
    // Display serial number
    String serialNum = _sensor->getSerialNumber();
    if (serialNum.length() > 0) {
        Serial.println("SEN66 Serial Number: " + serialNum);
    }
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    
    // Load configuration from NVS
    loadConfig();
    
    // Load metadata from NVS
    loadMetadata();
    
    // Initialize RTC system
    initializeRTC();
    
    // FastTWA already initialized in constructor
    
    Serial.println("Sensor ready!");
    return true;
}

bool OSHMonitor::startMeasurement() {
    if (!_sensor || !_sensor->startMeasurement()) {
        Serial.println("[ERROR] Start measurement failed");
        if (_sensor) {
            Serial.println("Error: " + _sensor->getLastError());
        }
        return false;
    }
    return true;
}

bool OSHMonitor::stopMeasurement() {
    if (!_sensor || !_sensor->stopMeasurement()) {
        Serial.println("[ERROR] Stop measurement failed");
        if (_sensor) {
            Serial.println("Error: " + _sensor->getLastError());
        }
        return false;
    }
    return true;
}

bool OSHMonitor::readSensor() {
    if (!_sensor) {
        Serial.println("[ERROR] Sensor not initialized");
        return false;
    }
    
    // Read full sensor data (raw + derived) from SEN66Core
    SEN66FullData sensorData;
    if (!_sensor->readFullData(sensorData)) {
        Serial.println("[ERROR] Failed to read sensor data");
        Serial.println("Error: " + _sensor->getLastError());
        return false;
    }
    
    // Copy raw data to platform data structure
    _currentData.temperature = sensorData.raw.temperature;
    _currentData.humidity = sensorData.raw.humidity;
    _currentData.vocIndex = sensorData.raw.vocIndex;
    _currentData.noxIndex = sensorData.raw.noxIndex;
    _currentData.pm1_0 = sensorData.raw.pm1_0;
    _currentData.pm2_5 = sensorData.raw.pm2_5;
    _currentData.pm4_0 = sensorData.raw.pm4_0;
    _currentData.pm10 = sensorData.raw.pm10;
    _currentData.co2 = sensorData.raw.co2;
    
    // Copy derived environmental metrics
    _currentData.dewPoint = sensorData.derived.dewPoint;
    _currentData.heatIndex = sensorData.derived.heatIndex;
    _currentData.absoluteHumidity = sensorData.derived.absoluteHumidity;
    
    // Add platform timestamp
    _currentData.timestamp = getCurrentTimestamp();
    
    return true;
}

SensorData OSHMonitor::getData() const {
    return _currentData;
}

void OSHMonitor::initializeFastTWA() {
    cleanupFastTWA(); // Clean up any existing instances
    
    _pm1_fastTWA = new FastTWA(_samplingInterval);
    _pm2_5_fastTWA = new FastTWA(_samplingInterval);
    _pm4_fastTWA = new FastTWA(_samplingInterval);
    _pm10_fastTWA = new FastTWA(_samplingInterval);
}

void OSHMonitor::cleanupFastTWA() {
    delete _pm1_fastTWA;
    delete _pm2_5_fastTWA;
    delete _pm4_fastTWA;
    delete _pm10_fastTWA;
    
    _pm1_fastTWA = nullptr;
    _pm2_5_fastTWA = nullptr;
    _pm4_fastTWA = nullptr;
    _pm10_fastTWA = nullptr;
}



void OSHMonitor::updateTWA(SensorData &data) {
    // Add current PM values to FastTWA instances for real-time estimates
    if (_pm1_fastTWA) {
        _pm1_fastTWA->addSample(data.pm1_0);
        data.twa_pm1_0 = _pm1_fastTWA->getCurrentTWA();
    }
    if (_pm2_5_fastTWA) {
        _pm2_5_fastTWA->addSample(data.pm2_5);
        data.twa_pm2_5 = _pm2_5_fastTWA->getCurrentTWA();
    }
    if (_pm4_fastTWA) {
        _pm4_fastTWA->addSample(data.pm4_0);
        data.twa_pm4_0 = _pm4_fastTWA->getCurrentTWA();
    }
    if (_pm10_fastTWA) {
        _pm10_fastTWA->addSample(data.pm10);
        data.twa_pm10 = _pm10_fastTWA->getCurrentTWA();
    }
}

// Environmental calculation methods moved to SEN66Core library

bool OSHMonitor::ensureLogFileExists() {
    if (!LittleFS.exists(_logFilePath)) {
        File file = LittleFS.open(_logFilePath, "w");
        if (!file) {
            return false;
        }
        
        // Write metadata as header comments (system metadata only)
        file.println("# OSH-Monitor Air Quality Data Log");
        file.println("# Device: " + getMetadata("device_name", "Unknown"));
        file.println("# Firmware Version: " + getMetadata("firmware_version", "Unknown"));
        file.println("# Session Start: " + getMetadata("session_start", "Not Set"));
        file.println("#");
        
        // Build dynamic CSV header with ALL non-system metadata as columns
        String header = "timestamp,local_time";
        
        // Add all metadata fields as columns (except system fields)
        for (const auto &pair : _metadata) {
            if (pair.first != "device_name" && pair.first != "firmware_version" && pair.first != "session_start") {
                header += "," + pair.first;
            }
        }
        
        // Add sensor data columns
        header += ",temperature,humidity,vocIndex,noxIndex,pm1_0,pm2_5,pm4_0,pm10,co2,dewPoint,heatIndex,absoluteHumidity,twa_pm1_0,twa_pm2_5,twa_pm4_0,twa_pm10";
        
        file.println(header);
        file.close();
    }
    
    return true;
}

String OSHMonitor::sensorDataToCSV(const SensorData &data) {
    String line = "";
    line += String(data.timestamp);
    line += "," + formatLocalTime(data.timestamp);
    
    // Add ALL metadata values as columns (in same order as header, excluding system fields)
    for (const auto &pair : _metadata) {
        if (pair.first != "device_name" && pair.first != "firmware_version" && pair.first != "session_start") {
            line += "," + pair.second;
        }
    }
    
    // Add sensor data
    line += "," + String(data.temperature, 2) + ",";
    line += String(data.humidity, 2) + ",";
    line += String(data.vocIndex, 1) + ",";
    line += String(data.noxIndex, 1) + ",";
    line += String(data.pm1_0, 2) + ",";
    line += String(data.pm2_5, 2) + ",";
    line += String(data.pm4_0, 2) + ",";
    line += String(data.pm10, 2) + ",";
    line += String(data.co2, 1) + ",";
    line += String(data.dewPoint, 2) + ",";
    line += String(data.heatIndex, 2) + ",";
    line += String(data.absoluteHumidity, 3) + ",";
    line += String(data.twa_pm1_0, 2) + ",";
    line += String(data.twa_pm2_5, 2) + ",";
    line += String(data.twa_pm4_0, 2) + ",";
    line += String(data.twa_pm10, 2);
    
    return line;
}

bool OSHMonitor::appendToLogFile(const String &line) {
    File file = LittleFS.open(_logFilePath, "a");
    if (!file) {
        return false;
    }
    
    file.println(line);
    file.close();
    
    return true;
}

bool OSHMonitor::logEntry(const SensorData &data) {
    if (!ensureLogFileExists()) {
        return false;
    }
    
    String csvLine = sensorDataToCSV(data);
    bool success = appendToLogFile(csvLine);
    
    // Check storage and warn if threshold exceeded (only once)
    if (success && !_storageWarningDisplayed) {
        StorageStats stats = getStorageStats();
        if (stats.percentUsed >= _config.storageWarningThreshold) {
            Serial.println("[WARNING] Storage threshold exceeded!");
            Serial.printf("  Used: %s / %s (%.1f%%)\n", 
                formatBytes(stats.usedBytes).c_str(),
                formatBytes(stats.totalBytes).c_str(),
                stats.percentUsed);
            Serial.printf("  Estimated time remaining: %.1f hours\n", stats.estimatedHoursRemaining);
            _storageWarningDisplayed = true;
        }
    }
    
    return success;
}

bool OSHMonitor::eraseLogs() {
    if (LittleFS.exists(_logFilePath)) {
        return LittleFS.remove(_logFilePath);
    }
    return true;
}

bool OSHMonitor::readLogLine(size_t index, String &line) {
    File file = LittleFS.open(_logFilePath, "r");
    if (!file) {
        return false;
    }
    
    size_t currentLine = 0;
    while (file.available()) {
        String readLine = file.readStringUntil('\n');
        if (currentLine == index) {
            line = readLine;
            file.close();
            return true;
        }
        currentLine++;
    }
    
    file.close();
    return false;
}

size_t OSHMonitor::getLogLineCount() {
    File file = LittleFS.open(_logFilePath, "r");
    if (!file) {
        return 0;
    }
    
    size_t count = 0;
    while (file.available()) {
        file.readStringUntil('\n');
        count++;
    }
    
    file.close();
    return count;
}

void OSHMonitor::setLogFilePath(const String &path) {
    _logFilePath = path;
}



uint32_t OSHMonitor::getUnixTime() const {
    // Use the new timestamp method which prioritizes RTC
    return const_cast<OSHMonitor*>(this)->getCurrentTimestamp();
}

bool OSHMonitor::isTimeSynchronized() const {
    // Only consider synchronized if RTC is initialized
    return _rtcInitialized;
}

void OSHMonitor::loadConfig() {
    _preferences.begin("osh-mon", false);  // false = read/write
    
    // Load configuration with defaults if not set
    _config.measurementInterval = _preferences.getUShort("measInterval", DEFAULT_MEASUREMENT_INTERVAL);
    _config.loggingInterval = _preferences.getUShort("logInterval", DEFAULT_LOGGING_INTERVAL);
    _config.samplingInterval = _preferences.getUShort("sampInterval", _samplingInterval);
    _config.utcOffset = _preferences.getShort("utcOffset", 0);
    _config.storageWarningThreshold = _preferences.getUChar("stor_warn", 80);
    
    _preferences.end();
    
    Serial.println("Configuration loaded:");
    Serial.printf("  Measurement Interval: %d seconds\n", _config.measurementInterval);
    Serial.printf("  Logging Interval: %d seconds\n", _config.loggingInterval);
    Serial.printf("  Sampling Interval: %d seconds\n", _config.samplingInterval);
    Serial.printf("  UTC Offset: %+d hours\n", _config.utcOffset);
    Serial.printf("  Storage Warning: %d%%\n", _config.storageWarningThreshold);
}

void OSHMonitor::saveConfig() {
    _preferences.begin("osh-mon", false);
    
    _preferences.putUShort("measInterval", _config.measurementInterval);
    _preferences.putUShort("logInterval", _config.loggingInterval);
    _preferences.putUShort("sampInterval", _config.samplingInterval);
    _preferences.putShort("utcOffset", _config.utcOffset);
    _preferences.putUChar("stor_warn", _config.storageWarningThreshold);
    
    _preferences.end();
    
    Serial.println("Configuration saved to NVS");
}

SensorConfig OSHMonitor::getConfig() const {
    return _config;
}

void OSHMonitor::setMeasurementInterval(uint16_t seconds) {
    if (seconds < 1) seconds = 1;  // Minimum 1 second
    _config.measurementInterval = seconds;
    Serial.printf("Measurement interval set to %d seconds\n", seconds);
}

void OSHMonitor::setLoggingInterval(uint16_t seconds) {
    _config.loggingInterval = seconds;
    Serial.printf("Logging interval set to %d seconds (0 = every measurement)\n", seconds);
}

uint16_t OSHMonitor::getMeasurementInterval() const {
    return _config.measurementInterval;
}

uint16_t OSHMonitor::getLoggingInterval() const {
    return _config.loggingInterval;
}

void OSHMonitor::setUtcOffset(int16_t offset) {
    // Clamp to reasonable UTC offset range (-12 to +14 hours)
    if (offset < -12) offset = -12;
    if (offset > 14) offset = 14;
    
    _config.utcOffset = offset;
    Serial.printf("UTC offset set to %+d hours\n", offset);
}

int16_t OSHMonitor::getUtcOffset() const {
    return _config.utcOffset;
}

String OSHMonitor::formatLocalTime(uint32_t unixTime) const {
    // Apply UTC offset (convert hours to seconds)
    int32_t localTime = unixTime + (_config.utcOffset * 3600);
    
    // Convert to date/time components
    // Note: This is a simplified calculation for demonstration
    // For production use, consider a proper time library
    
    int32_t days = localTime / 86400;  // 86400 seconds per day
    int32_t seconds = localTime % 86400;
    
    int32_t hours = seconds / 3600;
    seconds %= 3600;
    int32_t minutes = seconds / 60;
    seconds %= 60;
    
    // Calculate date (simplified - assumes Unix epoch start)
    // This is a basic calculation; real implementation would handle leap years properly
    int32_t year = 1970;
    int32_t daysInYear = 365;
    
    while (days >= daysInYear) {
        days -= daysInYear;
        year++;
        // Simplified leap year check
        daysInYear = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 366 : 365;
    }
    
    // Month calculation (simplified)
    int32_t month = 1;
    int32_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    // Adjust February for leap years
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth[1] = 29;
    }
    
    while (days >= daysInMonth[month - 1]) {
        days -= daysInMonth[month - 1];
        month++;
        if (month > 12) {
            month = 1;
            year++;
        }
    }
    
    int32_t day = days + 1;  // Convert from 0-based to 1-based
    
    // Format as yyyy-mm-dd_hh:mm:ss
    char buffer[20];
snprintf(buffer, sizeof(buffer), "%04ld-%02ld-%02ld_%02ld:%02ld:%02ld",
             year, month, day, hours, minutes, seconds);
    
    return String(buffer);
}

// ==================== Metadata Management ====================

void OSHMonitor::loadMetadata() {
    _preferences.begin("osh-meta", true);  // Read-only
    
    // Load all stored metadata keys
    // ESP32 Preferences doesn't have a way to list all keys, so we'll try common ones
    // and store them in a separate list key
    String keysList = _preferences.getString("_keys", "");
    
    if (keysList.length() > 0) {
        int start = 0;
        int end = keysList.indexOf(',');
        
        while (end >= 0) {
            String key = keysList.substring(start, end);
            String value = _preferences.getString(key.c_str(), "");
            if (value.length() > 0) {
                _metadata[key] = value;
            }
            start = end + 1;
            end = keysList.indexOf(',', start);
        }
        
        // Get last key (no trailing comma)
        if (start < keysList.length()) {
            String key = keysList.substring(start);
            String value = _preferences.getString(key.c_str(), "");
            if (value.length() > 0) {
                _metadata[key] = value;
            }
        }
    }
    
    _preferences.end();
    
    // Set default metadata if not present
    if (_metadata.find("device_name") == _metadata.end()) {
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        _metadata["device_name"] = "OSH-" + String(macStr).substring(9); // Last 3 octets
    }
    
    if (_metadata.find("firmware_version") == _metadata.end()) {
        _metadata["firmware_version"] = "1.1.0";
    }
    
    if (_metadata.find("session_start") == _metadata.end() || _metadata["session_start"].length() == 0) {
        if (_rtcInitialized) {
            _metadata["session_start"] = String(getRTCTime());
        } else {
            _metadata["session_start"] = "NOT_SYNCED";
        }
        saveMetadata();  // Save session start immediately
    }
    
    Serial.println("Metadata loaded from NVS");
}

void OSHMonitor::saveMetadata() {
    _preferences.begin("osh-meta", false);  // Read-write
    
    // Build keys list
    String keysList = "";
    for (const auto &pair : _metadata) {
        if (keysList.length() > 0) keysList += ",";
        keysList += pair.first;
        
        // Save each key-value pair
        _preferences.putString(pair.first.c_str(), pair.second.c_str());
    }
    
    // Save the keys list
    _preferences.putString("_keys", keysList.c_str());
    
    _preferences.end();
    
    Serial.println("Metadata saved to NVS");
}

bool OSHMonitor::shouldClearLogForMetadata(const String &key) const {
    // Only firmware_version and session_start are true system fields that shouldn't prompt
    // device_name should prompt since users expect confirmation when changing device identity
    return !(key == "firmware_version" || key == "session_start");
}

void OSHMonitor::setMetadata(const String &key, const String &value, bool clearLog) {
    // Check if this is the first time setting a dynamic metadata field
    bool isNewDynamicField = shouldClearLogForMetadata(key) && 
                             (_metadata.find(key) == _metadata.end() || _metadata[key].length() == 0);
    
    // Check if we're changing an existing dynamic metadata value
    bool isChangingDynamicField = shouldClearLogForMetadata(key) && 
                                  _metadata.find(key) != _metadata.end() && 
                                  _metadata[key] != value && 
                                  _metadata[key].length() > 0;
    
    _metadata[key] = value;
    saveMetadata();
    Serial.printf("Metadata set: %s = %s\n", key.c_str(), value.c_str());
    
    // Clear log file if requested or if changing dynamic metadata
    if (clearLog && LittleFS.exists(_logFilePath)) {
        Serial.println("⚠ Clearing existing log file due to metadata change...");
        LittleFS.remove(_logFilePath);
        Serial.println("✓ Log file cleared. New log will include updated metadata.");
    } else if (isNewDynamicField || isChangingDynamicField) {
        Serial.println();
        Serial.println("⚠ WARNING: You are changing metadata that appears in CSV columns.");
        Serial.println("   The existing log file should be cleared to maintain data consistency.");
        Serial.println("   Use the 'clear' command to remove the old log file.");
        Serial.println("   Or download it first with the 'dump' command.");
    }
}

String OSHMonitor::getMetadata(const String &key, const String &defaultValue) const {
    auto it = _metadata.find(key);
    if (it != _metadata.end()) {
        return it->second;
    }
    return defaultValue;
}

std::vector<String> OSHMonitor::getMetadataKeys() const {
    std::vector<String> keys;
    for (const auto &pair : _metadata) {
        keys.push_back(pair.first);
    }
    return keys;
}

void OSHMonitor::setUser(const String &user) {
    setMetadata("user", user);
}

void OSHMonitor::setProject(const String &project) {
    setMetadata("project", project);
}

void OSHMonitor::setLocation(const String &location) {
    setMetadata("location", location);
}

bool OSHMonitor::resetMetadata() {
    // Clear all non-system metadata from the map
    auto it = _metadata.begin();
    while (it != _metadata.end()) {
        if (it->first != "device_name" && it->first != "firmware_version" && it->first != "session_start") {
            it = _metadata.erase(it);
        } else {
            ++it;
        }
    }
    
    // Set default metadata keys with NOT_SET values
    _metadata["user"] = "NOT_SET";
    _metadata["project"] = "NOT_SET";
    _metadata["location"] = "NOT_SET";
    
    // Save to NVS
    saveMetadata();
    
    return true;
}

// Export CSV with OSHA-compliant TWA calculations
bool OSHMonitor::exportCSVWithTWA(const String &filename) {
    // Read existing CSV data
    File logFile = LittleFS.open(_logFilePath, "r");
    if (!logFile) {
        Serial.println("[ERROR] Cannot open log file for TWA export");
        return false;
    }
    
    String csvData = logFile.readString();
    logFile.close();
    
    // Create ExportTWA instance with PM parameters
    std::vector<String> pmParameters = {"pm1_0", "pm2_5", "pm4_0", "pm10"};
    ExportTWA twaCalculator(_config.samplingInterval, pmParameters, _config.utcOffset);
    
    // Calculate TWA using ExportTWA
    _lastTWAExport = twaCalculator.calculateFromCSV(csvData);
    
    // Create export file
    File exportFile = LittleFS.open(filename, "w");
    if (!exportFile) {
        Serial.println("[ERROR] Cannot create TWA export file");
        return false;
    }
    
    // Write enhanced header with TWA calculation results
    if (!writeExportHeader(exportFile)) {
        exportFile.close();
        return false;
    }
    
    // Parse and copy only the CSV data (skip all original comments and headers)
    int lineStart = 0;
    int lineEnd = csvData.indexOf('\n');
    bool csvHeaderWritten = false;
    while (lineEnd != -1) {
        String line = csvData.substring(lineStart, lineEnd);
        line.trim();
        
        // Skip ALL original header comments and the original CSV header line
        if (line.startsWith("#") || line.startsWith("timestamp,")) {
            // Skip - we've already written our enhanced header
        } else if (line.length() > 0) {
            // This is a data row - write it
            if (!csvHeaderWritten) {
                // Write our own CSV header first
                exportFile.println("timestamp,local_time,location,project,user,temperature,humidity,vocIndex,noxIndex,pm1_0,pm2_5,pm4_0,pm10,co2,dewPoint,heatIndex,absoluteHumidity,twa_pm1_0,twa_pm2_5,twa_pm4_0,twa_pm10");
                csvHeaderWritten = true;
            }
            exportFile.println(line);
        }
        
        lineStart = lineEnd + 1;
        lineEnd = csvData.indexOf('\n', lineStart);
    }
    
    // Handle the last line if it doesn't end with newline
    if (lineStart < csvData.length()) {
        String lastLine = csvData.substring(lineStart);
        lastLine.trim();
        if (!lastLine.startsWith("#") && !lastLine.startsWith("timestamp,") && lastLine.length() > 0) {
            if (!csvHeaderWritten) {
                exportFile.println("timestamp,local_time,location,project,user,temperature,humidity,vocIndex,noxIndex,pm1_0,pm2_5,pm4_0,pm10,co2,dewPoint,heatIndex,absoluteHumidity,twa_pm1_0,twa_pm2_5,twa_pm4_0,twa_pm10");
                csvHeaderWritten = true;
            }
            exportFile.println(lastLine);
        }
    }
    
    exportFile.close();
    return true;
}

// Get last TWA export result
TWAExportResult OSHMonitor::getLastTWAExport() const {
    return _lastTWAExport;
}

// Parse CSV line helper method
bool OSHMonitor::parseCSVLine(const String &line, SensorData &data, unsigned long &timestamp) {
    // Skip header lines or empty lines
    if (line.startsWith("timestamp") || line.length() == 0) {
        return false;
    }
    
    // Simple CSV parsing - find commas and extract values
    int commaCount = 0;
    int lastIndex = 0;
    
    for (int i = 0; i <= line.length(); i++) {
        if (i == line.length() || line.charAt(i) == ',') {
            String field = line.substring(lastIndex, i);
            field.trim();
            
            switch (commaCount) {
                case 0: timestamp = field.toInt(); break;
                case 2: data.pm1_0 = field.toFloat(); break;
                case 3: data.pm2_5 = field.toFloat(); break;
                case 4: data.pm4_0 = field.toFloat(); break;
                case 5: data.pm10 = field.toFloat(); break;
            }
            
            commaCount++;
            lastIndex = i + 1;
        }
    }
    
    return commaCount >= 6; // Ensure we have enough fields
}

// Calculate time-weighted TWA
float OSHMonitor::calculateTimeWeightedTWA(const std::vector<std::pair<unsigned long, float>> &dataPoints, 
                                               unsigned long periodStart, unsigned long periodEnd,
                                               unsigned long &gaps) {
    if (dataPoints.empty()) return 0.0f;
    
    float weightedSum = 0.0f;
    unsigned long totalTime = 0;
    gaps = 0;
    
    for (size_t i = 0; i < dataPoints.size() - 1; i++) {
        unsigned long currentTime = dataPoints[i].first;
        unsigned long nextTime = dataPoints[i + 1].first;
        float currentValue = dataPoints[i].second;
        
        // Skip if outside our period
        if (currentTime < periodStart || currentTime >= periodEnd) continue;
        
        unsigned long duration = nextTime - currentTime;
        
        // Check for data gaps (> 2x measurement interval)
        if (duration > _config.measurementInterval * 2) {
            gaps++;
        }
        
        weightedSum += currentValue * duration;
        totalTime += duration;
    }
    
    return (totalTime > 0) ? weightedSum / totalTime : 0.0f;
}

// Write OSHA-compliant export header
bool OSHMonitor::writeExportHeader(File &file) {
    
    file.println("# OSHA-Compliant 8-Hour Time-Weighted Average Report");
    file.println("# Generated by OSH-Monitor System");
    file.printf("# Export Time: %s\n", formatLocalTime(time(nullptr)).c_str());
    file.printf("# Period Start: %s\n", _lastTWAExport.exportStartTime.c_str());
    file.printf("# Period End: %s\n", _lastTWAExport.exportEndTime.c_str());
    file.println("# Reference: OSHA 29 CFR 1910.1000");
    file.println("#");
    file.println("# ========== TWA CALCULATION RESULTS ==========");
    file.printf("# Data Coverage: %.1f hours\n", _lastTWAExport.dataCoverageHours);
    file.printf("# OSHA Compliant: %s\n", _lastTWAExport.oshaCompliant ? "YES (≥8 hours)" : "NO (< 8 hours - insufficient data)");
    
    // Access TWA values from map
    auto it_pm1 = _lastTWAExport.parameterTWAs.find("pm1_0");
    auto it_pm25 = _lastTWAExport.parameterTWAs.find("pm2_5");
    auto it_pm4 = _lastTWAExport.parameterTWAs.find("pm4_0");
    auto it_pm10 = _lastTWAExport.parameterTWAs.find("pm10");
    
    if (it_pm1 != _lastTWAExport.parameterTWAs.end()) {
        file.printf("# PM1.0 8-hr TWA: %.3f µg/m³\n", it_pm1->second);
    }
    if (it_pm25 != _lastTWAExport.parameterTWAs.end()) {
        file.printf("# PM2.5 8-hr TWA: %.3f µg/m³\n", it_pm25->second);
    }
    if (it_pm4 != _lastTWAExport.parameterTWAs.end()) {
        file.printf("# PM4.0 8-hr TWA: %.3f µg/m³\n", it_pm4->second);
    }
    if (it_pm10 != _lastTWAExport.parameterTWAs.end()) {
        file.printf("# PM10 8-hr TWA: %.3f µg/m³\n", it_pm10->second);
    }
    
    file.printf("# Samples Analyzed: %lu\n", _lastTWAExport.samplesAnalyzed);
    file.printf("# Data Gaps Detected: %lu\n", _lastTWAExport.dataGaps);
    if (_lastTWAExport.dataCoverageHours < MIN_OSHA_HOURS) {
        file.println("#");
        file.println("# WARNING: Insufficient data for OSHA compliance");
        file.printf("# OSHA requires minimum %.0f hours of data\n", MIN_OSHA_HOURS);
    }
    file.println("# ===============================================");
    file.println("#");
    
    return true;
}

// ===============================================================================
// TWA implementations now provided by TWACore library
// ===============================================================================

// ============ RTC Implementation Functions ============

void OSHMonitor::initializeRTC() {
    Serial.println("[RTC] Initializing ESP32-S3 RTC...");
    
    // Check if RTC time is reasonable (after 2024)
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        if (tv.tv_sec > 1704067200) {  // January 1, 2024
            _rtcInitialized = true;
            _bootTime = tv.tv_sec;
            Serial.printf("[RTC] RTC already initialized with valid time: %s", ctime((time_t*)&tv.tv_sec));
        } else {
            Serial.printf("[RTC] RTC time invalid (%ld), needs synchronization\n", tv.tv_sec);
            _rtcInitialized = false;
        }
    } else {
        Serial.println("[RTC] Failed to read RTC time");
        _rtcInitialized = false;
    }
    
    _lastSyncTime = 0;
}

bool OSHMonitor::setRTCTime(unsigned long epochTime) {
    Serial.printf("[RTC] Setting RTC time to: %lu (%s", epochTime, ctime((time_t*)&epochTime));
    
    struct timeval tv;
    tv.tv_sec = epochTime;
    tv.tv_usec = 0;
    
    if (settimeofday(&tv, NULL) == 0) {
        _rtcInitialized = true;
        _lastSyncTime = epochTime;
        _bootTime = epochTime;
        Serial.println("[RTC] RTC time set successfully");
        return true;
    } else {
        Serial.println("[RTC] Failed to set RTC time");
        return false;
    }
}

unsigned long OSHMonitor::getRTCTime() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0) {
        return tv.tv_sec;
    } else {
        Serial.println("[RTC] Failed to read RTC time");
        return 0;
    }
}

bool OSHMonitor::isRTCInitialized() {
    return _rtcInitialized;
}

bool OSHMonitor::needsRTCSync() {
    if (!_rtcInitialized) {
        return true;
    }
    
    unsigned long currentTime = getRTCTime();
    unsigned long timeSinceSync = currentTime - _lastSyncTime;
    
    // Suggest sync if more than RTC_SYNC_INTERVAL_HOURS hours have passed
    return (timeSinceSync > (RTC_SYNC_INTERVAL_HOURS * 3600));
}

TimeSource OSHMonitor::getTimeSource() {
    if (_rtcInitialized) {
        return TIME_SOURCE_RTC;
    } else {
        return TIME_SOURCE_UPTIME;
    }
}

unsigned long OSHMonitor::getCurrentTimestamp() {
    if (_rtcInitialized) {
        return getRTCTime();
    } else {
        // Fallback to millis-based timestamp if RTC not available
        return millis() / 1000;
    }
}

String OSHMonitor::getRTCStatus() {
    String status = "RTC Status:\n";
    status += "  Initialized: " + String(_rtcInitialized ? "YES" : "NO") + "\n";
    
    if (_rtcInitialized) {
        unsigned long currentTime = getRTCTime();
        status += "  Current Time: " + String(currentTime) + " (" + String(ctime((time_t*)&currentTime)) + ")\n";
        
        if (_lastSyncTime > 0) {
            status += "  Last Sync: " + String(_lastSyncTime) + " (" + String(ctime((time_t*)&_lastSyncTime)) + ")\n";
            unsigned long timeSinceSync = currentTime - _lastSyncTime;
            status += "  Time Since Sync: " + String(timeSinceSync) + " seconds\n";
        } else {
            status += "  Last Sync: Never\n";
        }
        
        status += "  Needs Sync: " + String(needsRTCSync() ? "YES" : "NO") + "\n";
    }
    
    TimeSource source = getTimeSource();
    switch (source) {
        case TIME_SOURCE_RTC:
            status += "  Active Source: RTC Time\n";
            break;
        case TIME_SOURCE_UPTIME:
            status += "  Active Source: Millis Only (no RTC)\n";
            break;
        default:
            status += "  Active Source: Unknown\n";
            break;
    }
    
    return status;
}

// ============================================================================
// Storage Monitoring Functions
// ============================================================================

size_t OSHMonitor::calculateAverageBytesPerEntry() {
    File logFile = LittleFS.open(_logFilePath, "r");
    if (!logFile) {
        return 0;
    }
    
    size_t fileSize = logFile.size();
    size_t lineCount = 0;
    
    // Count lines in file
    while (logFile.available()) {
        String line = logFile.readStringUntil('\n');
        lineCount++;
    }
    logFile.close();
    
    // Subtract header line (lineCount includes header)
    if (lineCount <= 1) {
        return 0;  // No data entries yet, use default estimate
    }
    
    size_t dataLines = lineCount - 1;
    return fileSize / dataLines;
}

StorageStats OSHMonitor::getStorageStats() {
    StorageStats stats;
    
    // Get LittleFS capacity information
    stats.totalBytes = LittleFS.totalBytes();
    stats.usedBytes = LittleFS.usedBytes();
    stats.freeBytes = stats.totalBytes - stats.usedBytes;
    stats.percentUsed = (float)stats.usedBytes / (float)stats.totalBytes * 100.0;
    
    // Calculate average bytes per entry from actual log file
    stats.averageBytesPerEntry = calculateAverageBytesPerEntry();
    
    // If no average available, use conservative estimate (150 bytes)
    if (stats.averageBytesPerEntry == 0) {
        stats.averageBytesPerEntry = 150;
    }
    
    // Calculate estimated hours remaining
    // Account for TWA export overhead (1.5x multiplier)
    size_t effectiveBytesPerEntry = stats.averageBytesPerEntry * 1.5;
    
    if (effectiveBytesPerEntry > 0 && _config.loggingInterval > 0) {
        // Calculate entries that can fit in remaining space
        size_t remainingEntries = stats.freeBytes / effectiveBytesPerEntry;
        
        // Convert to hours based on logging interval
        stats.estimatedHoursRemaining = (float)remainingEntries * _config.loggingInterval / 3600.0;
    } else {
        stats.estimatedHoursRemaining = 0.0;
    }
    
    return stats;
}

String OSHMonitor::formatBytes(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return String(bytes / 1024.0, 2) + " KB";
    } else {
        return String(bytes / (1024.0 * 1024.0), 2) + " MB";
    }
}

void OSHMonitor::setStorageWarningThreshold(uint8_t percent) {
    // Clamp to valid range (1-99)
    if (percent < 1) percent = 1;
    if (percent > 99) percent = 99;
    
    _config.storageWarningThreshold = percent;
    saveConfig();
}

uint8_t OSHMonitor::getStorageWarningThreshold() const {
    return _config.storageWarningThreshold;
}

