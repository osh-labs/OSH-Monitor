/**
 * @file BasicSEN66.ino
 * @brief Minimal example demonstrating SEN66Core library usage
 * 
 * Reads sensor data every 5 seconds and prints to Serial.
 * Demonstrates raw data acquisition and derived environmental metrics.
 * 
 * Hardware: ESP32-S3 with SEN66 sensor connected via I2C
 * Connections:
 *   SDA -> GPIO 21
 *   SCL -> GPIO 22
 *   VDD -> 3.3V or 5V
 *   GND -> GND
 */

#include <SEN66Core.h>

SEN66Core sensor;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n========================================");
    Serial.println("  SEN66Core Basic Example");
    Serial.println("========================================\n");
    
    // Initialize sensor with default I2C pins (SDA=21, SCL=22)
    Serial.println("Initializing SEN66 sensor...");
    if (!sensor.begin(21, 22, 100000)) {
        Serial.println("❌ Failed to initialize SEN66!");
        Serial.println("Error: " + sensor.getLastError());
        Serial.println("\nPlease check:");
        Serial.println("  - I2C connections (SDA=21, SCL=22)");
        Serial.println("  - Sensor power supply");
        Serial.println("  - I2C pull-up resistors");
        while (1) {
            delay(100);
        }
    }
    
    Serial.println("✓ SEN66 initialized successfully!");
    
    // Get and display serial number
    String serialNum = sensor.getSerialNumber();
    if (serialNum.length() > 0) {
        Serial.println("Serial Number: " + serialNum);
    }
    
    Serial.println("\nStarting measurements (readings every 5 seconds)...");
    Serial.println("========================================\n");
}

void loop() {
    SEN66FullData data;
    
    if (sensor.readFullData(data)) {
        Serial.println("╔════════════════════════════════════════╗");
        Serial.println("║       SEN66 Measurement Data          ║");
        Serial.println("╠════════════════════════════════════════╣");
        
        // Environmental conditions
        Serial.println("║ Environmental Conditions:              ║");
        Serial.printf("║   Temperature:      %6.2f °C         ║\n", data.raw.temperature);
        Serial.printf("║   Humidity:         %6.2f %%          ║\n", data.raw.humidity);
        Serial.printf("║   Dew Point:        %6.2f °C         ║\n", data.derived.dewPoint);
        Serial.printf("║   Heat Index:       %6.2f °C         ║\n", data.derived.heatIndex);
        Serial.printf("║   Absolute Humid:   %6.2f g/m³       ║\n", data.derived.absoluteHumidity);
        
        Serial.println("║                                        ║");
        
        // Particulate matter
        Serial.println("║ Particulate Matter:                    ║");
        Serial.printf("║   PM1.0:            %6.2f µg/m³      ║\n", data.raw.pm1_0);
        Serial.printf("║   PM2.5:            %6.2f µg/m³      ║\n", data.raw.pm2_5);
        Serial.printf("║   PM4.0:            %6.2f µg/m³      ║\n", data.raw.pm4_0);
        Serial.printf("║   PM10:             %6.2f µg/m³      ║\n", data.raw.pm10);
        
        Serial.println("║                                        ║");
        
        // Gas measurements
        Serial.println("║ Gas Measurements:                      ║");
        Serial.printf("║   VOC Index:        %6.0f            ║\n", data.raw.vocIndex);
        Serial.printf("║   NOx Index:        %6.0f            ║\n", data.raw.noxIndex);
        Serial.printf("║   CO₂:              %6.0f ppm        ║\n", data.raw.co2);
        
        Serial.println("╚════════════════════════════════════════╝");
        Serial.println();
    } else {
        Serial.println("❌ Failed to read sensor!");
        Serial.println("Error: " + sensor.getLastError());
        Serial.println();
    }
    
    delay(5000);  // Read every 5 seconds
}
