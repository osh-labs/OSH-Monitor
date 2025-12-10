/**
 * SEN66Dosimetry.cpp
 * 
 * Implementation of SEN66 air quality sensor library with dosimetry and logging
 * 
 * MIT License
 */

#include "SEN66Dosimetry.h"
#include <math.h>

SEN66Dosimetry::SEN66Dosimetry(TwoWire &wire, uint16_t samplingInterval)
    : _wire(wire), 
      _samplingInterval(samplingInterval),
      _logFilePath("/sensor_log.csv"),
      _bufferIndex(0),
      _bufferFull(false),
      _timeOffset(0),
      _timeSynced(false),
      _syncMillis(0) {
    
    memset(&_currentData, 0, sizeof(SensorData));
    _bufferSize = TWA_WINDOW_SECONDS / _samplingInterval;
    
    // Set default configuration
    _config.measurementInterval = DEFAULT_MEASUREMENT_INTERVAL;
    _config.loggingInterval = DEFAULT_LOGGING_INTERVAL;
    _config.samplingInterval = samplingInterval;
}

bool SEN66Dosimetry::begin(int sdaPin, int sclPin, uint32_t i2cFreq) {
    // Initialize I2C
    _wire.begin(sdaPin, sclPin);
    _wire.setClock(i2cFreq);
    
    // Initialize Sensirion SEN66 sensor library
    _sensor.begin(_wire, SEN66_I2C_ADDR_6B);
    
    // Initialize LittleFS
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return false;
    }
    
    // Load configuration from NVS
    loadConfig();
    
    // Initialize TWA buffers
    initializeTWABuffers();
    
    // Reset the device
    int16_t error = _sensor.deviceReset();
    if (error != 0) {
        Serial.printf("[ERROR] Device reset failed with error: %d\n", error);
        return false;
    }
    Serial.println("SEN66 reset successful!");
    
    // Wait after reset
    delay(1200);
    
    // Get serial number for verification
    int8_t serialNumber[32] = {0};
    error = _sensor.getSerialNumber(serialNumber, 32);
    if (error == 0) {
        Serial.print("SEN66 Serial Number: ");
        Serial.println((const char*)serialNumber);
    }
    
    // Start measurement
    if (!startMeasurement()) {
        Serial.println("Failed to start SEN66 measurement");
        return false;
    }
    
    // Wait for sensor to stabilize
    Serial.println("Waiting for sensor to warm up (first measurements available in ~2 seconds)...");
    delay(2000);
    Serial.println("Sensor ready!");
    
    return true;
}

bool SEN66Dosimetry::startMeasurement() {
    int16_t error = _sensor.startContinuousMeasurement();
    if (error != 0) {
        Serial.printf("[ERROR] Start measurement failed with error: %d\n", error);
        return false;
    }
    return true;
}

bool SEN66Dosimetry::stopMeasurement() {
    int16_t error = _sensor.stopMeasurement();
    if (error != 0) {
        Serial.printf("[ERROR] Stop measurement failed with error: %d\n", error);
        return false;
    }
    return true;
}

bool SEN66Dosimetry::readMeasuredValues() {
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
        Serial.printf("[ERROR] Failed to read sensor values, error code: %d\n", error);
        return false;
    }
    
    // Store values in current data structure
    _currentData.pm1_0 = pm1_0;
    _currentData.pm2_5 = pm2_5;
    _currentData.pm4_0 = pm4_0;
    _currentData.pm10 = pm10;
    _currentData.humidity = humidity;
    _currentData.temperature = temperature;
    _currentData.vocIndex = vocIndex;
    _currentData.noxIndex = noxIndex;
    _currentData.co2 = co2;
    
    // Calculate timestamp: use Unix time if synced, otherwise use uptime
    if (_timeSynced) {
        uint32_t elapsedSeconds = (millis() - _syncMillis) / 1000;
        _currentData.timestamp = _timeOffset + elapsedSeconds;
    } else {
        _currentData.timestamp = millis() / 1000;
    }
    
    return true;
}

