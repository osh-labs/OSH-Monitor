/**
 * @file TWACore.cpp
 * @brief OSHA-compliant Time-Weighted Average calculation library implementation
 * 
 * @author Christopher Lee
 * @license GPL-3.0
 * @version 1.0.0
 */

#include "TWACore.h"

// ===============================================================================
// FastTWA Implementation - Dynamic circular buffer for real-time TWA estimates
// ===============================================================================

FastTWA::FastTWA(uint16_t samplingInterval) 
    : _samplingInterval(samplingInterval), _bufferIndex(0), _bufferFull(false), _sum(0.0f) {
    calculateBufferSize();
    _buffer.reserve(_bufferSize);
}

FastTWA::~FastTWA() {
    // std::vector automatically cleans up
}

void FastTWA::calculateBufferSize() {
    _bufferSize = TWA_WINDOW_SECONDS / _samplingInterval;
    // Ensure minimum buffer size for meaningful TWA
    if (_bufferSize < 10) _bufferSize = 10;
}

void FastTWA::updateSamplingInterval(uint16_t newInterval) {
    _samplingInterval = newInterval;
    size_t oldSize = _bufferSize;
    calculateBufferSize();
    
    if (_bufferSize != oldSize) {
        // Resize buffer, keeping most recent data
        std::vector<float> newBuffer;
        newBuffer.reserve(_bufferSize);
        
        if (_bufferFull && !_buffer.empty()) {
            // Copy from current index (oldest) to end, then from start to current index
            size_t keepCount = std::min(_bufferSize, _buffer.size());
            for (size_t i = 0; i < keepCount; i++) {
                size_t srcIndex = (_bufferIndex + _buffer.size() - keepCount + i) % _buffer.size();
                newBuffer.push_back(_buffer[srcIndex]);
            }
        } else if (!_buffer.empty()) {
            // Not full yet, just copy what we have
            size_t keepCount = std::min(_bufferSize, _buffer.size());
            for (size_t i = 0; i < keepCount; i++) {
                newBuffer.push_back(_buffer[i]);
            }
        }
        
        _buffer = std::move(newBuffer);
        _bufferIndex = 0;
        _bufferFull = (_buffer.size() >= _bufferSize);
        
        // Recalculate sum
        _sum = 0.0f;
        for (float value : _buffer) {
            _sum += value;
        }
    }
}

void FastTWA::addSample(float value) {
    if (_bufferFull) {
        // Remove old value from sum, add new value
        _sum = _sum - _buffer[_bufferIndex] + value;
        _buffer[_bufferIndex] = value;
        _bufferIndex = (_bufferIndex + 1) % _bufferSize;
    } else {
        // Still filling buffer
        _buffer.push_back(value);
        _sum += value;
        if (_buffer.size() >= _bufferSize) {
            _bufferFull = true;
        }
    }
}

float FastTWA::getCurrentTWA() const {
    if (_buffer.empty()) return 0.0f;
    return _sum / _buffer.size();
}

bool FastTWA::hasValidTWA() const {
    return _bufferFull; // Only consider valid when buffer is full (8 hours of data)
}

// ===============================================================================
// ExportTWA Implementation - OSHA-compliant regulatory TWA calculations
// ===============================================================================

ExportTWA::ExportTWA(uint16_t samplingInterval,
                     const std::vector<String>& parameterNames,
                     int16_t utcOffset)
    : _samplingInterval(samplingInterval),
      _parameterNames(parameterNames),
      _utcOffset(utcOffset) {
    // Gap threshold = 2x sampling interval for flexibility
    _gapThreshold = samplingInterval * 2;
}

TWAExportResult ExportTWA::calculateFromCSV(const String& csvData,
                                           unsigned long exportStart,
                                           unsigned long exportEnd) {
    TWAExportResult result = {};
    
    std::vector<MeasurementSample> samples;
    if (!parseDataPoints(csvData, samples)) {
        return result; // Return empty result on parse failure
    }
    
    if (samples.empty()) {
        return result;
    }
    
    // Determine period bounds
    unsigned long minTimestamp = samples[0].timestamp;
    unsigned long maxTimestamp = samples[0].timestamp;
    
    for (const auto& sample : samples) {
        if (sample.timestamp < minTimestamp) minTimestamp = sample.timestamp;
        if (sample.timestamp > maxTimestamp) maxTimestamp = sample.timestamp;
    }
    
    unsigned long periodStart = (exportStart == 0) ? minTimestamp : exportStart;
    unsigned long periodEnd = (exportEnd == 0) ? maxTimestamp : exportEnd;
    
    // Calculate TWA for each parameter
    for (size_t i = 0; i < _parameterNames.size(); i++) {
        unsigned long gaps = 0;
        float twaValue = calculateWeightedAverage(samples, i, periodStart, periodEnd, gaps);
        result.parameterTWAs[_parameterNames[i]] = twaValue;
        result.dataGaps += gaps;
    }
    
    // Average gaps across all parameters
    if (_parameterNames.size() > 0) {
        result.dataGaps /= _parameterNames.size();
    }
    
    // Calculate coverage and compliance
    result.dataCoverageHours = (periodEnd - periodStart) / 3600.0f;
    result.oshaCompliant = (result.dataCoverageHours >= MIN_OSHA_HOURS && result.dataCoverageHours <= 10.0f);
    result.exceedsMaxDuration = (result.dataCoverageHours > 10.0f);
    result.samplesAnalyzed = samples.size();
    
    result.exportStartTime = formatLocalTime(periodStart);
    result.exportEndTime = formatLocalTime(periodEnd);
    
    return result;
}

