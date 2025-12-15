/**
 * @file TWACore.h
 * @brief OSHA-compliant Time-Weighted Average calculation library
 * 
 * Provides fast real-time TWA estimation and comprehensive post-processing
 * regulatory compliance calculations for environmental monitoring applications.
 * 
 * @author Christopher Lee
 * @license GPL-3.0
 * @version 1.0.0
 */

#ifndef TWA_CORE_H
#define TWA_CORE_H

#include <Arduino.h>
#include <vector>
#include <map>

// Constants
#define TWA_WINDOW_SECONDS (8 * 3600)  // 8 hours (OSHA standard)
#define MIN_OSHA_HOURS 8.0             // Minimum hours for OSHA compliance

/**
 * @brief Generic measurement sample for TWA calculations
 * 
 * Supports single or multi-parameter measurements with timestamp.
 * Values array indexed by parameter position (e.g., [0]=PM1.0, [1]=PM2.5, etc.)
 */
struct MeasurementSample {
    unsigned long timestamp;      // Unix timestamp (seconds since epoch)
    std::vector<float> values;    // Measurement values for all parameters
};

/**
 * @brief TWA calculation result for regulatory compliance reporting
 * 
 * Contains calculated TWA values for all parameters plus metadata required
 * for OSHA 29 CFR 1910.1000 compliance and audit trail documentation.
 */
struct TWAExportResult {
    std::map<String, float> parameterTWAs;  // TWA values keyed by parameter name
    float dataCoverageHours;                // Hours of data analyzed
    bool oshaCompliant;                     // true if >= MIN_OSHA_HOURS
    bool exceedsMaxDuration;                // true if > 10 hours (multi-shift)
    unsigned long samplesAnalyzed;          // Total samples processed
    unsigned long dataGaps;                 // Number of detected data gaps
    String exportStartTime;                 // Local time string (formatted)
    String exportEndTime;                   // Local time string (formatted)
};

/**
 * @brief Fast TWA calculator for real-time exposure estimates
 * 
 * Implements efficient circular buffer for constant-time TWA calculation.
 * Uses O(1) updates by maintaining running sum. Buffer size automatically
 * calculated from sampling interval to maintain 8-hour window.
 * 
 * @note Thread-safe for single producer/consumer. Not thread-safe for
 *       concurrent writes or sampling interval changes.
 */
class FastTWA {
public:
    /**
     * @brief Construct FastTWA calculator
     * @param samplingInterval Seconds between samples (e.g., 60 for 1-minute intervals)
     */
    FastTWA(uint16_t samplingInterval);
    
    /**
     * @brief Destructor - automatically cleans up buffer
     */
    ~FastTWA();
    
    /**
     * @brief Update sampling interval and resize buffer
     * @param newInterval New sampling interval in seconds
     * 
     * Preserves most recent data when resizing. Recalculates buffer size
     * to maintain 8-hour window with new interval.
     */
    void updateSamplingInterval(uint16_t newInterval);
    
    /**
     * @brief Add measurement sample to TWA calculation
     * @param value Measurement value (any unit - µg/m³, ppm, dBA, etc.)
     * 
     * O(1) operation using circular buffer. Updates running sum automatically.
     */
    void addSample(float value);
    
    /**
     * @brief Get current 8-hour TWA estimate
     * @return Time-weighted average of all samples in buffer
     * 
     * Returns 0.0 if buffer is empty. Use hasValidTWA() to check if
     * result represents full 8-hour period.
     */
    float getCurrentTWA() const;
    
    /**
     * @brief Check if TWA represents full 8-hour period
     * @return true if buffer contains >= 8 hours of data
     * 
     * Returns false during initial fill period (< 8 hours since start).
     * OSHA compliance requires full 8-hour coverage.
     */
    bool hasValidTWA() const;
    
private:
    std::vector<float> _buffer;      // Circular buffer for sample values
    size_t _bufferSize;              // Calculated from TWA_WINDOW_SECONDS / _samplingInterval
    size_t _bufferIndex;             // Current write position (wraps at _bufferSize)
    bool _bufferFull;                // True when buffer has 8 hours of data
    float _sum;                      // Running sum for O(1) average calculation
    uint16_t _samplingInterval;      // Seconds between samples
    
