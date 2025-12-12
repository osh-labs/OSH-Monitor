/**
 * ===============================================================================
 * OSH-Monitor Main Application
 * 
 * Open-source occupational safety & health monitoring platform
 * Includes real-time measurements, 8-hour TWA calculations, and CSV logging
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
 * 
 * Hardware Requirements:
 * - ESP32-S3 (Adafruit Feather ESP32-S3 Reverse TFT)
 * - Sensirion SEN66 air quality sensor
 * =============================================================================== 
 * I2C Connections:
 * - SDA: GPIO 3
 * - SCL: GPIO 4
 */

#include <Arduino.h>
#include "OSHMonitor.h"

// Firmware Version
#define FIRMWARE_VERSION "1.2.0"

// I2C Pin Definitions
#define SDA_PIN 3
#define SCL_PIN 4

// Create platform instance with 20-second sampling interval for TWA calculations
OSHMonitor airQualitySensor(Wire, 20);

// Timing variables
unsigned long lastMeasurementTime = 0;
unsigned long lastLoggingTime = 0;
unsigned long measurementCount = 0;

// Function declarations
void handleSerialCommands();
void dumpCSVFile();
void dumpTWAFile();
void listFiles();
void showHelp();
void showMetadata();

void printWelcomeBanner() {
    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════════════════════════╗");
    Serial.println("║             OSH-Monitor Air Quality System               ║");
    Serial.println("║     Environmental Monitoring & Particulate Dosimetry     ║");
    Serial.println("╚══════════════════════════════════════════════════════════╝");
    Serial.println();
}

void printSensorData(const SensorData &data) {
    Serial.printf("\n--- Measurement #%lu (Time: %lu sec) ---\n\n", measurementCount, data.timestamp);
    
    Serial.println("ENVIRONMENTAL CONDITIONS:");
    Serial.printf("  Temperature:        %.2f C\n", data.temperature);
    Serial.printf("  Humidity:           %.2f %%RH\n", data.humidity);
    Serial.printf("  Dew Point:          %.2f C\n", data.dewPoint);
    Serial.printf("  Heat Index:         %.2f C\n", data.heatIndex);
    Serial.printf("  Absolute Humidity:  %.3f g/m3\n", data.absoluteHumidity);
    
    Serial.println("\nAIR QUALITY INDICES:");
    Serial.printf("  VOC Index:          %.1f\n", data.vocIndex);
    Serial.printf("  NOx Index:          %.1f\n", data.noxIndex);
    Serial.printf("  CO2:                %.1f ppm\n", data.co2);
    
    Serial.println("\nPARTICULATE MATTER (Current):");
    Serial.printf("  PM1.0:              %.2f ug/m3\n", data.pm1_0);
    Serial.printf("  PM2.5:              %.2f ug/m3\n", data.pm2_5);
    Serial.printf("  PM4.0:              %.2f ug/m3\n", data.pm4_0);
    Serial.printf("  PM10:               %.2f ug/m3\n", data.pm10);
    
    Serial.println("\n8-HOUR TIME-WEIGHTED AVERAGE (TWA):");
    Serial.printf("  TWA PM1.0:          %.2f ug/m3\n", data.twa_pm1_0);
    Serial.printf("  TWA PM2.5:          %.2f ug/m3\n", data.twa_pm2_5);
    Serial.printf("  TWA PM4.0:          %.2f ug/m3\n", data.twa_pm4_0);
    Serial.printf("  TWA PM10:           %.2f ug/m3\n", data.twa_pm10);
    Serial.println();
}

