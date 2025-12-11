# SEN66-Dosimetry

**Advanced Air-Quality Acquisition, Dosimetry, and CSV Logging for the Sensirion SEN66**

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Library-orange)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue)](https://www.arduino.cc/)
[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-green)](https://www.espressif.com/en/products/socs/esp32-s3)
[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

## Overview

SEN66-Dosimetry is an open-source PlatformIO/Arduino library for ESP32-S3 devices that provides high-integrity sensor acquisition, derived environmental calculations, particulate dosimetry, CSV data logging, and interactive command-line interface for the Sensirion SEN66 air-quality module.

The system includes both model firmware for ESP32-S3 and a comprehensive Python CLI tool for device interaction, configuration, and data management.

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
- **Localized timestamps** with configurable UTC offset (-12 to +14 hours)
- Dual timestamp format: Unix timestamp + human-readable local time

### 🖥️ Interactive CLI Tool
- **Python-based command-line interface** (`sen66_cli.py`)
- Real-time sensor monitoring and configuration
- CSV download and backup functionality
- Timezone/UTC offset configuration
- Metadata management system
- Serial command interface for firmware interaction
- Auto-detection of ESP32-S3 devices

### ⚙️ Configuration & Metadata
- **Persistent configuration** stored in NVS (Non-Volatile Storage)
- Configurable measurement and logging intervals
- **UTC offset support** for localized timestamps
- Metadata system for project/user/location tracking
- Serial command interface (config, prefs, meta, etc.)

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
timestamp,local_time,pm1_0,pm2_5,pm10_0,pm4_0,humidity,temperature,voc_index,nox_index,pm1_0_twa,pm2_5_twa,pm10_0_twa,pm4_0_twa
```

**Note**: The `local_time` column shows timestamps in `yyyy-mm-dd_hh:mm:ss` format adjusted by the configured UTC offset.

## CLI Tool Usage

### Interactive Console
```bash
python sen66_cli.py console
```

### Available Commands
- `status` - Show current sensor measurements
- `config` - Display current configuration including UTC offset
- `timezone <offset>` - Set UTC offset (-12 to +14 hours)
- `prefs <key> <value>` - Configure measurement/logging intervals
- `download [file]` - Download CSV log file
- `timesync` - Synchronize device time with PC
- `metadata` - Show/manage metadata (user, project, location)
- `clear` - Clear CSV log file
- `monitor` - Real-time sensor monitoring
- `about` - Show project information and license

### Command Line Examples
```bash
# Set timezone to EST (UTC-5)
python sen66_cli.py timezone --offset -5

# Download log file
python sen66_cli.py download --output my_data.csv

# Show current status
python sen66_cli.py status
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

This project is licensed under the GNU General Public License v3.0 (GPLv3) - see the [LICENSE](LICENSE) file for details.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

## References

- [Sensirion SEN66 Datasheet](https://sensirion.com/products/catalog/SEN66/)
- [ESP32-S3 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- [PlatformIO Documentation](https://docs.platformio.org/)

## Author

Christopher Lee

## Acknowledgments

- Sensirion AG for the SEN66 sensor
- ESP32 community
- PlatformIO team
