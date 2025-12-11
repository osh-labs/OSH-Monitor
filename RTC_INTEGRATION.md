# ESP32-S3 RTC Integration for SEN66-Dosimetry

## Overview

The SEN66-Dosimetry project has been enhanced with ESP32-S3 Real-Time Clock (RTC) integration to provide accurate and persistent timekeeping across power cycles. This addresses critical issues with timestamp corruption and improves the reliability of TWA (Time-Weighted Average) calculations for OSHA compliance monitoring.

## Problem Solved

### Before RTC Integration
- **Timestamp corruption**: ESP32 resets caused backwards timestamps and impossible TWA values (e.g., 1,193,046 hours coverage)
- **Manual synchronization**: Required external time sync after every reboot
- **Data continuity issues**: Lost time reference across power cycles
- **Dependency on external sync**: Relied on Python CLI or serial commands for time

### After RTC Integration
- **Persistent timekeeping**: Continuous time across power cycles using ESP32-S3 built-in RTC
- **Automatic initialization**: RTC automatically maintains time with crystal accuracy
- **Robust TWA calculations**: Eliminates impossible time values and improves compliance reliability
- **Reduced maintenance**: Minimal manual intervention required

## Technical Implementation

### Hardware Requirements
- **ESP32-S3** with built-in RTC capability
- **32.768kHz crystal** for RTC accuracy (typically built into dev boards)
- **Backup power** (optional): For extended timekeeping during extended power loss

### Software Architecture

#### 1. Time Source Hierarchy
```cpp
enum TimeSource {
    TIME_SOURCE_RTC,      // ESP32-S3 RTC (primary)
    TIME_SOURCE_UPTIME    // Fallback millis() counter
};
```

#### 2. Core RTC Functions
- `initializeRTC()`: Initialize and validate RTC on boot
- `setRTCTime()`: Synchronize RTC to Unix timestamp
- `getRTCTime()`: Read current RTC timestamp
- `getCurrentTimestamp()`: Intelligent timestamp selection
- `needsRTCSync()`: Check if periodic sync is recommended

#### 3. Simplified Design
The system uses ESP32-S3 RTC as the primary time source, with millis()-based fallback only when RTC is not available. This eliminates complexity and potential confusion from multiple timing methods.

## Usage Guide

### Serial Commands (ESP32)

#### Check RTC Status
```
rtc status
```
Shows detailed RTC information:
- Initialization status
- Current RTC time
- Last synchronization time
- Active time source
- Sync recommendations

#### Synchronize RTC
```
rtc sync <unix_timestamp>
```
Sets the ESP32 RTC to the specified Unix timestamp.

Example:
```
rtc sync 1702982400
```

### Python CLI Tool

#### Show RTC Status
```bash
# Command line
python sen66_cli.py rtc-status

# Interactive console
> rtc status
```

#### Synchronize RTC with PC Time
```bash
# Command line
python sen66_cli.py rtc-sync

# Interactive console
> rtc sync
```

### Simplified Architecture
The legacy `timesync` functionality has been removed to streamline the system. The SEN66-Dosimetry now uses only ESP32-S3 RTC for timekeeping, with millis() fallback when RTC is unavailable.

## Configuration

### Default Behavior
1. **Boot Sequence**: RTC initialization runs automatically in `begin()`
2. **Time Source Selection**: Automatic priority: RTC → Legacy Sync → Millis Only
3. **Sync Interval**: RTC sync recommended every 24 hours (86400 seconds)
4. **Validation**: RTC time must be after January 1, 2024 to be considered valid

### Customization
Modify these constants in `SEN66Dosimetry.h`:
```cpp
#define RTC_SYNC_INTERVAL 86400     // Sync recommendation interval (seconds)
#define RTC_MIN_VALID_TIME 1704067200  // January 1, 2024 (Unix timestamp)
```

## Benefits for OSHA Compliance

### Enhanced Reliability
- **Continuous TWA calculations**: No time gaps from resets
- **Accurate audit trails**: Reliable timestamps for regulatory documentation
- **Data integrity**: Eliminates backwards time jumps that corrupt calculations

### Improved Operational Efficiency
- **Reduced maintenance**: Less manual time synchronization
- **Automatic recovery**: RTC maintains time through power cycles
- **Better monitoring**: Clear status reporting for time source validation

### Regulatory Advantages
- **29 CFR 1910.1000 compliance**: More reliable 8-hour TWA calculations
- **Audit readiness**: Consistent timestamp methodology
- **Documentation quality**: Improved data traceability

## Migration Guide

### Existing Deployments
1. **Firmware update required**: Flash updated firmware with RTC-only functionality
2. **Breaking change**: Legacy `timesync` and `sync` commands have been removed
3. **New commands only**: Use `rtc-sync` and `rtc-status` for time management

### Recommended Upgrade Process
1. **Deploy firmware**: Flash updated firmware with RTC-only support
2. **Initial sync**: Run `python sen66_cli.py rtc-sync` to initialize RTC
3. **Verify operation**: Check `python sen66_cli.py rtc-status` to confirm proper operation
4. **Update scripts**: Replace any usage of legacy `sync` commands with `rtc-sync`

## Troubleshooting

### RTC Not Initialized
**Symptoms**: `rtc status` shows "Initialized: NO"
**Solution**: Run RTC synchronization command
```bash
python sen66_cli.py rtc-sync
```

### Time Drift
**Symptoms**: RTC time drifting from actual time
**Causes**: 
- Crystal accuracy variations
- Temperature effects
- Extended operation without sync

**Solution**: Regular synchronization (recommended every 24 hours)

### Fallback to Legacy Mode
**Symptoms**: Active Source shows "Legacy Sync" instead of "RTC Time"
**Causes**:
- RTC not properly initialized
- Invalid RTC time detected

**Solution**: Re-initialize RTC with valid timestamp

## Implementation Details

### Code Changes Summary

#### SEN66Dosimetry.h
- Added RTC includes (`time.h`, `sys/time.h`)
- Added `TimeSource` enumeration
- Added RTC method declarations
- Added RTC private variables

#### SEN66Dosimetry.cpp
- Implemented RTC initialization in `begin()`
- Added RTC management functions
- Updated timestamp generation to use `getCurrentTimestamp()`
- Enhanced `isTimeSynchronized()` to include RTC

#### main.cpp
- Added `rtc status` and `rtc sync` serial commands
- Updated help text with RTC options
- Enhanced command parsing for RTC operations

#### sen66_cli.py
- Added `rtc_status()` and `rtc_sync()` methods
- Enhanced command-line argument parsing
- Updated interactive console with RTC commands
- Improved help documentation

### Future Enhancements
1. **Automatic network time sync**: Integration with NTP servers
2. **Timezone awareness**: Enhanced UTC offset handling
3. **RTC calibration**: Dynamic crystal accuracy adjustment
4. **Extended battery backup**: Hardware support for longer offline operation

## Conclusion

The ESP32-S3 RTC integration significantly improves the reliability and accuracy of the SEN66-Dosimetry system. By providing persistent timekeeping across power cycles, it eliminates critical issues with timestamp corruption while maintaining full backward compatibility with existing deployments.

This enhancement is particularly valuable for industrial air quality monitoring applications where OSHA compliance and accurate TWA calculations are essential for regulatory reporting and workplace safety documentation.