void setup() {
    // Initialize Serial communication
    Serial.begin(115200);
    delay(1000);  // Wait for serial to stabilize
    
    printWelcomeBanner();
    
    Serial.print("Firmware Version: ");
    Serial.println(FIRMWARE_VERSION);
    Serial.println();
    
    Serial.println("🔧 Initializing system...");
    Serial.printf("   - I2C Bus (SDA=%d, SCL=%d)\n", SDA_PIN, SCL_PIN);
    Serial.println("   - LittleFS Filesystem");
    Serial.println("   - SEN66 Sensor");
    Serial.println();
    
    // Initialize the SEN66 dosimetry library
    if (!airQualitySensor.begin(SDA_PIN, SCL_PIN, 100000)) {
        Serial.println("❌ ERROR: Failed to initialize OSH-Monitor platform!");
        Serial.println();
        Serial.println("Troubleshooting:");
        Serial.printf("  1. Check I2C connections (SDA=%d, SCL=%d)\n", SDA_PIN, SCL_PIN);
        Serial.println("  2. Verify SEN66 is powered correctly");
        Serial.println("  3. Ensure sensor I2C address is 0x6B");
        Serial.println("  4. Check LittleFS partition is available");
        Serial.println();
        Serial.println("System halted. Reset to try again.");
        
        while (1) {
            delay(1000);
        }
    }
    
    // Store firmware and library versions in metadata
    airQualitySensor.setMetadata("firmware_version", FIRMWARE_VERSION);
    airQualitySensor.setMetadata("sen66core_version", "1.1.1");
    airQualitySensor.setMetadata("twacore_version", "1.0.0");
    
    Serial.println("✓ Initialization successful!");
    Serial.println();
    
    // Display current configuration
    SensorConfig config = airQualitySensor.getConfig();
    Serial.println("📊 Starting continuous monitoring...");
    Serial.printf("   Measurement interval: %d seconds\n", config.measurementInterval);
    Serial.printf("   Logging interval: %d seconds\n", config.loggingInterval);
    Serial.printf("   UTC offset: %+d hours\n", config.utcOffset);
    Serial.println("   TWA calculation window: 8 hours");
    Serial.println("   Log file: /sensor_log.csv");
    Serial.println("   Time sync: Not synchronized (using uptime)");
    Serial.println();
    Serial.println("💡 Serial Commands Available:");
    Serial.println("   help            - Show available commands");
    Serial.println("   dump            - Display CSV file contents");
    Serial.println("   export_twa      - Export 8-hour TWA calculations");
    Serial.println("   list            - List all files in filesystem");
    Serial.println("   clear           - Clear the CSV log file");
    Serial.println("   rtc status      - Show RTC status and timing info");
    Serial.println("   rtc sync <time> - Synchronize ESP32 RTC to Unix time");
    Serial.println("   config          - Show current configuration");
    Serial.println("   prefs <key> <val> - Set configuration value");
    Serial.println("   metadata        - Show all metadata");
    Serial.println("   meta <key> <val> - Set metadata value");
    Serial.println("   resetmeta       - Reset metadata to defaults");
    Serial.println();
    
    delay(2000);  // Allow sensor to stabilize
}

void loop() {
    unsigned long currentTime = millis();
    
    // Handle serial commands
    handleSerialCommands();
    
    // Get current configuration
    uint16_t measurementInterval = airQualitySensor.getMeasurementInterval() * 1000UL;  // Convert to ms
    uint16_t loggingInterval = airQualitySensor.getLoggingInterval() * 1000UL;  // Convert to ms
    
    // Check if it's time for a new measurement
    if (currentTime - lastMeasurementTime >= measurementInterval) {
        lastMeasurementTime = currentTime;
        measurementCount++;
        
        // Read sensor data
        if (airQualitySensor.readSensor()) {
            // Get the complete data structure
            SensorData data = airQualitySensor.getData();
            
            // Update 8-hour TWA calculations
            airQualitySensor.updateTWA(data);
            
            // Print formatted data to Serial
            printSensorData(data);
            
            // Determine if we should log this measurement
            bool shouldLog = (loggingInterval == 0) ||  // 0 = log every measurement
                           (currentTime - lastLoggingTime >= loggingInterval);
            
            if (shouldLog) {
                lastLoggingTime = currentTime;
                
                // Log to CSV file
                if (airQualitySensor.logEntry(data)) {
                    Serial.println("✓ Data logged to CSV file");
                } else {
                    Serial.println("⚠ Warning: Failed to log data to file");
                }
            }
            
            Serial.println("═══════════════════════════════════════════════════════════");
            Serial.println();
            
        } else {
            Serial.println("❌ ERROR: Failed to read sensor data");
            Serial.println("   Retrying at next interval...");
            Serial.println();
        }
    }
    
    // Keep the system responsive
    delay(100);
}

