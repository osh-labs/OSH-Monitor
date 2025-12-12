# OSH-Monitor

**Open-Source Occupational Safety & Health Monitoring Platform**

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Platform-orange)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue)](https://www.arduino.cc/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32--S3-green)](https://www.espressif.com/en/products/socs/esp32-s3)
[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)

## Overview

OSH-Monitor is a complete **sensor-agnostic monitoring platform** for ESP32-S3 devices that orchestrates specialized hardware and analytics libraries to provide high-integrity environmental monitoring, OSHA-compliant dosimetry, and regulatory reporting. Built on PlatformIO/Arduino, the platform architecture separates hardware abstraction (sensor drivers), mathematical analytics (TWA calculations), and platform orchestration (data management, logging, user interface) into independent, reusable libraries.

The platform includes:
- **Complete ESP32-S3 firmware** with modular three-library architecture
- **Sensor abstraction layer** supporting multiple air quality sensors
- **OSHA-compliant analytics** with dual-mode TWA calculations
- **Python CLI tool** for device interaction, configuration, and data management
- **Regulatory reporting** with automated OSHA 29 CFR 1910.1000 compliance exports

## Architecture

### Three-Library Platform Design

OSH-Monitor is built as a **platform orchestration layer** that coordinates specialized libraries, enabling sensor-agnostic monitoring and reusable analytics components:

**1. Hardware Abstraction Layer (Sensor Libraries)**
- **SEN66Core** - Sensirion SEN66 air quality sensor driver
  - I2C communication with CRC-8 validation
  - Raw sensor acquisition (PM, VOC, NOx, CO₂, temp, humidity)
  - Derived environmental calculations (dew point, heat index, absolute humidity)
  - Lives in: `lib/SEN66Core/`
- **Future sensors**: BME680Core, SCD40Core, PMS5003Core, etc.

**2. Mathematics & Analytics Layer (Sensor-Agnostic)**
- **TWACore** - OSHA-compliant Time-Weighted Average calculations
  - Generic TWA engine supporting arbitrary parameters
  - Dual-mode architecture (FastTWA + ExportTWA)
  - Regulatory compliance reporting (OSHA 29 CFR 1910.1000)
  - Lives in: `lib/TWACore/`
  - Reusable across noise, gas, radiation, and environmental monitoring

**3. Platform Orchestration Layer (OSH-Monitor)**
- **OSHMonitor** - Main platform firmware integrating all components
  - Sensor lifecycle management and data coordination
  - CSV data logging with LittleFS filesystem
  - ESP32 RTC time management with UTC offset support
  - NVS-backed configuration and metadata persistence
  - Serial command interface and Python CLI integration
  - Lives in: `src/OSHMonitor.h/.cpp`

### Extensibility Benefits

✅ **Add sensors** without modifying platform code (plug-in architecture)  
✅ **Reuse TWA calculations** in other monitoring applications  
✅ **Independent versioning** of sensor drivers and analytics  
✅ **Clear separation** of hardware concerns from business logic  
✅ **Testable components** with well-defined interfaces

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

### ⏱️ Time-Weighted Average (TWA) System
**OSHA-Compliant 8-Hour Exposure Calculations**

The platform provides dual-mode TWA calculations for real-time monitoring and regulatory reporting:

- **FastTWA**: Real-time TWA display updated every measurement cycle for live monitoring
- **ExportTWA**: Precise OSHA-compliant 8-hour TWA reports from stored CSV data
- **Compliance**: Automated validation against OSHA 29 CFR 1910.1000 standards (≥8 hours data)
- **Sensor-Agnostic**: Reusable TWA engine supporting any environmental parameters

> **📘 For Implementation Details**: See [TWACore Developer Guide](docs/TWACore-Developer-Guide.md) for architecture details, algorithms, design patterns, and integration examples.

### 💾 CSV Logging (LittleFS)
- Append-only logging
- Auto-create files with header row
- Buffered writes to minimize flash wear
- Line-by-line retrieval
- Safe erase behavior
- **Localized timestamps** with configurable UTC offset (-12 to +14 hours)
- Dual timestamp format: Unix timestamp + human-readable local time

### 🖥️ Interactive CLI Tool
- **Python-based command-line interface** (`osh_cli.py`)
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

### Hardware Configuration

The platform uses I2C communication with the following pin configuration for Adafruit Feather ESP32-S3 Reverse TFT:
- **SDA:** GPIO 3
- **SCL:** GPIO 4
- **Frequency:** 100 kHz (I2C standard mode)

The SEN66 sensor uses a fixed I2C address (0x6B) and can share the I2C bus with other devices.

> **📘 For Hardware Details**: See [SEN66Core Developer Guide](docs/SEN66Core-Developer-Guide.md) for I2C timing requirements, alternative pin configurations, wiring diagrams, and multi-sensor bus configurations.

## Installation

### Deploying the Platform

OSH-Monitor is a **complete firmware platform**, not a library to be included in other projects. Deploy it as a standalone PlatformIO project:

#### Method 1: Clone Repository (Recommended)
```bash
git clone https://github.com/duluthmachineworks/OSH-Monitor.git
cd OSH-Monitor
pio run -e adafruit_feather_esp32s3_reversetft
pio run -t upload
```

#### Method 2: Fork for Customization
1. Fork the repository on GitHub
2. Clone your fork locally
3. Modify sensor configurations in `src/main.cpp`
4. Add new sensor libraries to `lib/` directory
5. Update `platformio.ini` if needed
6. Build and deploy

### Using Individual Libraries

The platform's component libraries can be extracted and used independently:

**SEN66Core** (Sensor Driver):
```ini
lib_deps = 
    https://github.com/duluthmachineworks/OSH-Monitor.git#lib/SEN66Core
```

**TWACore** (TWA Calculations):
```ini
lib_deps = 
    https://github.com/duluthmachineworks/OSH-Monitor.git#lib/TWACore
```

**Note**: These are published as part of the OSH-Monitor platform repository but can be referenced independently for use in other projects.

## Quick Start

### 1. Deploy the Platform
```bash
# Clone and build
git clone https://github.com/duluthmachineworks/OSH-Monitor.git
cd OSH-Monitor
pio run -t upload

# Open serial monitor to see output
pio device monitor
```

### 2. Configure via Python CLI
```bash
# Install Python CLI dependencies
pip install pyserial

# Synchronize time (critical for TWA calculations)
python osh_cli.py rtc-sync

# Set timezone (example: EST = UTC-5)
python osh_cli.py console
> timezone -5

# Check status
> status
```

### 3. Monitor and Collect Data
```bash
# Real-time monitoring
python osh_cli.py monitor

# Download data with automatic TWA export
python osh_cli.py download --output my_data.csv
```

### Platform Code Structure (src/main.cpp)
```cpp
#include <Arduino.h>
#include "OSHMonitor.h"

// I2C Pin Definitions (Adafruit Feather ESP32-S3 Reverse TFT)
#define SDA_PIN 3
#define SCL_PIN 4

// Platform instantiation with 20-second sampling
OSHMonitor airQualitySensor(Wire, 20);

void setup() {
    Serial.begin(115200);
    
    // Initialize platform (I2C SDA=3, SCL=4)
    if (!airQualitySensor.begin(SDA_PIN, SCL_PIN)) {
        Serial.println("Platform initialization failed!");
        while(1) delay(1000);
    }
    
    Serial.println("OSH-Monitor Platform Ready!");
}

void loop() {
    // Platform orchestration loop
    if (airQualitySensor.readSensor()) {
        SensorData data = airQualitySensor.getData();
        airQualitySensor.updateTWA(data);  // Real-time TWA
        airQualitySensor.logEntry(data);   // CSV logging
        
        Serial.printf("Temp: %.2f°C, PM2.5: %.2f µg/m³\n", 
                     data.temperature, data.pm2_5);
    }
    
    delay(20000);  // 20-second measurement interval
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

The platform provides a simple high-level API for sensor operations, data logging, and TWA calculations:

```cpp
// Initialization
OSHMonitor airQualitySensor(Wire, 20);  // 20-second sampling interval
airQualitySensor.begin(3, 4);            // I2C pins: SDA=3, SCL=4

// Core operations in main loop
airQualitySensor.readSensor();                // Read all measurements
SensorData data = airQualitySensor.getData(); // Get sensor data
airQualitySensor.updateTWA(data);             // Update real-time TWA
airQualitySensor.logEntry(data);              // Log to CSV

// TWA export and compliance
airQualitySensor.exportCSVWithTWA("/twa_export.csv");
TWAExportResult report = airQualitySensor.getLastTWAExport();
```

> **📘 For Complete API Documentation**: See developer guides for detailed method signatures, data structures, error handling, and integration patterns:
> - [SEN66Core Developer Guide](docs/SEN66Core-Developer-Guide.md) - Sensor API and data structures
> - [TWACore Developer Guide](docs/TWACore-Developer-Guide.md) - TWA calculation API
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
# Generated by OSH-Monitor System
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
python osh_cli.py console
```

### Available Commands
- `status` - Show current sensor measurements with live TWA values
- `storage` - Display filesystem storage statistics and capacity estimates
- `config` - Display current configuration including UTC offset
- `timezone <offset>` - Set UTC offset (-12 to +14 hours)
- `prefs <key> <value>` - Configure measurement/logging intervals and storage warning threshold
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
python osh_cli.py timezone --offset -5

# Synchronize time before data collection (critical for TWA accuracy)
python osh_cli.py sync

# Download log file (automatically creates TWA export if ≥8hrs data)
python osh_cli.py download --output my_data.csv

# Generate dedicated OSHA-compliant TWA export
python osh_cli.py console
> export_twa

# Show current status with live TWA values
python osh_cli.py status

# Check storage capacity and estimated logging time remaining
python osh_cli.py console
> storage

# Configure storage warning threshold (1-99%, default 80)
python osh_cli.py console
> prefs storage_warning 85

# Monitor with real-time TWA calculations
python osh_cli.py monitor
```

## Developer Documentation

For detailed technical documentation on platform internals:

- **[TWACore Developer Guide](docs/TWACore-Developer-Guide.md)** - Complete guide to TWA calculation architecture, algorithms, design patterns, common pitfalls, and integration examples for new sensor types
- **[SEN66Core Developer Guide](docs/SEN66Core-Developer-Guide.md)** - Sensor driver implementation details and hardware integration
- **[CLI Developer Guide](docs/CLI_README.md)** - Python CLI architecture and command implementation

## Examples

See the `examples/` directory for complete examples:
- **BasicLogger**: Simple periodic logging with serial output

## Wiring

Connect the SEN66 sensor to ESP32-S3 using I2C (Adafruit Feather ESP32-S3 Reverse TFT):

| SEN66 Pin | ESP32-S3 Pin | Description |
|-----------|--------------|-------------|
| VDD       | 3.3V or 5V   | Power supply |
| GND       | GND          | Ground |
| SDA       | GPIO 3       | I2C Data |
| SCL       | GPIO 4       | I2C Clock |

> **📘 For Detailed Wiring**: See [SEN66Core Developer Guide](docs/SEN66Core-Developer-Guide.md) for timing requirements, multi-sensor configurations, and troubleshooting.

## Performance

- **Sampling Rate**: 10-60 seconds (configurable, default 20s)
- **TWA Window**: 8 hours (OSHA standard)
- **Data Storage**: LittleFS filesystem on internal flash
- **Memory Footprint**: ~32KB RAM, ~440KB flash
- **Real-time TWA**: <1ms calculation latency
- **Export Generation**: <5s for complete dataset analysis

## Troubleshooting

### TWA Calculation Issues

**Problem**: TWA values show as zero or unrealistic numbers  
**Solutions**:
1. Ensure device time is synchronized: `python osh_cli.py sync`
2. Allow sufficient data collection time (≥8 hours for OSHA compliance)  
3. Check for board resets that corrupt timestamps
4. Clear logs and collect fresh data after time sync

**Problem**: "Insufficient data" message in TWA export  
**Solutions**:
1. Verify data coverage ≥8 hours using `python osh_cli.py status`
2. Check measurement interval settings: `python osh_cli.py config`
3. Ensure continuous operation without power interruptions

**Problem**: Incorrect timestamp formatting in CSV  
**Solutions**:
1. Set correct UTC offset: `python osh_cli.py timezone --offset <hours>`
2. Synchronize device time before data collection
3. Verify timezone setting in CLI console: `config` command

**Problem**: Large data coverage values (thousands of hours)  
**Solutions**:
1. This indicates timestamp corruption from board resets
2. Sync time: `python osh_cli.py sync`  
3. Clear corrupted logs: `python osh_cli.py clear`
4. Allow fresh data collection with stable power

## Platform Status

### Current Release: v1.1.0

**Supported Hardware:**
- ✅ ESP32-S3 (Adafruit Feather ESP32-S3 Reverse TFT)
- ✅ Generic ESP32-S3 boards with Arduino framework support

**Supported Sensors:**
- ✅ Sensirion SEN66 (via SEN66Core v1.0.1)
- 🔄 BME680 (planned)
- 🔄 SCD40 (planned)
- 🔄 PMS5003 (planned)

**Production Readiness:**
- ✅ Firmware: Production-ready
- ✅ SEN66Core: Stable
- ✅ TWACore: Stable
- ✅ Python CLI: Stable
- ✅ OSHA Compliance: Validated

### Roadmap

**Near-term:**
- Additional sensor driver libraries
- File system integrity and data validation
- Sensor health monitoring and diagnostics
- Alarm/threshold system for real-time alerts
- Web interface for configuration
- Battery-powered operation modes

**Long-term:**
- Wi-Fi time synchronization (NTP/SNTP)
- GPS time synchronization
- Additional data export formats (JSON, Excel)
- Cloud data synchronization

## Contributing

Contributions to the OSH-Monitor platform are welcome! Areas for contribution:

- **New sensor drivers** (add to `lib/` following SEN66Core pattern)
- **Platform enhancements** (RTC improvements, metadata system, CLI features)
- **Analytics extensions** (new TWA algorithms, compliance standards)
- **Documentation** (sensor integration guides, deployment tutorials)
- **Testing** (hardware validation, edge case discovery)

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on platform architecture and coding standards.

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