bool SEN66Dosimetry::readSensor() {
    if (!readMeasuredValues()) {
        return false;
    }
    
    // Calculate derived environmental metrics
    _currentData.dewPoint = calculateDewPoint(_currentData.temperature, _currentData.humidity);
    _currentData.heatIndex = calculateHeatIndex(_currentData.temperature, _currentData.humidity);
    _currentData.absoluteHumidity = calculateAbsoluteHumidity(_currentData.temperature, _currentData.humidity);
    
    return true;
}

SensorData SEN66Dosimetry::getData() const {
    return _currentData;
}

void SEN66Dosimetry::initializeTWABuffers() {
    _pm1_buffer.clear();
    _pm2_5_buffer.clear();
    _pm4_buffer.clear();
    _pm10_buffer.clear();
    
    _pm1_buffer.reserve(_bufferSize);
    _pm2_5_buffer.reserve(_bufferSize);
    _pm4_buffer.reserve(_bufferSize);
    _pm10_buffer.reserve(_bufferSize);
    
    _bufferIndex = 0;
    _bufferFull = false;
}

void SEN66Dosimetry::addToTWABuffer(std::vector<float> &buffer, float value) {
    if (_bufferFull) {
        // Circular buffer: overwrite oldest value
        buffer[_bufferIndex] = value;
    } else {
        // Still filling buffer
        buffer.push_back(value);
        if (buffer.size() >= _bufferSize) {
            _bufferFull = true;
        }
    }
}

float SEN66Dosimetry::calculateTWAFromBuffer(const std::vector<float> &buffer) {
    if (buffer.empty()) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (float value : buffer) {
        sum += value;
    }
    
    return sum / buffer.size();
}

void SEN66Dosimetry::updateTWA(SensorData &data) {
    // Add current PM values to buffers
    addToTWABuffer(_pm1_buffer, data.pm1_0);
    addToTWABuffer(_pm2_5_buffer, data.pm2_5);
    addToTWABuffer(_pm4_buffer, data.pm4_0);
    addToTWABuffer(_pm10_buffer, data.pm10);
    
    // Calculate TWAs
    data.twa_pm1_0 = calculateTWAFromBuffer(_pm1_buffer);
    data.twa_pm2_5 = calculateTWAFromBuffer(_pm2_5_buffer);
    data.twa_pm4_0 = calculateTWAFromBuffer(_pm4_buffer);
    data.twa_pm10 = calculateTWAFromBuffer(_pm10_buffer);
    
    // Advance buffer index for circular behavior
    if (_bufferFull) {
        _bufferIndex = (_bufferIndex + 1) % _bufferSize;
    }
}

float SEN66Dosimetry::calculateDewPoint(float temp, float humidity) {
    // Magnus formula for dew point calculation
    const float a = 17.27;
    const float b = 237.7;
    
    float alpha = ((a * temp) / (b + temp)) + log(humidity / 100.0);
    float dewPoint = (b * alpha) / (a - alpha);
    
    return dewPoint;
}

float SEN66Dosimetry::calculateHeatIndex(float temp, float humidity) {
    // Convert to Fahrenheit for NOAA formula
    float tempF = temp * 9.0 / 5.0 + 32.0;
    
    // Simple formula for low temperatures
    if (tempF < 80.0) {
        return temp;  // Heat index not applicable
    }
    
    // Steadman formula (simplified)
    float hi = -42.379 + 2.04901523 * tempF + 10.14333127 * humidity
               - 0.22475541 * tempF * humidity - 0.00683783 * tempF * tempF
               - 0.05481717 * humidity * humidity + 0.00122874 * tempF * tempF * humidity
               + 0.00085282 * tempF * humidity * humidity - 0.00000199 * tempF * tempF * humidity * humidity;
    
    // Convert back to Celsius
    return (hi - 32.0) * 5.0 / 9.0;
}