void handleSerialCommands() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        

        
        // Handle RTC commands
        if (command.startsWith("rtc ")) {
            String rtcCommand = command.substring(4);
            rtcCommand.trim();
            
            if (rtcCommand == "status") {
                Serial.println("\n🕐 RTC Status Report:");
                Serial.println("═══════════════════════════════════════════════════════════");
                Serial.print(airQualitySensor.getRTCStatus());
                Serial.println("═══════════════════════════════════════════════════════════\n");
            } else if (rtcCommand.startsWith("sync ")) {
                String unixTimeStr = rtcCommand.substring(5);
                unixTimeStr.trim();
                uint32_t unixTime = unixTimeStr.toInt();
                
                if (unixTime > 0) {
                    if (airQualitySensor.setRTCTime(unixTime)) {
                        Serial.printf("✓ ESP32 RTC synchronized to Unix timestamp: %lu\n", unixTime);
                        Serial.println("✓ RTC will maintain time across power cycles\n");
                    } else {
                        Serial.println("❌ ERROR: Failed to set RTC time\n");
                    }
                } else {
                    Serial.println("❌ ERROR: Invalid Unix timestamp\n");
                    Serial.println("Usage: rtc sync <unix_timestamp>\n");
                }
            } else {
                Serial.println("❌ ERROR: Unknown RTC command\n");
                Serial.println("Available RTC commands:");
                Serial.println("   rtc status       - Show RTC status");
                Serial.println("   rtc sync <time>  - Synchronize RTC\n");
            }
            return;
        }
        
        // Check for meta command before lowercasing
        if (command.startsWith("meta ")) {
            String params = command.substring(5);
            params.trim();
            int spacePos = params.indexOf(' ');
            
            if (spacePos > 0) {
                String key = params.substring(0, spacePos);
                String value = params.substring(spacePos + 1);
                key.trim();
                value.trim();
                
                // Check if this metadata change affects CSV structure
                if (airQualitySensor.shouldClearLogForMetadata(key) && LittleFS.exists("/sensor_log.csv")) {
                    String oldValue = airQualitySensor.getMetadata(key);
                    
                    // Prompt if changing existing value or setting for first time when log exists
                    if ((oldValue.length() > 0 && oldValue != value) || (oldValue.length() == 0)) {
                        Serial.println("\n⚠ WARNING: Changing this metadata will affect CSV data columns!");
                        Serial.println("   Current value: " + oldValue);
                        Serial.println("   New value: " + value);
                        Serial.println();
                        Serial.println("Options:");
                        Serial.println("  1. Type 'download' to save existing log first");
                        Serial.println("  2. Type 'yes' to clear log and set new metadata");
                        Serial.println("  3. Press Enter to cancel");
                        Serial.print("\nYour choice: ");
                        
                        // Wait for response with timeout
                        unsigned long timeout = millis() + 30000; // 30 second timeout
                        String response = "";
                        
                        while (millis() < timeout) {
                            if (Serial.available()) {
                                response = Serial.readStringUntil('\n');
                                response.trim();
                                response.toLowerCase();
                                break;
                            }
                            delay(100);
                        }
                        
                        Serial.println();
                        
                        if (response == "download" || response == "dump") {
                            Serial.println("📄 Outputting CSV data for download...");
                            Serial.println("═══════════════════════════════════════════════════════════\n");
                            // Trigger dump for Python CLI to capture and save
                            dumpCSVFile();
                            Serial.println("\n═══════════════════════════════════════════════════════════");
                            Serial.println("✓ CSV output complete. Python CLI should have saved the file.");
                            Serial.println("\nℹ You can now:");
                            Serial.println("  - Run the meta command again and choose 'yes' to proceed");
                            Serial.println("  - Or cancel and keep the existing log file\n");
                            return;
                        } else if (response == "yes") {
                            airQualitySensor.setMetadata(key, value, true); // true = clear log
                            Serial.println();
                        } else {
                            Serial.println("❌ Metadata change cancelled.\n");
                            return;
                        }
                    } else if (oldValue.length() == 0) {
                        // First time setting this field - just warn
                        airQualitySensor.setMetadata(key, value);
                        Serial.println();
                    } else {
                        // Same value, no change needed
                        Serial.println("ℹ Metadata unchanged (same value).\n");
                        return;
                    }
                } else {
                    // Non-dynamic metadata or no existing log file
                    airQualitySensor.setMetadata(key, value);
                    Serial.println();
                }
            } else {
                Serial.println("❌ ERROR: Invalid format\n");
                Serial.println("Usage: meta <key> <value>\n");
                Serial.println("Examples:");
                Serial.println("  meta user John_Doe");
                Serial.println("  meta project Lab_Study_2025");
                Serial.println("  meta location Building_A_Room_203\n");
            }
            return;
        }
        
        // Check for resetmeta command
        if (command == "resetmeta") {
            Serial.println("\n⚠ WARNING: This will reset all metadata to default state!");
            Serial.println("   - Keeps: device_name, firmware_version, session_start");
            Serial.println("   - Resets: user, project, location (empty values)");
            Serial.println("   - Deletes: All other custom metadata");
            Serial.println("   - Clears: CSV log file");
            Serial.println();
            Serial.print("Type 'yes' to confirm: ");
            
            // Wait for response with timeout
            unsigned long timeout = millis() + 15000; // 15 second timeout
            String response = "";
            
            while (millis() < timeout) {
                if (Serial.available()) {
                    response = Serial.readStringUntil('\n');
                    response.trim();
                    response.toLowerCase();
                    break;
                }
                delay(100);
            }
            
            Serial.println();
            
            if (response == "yes") {
                // Clear log file first
                if (LittleFS.exists("/sensor_log.csv")) {
                    if (LittleFS.remove("/sensor_log.csv")) {
                        Serial.println("✓ Log file cleared");
                    } else {
                        Serial.println("❌ ERROR: Failed to clear log file");
                    }
                }
                
                // Reset metadata
                if (airQualitySensor.resetMetadata()) {
                    Serial.println("✓ Metadata reset to defaults\n");
                } else {
                    Serial.println("❌ ERROR: Failed to reset metadata\n");
                }
            } else {
                Serial.println("❌ Metadata reset cancelled.\n");
            }
            return;
        }
        
        // Check for prefs command before lowercasing (also accept 'set' for compatibility)
        if (command.startsWith("prefs ") || command.startsWith("set ")) {
            String params = command.substring(command.indexOf(' ') + 1);
            params.trim();
            int spacePos = params.indexOf(' ');
            
            if (spacePos > 0) {
                String key = params.substring(0, spacePos);
                String valueStr = params.substring(spacePos + 1);
                key.toLowerCase();
                uint16_t value = valueStr.toInt();
                
                if (key == "measurement" || key == "meas") {
                    airQualitySensor.setMeasurementInterval(value);
                    airQualitySensor.saveConfig();
                } else if (key == "logging" || key == "log") {
                    airQualitySensor.setLoggingInterval(value);
                    airQualitySensor.saveConfig();
                } else if (key == "utc" || key == "timezone" || key == "offset") {
                    int16_t offset = valueStr.toInt();
                    airQualitySensor.setUtcOffset(offset);
                    airQualitySensor.saveConfig();
                } else if (key == "storage_warning" || key == "stor_warn") {
                    uint8_t threshold = (uint8_t)value;
                    if (threshold >= 1 && threshold <= 99) {
                        airQualitySensor.setStorageWarningThreshold(threshold);
                        Serial.printf("✓ Storage warning threshold set to %d%%\n", threshold);
                    } else {
                        Serial.println("❌ Storage threshold must be between 1-99%");
                    }
                } else {
                    Serial.printf("❌ Unknown setting: %s\n", key.c_str());
                    Serial.println("Available settings: measurement, logging, utc, storage_warning\n");
                }
            } else {
                Serial.println("❌ ERROR: Invalid format\n");
                Serial.println("Usage: prefs <measurement|logging> <seconds>\n");
                Serial.println("       prefs utc <offset_hours>  (e.g., prefs utc -5 for EST)\n");
                Serial.println("       prefs storage_warning <percent>  (e.g., prefs storage_warning 80)\n");
            }
            return;
        }
        
        command.toLowerCase();
        
        if (command == "help" || command == "h" || command == "?") {
            showHelp();
        } else if (command == "dump" || command == "d") {
            dumpCSVFile();
        } else if (command == "dump_twa") {
            dumpTWAFile();
        } else if (command == "list" || command == "ls") {
            listFiles();
        } else if (command == "clear" || command == "c") {
            Serial.println("\n⚠ WARNING: This will permanently delete the log file!");
            Serial.print("Type 'yes' to confirm deletion: ");
            
            // Wait for response with timeout
            unsigned long timeout = millis() + 15000; // 15 second timeout
            String response = "";
            
            while (millis() < timeout) {
                if (Serial.available()) {
                    response = Serial.readStringUntil('\n');
                    response.trim();
                    response.toLowerCase();
                    break;
                }
                delay(100);
            }
            
            Serial.println();
            
            if (response == "yes") {
                Serial.println("⚠ Clearing CSV log file...");
                LittleFS.remove("/sensor_log.csv");
                Serial.println("✓ Log file cleared!\n");
            } else {
                Serial.println("❌ Clear operation cancelled.\n");
            }
        } else if (command == "config" || command == "cfg") {
            SensorConfig config = airQualitySensor.getConfig();
            Serial.println("\n📋 Current Configuration:");
            Serial.println("═══════════════════════════════════════════════════════════");
            Serial.println("  Setting                    Key               Value");
            Serial.println("  ─────────────────────────  ────────────────  ─────────────");
            Serial.printf("  Measurement Interval       measurement       %d seconds\n", config.measurementInterval);
            Serial.printf("  Logging Interval           logging           %d seconds\n", config.loggingInterval);
            Serial.printf("  UTC Offset                 utc               %+d hours\n", config.utcOffset);
            Serial.printf("  Storage Warning Threshold  storage_warning   %d%%\n", config.storageWarningThreshold);
            Serial.printf("  Sampling Interval (TWA)    (read-only)       %d seconds\n", config.samplingInterval);
            Serial.println("═══════════════════════════════════════════════════════════");
            Serial.println("\n💡 Tip: Use 'prefs <key> <value>' to change configuration settings");
        } else if (command == "metadata" || command == "meta") {
            showMetadata();
        } else if (command == "export_twa" || command == "twa") {
            Serial.println("\n📊 Calculating OSHA-compliant 8-hour TWA...");
            
            if (airQualitySensor.exportCSVWithTWA("/twa_export.csv")) {
                TWAExportResult twa = airQualitySensor.getLastTWAExport();
                
                Serial.println("✓ TWA Export Complete!");
                Serial.printf("📈 Data Coverage: %.1f hours\n", twa.dataCoverageHours);
                Serial.printf("🏭 OSHA Compliant: %s\n", 
                             twa.oshaCompliant ? "YES (≥8 hours)" : "NO (< 8 hours - insufficient data)");
                
                // Access TWA values from map
                auto it_pm25 = twa.parameterTWAs.find("pm2_5");
                auto it_pm10 = twa.parameterTWAs.find("pm10");
                if (it_pm25 != twa.parameterTWAs.end()) {
                    Serial.printf("📋 PM2.5 8-hr TWA: %.3f µg/m³\n", it_pm25->second);
                }
                if (it_pm10 != twa.parameterTWAs.end()) {
                    Serial.printf("📋 PM10 8-hr TWA: %.3f µg/m³\n", it_pm10->second);
                }
                
                Serial.printf("📁 Export file: /twa_export.csv\n");
                Serial.printf("📊 Samples analyzed: %lu\n", twa.samplesAnalyzed);
                if (twa.dataGaps > 0) {
                    Serial.printf("⚠ Data gaps detected: %lu\n", twa.dataGaps);
                }
                Serial.println();
            } else {
                Serial.println("❌ TWA export failed. Check log file.");
            }
        } else if (command == "storage" || command == "stor") {
            StorageStats stats = airQualitySensor.getStorageStats();
            
            Serial.println("\n💾 Storage Statistics:");
            Serial.println("═══════════════════════════════════════════════════════════");
            Serial.printf("  Total Capacity:         %s\n", airQualitySensor.formatBytes(stats.totalBytes).c_str());
            Serial.printf("  Used:                   %s (%.1f%%)\n", 
                airQualitySensor.formatBytes(stats.usedBytes).c_str(), 
                stats.percentUsed);
            Serial.printf("  Free:                   %s\n", airQualitySensor.formatBytes(stats.freeBytes).c_str());
            Serial.printf("  Avg bytes/entry:        %zu bytes\n", stats.averageBytesPerEntry);
            Serial.printf("  Estimated remaining:    %.1f hours\n", stats.estimatedHoursRemaining);
            Serial.printf("  Warning threshold:      %d%%\n", airQualitySensor.getStorageWarningThreshold());
            
            if (stats.percentUsed >= airQualitySensor.getStorageWarningThreshold()) {
                Serial.println("\n⚠ WARNING: Storage threshold exceeded!");
            }
            Serial.println("═══════════════════════════════════════════════════════════\n");
        } else if (command.length() > 0) {
            Serial.println("\n❌ Unknown command. Type 'help' for available commands.\n");
        }
    }
}

