/**
 * @file GenericTWA.ino
 * @brief Generic TWACore library example with simulated 2-parameter sensor
 * 
 * Demonstrates TWACore library usage independent of SEN66 sensor.
 * Simulates a dual-parameter environmental sensor (temperature and humidity)
 * with real-time FastTWA estimation and OSHA-compliant ExportTWA calculations.
 * 
 * @author Christopher Lee
 * @license GPL-3.0
 */

#include <Arduino.h>
#include <TWACore.h>

// Simulation parameters
#define SAMPLING_INTERVAL 60        // 60 seconds between samples
#define SIMULATION_HOURS 9          // Simulate 9 hours of data (exceeds 8-hour OSHA requirement)
#define SAMPLES_PER_HOUR 60         // 60 samples per hour (1-minute intervals)

// TWA instances for real-time estimation
FastTWA* tempTWA = nullptr;
FastTWA* humidityTWA = nullptr;

// Simulated sensor data storage for CSV generation
String csvData = "";
unsigned long startTimestamp = 1704067200;  // January 1, 2024, 00:00:00 UTC

/**
 * @brief Simulate sensor reading with realistic environmental variations
 */
void simulateSensorReading(unsigned long timestamp, float& temp, float& humidity) {
    // Base values with sinusoidal variation simulating daily temperature/humidity changes
    float hourOfDay = ((timestamp - startTimestamp) / 3600.0f);
    
    // Temperature: 20°C baseline, ±5°C daily variation
    temp = 20.0f + 5.0f * sin(hourOfDay * PI / 12.0f);
    temp += random(-10, 10) / 10.0f;  // Add ±1°C noise
    
    // Humidity: 50% baseline, ±15% daily variation (inverse of temperature)
    humidity = 50.0f - 15.0f * sin(hourOfDay * PI / 12.0f);
    humidity += random(-20, 20) / 10.0f;  // Add ±2% noise
    
    // Clamp to realistic ranges
    if (temp < 15.0f) temp = 15.0f;
    if (temp > 30.0f) temp = 30.0f;
    if (humidity < 30.0f) humidity = 30.0f;
    if (humidity > 80.0f) humidity = 80.0f;
}

/**
 * @brief Generate CSV data for TWA export calculations
 */
void generateSimulatedCSV() {
    Serial.println("\n📊 Generating simulated sensor data...");
    
    // CSV header
    csvData = "timestamp,temperature,humidity\n";
    
    unsigned long totalSamples = SIMULATION_HOURS * SAMPLES_PER_HOUR;
    unsigned long timestamp = startTimestamp;
    
    for (unsigned long i = 0; i < totalSamples; i++) {
        float temp, humidity;
        simulateSensorReading(timestamp, temp, humidity);
        
        // Add to CSV
        csvData += String(timestamp) + ",";
        csvData += String(temp, 2) + ",";
        csvData += String(humidity, 2) + "\n";
        
        // Update real-time TWA
        tempTWA->addSample(temp);
        humidityTWA->addSample(humidity);
        
        timestamp += SAMPLING_INTERVAL;
        
        // Progress indicator
        if ((i + 1) % (SAMPLES_PER_HOUR * 2) == 0) {  // Every 2 hours
            float hoursComplete = (float)(i + 1) / SAMPLES_PER_HOUR;
            Serial.printf("  %.1f hours simulated... ", hoursComplete);
            Serial.printf("(Temp TWA: %.2f°C, Humidity TWA: %.2f%%)\n", 
                         tempTWA->getCurrentTWA(), humidityTWA->getCurrentTWA());
        }
    }
    
    Serial.printf("✓ Generated %lu samples over %d hours\n", totalSamples, SIMULATION_HOURS);
}

/**
 * @brief Display real-time TWA estimates
 */
void displayFastTWA() {
    Serial.println("\n═══════════════════════════════════════════════════");
    Serial.println("  REAL-TIME 8-HOUR TWA ESTIMATES (FastTWA)");
    Serial.println("═══════════════════════════════════════════════════");
    
    Serial.printf("Temperature 8-hr TWA:  %.2f°C\n", tempTWA->getCurrentTWA());
    Serial.printf("  Valid 8-hr window:   %s\n", tempTWA->hasValidTWA() ? "YES" : "NO (< 8 hours)");
    
    Serial.printf("\nHumidity 8-hr TWA:     %.2f%%\n", humidityTWA->getCurrentTWA());
    Serial.printf("  Valid 8-hr window:   %s\n", humidityTWA->hasValidTWA() ? "YES" : "NO (< 8 hours)");
    
    Serial.println("═══════════════════════════════════════════════════\n");
}

/**
 * @brief Calculate and display OSHA-compliant export TWA
 */
