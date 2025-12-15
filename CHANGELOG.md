# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned Features
- JSON export helpers
- Optional binary logging format
- AQI (US/EU) calculators
- External RTC synchronization utilities
- BLE streaming of log entries
- Additional dosimetry windows (1h, 24h, custom)
- CO₂ trend analytics

## [1.2.1] - 2025-12-15

### Fixed
- TWA export failing on large datasets (>4000 data points, ~642KB CSV files exceeding ESP32-S3 heap memory)
- Storage command returning "no storage information received" due to memory overflow
- Clear command taking 10-30 seconds to erase large log files

### Changed
- **TWACore 1.0.0 → 1.1.0**: Added duration validation in analytics layer
  - Added `exceedsMaxDuration` field to `TWAExportResult` struct
  - Enforces single-shift validation: 8-10 hours valid, >10 hours rejected
  - Prevents incorrect OSHA compliance reporting across shift boundaries
- **Performance**: Replaced slow `LittleFS.remove()` with instant truncate (`open("w")`) in 4 locations:
  - `OSHMonitor::eraseLogs()`
  - `OSHMonitor::setMetadata()` (log clearing)
  - `main.cpp` clear command
  - `main.cpp` resetmeta command
  - Result: Clear operations now complete in <100ms (previously 10-30 seconds)
- Export headers now show actual dataset duration dynamically
- Duration validation moved from platform layer (OSHMonitor) to analytics layer (TWACore) for proper separation of concerns

### Added
- 10-hour maximum duration limit for on-device TWA export
- Version management checklist in `claude.md` documenting all version number locations
- Duration validation documentation in TWACore Developer Guide
- TWA duration limits note in main README

### Technical Details
- Architecture: Duration validation now enforced at TWACore library level (analytics layer)
- OSHA Compliance: Single-shift boundary enforcement prevents multi-shift data aggregation
- Memory: Validates duration before processing to fail fast on datasets that would exceed heap
- Build: RAM usage 9.7% (31,716/327,680 bytes), Flash 34.9% (457,257/1,310,720 bytes)

## [1.2.0] - 2025-12-12

### Added
- **Storage monitoring system** with dynamic capacity estimation
  - `StorageStats` structure tracking total/used/free bytes, percentage, hours remaining
  - `getStorageStats()` method with 1.5x multiplier for TWA export overhead
  - `calculateAverageBytesPerEntry()` for dynamic log file analysis
  - Configurable storage warning threshold (default 80%, range 1-99%)
  - Automatic warning when threshold exceeded during logging
  - `storage` command displaying filesystem statistics
  - `prefs storage_warning <1-99>` command for threshold configuration
- **Firmware version tracking**
  - `FIRMWARE_VERSION` constant in main.cpp (1.2.0)
  - Version displayed on startup banner
  - Firmware, SEN66Core, and TWACore versions stored in metadata
  - Versions persist across reboots and appear in CSV exports
- CLI tool integration for remote storage monitoring
- Human-readable byte formatting (B, KB, MB) throughout codebase
- NVS persistence for storage warning threshold

### Changed
- Updated README.md with storage commands
- Updated CLI-Quickstart.md with storage monitoring documentation

### Technical Details
- Build: RAM usage 9.7% (31,716 bytes), Flash 34.8% (455,865 bytes)
- Storage estimation accounts for TWA export overhead
- Hours remaining calculated from current logging interval

## [1.1.1] - 2025-12-12

### Fixed (SEN66Core)
- Error handling distinction between hardware failures and validation failures
- `readRawData()` now properly sets ERROR state only on I2C communication failures
- Validation failures return false without state change for graceful recovery

### Changed (SEN66Core)
- Consistent error message format: [Hardware] vs [Validation] prefixes
- Maintain idempotent start/stop behavior for power cycling
- Updated SEN66Core-Developer-Guide.md with error handling philosophy

### Technical Details
- SEN66Core: 1.1.0 → 1.1.1
- Rationale: Hardware errors require reset; validation errors allow immediate retry

## [1.1.0] - 2025-12-12

### Added (SEN66Core)
- **Sensor state tracking** for power management
  - `SensorState` enum: UNINITIALIZED, INITIALIZING, IDLE, MEASURING, ERROR
  - `getState()` and `isMeasuring()` query methods
  - Idempotent `start/stopMeasurement()` with state validation
