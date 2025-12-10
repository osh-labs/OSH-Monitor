SEN66-Dosimetry Project Context

Open-Source Arduino/PlatformIO Library for ESP32-S3
Advanced Air-Quality Acquisition, Dosimetry, and CSV Logging for the Sensirion SEN66

1. Overview

SEN66-Dosimetry is an open-source PlatformIO/Arduino library for ESP32-S3 devices, providing high-integrity sensor acquisition, derived environmental calculations, particulate dosimetry, and CSV data logging for the Sensirion SEN66 air-quality module.

The library is headless and provides no display/UI components. Its purpose is to serve as a robust sensor-data and exposure-analysis engine for long-duration environmental monitoring.

2. Purpose and Goals

The library aims to:

Read all raw SEN66 measurements over I²C

Compute multiple derived environmental metrics

Maintain rolling 8-hour time-weighted averages (TWAs) for PM exposure

Log timestamped readings to CSV files stored in LittleFS

Provide a stable, reusable C++ API suitable for industrial, research, and IoT applications

Be publishable as an open-source PlatformIO library

3. Supported Hardware and Software
Hardware

ESP32-S3 microcontrollers

Sensirion SEN66 module (I²C)

ESP32 internal flash memory using LittleFS

Software

PlatformIO with Arduino framework

C++ library exported according to PlatformIO library conventions

Compatible with other ESP32 variants that support LittleFS

4. Raw Sensor Data Supported

The SEN66-Dosimetry library must read and validate the following SEN66 measurements:

Temperature

Relative Humidity

VOC Index

NOx Index

PM1.0

PM2.5

PM4.0

PM10

CO₂ concentration (ppm)

All communication must include CRC-8 validation using the Sensirion polynomial (0x31, init 0xFF).

5. Derived Environmental Metrics

The library computes several environmental quantities:

Dew point (Magnus-type formula preferred)

Heat index (NOAA/Steadman approximation)

Absolute humidity (g/m³)

These values must be included alongside raw measurements for logging and downstream processing.

6. Dosimetry and 8-Hour TWA Calculations

The library performs continuous 8-hour arithmetic TWAs for:

PM1.0

PM2.5

PM4.0

PM10

Requirements:

Maintain RAM-resident circular buffers sized according to sampling interval

Efficient insertion and rollover behavior

Buffer resets cleanly on restart

TWA values updated immediately after each new sensor sample

No flash writes for TWA history (RAM only)

7. Unified Data Structure

The library exposes all raw and derived values in a single struct:

struct SensorData {
    // Raw measurements
    float temperature;
    float humidity;
    float vocIndex;
    float noxIndex;
    float pm1_0;
    float pm2_5;
    float pm4_0;
    float pm10;
    float co2;  // ppm

    // Derived metrics
    float dewPoint;
    float heatIndex;
    float absoluteHumidity;

    // 8-hour TWAs
    float twa_pm1_0;
    float twa_pm2_5;
    float twa_pm4_0;
    float twa_pm10;

    uint32_t timestamp; // epoch or uptime in seconds
};

8. CSV Logging System (LittleFS)

The CSV logging subsystem must support:

Append-only logging

Auto-create files with header row

Buffered writes to minimize flash wear

Optional file rotation by size or record count

Line-by-line retrieval for exporting or analysis

Safe erase behavior

Required CSV Header
timestamp,temperature,humidity,vocIndex,noxIndex,pm1_0,pm2_5,pm4_0,pm10,co2,
dewPoint,heatIndex,absoluteHumidity,
twa_pm1_0,twa_pm2_5,twa_pm4_0,twa_pm10


Each sample produces one row of CSV-formatted data.

9. Public API Requirements

The library must expose a clean, well-documented C++ API, including but not limited to:

bool begin();

bool readSensor();

SensorData getData();

void updateTWA(SensorData &data);

bool logEntry(const SensorData &data);

bool eraseLogs();

bool readLogLine(size_t index, String &line);

Design principles:

No UI or hardware display assumptions

No blocking delays beyond sensor needs

Predictable, memory-efficient behavior

All heavy operations isolated from the main acquisition loop

10. Repository Structure (Proposed)
SEN66-Dosimetry/
│
├── src/
│   ├── SEN66Dosimetry.cpp
│   └── SEN66Dosimetry.h
│
├── examples/
│   └── BasicLogger/
│       └── main.cpp
│
├── test/
│   └── basic/
│
├── library.json
├── README.md
├── CHANGELOG.md
├── LICENSE
└── CONTRIBUTING.md

11. PlatformIO Integration

A library.json file will define:

Name: SEN66-Dosimetry

Frameworks: Arduino

Platforms: espressif32

Semantic versioning (e.g., 0.1.0)

Repository URL

MIT or Apache 2.0 license

Exported include directories: /src

The library will be installable through PlatformIO registry once published.

12. Future Enhancements (Library Only)

Possible future extensions that remain display-independent:

JSON export helpers

Optional binary logging

AQI (US/EU) calculators

RTC synchronization utilities

BLE or serial streaming of log entries

Additional dosimetry windows (1h, 24h, custom)

CO₂ trend analytics

13. Summary

SEN66-Dosimetry is a standalone, headless, extensible air-quality measurement and dosimetry library for ESP32-S3 systems. It integrates SEN66 sensor acquisition, environmental math, long-term particulate exposure tracking, and reliable on-device CSV logging into a single, cohesive module designed for long-duration operation and platform-level reusability.