void calculateExportTWA() {
    Serial.println("\n═══════════════════════════════════════════════════");
    Serial.println("  OSHA-COMPLIANT EXPORT TWA (ExportTWA)");
    Serial.println("═══════════════════════════════════════════════════");
    
    // Create ExportTWA calculator
    std::vector<String> parameters = {"temperature", "humidity"};
    ExportTWA twaCalculator(SAMPLING_INTERVAL, parameters, 0);  // UTC+0 for simplicity
    
    // Calculate TWA from CSV data
    TWAExportResult result = twaCalculator.calculateFromCSV(csvData);
    
    // Display results
    Serial.printf("Period Start:          %s\n", result.exportStartTime.c_str());
    Serial.printf("Period End:            %s\n", result.exportEndTime.c_str());
    Serial.printf("Data Coverage:         %.1f hours\n", result.dataCoverageHours);
    Serial.printf("OSHA Compliant:        %s\n", result.oshaCompliant ? "YES (≥8 hours)" : "NO (< 8 hours)");
    Serial.printf("Samples Analyzed:      %lu\n", result.samplesAnalyzed);
    Serial.printf("Data Gaps Detected:    %lu\n", result.dataGaps);
    
    Serial.println("\nCalculated TWA Values:");
    for (const auto& pair : result.parameterTWAs) {
        Serial.printf("  %s:  %.2f\n", pair.first.c_str(), pair.second);
    }
    
    Serial.println("═══════════════════════════════════════════════════\n");
}

/**
 * @brief Compare FastTWA vs ExportTWA results
 */
void compareResults() {
    Serial.println("\n═══════════════════════════════════════════════════");
    Serial.println("  FASTTWA vs EXPORTTWA COMPARISON");
    Serial.println("═══════════════════════════════════════════════════");
    
    // Recalculate ExportTWA for comparison
    std::vector<String> parameters = {"temperature", "humidity"};
    ExportTWA twaCalculator(SAMPLING_INTERVAL, parameters, 0);
    TWAExportResult result = twaCalculator.calculateFromCSV(csvData);
    
    float fastTempTWA = tempTWA->getCurrentTWA();
    float fastHumidityTWA = humidityTWA->getCurrentTWA();
    float exportTempTWA = result.parameterTWAs["temperature"];
    float exportHumidityTWA = result.parameterTWAs["humidity"];
    
    Serial.println("Temperature:");
    Serial.printf("  FastTWA:    %.2f°C\n", fastTempTWA);
    Serial.printf("  ExportTWA:  %.2f°C\n", exportTempTWA);
    Serial.printf("  Difference: %.2f°C (%.2f%%)\n", 
                 abs(fastTempTWA - exportTempTWA),
                 abs(fastTempTWA - exportTempTWA) / exportTempTWA * 100.0f);
    
    Serial.println("\nHumidity:");
    Serial.printf("  FastTWA:    %.2f%%\n", fastHumidityTWA);
    Serial.printf("  ExportTWA:  %.2f%%\n", exportHumidityTWA);
    Serial.printf("  Difference: %.2f%% (%.2f%%)\n", 
                 abs(fastHumidityTWA - exportHumidityTWA),
                 abs(fastHumidityTWA - exportHumidityTWA) / exportHumidityTWA * 100.0f);
    
    Serial.println("\nNote: FastTWA provides real-time estimates using circular buffer.");
    Serial.println("      ExportTWA performs time-weighted calculations on complete dataset.");
    Serial.println("      Small differences are expected due to algorithmic differences.");
    Serial.println("═══════════════════════════════════════════════════\n");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n╔═══════════════════════════════════════════════════╗");
    Serial.println("║     TWACore Generic Example - 2-Parameter Demo   ║");
    Serial.println("║                                                   ║");
    Serial.println("║  Demonstrates OSHA-compliant TWA calculations     ║");
    Serial.println("║  for generic environmental sensors               ║");
    Serial.println("╚═══════════════════════════════════════════════════╝\n");
    
    // Initialize FastTWA instances
    Serial.println("📋 Initializing TWA calculators...");
    tempTWA = new FastTWA(SAMPLING_INTERVAL);
    humidityTWA = new FastTWA(SAMPLING_INTERVAL);
    Serial.printf("   Sampling interval: %d seconds\n", SAMPLING_INTERVAL);
    Serial.printf("   8-hour window: %d samples\n", TWA_WINDOW_SECONDS / SAMPLING_INTERVAL);
    
    // Generate simulated data
    generateSimulatedCSV();
    
    // Display FastTWA results (real-time estimates)
    displayFastTWA();
    
    // Calculate ExportTWA results (regulatory compliance)
    calculateExportTWA();
    
    // Compare both methods
    compareResults();
    
    Serial.println("✓ TWACore example complete!");
    Serial.println("\nThis example demonstrates:");
    Serial.println("  • FastTWA for real-time 8-hour TWA estimation");
    Serial.println("  • ExportTWA for OSHA-compliant regulatory calculations");
    Serial.println("  • Generic parameter support (works with any sensor type)");
    Serial.println("  • Dynamic CSV column mapping");
    Serial.println("  • Gap detection and OSHA compliance checking\n");
}

void loop() {
    // Example complete - nothing to do in loop
    delay(10000);
}
