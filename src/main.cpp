/**
 * SEN66-Dosimetry Main Application
 * 
 * Advanced air quality monitoring with the Sensirion SEN66 sensor
 * Includes real-time measurements, 8-hour TWA calculations, and CSV logging
 * 
 * Hardware Requirements:
 * - ESP32-S3 (Adafruit Feather ESP32-S3 Reverse TFT)
 * - Sensirion SEN66 air quality sensor
 * 
 * I2C Connections:
 * - SDA: GPIO 3
 * - SCL: GPIO 4
 */

#include <Arduino.h>
#include "SEN66Dosimetry.h"

// I2C Pin Definitions
#define SDA_PIN 3
#define SCL_PIN 4

// Create sensor instance with 20-second sampling interval for TWA (debugging)
SEN66Dosimetry airQualitySensor(Wire, 20);

// Timing variables
unsigned long lastMeasurementTime = 0;
unsigned long lastLoggingTime = 0;
unsigned long measurementCount = 0;

// Function declarations
void handleSerialCommands();
void dumpCSVFile();
void listFiles();
void showHelp();

void printWelcomeBanner() {
    Serial.println("\n");
    Serial.println("╔══════════════════════════════════════════════════════════╗");
    Serial.println("║          SEN66-Dosimetry Air Quality Monitor             ║");
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
    
    Serial.println("🔧 Initializing system...");
    Serial.printf("   - I2C Bus (SDA=%d, SCL=%d)\n", SDA_PIN, SCL_PIN);
    Serial.println("   - LittleFS Filesystem");
    Serial.println("   - SEN66 Sensor");
    Serial.println();
    
    // Initialize the SEN66 dosimetry library
    if (!airQualitySensor.begin(SDA_PIN, SCL_PIN, 100000)) {
        Serial.println("❌ ERROR: Failed to initialize SEN66-Dosimetry!");
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
    
    Serial.println("✓ Initialization successful!");
    Serial.println();
    
    // Display current configuration
    SensorConfig config = airQualitySensor.getConfig();
    Serial.println("📊 Starting continuous monitoring...");
    Serial.printf("   Measurement interval: %d seconds\n", config.measurementInterval);
    Serial.printf("   Logging interval: %d seconds\n", config.loggingInterval);
    Serial.println("   TWA calculation window: 8 hours");
    Serial.println("   Log file: /sensor_log.csv");
    Serial.println("   Time sync: Not synchronized (using uptime)");
    Serial.println();
    Serial.println("💡 Serial Commands Available:");
    Serial.println("   help            - Show available commands");
    Serial.println("   dump            - Display CSV file contents");
    Serial.println("   list            - List all files in filesystem");
    Serial.println("   clear           - Clear the CSV log file");
    Serial.println("   sync <time>     - Synchronize Unix timestamp");
    Serial.println("   config          - Show current configuration");
    Serial.println("   set <key> <val> - Set configuration value");
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
        
        // Check for time sync command first (case-sensitive for parsing)
        if (command.startsWith("sync ")) {
            String unixTimeStr = command.substring(5);
            unixTimeStr.trim();
            uint32_t unixTime = unixTimeStr.toInt();
            
            if (unixTime > 0) {
                airQualitySensor.setUnixTime(unixTime);
                Serial.printf("✓ Time synchronized to Unix timestamp: %lu\n\n", unixTime);
            } else {
                Serial.println("❌ ERROR: Invalid Unix timestamp\n");
                Serial.println("Usage: sync <unix_timestamp>\n");
            }
            return;
        }
        
        // Check for set command before lowercasing
        if (command.startsWith("set ")) {
            String params = command.substring(4);
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
                } else {
                    Serial.printf("❌ Unknown setting: %s\n", key.c_str());
                    Serial.println("Available settings: measurement, logging\n");
                }
            } else {
                Serial.println("❌ ERROR: Invalid format\n");
                Serial.println("Usage: set <measurement|logging> <seconds>\n");
            }
            return;
        }
        
        command.toLowerCase();
        
        if (command == "help" || command == "h" || command == "?") {
            showHelp();
        } else if (command == "dump" || command == "d") {
            dumpCSVFile();
        } else if (command == "list" || command == "ls") {
            listFiles();
        } else if (command == "clear" || command == "c") {
            Serial.println("\n⚠ Clearing CSV log file...");
            LittleFS.remove("/sensor_log.csv");
            Serial.println("✓ Log file cleared!\n");
        } else if (command == "config" || command == "cfg") {
            SensorConfig config = airQualitySensor.getConfig();
            Serial.println("\n📋 Current Configuration:");
            Serial.println("═══════════════════════════════════════════════════════════");
            Serial.println("  Setting                    Key           Value");
            Serial.println("  ─────────────────────────  ────────────  ─────────────");
            Serial.printf("  Measurement Interval       measurement   %d seconds\n", config.measurementInterval);
            Serial.printf("  Logging Interval           logging       %d seconds\n", config.loggingInterval);
            Serial.printf("  Sampling Interval (TWA)    (read-only)   %d seconds\n", config.samplingInterval);
            Serial.println("═══════════════════════════════════════════════════════════\n");
        } else if (command.length() > 0) {
            Serial.println("\n❌ Unknown command. Type 'help' for available commands.\n");
        }
    }
}

void showHelp() {
    Serial.println("\n╔═════════════════════════════════════════════════════════════╗");
    Serial.println("║             SEN66-Dosimetry Serial Commands                 ║");
    Serial.println("╠═════════════════════════════════════════════════════════════╣");
    Serial.println("║ help, h, ?              - Show this help message            ║");
    Serial.println("║ dump, d                 - Display CSV file contents         ║");
    Serial.println("║ list, ls                - List all files in filesystem      ║");
    Serial.println("║ clear, c                - Clear the CSV log file            ║");
    Serial.println("║ sync <unix_time>        - Synchronize system time           ║");
    Serial.println("║ config, cfg             - Show current configuration        ║");
    Serial.println("║ set <key> <value>       - Set configuration value           ║");
    Serial.println("║   Keys: measurement, logging (intervals in seconds)         ║");
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
    while (file.available()) {
        String line = file.readStringUntil('\n');
        lineNum++;
        
        // Add line numbers for easier reading
        if (lineNum == 1) {
            Serial.print("[HEADER] ");
        } else {
            Serial.printf("[%4d] ", lineNum - 1);
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