void showHelp() {
    Serial.println("\n╔═════════════════════════════════════════════════════════════╗");
    Serial.println("║                OSH-Monitor Serial Commands                  ║");
    Serial.println("╠═════════════════════════════════════════════════════════════╣");
    Serial.println("║ help, h, ?              - Show this help message            ║");
    Serial.println("║ dump, d                 - Display CSV file contents         ║");
    Serial.println("║ export_twa, twa         - Export 8-hour TWA calculations    ║");
    Serial.println("║ storage, stor           - Show filesystem storage stats     ║");
    Serial.println("║ list, ls                - List all files in filesystem      ║");
    Serial.println("║ clear, c                - Clear the CSV log file            ║");
    Serial.println("║ rtc status              - Show ESP32 RTC status & timing    ║");
    Serial.println("║ rtc sync <unix_time>    - Synchronize ESP32 RTC             ║");
    Serial.println("║ config, cfg             - Show current configuration        ║");
    Serial.println("║ prefs <key> <value>     - Set configuration value           ║");
    Serial.println("║   Keys: measurement, logging (seconds), utc (offset hours)   ║");
    Serial.println("║         storage_warning (percent threshold)                  ║");
    Serial.println("║ metadata, meta          - Show all metadata                 ║");
    Serial.println("║ meta <key> <value>      - Set metadata value                ║");
    Serial.println("║   Common: user, project, location                           ║");
    Serial.println("║ resetmeta               - Reset all metadata to defaults    ║");
    Serial.println("╚═════════════════════════════════════════════════════════════╝\n");
}

