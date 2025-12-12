# SEN66Core

**Hardware Abstraction Library for Sensirion SEN66 Air Quality Sensor**

[![License](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32--S3-green)](https://www.espressif.com/en/products/socs/esp32-s3)

## Overview

SEN66Core is a hardware abstraction library that provides a clean API for interfacing with the Sensirion SEN66 multi-sensor module. It handles I2C communication, CRC validation, raw data acquisition, and environmental metric calculations, making it easy to integrate the SEN66 into any ESP32-based project.

## Features

- 🔌 **I2C Communication** - Automatic initialization and error handling
- ✅ **CRC Validation** - Built-in data integrity checking via Sensirion library
- 📊 **Raw Sensor Data** - Temperature, humidity, PM1.0/2.5/4.0/10, VOC, NOx, CO₂
- 🧮 **Derived Metrics** - Dew point, heat index, absolute humidity calculations
- 🛡️ **Error Management** - Comprehensive error reporting and status checking
- 🔄 **Sensor Control** - Start/stop measurements, device reset, serial number retrieval

## Measured Parameters

### Raw Measurements
- **Temperature** (°C)
- **Relative Humidity** (%)
- **Particulate Matter** - PM1.0, PM2.5, PM4.0, PM10 (µg/m³)
- **VOC Index** (1-500)
- **NOx Index** (1-500)
- **CO₂** (ppm)

### Derived Environmental Metrics
- **Dew Point** (°C) - Magnus formula
- **Heat Index** (°C) - NOAA/Steadman approximation
- **Absolute Humidity** (g/m³) - Magnus-Tetens calculation

## Installation

### PlatformIO

Add to your `platformio.ini`:

```ini
lib_deps = 
    https://github.com/duluthmachineworks/SEN66-Dosimetry.git#lib/SEN66Core
    Sensirion I2C SEN66
```

### Arduino IDE

1. Download this library
2. Place in Arduino libraries folder
3. Install [Sensirion I2C SEN66 library](https://github.com/Sensirion/arduino-i2c-sen66) via Library Manager

## Hardware Setup

### Wiring (ESP32-S3)

```
SEN66     ESP32-S3
------    --------
VDD   →   3.3V or 5V
GND   →   GND
SDA   →   GPIO 21 (default, configurable)
SCL   →   GPIO 22 (default, configurable)
```

### I2C Pull-up Resistors

The SEN66 requires pull-up resistors on SDA and SCL lines (typically 4.7kΩ - 10kΩ). Many development boards include these; check your schematic.

## Quick Start

```cpp
#include <SEN66Core.h>

SEN66Core sensor;

void setup() {
    Serial.begin(115200);
    
    // Initialize sensor (SDA=21, SCL=22, 100kHz)
    if (!sensor.begin(21, 22, 100000)) {
        Serial.println("Failed to initialize SEN66!");
        Serial.println("Error: " + sensor.getLastError());
        while (1) delay(100);
    }
    
    Serial.println("SEN66 initialized!");
    Serial.println("S/N: " + sensor.getSerialNumber());
}

void loop() {
    SEN66FullData data;
    
    if (sensor.readFullData(data)) {
        Serial.printf("Temp: %.2f°C, Humidity: %.2f%%\\n", 
                      data.raw.temperature, data.raw.humidity);
        Serial.printf("PM2.5: %.2f µg/m³, CO2: %.0f ppm\\n", 
                      data.raw.pm2_5, data.raw.co2);
        Serial.printf("Dew Point: %.2f°C\\n", data.derived.dewPoint);
    } else {
        Serial.println("Read error: " + sensor.getLastError());
    }
    
    delay(5000);
}
```

## API Reference

### Class: SEN66Core

#### Constructor

```cpp
SEN66Core(TwoWire &wire = Wire)
```

Create sensor instance with specified I2C bus.

#### Initialization

```cpp
bool begin(int sdaPin = 21, int sclPin = 22, uint32_t i2cFreq = 100000)
```

Initialize sensor, I2C bus, and start measurements.

**Parameters:**
- `sdaPin` - I2C SDA pin (default: 21)
- `sclPin` - I2C SCL pin (default: 22)
- `i2cFreq` - I2C frequency in Hz (default: 100000)

**Returns:** `true` if initialization successful

#### Sensor Control

```cpp
bool startMeasurement()
bool stopMeasurement()
bool deviceReset()
String getSerialNumber()
```

Control sensor operation and retrieve device information.

#### Data Acquisition

```cpp
bool readRawData(SEN66RawData &data)
bool readFullData(SEN66FullData &data)
```

Read sensor measurements. `readFullData()` includes raw data plus calculated derived metrics.

#### Static Calculation Methods

```cpp
static float calculateDewPoint(float temp, float humidity)
static float calculateHeatIndex(float temp, float humidity)
static float calculateAbsoluteHumidity(float temp, float humidity)
```

Calculate environmental metrics from temperature and humidity. Can be used independently of sensor reads.

#### Status Methods

```cpp
bool isReady() const
String getLastError() const
```

Check sensor status and retrieve error messages.

### Data Structures

#### SEN66RawData

```cpp
struct SEN66RawData {
    float temperature;    // °C
    float humidity;       // %RH
    float vocIndex;       // 1-500
    float noxIndex;       // 1-500
    float pm1_0;          // µg/m³
    float pm2_5;          // µg/m³
    float pm4_0;          // µg/m³
    float pm10;           // µg/m³
    float co2;            // ppm
};
```

#### SEN66DerivedData

```cpp
struct SEN66DerivedData {
    float dewPoint;           // °C
    float heatIndex;          // °C
    float absoluteHumidity;   // g/m³
};
```

#### SEN66FullData

```cpp
struct SEN66FullData {
    SEN66RawData raw;
    SEN66DerivedData derived;
    bool valid;  // true if read successful
};
```

## Environmental Calculations

### Dew Point (Magnus Formula)

$$T_d = \\frac{b \\cdot \\alpha}{a - \\alpha}$$

where $\\alpha = \\frac{a \\cdot T}{b + T} + \\ln(RH/100)$, $a = 17.27$, $b = 237.7$

### Heat Index (NOAA/Steadman)

Rothfusz regression for temperatures > 80°F:

$$HI = -42.379 + 2.049T + 10.143RH - 0.225T \\cdot RH - 0.00684T^2 - 0.0548RH^2 + 0.00123T^2 \\cdot RH + 0.000853T \\cdot RH^2 - 0.00000199T^2 \\cdot RH^2$$

### Absolute Humidity

$$AH = \\frac{e \\cdot M}{R \\cdot T}$$

where $e$ is vapor pressure (Pa), $M = 18.015$ g/mol, $R = 8.314$ J/(mol·K)

## Examples

See [`examples/BasicSEN66/BasicSEN66.ino`](examples/BasicSEN66/BasicSEN66.ino) for a complete working example with formatted serial output.

## Integration with OSH-Monitor

SEN66Core is designed as the hardware abstraction layer for the OSH-Monitor platform. The platform handles:
- CSV logging with timestamps
- Time-weighted average (TWA) calculations
- RTC management
- Serial CLI interface
- Metadata and configuration

Use SEN66Core directly for simple sensor projects, or integrate with OSH-Monitor for full environmental monitoring and compliance reporting.

## Error Handling

Always check return values and use `getLastError()` for diagnostics:

```cpp
if (!sensor.readFullData(data)) {
    Serial.println("Read failed!");
    Serial.println("Reason: " + sensor.getLastError());
    
    // Possible errors:
    // - "Sensor not initialized"
    // - "Failed to read sensor values, error code: X"
    // - "Device reset failed with error: X"
}
```

## Troubleshooting

### Initialization Fails
- Check I2C wiring (SDA, SCL, power, ground)
- Verify pull-up resistors are present (4.7kΩ - 10kΩ)
- Confirm sensor power supply (3.3V or 5V)
- Try different I2C pins if using non-default

### Invalid Readings
- Allow 2 seconds warm-up time after `begin()`
- Check for I2C bus conflicts with other devices
- Verify I2C frequency (default 100kHz recommended)

### CRC Errors
- Check for loose connections
- Reduce I2C bus length/capacitance
- Add/adjust pull-up resistor values

## Performance Notes

- **Measurement Interval**: Sensor provides new data ~1 Hz
- **I2C Communication**: ~20ms per read at 100kHz
- **Memory Footprint**: ~200 bytes RAM for data structures
- **Calculation Overhead**: Derived metrics add ~1ms processing time

## License

GNU General Public License v3.0 (GPLv3)

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

## Author

Christopher Lee - [https://github.com/duluthmachineworks](https://github.com/duluthmachineworks)

## Related Projects

- **OSH-Monitor** - Open Source Hardware platform for occupational safety monitoring
- **TWACore** - OSHA-compliant time-weighted average calculation library

## Contributing

Contributions are welcome! Please submit pull requests or open issues on the [GitHub repository](https://github.com/duluthmachineworks/SEN66-Dosimetry).
