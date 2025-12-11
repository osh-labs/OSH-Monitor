/**
 * BasicLogger Example
 * 
 * Demonstrates basic usage of the SEN66-Dosimetry library:
 * - Initialize sensor and filesystem
 * - Read sensor data periodically
 * - Calculate 8-hour TWA for particulate matter
 * - Log all data to CSV file with metadata
 * - Print readings to Serial
 * - Interactive serial commands for control
 * 
 * Hardware:
 * - ESP32-S3 board
 * - Sensirion SEN66 sensor connected via I2C
 *   - SDA: GPIO 3
 *   - SCL: GPIO 4
 * 
 * Serial Commands:
 * - help, h, ?              - Show help message
 * - dump, d                 - Display CSV file contents
 * - list, ls                - List all files
 * - clear, c                - Clear CSV log file
 * - sync <unix_time>        - Synchronize time
 * - config, cfg             - Show configuration
 * - set <key> <value>       - Set config (measurement, logging)
 * - metadata, meta          - Show all metadata
 * - meta <key> <value>      - Set metadata value
 */

#include <Arduino.h>
#include <LittleFS.h>
#include "SEN66Dosimetry.h"

// I2C Pin Definitions
#define SDA_PIN 3
#define SCL_PIN 4

// Create sensor instance with 60-second sampling interval
SEN66Dosimetry sensor(Wire, 60);

// Timing variables
unsigned long lastReadTime = 0;
unsigned long lastLogTime = 0;

// Forward declarations
void handleSerialCommands();
void showHelp();
void dumpCSVFile();
void listFiles();
void showMetadata();
void printSensorData(const SensorData &data);

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);  // Wait for Serial connection
    }
    
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║         SEN66-Dosimetry Air Quality Monitor                ║");
    Serial.println("║              with Metadata Support                         ║");
    Serial.println("╚════════════════════════════════════════════════════════════╝\n");
    
    Serial.println("🔧 Initializing sensor and filesystem...");
    
    // Initialize the library with defined I2C pins
    if (!sensor.begin(SDA_PIN, SCL_PIN)) {
        Serial.println("\n❌ ERROR: Failed to initialize SEN66-Dosimetry!");
        Serial.println("Troubleshooting:");
        Serial.println("  1. Check I2C wiring (SDA=GPIO3, SCL=GPIO4)");
        Serial.println("  2. Verify sensor power (3.3V or 5V)");
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
    SensorConfig config = sensor.getConfig();
    Serial.println("📊 Starting continuous monitoring...");
    Serial.printf("   Measurement interval: %d seconds\n", config.measurementInterval);
    Serial.printf("   Logging interval: %d seconds\n", config.loggingInterval);
    Serial.println("   TWA calculation window: 8 hours");
    Serial.println("   Log file: /sensor_log.csv");
    
    if (sensor.isTimeSynchronized()) {
        Serial.printf("   Time sync: Synchronized (Unix: %lu)\n", sensor.getUnixTime());
    } else {
        Serial.println("   Time sync: Not synchronized (using uptime)");
    }
    
    Serial.println();
    Serial.println("💡 Serial Commands Available:");
    Serial.println("   help            - Show available commands");
    Serial.println("   dump            - Display CSV file contents");
    Serial.println("   list            - List all files in filesystem");
    Serial.println("   clear           - Clear the CSV log file");
    Serial.println("   sync <time>     - Synchronize Unix timestamp");
    Serial.println("   config          - Show current configuration");
    Serial.println("   set <key> <val> - Set configuration value");
    Serial.println("   metadata        - Show all metadata");
    Serial.println("   meta <key> <val> - Set metadata value");
    Serial.println();
    
    delay(2000);  // Allow sensor to stabilize
}

void loop() {
    unsigned long currentTime = millis();
    
    // Check if it's time to read sensor
    if (currentTime - lastReadTime >= READ_INTERVAL) {
        lastReadTime = currentTime;
        
        // Read sensor data
        if (sensor.readSensor()) {
            // Get the data
            SensorData data = sensor.getData();
            
            // Update TWA calculations
            sensor.updateTWA(data);
            
            // Log to CSV file
            if (sensor.logEntry(data)) {
                Serial.print("✓ Logged | ");
            } else {
                Serial.print("✗ Log Failed | ");
            }
            
            // Print to Serial in CSV format
            Serial.print(data.timestamp); Serial.print(",");
            Serial.print(data.temperature, 2); Serial.print(",");
            Serial.print(data.humidity, 2); Serial.print(",");
            Serial.print(data.vocIndex, 1); Serial.print(",");
            Serial.print(data.noxIndex, 1); Serial.print(",");
            Serial.print(data.pm1_0, 2); Serial.print(",");
            Serial.print(data.pm2_5, 2); Serial.print(",");
            Serial.print(data.pm4_0, 2); Serial.print(",");
            Serial.print(data.pm10, 2); Serial.print(",");
            Serial.print(data.co2, 1); Serial.print(",");
            Serial.print(data.dewPoint, 2); Serial.print(",");
            Serial.print(data.heatIndex, 2); Serial.print(",");
            Serial.print(data.absoluteHumidity, 3); Serial.print(",");
            Serial.print(data.twa_pm1_0, 2); Serial.print(",");
            Serial.print(data.twa_pm2_5, 2); Serial.print(",");
            Serial.print(data.twa_pm4_0, 2); Serial.print(",");
            Serial.println(data.twa_pm10, 2);
            
        } else {
            Serial.println("ERROR: Failed to read sensor data!");
        }
    }
    
    // Add other non-blocking tasks here
    delay(100);
}