void dumpCSVFile() {
    Serial.println("\n📄 Dumping CSV file: /sensor_log.csv");
    Serial.println("═══════════════════════════════════════════════════════════");
    
    if (!LittleFS.exists("/sensor_log.csv")) {
        Serial.println("❌ CSV file does not exist yet.");
        Serial.println("   Wait for first measurement to create the file.\n");
        return;
    }
    
    File file = LittleFS.open("/sensor_log.csv", "r");
    if (!file) {
        Serial.println("❌ ERROR: Failed to open CSV file for reading.\n");
        return;
    }
    
    // Display file size
    size_t fileSize = file.size();
    Serial.printf("File size: %zu bytes\n\n", fileSize);
    
    // Read and display file contents
    int lineNum = 0;
    int dataNum = 0;
    bool headerSeen = false;
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        lineNum++;
        
        // Determine line type
        if (line.startsWith("#")) {
            // Comment line
            Serial.print("[COMMENT] ");
        } else if (!headerSeen && line.indexOf(',') >= 0) {
            // First non-comment line with commas is the header
            Serial.print("[HEADER] ");
            headerSeen = true;
        } else {
            // Data lines
            dataNum++;
            Serial.printf("[%4d] ", dataNum);
        }
        Serial.println(line);
    }
    
    file.close();
    Serial.println("═══════════════════════════════════════════════════════════");
    Serial.printf("✓ Displayed %d lines (%zu bytes total)\n\n", lineNum, fileSize);
}