    /**
     * @brief Calculate buffer size based on sampling interval
     * 
     * Ensures minimum buffer size of 10 samples for meaningful TWA.
     */
    void calculateBufferSize();
};

/**
 * @brief Export TWA calculator for OSHA-compliant regulatory reporting
 * 
 * Processes complete CSV datasets with time-weighted calculations accounting
 * for variable sampling intervals and data gaps. Supports dynamic column
 * mapping to handle metadata changes and arbitrary CSV structures.
 * 
 * Uses Template Method pattern - override formatOutput() for custom export
 * formats (JSON, XML, database, etc.)
 */
class ExportTWA {
public:
    /**
     * @brief Construct ExportTWA calculator
     * @param samplingInterval Expected seconds between samples (used for gap detection)
     * @param parameterNames Ordered list of parameter names to calculate TWA for
     * @param utcOffset Timezone offset in hours (-12 to +14) for timestamp formatting
     * 
     * Gap threshold automatically set to samplingInterval * 2.
     * Parameter names must match CSV column headers exactly.
     */
    ExportTWA(uint16_t samplingInterval, 
              const std::vector<String>& parameterNames, 
              int16_t utcOffset);
    
    /**
     * @brief Calculate OSHA-compliant TWA from CSV data
     * @param csvData Complete CSV file as String (including header)
     * @param exportStart Start timestamp (0 = use earliest data)
     * @param exportEnd End timestamp (0 = use latest data)
     * @return TWAExportResult with calculated values and compliance metadata
     * 
     * Parses CSV header for dynamic column mapping, extracts measurement
     * samples, calculates time-weighted averages accounting for gaps,
     * and determines OSHA compliance (>= 8 hours coverage).
     */
    TWAExportResult calculateFromCSV(const String& csvData,
                                    unsigned long exportStart = 0,
                                    unsigned long exportEnd = 0);
    
protected:
    /**
     * @brief Format output for export (Template Method hook)
     * @param result TWA calculation results
     * @return Formatted string for export (CSV, JSON, etc.)
     * 
     * Override in subclass for custom export formats.
     * Base implementation returns CSV format.
     */
    virtual String formatOutput(const TWAExportResult& result);
    
private:
    uint16_t _samplingInterval;              // Expected seconds between samples
    std::vector<String> _parameterNames;     // Parameter names in order
    int16_t _utcOffset;                      // Timezone offset (hours)
    unsigned long _gapThreshold;             // Seconds threshold for gap detection
    
    /**
     * @brief Parse CSV data into measurement samples with dynamic column mapping
     * @param csvData Complete CSV file as String
     * @param samples Output vector of parsed measurement samples
     * @return true if parsing succeeded and samples extracted
     * 
     * Scans header row to build column index map, then extracts parameter
     * values by name lookup. Supports arbitrary metadata columns between
     * timestamp and measurement fields.
     */
    bool parseDataPoints(const String& csvData, std::vector<MeasurementSample>& samples);
    
    /**
     * @brief Calculate time-weighted average for single parameter
     * @param samples Parsed measurement samples
     * @param paramIndex Index of parameter in values array
     * @param periodStart Start timestamp for analysis window
     * @param periodEnd End timestamp for analysis window
     * @param gaps Output: number of detected data gaps
     * @return Time-weighted average value
     * 
     * Weights each sample by duration until next sample. Detects gaps
     * when duration exceeds _gapThreshold (samplingInterval * 2).
     */
    float calculateWeightedAverage(const std::vector<MeasurementSample>& samples,
                                   size_t paramIndex,
                                   unsigned long periodStart,
                                   unsigned long periodEnd,
                                   unsigned long& gaps);
    
    /**
     * @brief Format Unix timestamp as local time string
     * @param timestamp Unix timestamp (seconds since epoch)
     * @return Formatted string "YYYY-MM-DD_HH:MM:SS" in local timezone
     * 
     * Applies UTC offset before formatting.
     */
    String formatLocalTime(unsigned long timestamp);
};

#endif // TWA_CORE_H