bool ExportTWA::parseDataPoints(const String& csvData, std::vector<MeasurementSample>& samples) {
    samples.clear();
    
    int lineStart = 0;
    int lineEnd = csvData.indexOf('\n');
    
    std::map<String, int> columnIndex;  // Maps parameter name to column index
    int timestampColumn = -1;
    bool headerParsed = false;
    
    while (lineEnd != -1) {
        String line = csvData.substring(lineStart, lineEnd);
        line.trim();
        
        // Skip empty lines and comments
        if (line.length() == 0 || line.startsWith("#")) {
            lineStart = lineEnd + 1;
            lineEnd = csvData.indexOf('\n', lineStart);
            continue;
        }
        
        // Parse header row on first non-comment line
        if (!headerParsed) {
            int colIdx = 0;
            int fieldStart = 0;
            
            for (int i = 0; i <= line.length(); i++) {
                if (i == line.length() || line.charAt(i) == ',') {
                    String fieldName = line.substring(fieldStart, i);
                    fieldName.trim();
                    
                    // Identify timestamp column
                    if (fieldName == "timestamp") {
                        timestampColumn = colIdx;
                    }
                    
                    // Map parameter columns
                    for (const String& paramName : _parameterNames) {
                        if (fieldName == paramName) {
                            columnIndex[paramName] = colIdx;
                            break;
                        }
                    }
                    
                    colIdx++;
                    fieldStart = i + 1;
                }
            }
            
            headerParsed = true;
            
            // Validate that we found all required columns
            if (timestampColumn == -1) {
                return false;  // Missing timestamp column
            }
            
            for (const String& paramName : _parameterNames) {
                if (columnIndex.find(paramName) == columnIndex.end()) {
                    return false;  // Missing parameter column
                }
            }
            
            lineStart = lineEnd + 1;
            lineEnd = csvData.indexOf('\n', lineStart);
            continue;
        }
        
        // Parse data row
        MeasurementSample sample;
        sample.values.resize(_parameterNames.size(), 0.0f);
        
        int colIdx = 0;
        int fieldStart = 0;
        bool validRow = true;
        
        for (int i = 0; i <= line.length(); i++) {
            if (i == line.length() || line.charAt(i) == ',') {
                String field = line.substring(fieldStart, i);
                field.trim();
                
                // Extract timestamp
                if (colIdx == timestampColumn) {
                    sample.timestamp = field.toInt();
                }
                
                // Extract parameter values
                for (size_t p = 0; p < _parameterNames.size(); p++) {
                    if (columnIndex[_parameterNames[p]] == colIdx) {
                        sample.values[p] = field.toFloat();
                        break;
                    }
                }
                
                colIdx++;
                fieldStart = i + 1;
            }
        }
        
        // Add sample if it has valid data
        if (validRow && sample.timestamp > 0) {
            samples.push_back(sample);
        }
        
        lineStart = lineEnd + 1;
        lineEnd = csvData.indexOf('\n', lineStart);
    }
    
    return !samples.empty();
}

float ExportTWA::calculateWeightedAverage(const std::vector<MeasurementSample>& samples,
                                         size_t paramIndex,
                                         unsigned long periodStart,
                                         unsigned long periodEnd,
                                         unsigned long& gaps) {
    if (samples.empty() || paramIndex >= _parameterNames.size()) return 0.0f;
    
    float weightedSum = 0.0f;
    unsigned long totalTime = 0;
    gaps = 0;
    
    for (size_t i = 0; i < samples.size() - 1; i++) {
        const MeasurementSample& current = samples[i];
        const MeasurementSample& next = samples[i + 1];
        
        // Skip if outside period
        if (current.timestamp < periodStart || current.timestamp >= periodEnd) continue;
        
        unsigned long duration = next.timestamp - current.timestamp;
        
        // Check for data gaps using dynamic threshold
        if (duration > _gapThreshold) {
            gaps++;
        }
        
        // Ensure paramIndex is within bounds for this sample
        if (paramIndex < current.values.size()) {
            float value = current.values[paramIndex];
            weightedSum += value * duration;
            totalTime += duration;
        }
    }
    
    return (totalTime > 0) ? weightedSum / totalTime : 0.0f;
}

String ExportTWA::formatLocalTime(unsigned long timestamp) {
    timestamp += (_utcOffset * 3600); // Apply UTC offset
    
    time_t t = timestamp;
    struct tm *timeInfo = gmtime(&t);
    
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H:%M:%S", timeInfo);
    return String(buffer);
}

String ExportTWA::formatOutput(const TWAExportResult& result) {
    // Base implementation returns CSV format
    // Override in subclass for JSON, XML, etc.
    String output = "# OSHA-Compliant 8-Hour Time-Weighted Average Report\n";
    output += "# Period Start: " + result.exportStartTime + "\n";
    output += "# Period End: " + result.exportEndTime + "\n";
    output += "# Data Coverage: " + String(result.dataCoverageHours, 1) + " hours\n";
    output += "# OSHA Compliant: " + String(result.oshaCompliant ? "YES" : "NO") + "\n";
    output += "# Samples Analyzed: " + String(result.samplesAnalyzed) + "\n";
    output += "# Data Gaps Detected: " + String(result.dataGaps) + "\n";
    output += "#\n";
    
    // Parameter TWA values
    for (const auto& pair : result.parameterTWAs) {
        output += "# " + pair.first + " 8-hr TWA: " + String(pair.second, 3) + "\n";
    }
    
    return output;
}