- I2C pin configuration documentation in SEN66Core Developer Guide

### Changed
- README streamlined to remove technical details duplicated in developer guides
- All code examples updated to use actual GPIO pins (3/4 for Adafruit Feather ESP32-S3)
- API examples corrected to match implementation (variable names, method signatures)
- Developer guide table formatting improvements

### Technical Details
- SEN66Core: 1.0.1 → 1.1.0
- Enables safe power cycling and battery-powered operation modes

### Refactored
- Replaced `_initialized` boolean with `_state` enum in SEN66Core
- Platform architecture emphasized throughout documentation

## [1.0.5] - 2025-12-11

### Added
- **TWACore library**: Standalone sensor-agnostic OSHA-compliant TWA calculation library
  - Generic map-based API supporting arbitrary parameters
  - Dynamic CSV column mapping with header row scanning
  - `FastTWA` class: Real-time circular buffer (RAM-resident, O(1) updates)
  - `ExportTWA` class: Regulatory compliance (CSV analysis, time-weighted precision)
  - Comprehensive documentation with reusability examples
- RTC functionality utilizing internal ESP32 RTC
- Compliance TWA export option for regulatory reporting
- Customizable metadata handling system
- Time localization support

### Changed
- Removed 330+ lines of duplicate TWA code from platform layer
- Updated all headers and license information
- README updated with three-library architecture explanation

### Fixed (SEN66Core)
- Complete NOAA heat index algorithm implementation
- Heat index calculation accuracy improvements

### Technical Details
- TWACore: Initial release (v1.0.0)
- SEN66Core: 1.0.0 → 1.0.1
- Dual TWA architecture: FastTWA for monitoring, ExportTWA for compliance

## [1.1.0] - 2025-12-11 (Platform Rename)

### Changed
- **Major refactor**: SEN66-Dosimetry → OSH-Monitor platform architecture
  - Platform class: `SEN66Dosimetry` → `OSHMonitor` (71 methods updated)
  - Source files: `SEN66Dosimetry.h/.cpp` → `OSHMonitor.h/.cpp`
  - CLI tool: `sen66_cli.py` → `osh_cli.py` (`SEN66CLI` → `OSHCLI`)
  - NVS namespaces: `"sen66"` → `"osh-mon"`, `"sen66-meta"` → `"osh-meta"`
  - Header guards: `SEN66_DOSIMETRY_H` → `OSH_MONITOR_H`
  - CSV headers: "OSH-Monitor Air Quality Data Log"
- Repository moved to duluthmachineworks/OSH-Monitor
- Documentation rewritten to emphasize sensor-agnostic platform architecture
- README roadmap focused on file system integrity, sensor health, alarms

### Preserved
- SEN66Core and TWACore libraries unchanged (sensor-agnostic design)
- All functionality intact, zero breaking changes to core features
- Data structures and API compatibility maintained

### Added
- SEN66Core hardware abstraction layer extraction
- Comprehensive documentation suite
- Platform deployment workflow guidance

### Technical Details
- Transformation from sensor-specific library to sensor-agnostic platform
- Three-library architecture: Hardware (SEN66Core) + Analytics (TWACore) + Platform (OSHMonitor)
- Build verified, hardware validated, TWA calculations operational

## [0.1.0] - 2025-12-10

### Added
- Initial release of OSH-Monitor library
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

[Unreleased]: https://github.com/osh-labs/OSH-Monitor/compare/v1.2.1...HEAD
[1.2.1]: https://github.com/osh-labs/OSH-Monitor/compare/v1.2.0...v1.2.1
[1.2.0]: https://github.com/osh-labs/OSH-Monitor/compare/v1.1.1...v1.2.0
[1.1.1]: https://github.com/osh-labs/OSH-Monitor/compare/v1.1.0...v1.1.1
[1.1.0]: https://github.com/osh-labs/OSH-Monitor/compare/v1.0.5...v1.1.0
[1.0.5]: https://github.com/osh-labs/OSH-Monitor/compare/v0.1.0...v1.0.5
[0.1.0]: https://github.com/osh-labs/OSH-Monitor/releases/tag/v0.1.0