float SEN66Dosimetry::calculateAbsoluteHumidity(float temp, float humidity) {
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

bool SEN66Dosimetry::ensureLogFileExists() {
    if (!LittleFS.exists(_logFilePath)) {
        File file = LittleFS.open(_logFilePath, "w");
        if (!file) {
            return false;
        }
        
        // Write CSV header
        file.println("timestamp,temperature,humidity,vocIndex,noxIndex,pm1_0,pm2_5,pm4_0,pm10,co2,dewPoint,heatIndex,absoluteHumidity,twa_pm1_0,twa_pm2_5,twa_pm4_0,twa_pm10");
        file.close();
    }
    
    return true;
}

String SEN66Dosimetry::sensorDataToCSV(const SensorData &data) {
    String line = "";
    line += String(data.timestamp) + ",";
    line += String(data.temperature, 2) + ",";
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

bool SEN66Dosimetry::appendToLogFile(const String &line) {
    File file = LittleFS.open(_logFilePath, "a");
    if (!file) {
        return false;
    }
    
    file.println(line);
    file.close();
    
    return true;
}

bool SEN66Dosimetry::logEntry(const SensorData &data) {
    if (!ensureLogFileExists()) {
        return false;
    }
    
    String csvLine = sensorDataToCSV(data);
    return appendToLogFile(csvLine);
}

bool SEN66Dosimetry::eraseLogs() {
    if (LittleFS.exists(_logFilePath)) {
        return LittleFS.remove(_logFilePath);
    }
    return true;
}

bool SEN66Dosimetry::readLogLine(size_t index, String &line) {
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

size_t SEN66Dosimetry::getLogLineCount() {
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

void SEN66Dosimetry::setLogFilePath(const String &path) {
    _logFilePath = path;
}

void SEN66Dosimetry::setUnixTime(uint32_t unixTime) {
    _timeOffset = unixTime;
    _syncMillis = millis();
    _timeSynced = true;
    
    Serial.printf("Time synchronized: Unix time = %lu\n", unixTime);
}

uint32_t SEN66Dosimetry::getUnixTime() const {
    if (!_timeSynced) {
        return 0;  // Not synchronized
    }
    
    uint32_t elapsedSeconds = (millis() - _syncMillis) / 1000;
    return _timeOffset + elapsedSeconds;
}

bool SEN66Dosimetry::isTimeSynchronized() const {
    return _timeSynced;
}

void SEN66Dosimetry::loadConfig() {
    _preferences.begin("sen66", false);  // false = read/write
    
    // Load configuration with defaults if not set
    _config.measurementInterval = _preferences.getUShort("measInterval", DEFAULT_MEASUREMENT_INTERVAL);
    _config.loggingInterval = _preferences.getUShort("logInterval", DEFAULT_LOGGING_INTERVAL);
    _config.samplingInterval = _preferences.getUShort("sampInterval", _samplingInterval);
    
    _preferences.end();
    
    Serial.println("Configuration loaded:");
    Serial.printf("  Measurement Interval: %d seconds\n", _config.measurementInterval);
    Serial.printf("  Logging Interval: %d seconds\n", _config.loggingInterval);
    Serial.printf("  Sampling Interval: %d seconds\n", _config.samplingInterval);
}

void SEN66Dosimetry::saveConfig() {
    _preferences.begin("sen66", false);
    
    _preferences.putUShort("measInterval", _config.measurementInterval);
    _preferences.putUShort("logInterval", _config.loggingInterval);
    _preferences.putUShort("sampInterval", _config.samplingInterval);
    
    _preferences.end();
    
    Serial.println("Configuration saved to NVS");
}

SensorConfig SEN66Dosimetry::getConfig() const {
    return _config;
}

void SEN66Dosimetry::setMeasurementInterval(uint16_t seconds) {
    if (seconds < 1) seconds = 1;  // Minimum 1 second
    _config.measurementInterval = seconds;
    Serial.printf("Measurement interval set to %d seconds\n", seconds);
}

void SEN66Dosimetry::setLoggingInterval(uint16_t seconds) {
    _config.loggingInterval = seconds;
    Serial.printf("Logging interval set to %d seconds (0 = every measurement)\n", seconds);
}

uint16_t SEN66Dosimetry::getMeasurementInterval() const {
    return _config.measurementInterval;
}

uint16_t SEN66Dosimetry::getLoggingInterval() const {
    return _config.loggingInterval;
}

