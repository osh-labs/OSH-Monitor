/**
 * BasicLogger Example
 * 
 * Demonstrates basic usage of the SEN66-Dosimetry library:
 * - Initialize sensor and filesystem
 * - Read sensor data periodically
 * - Calculate 8-hour TWA for particulate matter
 * - Log all data to CSV file
 * - Print readings to Serial
 * 
 * Hardware:
 * - ESP32-S3 board
 * - Sensirion SEN66 sensor connected via I2C
 *   - SDA: GPIO 42 (default)
 *   - SCL: GPIO 41 (default)
 */

#include <Arduino.h>
#include "SEN66Dosimetry.h"

// I2C Pin Definitions
#define SDA_PIN 42
#define SCL_PIN 41

// Create sensor instance with 60-second sampling interval
SEN66Dosimetry sensor(Wire, 60);

// Timing variables
unsigned long lastReadTime = 0;
const unsigned long READ_INTERVAL = 60000;  // 60 seconds in milliseconds

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);  // Wait for Serial connection
    }
    
    Serial.println("\n=== SEN66-Dosimetry Basic Logger ===");
    Serial.println("Initializing sensor and filesystem...");
    
    // Initialize the library with defined I2C pins
    if (!sensor.begin(SDA_PIN, SCL_PIN)) {
        Serial.println("ERROR: Failed to initialize SEN66-Dosimetry!");
        Serial.println("Check connections and try again.");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("Initialization successful!");
    Serial.println("Starting measurement loop...\n");
    
    // Print CSV header for serial output
    Serial.println("Time(s),Temp(°C),RH(%),VOC,NOx,PM1.0,PM2.5,PM4.0,PM10,CO2(ppm),Dew(°C),HI(°C),AH(g/m³),TWA_PM1.0,TWA_PM2.5,TWA_PM4.0,TWA_PM10");
    Serial.println("---");
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
