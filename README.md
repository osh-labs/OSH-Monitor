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

### ⏱️ Advanced TWA (Time-Weighted Average) System
**Dual-Mode TWA Architecture for Real-Time Monitoring & Regulatory Compliance**

#### 🔄 FastTWA (Real-Time Calculations)
- **Purpose**: Live TWA display during monitoring
- **Method**: Efficient circular buffer with rolling averages
- **Update Rate**: Every measurement cycle (10-60 seconds)
- **Memory**: RAM-resident for fast access
- **Window**: Configurable (default 8 hours)
- **Parameters**: PM1.0, PM2.5, PM4.0, PM10

#### 📋 ExportTWA (Regulatory Compliance)
- **Purpose**: OSHA-compliant 8-hour TWA calculations for regulatory reporting
- **Method**: Precise weighted-average calculations from stored CSV data
- **Data Source**: Complete historical data from LittleFS storage  
- **Compliance**: OSHA 29 CFR 1910.1000 standards
- **Features**:
  - Gap detection and analysis
  - Data coverage validation (≥8 hours required)
  - Chronological timestamp verification
  - Precise time-weighted calculations

#### 🏭 OSHA Compliance Features
- **Minimum Data Requirements**: 8+ hours of continuous data
- **Gap Tolerance**: Automatic detection of data interruptions
- **Regulatory Headers**: Complete compliance documentation in CSV exports
- **Audit Trail**: Comprehensive calculation metadata including:
  - Data coverage period
  - Sample count and analysis
  - Compliance status determination
  - Gap detection results

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
- **Automatic TWA Export Generation**: Creates OSHA-compliant reports during downloads
- Timezone/UTC offset configuration
- Metadata management system
- Serial command interface for firmware interaction
- Auto-detection of ESP32-S3 devices
- **TWA Export Commands**: `export_twa` for dedicated regulatory exports

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

// Create sensor instance with 20-second sampling interval for TWA
SEN66Dosimetry sensor(Wire, 20);

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
    
    delay(20000);  // Wait 20 seconds
}
```

## Serial Commands (Direct Firmware Interface)

When connected to the ESP32-S3 via serial monitor, the following commands are available:

### Basic Commands
- `help` - Show all available commands  
- `status` - Display current sensor readings and TWA values
- `config` - Show current configuration settings

### Data Management  
- `dump` - Output complete CSV file contents via serial
- **`dump_twa`** - Output OSHA-compliant TWA export file
- `clear` - Permanently delete CSV log file  
- `list` - List all files in LittleFS filesystem

### Configuration
- `timesync <unix_timestamp>` - Synchronize device time
- `prefs <key> <value>` - Set configuration parameters
  - `measurement <seconds>` - Measurement interval (10-3600s)
  - `logging <seconds>` - Logging interval (0=every measurement, or 10-3600s)  
  - `utc <offset>` - UTC offset in hours (-12 to +14)

### Metadata Management
- `metadata` - Show all metadata values
- `meta <key> <value>` - Set metadata (user, project, location)  
- `resetmeta` - Reset all metadata to defaults

### TWA Operations
- **`export_twa`** - Generate and save OSHA-compliant TWA export to filesystem
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

### TWA Export & Compliance

```cpp
TWAExportResult exportCSVWithTWA();              // Generate OSHA-compliant TWA export
bool dumpTWAFile();                              // Output TWA export via serial
```

**TWAExportResult Structure:**
```cpp
struct TWAExportResult {
    float twa_pm1_0, twa_pm2_5, twa_pm4_0, twa_pm10;  // 8-hour TWAs (µg/m³)
    float dataCoverageHours;                            // Total hours analyzed
    bool oshaCompliant;                                 // ≥8 hours requirement
    uint32_t samplesAnalyzed;                           // Number of data points
    uint32_t dataGaps;                                  // Detected interruptions
    String exportStartTime, exportEndTime;             // Time period bounds
};
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

### Standard Sensor Log
```csv
timestamp,local_time,location,project,user,temperature,humidity,vocIndex,noxIndex,pm1_0,pm2_5,pm4_0,pm10,co2,dewPoint,heatIndex,absoluteHumidity,twa_pm1_0,twa_pm2_5,twa_pm4_0,twa_pm10
```

### OSHA-Compliant TWA Export
TWA export files include comprehensive regulatory headers:
```csv
# OSHA-Compliant 8-Hour Time-Weighted Average Report
# Generated by SEN66 Dosimetry System
# Export Time: 2025-12-11_16:00:00
# Period Start: 2025-12-11_08:00:00  
# Period End: 2025-12-11_16:00:00
# Reference: OSHA 29 CFR 1910.1000
#
# ========== TWA CALCULATION RESULTS ==========
# Data Coverage: 8.0 hours
# OSHA Compliant: YES (≥8 hours)
# PM1.0 8-hr TWA: 2.45 µg/m³
# PM2.5 8-hr TWA: 3.12 µg/m³  
# PM4.0 8-hr TWA: 3.78 µg/m³
# PM10 8-hr TWA: 4.23 µg/m³
# Samples Analyzed: 480
# Data Gaps Detected: 0
# ===============================================
#
[CSV data follows...]
```