void listFiles() {
    Serial.println("\n📁 LittleFS File System Contents");
    Serial.println("═══════════════════════════════════════════════════════════");
    
    File root = LittleFS.open("/");
    if (!root || !root.isDirectory()) {
        Serial.println("❌ ERROR: Failed to open root directory.\n");
        return;
    }
    
    File file = root.openNextFile();
    int fileCount = 0;
    size_t totalSize = 0;
    
    if (!file) {
        Serial.println("(empty - no files found)");
    } else {
        Serial.println("Filename               Size (bytes)");
        Serial.println("───────────────────────────────────────────────────────────");
        
        while (file) {
            fileCount++;
            size_t size = file.size();
            totalSize += size;
            
            Serial.printf("%-20s  %8zu\n", file.name(), size);
            file = root.openNextFile();
        }
        
        Serial.println("───────────────────────────────────────────────────────────");
        Serial.printf("Total: %d file(s), %zu bytes\n", fileCount, totalSize);
    }
    
    // Show filesystem info
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    Serial.println();
    Serial.printf("Filesystem: %zu bytes total, %zu bytes used (%.1f%%)\n", 
                  totalBytes, usedBytes, (usedBytes * 100.0) / totalBytes);
    Serial.println("═══════════════════════════════════════════════════════════\n");
}

