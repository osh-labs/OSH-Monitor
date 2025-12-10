# SEN66-Dosimetry

**Advanced Air-Quality Acquisition, Dosimetry, and CSV Logging for the Sensirion SEN66**

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Library-orange)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue)](https://www.arduino.cc/)
[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-green)](https://www.espressif.com/en/products/socs/esp32-s3)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Overview

SEN66-Dosimetry is an open-source PlatformIO/Arduino library for ESP32-S3 devices that provides high-integrity sensor acquisition, derived environmental calculations, particulate dosimetry, and CSV data logging for the Sensirion SEN66 air-quality module.

The library is **headless** and provides no display/UI components. Its purpose is to serve as a robust sensor-data and exposure-analysis engine for long-duration environmental monitoring.

## Features

### 🌡️ Raw Sensor Data
- Temperature
- Relative Humidity
- VOC Index
- NOx Index
- PM1.0, PM2.5, PM4.0, PM10
- CO₂ concentration (ppm)
- CRC-8 validation using Sensirion polynomial

### 📊 Derived Environmental Metrics
- **Dew Point** (Magnus-type formula)
- **Heat Index** (NOAA/Steadman approximation)
- **Absolute Humidity** (g/m³)

### ⏱️ Dosimetry & 8-Hour TWA
- Continuous 8-hour arithmetic Time-Weighted Averages for:
  - PM1.0
  - PM2.5
  - PM4.0
  - PM10
- RAM-resident circular buffers
- Efficient insertion and rollover behavior

### 💾 CSV Logging (LittleFS)
- Append-only logging
- Auto-create files with header row
- Buffered writes to minimize flash wear
- Line-by-line retrieval
- Safe erase behavior

## Hardware Requirements

- **MCU**: ESP32-S3 (or compatible ESP32 with LittleFS support)
- **Sensor**: Sensirion SEN66 air quality module
- **Interface**: I²C communication
- **Storage**: Internal flash memory with LittleFS

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps = 
    https://github.com/yourusername/SEN66-Dosimetry.git
```

Or install directly:
```bash
pio lib install SEN66-Dosimetry
```

### Arduino IDE

1. Download this repository as ZIP
2. In Arduino IDE: Sketch → Include Library → Add .ZIP Library
3. Select the downloaded ZIP file

## Quick Start

```cpp
#include <Arduino.h>
#include "SEN66Dosimetry.h"

// Create sensor instance with 60-second sampling interval
SEN66Dosimetry sensor(Wire, 60);

void setup() {
    Serial.begin(115200);
    
    // Initialize (I2C SDA=21, SCL=22)
    if (!sensor.begin(21, 22)) {
        Serial.println("Initialization failed!");
        while(1) delay(1000);
    }
    
    Serial.println("SEN66 ready!");
}

void loop() {
    // Read sensor
    if (sensor.readSensor()) {
        SensorData data = sensor.getData();
        
        // Update TWA
        sensor.updateTWA(data);
        
        // Log to CSV
        sensor.logEntry(data);
        
        // Print values
        Serial.printf("Temp: %.2f°C, PM2.5: %.2f µg/m³\n", 
                     data.temperature, data.pm2_5);
    }
    
    delay(60000);  // Wait 1 minute
}
```

## API Reference

### Initialization

```cpp
SEN66Dosimetry(TwoWire &wire = Wire, uint16_t samplingInterval = 60);
bool begin(int sdaPin = 21, int sclPin = 22, uint32_t i2cFreq = 100000);
```

### Sensor Operations

```cpp
bool readSensor();                    // Read all measurements from SEN66
SensorData getData() const;           // Get current sensor data
void updateTWA(SensorData &data);     // Update 8-hour TWA calculations
bool startMeasurement();              // Start sensor measurement mode
bool stopMeasurement();               // Stop sensor measurement mode
```

### Data Logging

```cpp
bool logEntry(const SensorData &data);           // Log entry to CSV
bool eraseLogs();                                // Erase all log files
bool readLogLine(size_t index, String &line);    // Read specific log line
size_t getLogLineCount();                        // Get total log lines
void setLogFilePath(const String &path);         // Set custom log path
```

### Data Structure

```cpp
struct SensorData {
    // Raw measurements
    float temperature, humidity, vocIndex, noxIndex;
    float pm1_0, pm2_5, pm4_0, pm10, co2;
    
    // Derived metrics
    float dewPoint, heatIndex, absoluteHumidity;
    
    // 8-hour TWAs
    float twa_pm1_0, twa_pm2_5, twa_pm4_0, twa_pm10;
    
    uint32_t timestamp;
};
```

## CSV Format

```csv
timestamp,temperature,humidity,vocIndex,noxIndex,pm1_0,pm2_5,pm4_0,pm10,co2,dewPoint,heatIndex,absoluteHumidity,twa_pm1_0,twa_pm2_5,twa_pm4_0,twa_pm10
```

## Examples

See the `examples/` directory for complete examples:
- **BasicLogger**: Simple periodic logging with serial output

## Wiring

### Standard I2C Connection

| SEN66 Pin | ESP32-S3 Pin | Description |
|-----------|--------------|-------------|
| VDD       | 3.3V/5V      | Power supply |
| GND       | GND          | Ground |
| SDA       | GPIO 21      | I2C Data |
| SCL       | GPIO 22      | I2C Clock |

## Performance

- **RAM Usage**: Approximately 30KB (includes TWA buffers)
- **Flash Usage**: ~292KB compiled binary
- **Sampling Rate**: Configurable (default 60 seconds)
- **TWA Window**: 8 hours (480 samples at 60s interval)

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## References

- [Sensirion SEN66 Datasheet](https://sensirion.com/products/catalog/SEN66/)
- [ESP32-S3 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [PlatformIO Documentation](https://docs.platformio.org/)

## Author

Your Name - [your.email@example.com](mailto:your.email@example.com)

## Acknowledgments

- Sensirion AG for the SEN66 sensor
- ESP32 community
- PlatformIO team