**Note**: The `local_time` column shows timestamps in `yyyy-mm-dd_hh:mm:ss` format adjusted by the configured UTC offset.

## CLI Tool Usage

### Interactive Console
```bash
python sen66_cli.py console
```

### Available Commands
- `status` - Show current sensor measurements with live TWA values
- `config` - Display current configuration including UTC offset
- `timezone <offset>` - Set UTC offset (-12 to +14 hours)
- `prefs <key> <value>` - Configure measurement/logging intervals
- `download [file]` - Download CSV log file (automatically generates TWA export)
- **`export_twa`** - Generate dedicated OSHA-compliant TWA export
- `timesync` - Synchronize device time with PC (essential for accurate TWA calculations)
- `metadata` - Show/manage metadata (user, project, location)
- `clear` - Clear CSV log file
- `monitor` - Real-time sensor monitoring with live TWA display
- `about` - Show project information and license

### Command Line Examples
```bash
# Set timezone to EST (UTC-5)
python sen66_cli.py timezone --offset -5

# Synchronize time before data collection (critical for TWA accuracy)
python sen66_cli.py sync

# Download log file (automatically creates TWA export if ≥8hrs data)
python sen66_cli.py download --output my_data.csv

# Generate dedicated OSHA-compliant TWA export
python sen66_cli.py console
> export_twa

# Show current status with live TWA values
python sen66_cli.py status

# Monitor with real-time TWA calculations
python sen66_cli.py monitor
```

## TWA Calculation Mechanics

### Data Flow Architecture
```
1. Raw Sensor Data → 2. FastTWA (Live Display) → 3. CSV Storage
                  ↘                           ↗
                    4. ExportTWA (Regulatory) ←
```

### Calculation Methods

#### FastTWA Algorithm
- **Circular Buffer**: Maintains last N samples in RAM
- **Rolling Average**: Efficient O(1) updates using sum maintenance
- **Real-Time**: Updated every measurement cycle
- **Purpose**: Live monitoring and user feedback

#### ExportTWA Algorithm  
- **Time-Weighted Calculation**: `TWA = Σ(concentration × time_interval) / total_time`
- **Gap Handling**: Automatic detection of data interruptions
- **Chronological Verification**: Ensures proper timestamp ordering
- **Data Source**: Complete CSV file analysis for maximum accuracy

### OSHA Compliance Implementation

#### Regulatory Requirements (29 CFR 1910.1000)
- **Minimum Duration**: 8 hours of data required
- **Sampling Frequency**: Regular intervals (10-60 seconds supported)
- **Documentation**: Complete audit trail with calculation metadata
- **Gap Analysis**: Detection and reporting of data interruptions

#### Quality Assurance Features
- **Timestamp Validation**: Ensures chronological data order
- **Coverage Analysis**: Calculates actual monitoring duration
- **Sample Integrity**: Validates data completeness
- **Compliance Reporting**: Clear pass/fail determination

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

### Memory & Storage
- **RAM Usage**: ~32KB (includes dual TWA buffers)
- **Flash Usage**: ~440KB compiled binary
- **CSV Storage**: LittleFS filesystem on internal flash
- **FastTWA Buffer**: 1440 samples (24 hours at 60s interval)
- **ExportTWA**: Dynamic analysis of complete dataset

### Timing & Accuracy
- **Sampling Rate**: Configurable (10-60 seconds, default 20s)
- **TWA Window**: 8 hours (OSHA standard)  
- **Timestamp Resolution**: 1-second precision
- **UTC Offset Range**: -12 to +14 hours
- **Calculation Latency**: 
  - FastTWA: <1ms (real-time)
  - ExportTWA: <5s (full dataset analysis)

## Troubleshooting

### TWA Calculation Issues

**Problem**: TWA values show as zero or unrealistic numbers  
**Solutions**:
1. Ensure device time is synchronized: `python sen66_cli.py sync`
2. Allow sufficient data collection time (≥8 hours for OSHA compliance)  
3. Check for board resets that corrupt timestamps
4. Clear logs and collect fresh data after time sync

**Problem**: "Insufficient data" message in TWA export  
**Solutions**:
1. Verify data coverage ≥8 hours using `python sen66_cli.py status`
2. Check measurement interval settings: `python sen66_cli.py config`
3. Ensure continuous operation without power interruptions

**Problem**: Incorrect timestamp formatting in CSV  
**Solutions**:
1. Set correct UTC offset: `python sen66_cli.py timezone --offset <hours>`
2. Synchronize device time before data collection
3. Verify timezone setting in CLI console: `config` command

**Problem**: Large data coverage values (thousands of hours)  
**Solutions**:
1. This indicates timestamp corruption from board resets
2. Sync time: `python sen66_cli.py sync`  
3. Clear corrupted logs: `python sen66_cli.py clear`
4. Allow fresh data collection with stable power

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