void dumpTWAFile() {
    Serial.println("\n📄 Dumping TWA export file: /twa_export.csv");
    Serial.println("═══════════════════════════════════════════════════════════");
    
    if (!LittleFS.exists("/twa_export.csv")) {
        Serial.println("❌ TWA export file does not exist yet.");
        Serial.println("   Use 'export_twa' command to create the file first.\n");
        return;
    }
    
    File file = LittleFS.open("/twa_export.csv", "r");
    if (!file) {
        Serial.println("❌ ERROR: Failed to open TWA export file for reading.\n");
        return;
    }
    
    // Display file size
    size_t fileSize = file.size();
    Serial.printf("File size: %zu bytes\n\n", fileSize);
    
    // Read and display file contents
    int lineNum = 0;
    int dataNum = 0;
    bool headerSeen = false;
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim(); // Remove trailing whitespace/newlines
        lineNum++;
        
        if (line.length() == 0) continue; // Skip empty lines
        
        if (line.startsWith("#")) {
            // Comment line
            Serial.printf("[COMMENT]%s\n", line.c_str());
        } else {
            // Data line (including header)
            if (!headerSeen && line.indexOf(',') >= 0) {
                Serial.printf("[HEADER]%s\n", line.c_str());
                headerSeen = true;
            } else {
                Serial.printf("[DATA]%s\n", line.c_str());
                dataNum++;
            }
        }
    }
    
    file.close();
    Serial.printf("\nTotal lines: %d (including %d data rows)\n", lineNum, dataNum);
    Serial.println("═══════════════════════════════════════════════════════════\n");
}

void showMetadata() {
    Serial.println("\n📝 Current Metadata:");
    Serial.println("═══════════════════════════════════════════════════════════");
    
    std::vector<String> keys = airQualitySensor.getMetadataKeys();
    
    if (keys.empty()) {
        Serial.println("  (no metadata set)");
    } else {
        Serial.println("  Key                    Value");
        Serial.println("  ─────────────────────  ────────────────────────────────");
        
        // Show default metadata first
        const char* defaultKeys[] = {"device_name", "firmware_version", "session_start"};
        for (int i = 0; i < 3; i++) {
            String key = defaultKeys[i];
            String value = airQualitySensor.getMetadata(key);
            if (value.length() > 0) {
                Serial.printf("  %-20s  %s\n", key.c_str(), value.c_str());
            }
        }
        
        // Show dynamic metadata
        const char* dynamicKeys[] = {"user", "project", "location"};
        bool hasDynamic = false;
        for (int i = 0; i < 3; i++) {
            String key = dynamicKeys[i];
            String value = airQualitySensor.getMetadata(key);
            if (value.length() > 0) {
                if (!hasDynamic) {
                    Serial.println();
                    hasDynamic = true;
                }
                Serial.printf("  %-20s  %s\n", key.c_str(), value.c_str());
            }
        }
        
        // Show any other custom metadata
        bool hasOther = false;
        for (const String& key : keys) {
            bool isDefaultOrDynamic = false;
            for (int i = 0; i < 3; i++) {
                if (key == defaultKeys[i] || key == dynamicKeys[i]) {
                    isDefaultOrDynamic = true;
                    break;
                }
            }
            if (!isDefaultOrDynamic) {
                if (!hasOther) {
                    Serial.println();
                    hasOther = true;
                }
                String value = airQualitySensor.getMetadata(key);
                Serial.printf("  %-20s  %s\n", key.c_str(), value.c_str());
            }
        }
    }
    
    Serial.println("═══════════════════════════════════════════════════════════");
    Serial.println("\n💡 Tip: Use 'meta <key> <value>' to set metadata");
    Serial.println("   Example: meta user John_Doe\n");
}