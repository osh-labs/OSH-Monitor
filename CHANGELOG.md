# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned Features
- JSON export helpers
- Optional binary logging format
- AQI (US/EU) calculators
- RTC synchronization utilities
- BLE streaming of log entries
- Additional dosimetry windows (1h, 24h, custom)
- CO₂ trend analytics

## [0.1.0] - 2025-12-10

### Added
- Initial release of SEN66-Dosimetry library
- Core sensor acquisition from Sensirion SEN66
  - Temperature, Humidity, VOC Index, NOx Index
  - PM1.0, PM2.5, PM4.0, PM10
  - CO₂ concentration (if supported by sensor)
- CRC-8 validation for all sensor communications
- Derived environmental metrics
  - Dew point calculation (Magnus formula)
  - Heat index calculation (NOAA/Steadman)
  - Absolute humidity (g/m³)
- 8-hour Time-Weighted Average (TWA) calculations
  - RAM-resident circular buffers
  - Configurable sampling intervals
  - Automatic rollover behavior
- CSV logging system with LittleFS
  - Append-only operation
  - Automatic file creation with headers
  - Buffered writes for flash longevity
  - Line-by-line retrieval API
  - Safe log erasure
- Complete public API
  - `begin()` - Initialize system
  - `readSensor()` - Read all measurements
  - `getData()` - Retrieve current data
  - `updateTWA()` - Update exposure calculations
  - `logEntry()` - Write to CSV
  - `eraseLogs()` - Clear all logs
  - `readLogLine()` - Read specific log entry
- Example sketches
  - BasicLogger - Simple periodic logging
- Documentation
  - README with quick start guide
  - API reference
  - Hardware wiring guide
- PlatformIO library configuration
  - library.json metadata
  - ESP32-S3 platform support
  - Arduino framework compatibility

### Technical Details
- I²C communication at 100kHz (configurable)
- Sensirion CRC-8 polynomial (0x31, init 0xFF)
- Default 60-second sampling interval
- 8-hour TWA window (480 samples)
- Minimal flash wear with buffered writes
- Non-blocking operation suitable for long-term deployment

[Unreleased]: https://github.com/yourusername/SEN66-Dosimetry/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/yourusername/SEN66-Dosimetry/releases/tag/v0.1.0
