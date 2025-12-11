#!/usr/bin/env python3
"""
===============================================================================
SEN66-Dosimetry CLI Tool
A command-line interface for interacting with the SEN66 air quality monitor

Project: SEN66-Dosimetry
Creator: Christopher Lee
License: GNU General Public License v3.0 (GPLv3)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.
===============================================================================

Usage:
    python sen66_cli.py                               # Interactive console mode
    python sen66_cli.py console [--port COM5]         # Interactive console mode
    python sen66_cli.py status [--port COM5]
    python sen66_cli.py clear [--port COM5]
    python sen66_cli.py download [--port COM5] [--output data.csv]
    python sen66_cli.py sync [--port COM5]
    python sen66_cli.py timezone --offset -5 [--port COM5]
    python sen66_cli.py monitor [--port COM5]
    python sen66_cli.py list-ports
"""

import argparse
import sys
import time
from pathlib import Path

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    print("Error: pyserial not installed.")
    print("Install it with: pip install pyserial")
    sys.exit(1)


class SEN66CLI:
    """CLI interface for SEN66-Dosimetry board"""
    
    def __init__(self, port=None, baudrate=115200, timeout=2):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
        
    def connect(self):
        """Connect to the serial port"""
        if not self.port:
            self.port = self.auto_detect_port()
            if not self.port:
                print("Error: No SEN66 board detected. Please specify port with --port")
                return False
                
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=self.timeout)
            time.sleep(0.5)  # Allow connection to stabilize
            # Flush any existing data
            self.ser.reset_input_buffer()
            print(f"Connected to {self.port}")
            return True
        except serial.SerialException as e:
            print(f"Error connecting to {self.port}: {e}")
            return False
            
    def disconnect(self):
        """Disconnect from serial port"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("Disconnected")
            
    def auto_detect_port(self):
        """Attempt to auto-detect the SEN66 board"""
        ports = serial.tools.list_ports.comports()
        
        # Look for ESP32-S3 USB device
        for port in ports:
            if "USB" in port.description or "ESP32" in port.description:
                print(f"Auto-detected: {port.device} ({port.description})")
                return port.device
        
        # If nothing found, return first available port
        if ports:
            print(f"Using first available port: {ports[0].device}")
            return ports[0].device
            
        return None
        
    def flush_input_buffer(self):
        """Flush any pending serial input to clear old measurements"""
        if self.ser and self.ser.is_open:
            self.ser.reset_input_buffer()
            time.sleep(0.1)
    
    def send_command(self, command, flush_first=True):
        """Send a command to the board"""
        if not self.ser or not self.ser.is_open:
            print("Error: Not connected")
            return False
            
        try:
            # Clear buffer of any pending measurements before sending command
            if flush_first:
                self.flush_input_buffer()
            
            self.ser.write((command + '\n').encode())
            time.sleep(0.2)  # Give board time to process
            return True
        except Exception as e:
            print(f"Error sending command: {e}")
            return False
            
    def read_until_prompt(self, timeout=5):
        """Read serial output until we see a measurement or timeout"""
        start_time = time.time()
        output = []
        
        while time.time() - start_time < timeout:
            if self.ser.in_waiting:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    output.append(line)
                    
                # Stop if we see a complete measurement
                if "TWA PM10:" in line:
                    break
            else:
                time.sleep(0.1)
                
        return output
        
    def get_status(self):
        """Get the most recent measurement"""
        print("\nFetching current status...\n")
        
        # Read any pending output
        lines = self.read_until_prompt(timeout=3)
        
        if lines:
            # Print the last measurement found
            in_measurement = False
            measurement_lines = []
            
            for line in lines:
                if "Measurement #" in line or "---" in line:
                    in_measurement = True
                    measurement_lines = [line]
                elif in_measurement:
                    measurement_lines.append(line)
                    if "TWA PM10:" in line:
                        break
                        
            if measurement_lines:
                for line in measurement_lines:
                    print(line)
            else:
                print("Waiting for next measurement...")
                lines = self.read_until_prompt(timeout=25)
                for line in lines:
                    print(line)
        else:
            print("No data available. Waiting for measurement...")
            lines = self.read_until_prompt(timeout=25)
            for line in lines:
                print(line)
                
    def clear_log(self):
        """Clear the CSV log file"""
        # Flush any pending data
        self.flush_input_buffer()
        
        if self.send_command("clear"):
            # Wait for the board's confirmation prompt
            time.sleep(0.3)
            
            # Read and display the board's warning message
            prompt_timeout = time.time() + 3
            while time.time() < prompt_timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                        if "Type 'yes' to confirm" in line:
                            break
                else:
                    time.sleep(0.1)
            
            # Get user confirmation
            confirm = input().strip().lower()
            
            # Send confirmation to board
            self.ser.write(f"{confirm}\n".encode())
            self.ser.flush()
            
            # Read response
            time.sleep(0.5)
            response_timeout = time.time() + 3
            while time.time() < response_timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                        if "cleared" in line.lower() or "cancelled" in line.lower():
                            break
                else:
                    time.sleep(0.1)
                    if not self.ser.in_waiting:
                        break
        else:
            print("Failed to send clear command")
    

    
    def rtc_status(self):
        """Show ESP32 RTC status and timing information"""
        print("\nRetrieving RTC status...")
        
        if self.send_command("rtc status"):
            time.sleep(0.5)
            
            # Read the RTC status response, filtering out sensor measurements
            timeout = time.time() + 5
            collecting_response = False
            rtc_response_complete = False
            
            while time.time() < timeout and not rtc_response_complete:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Filter out sensor measurement lines
                        if any(keyword in line for keyword in ["PM1.0", "PM2.5", "PM4.0", "PM10", "Temperature", "Humidity", "VOC", "NOx", "CO2", "Timestamp", "Fast TWA", "Export TWA"]):
                            continue  # Skip sensor measurement data
                        
                        # Only print RTC-related lines
                        if ("RTC Status" in line or "═══" in line or 
                            "Initialized:" in line or "Current Time:" in line or 
                            "Last Sync:" in line or "Time Since Sync:" in line or 
                            "Needs Sync:" in line or "Active Source:" in line or
                            "RTC Time" in line or "Legacy Sync" in line or "Millis Only" in line):
                            print(line)
                            collecting_response = True
                            
                            # Check if this is the end of RTC status
                            if collecting_response and "Active Source:" in line:
                                rtc_response_complete = True
                else:
                    time.sleep(0.1)
        else:
            print("Failed to send RTC status command")
    
    def rtc_sync(self):
        """Synchronize the ESP32 RTC with current Unix timestamp"""
        import time as time_module
        
        unix_time = int(time_module.time())
        print(f"\nSynchronizing ESP32 RTC to Unix timestamp: {unix_time}")
        
        if self.send_command(f"rtc sync {unix_time}"):
            time.sleep(0.5)
            
            # Read the sync response, filtering out sensor measurements
            timeout = time.time() + 3
            while time.time() < timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Filter out sensor measurement lines
                        if any(keyword in line for keyword in ["PM1.0", "PM2.5", "PM4.0", "PM10", "Temperature", "Humidity", "VOC", "NOx", "CO2", "Timestamp", "Fast TWA", "Export TWA"]):
                            continue  # Skip sensor measurement data
                        
                        # Only print RTC sync related lines
                        if ("[RTC]" in line or "synchronized" in line.lower() or 
                            "error" in line.lower() or "power cycles" in line.lower() or
                            "Setting RTC time" in line or "RTC time set" in line or
                            "Failed to set RTC" in line):
                            print(line)
                            if ("synchronized" in line.lower() or "error" in line.lower() or 
                                "power cycles" in line.lower()):
                                break
                else:
                    time.sleep(0.1)
        else:
            print("Failed to send RTC sync command")
    
    def show_config(self):
        """Show current board configuration"""
        print("\nRetrieving configuration...")
        
        if self.send_command("config"):
            time.sleep(0.5)
            
            # Read response, filtering out measurement data
            timeout = time.time() + 3
            in_config = False
            
            while time.time() < timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Start capturing when we see the config header
                        if "Current Configuration" in line or "═" in line:
                            in_config = True
                            print(line)
                        # Stop if we see measurement indicators
                        elif "Measurement #" in line or "ENVIRONMENTAL" in line:
                            break
                        # Print config lines
                        elif in_config:
                            print(line)
                            # Stop after the final separator or tip
                            if "Tip:" in line or ("═" in line and in_config and "Configuration" not in line):
                                # Read one more line after tip
                                time.sleep(0.1)
                                if self.ser.in_waiting:
                                    extra = self.ser.readline().decode('utf-8', errors='ignore').strip()
                                    if extra:
                                        print(extra)
                                break
                else:
                    time.sleep(0.1)
        else:
            print("Failed to send config command")
    
    def set_config(self, key, value):
        """Set a configuration value"""
        print(f"\nSetting {key} to {value}...")
        
        if self.send_command(f"prefs {key} {value}"):
            time.sleep(0.5)
            
            # Read response, filtering out measurement data
            timeout = time.time() + 3
            
            while time.time() < timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Print response lines but stop if we see measurements
                        if "Measurement #" in line or "ENVIRONMENTAL" in line:
                            break
                        print(line)
                        # Stop after confirmation messages
                        if "interval set to" in line.lower() or "saved to NVS" in line:
                            time.sleep(0.2)  # Brief pause
                            break
                else:
                    time.sleep(0.1)
        else:
            print("Failed to send prefs command")
    
    def set_timezone(self, offset_str):
        """Set UTC offset (timezone) with validation"""
        try:
            offset = int(offset_str)
            if offset < -12 or offset > 14:
                print(f"❌ Error: UTC offset must be between -12 and +14 hours (got {offset})")
                print("   Common timezones:")
                print("   -12: Baker Island Time")
                print("    -8: PST (Pacific Standard Time)")
                print("    -5: EST (Eastern Standard Time)")
                print("     0: UTC/GMT")
                print("    +1: CET (Central European Time)")
                print("    +8: CST (China Standard Time)")
                print("    +9: JST (Japan Standard Time)")
                print("   +14: LINT (Line Islands Time)")
                return False
            
            print(f"\n🌍 Setting UTC offset to {offset:+d} hours...")
            
            if self.send_command(f"prefs utc {offset}"):
                time.sleep(0.5)
                
                # Read response, filtering out measurement data
                timeout = time.time() + 3
                
                while time.time() < timeout:
                    if self.ser.in_waiting:
                        line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                        if line:
                            # Print response lines but stop if we see measurements
                            if "Measurement #" in line or "ENVIRONMENTAL" in line:
                                break
                            print(line)
                            # Stop after confirmation messages
                            if "utc offset set to" in line.lower() or "saved to NVS" in line:
                                print(f"✓ Timezone set to UTC{offset:+d}")
                                time.sleep(0.2)  # Brief pause
                                break
                    else:
                        time.sleep(0.1)
                return True
            else:
                print("Failed to send UTC offset command")
                return False
                
        except ValueError:
            print(f"❌ Error: Invalid offset '{offset_str}'. Must be an integer between -12 and +14")
            print("   Examples: timezone -5  (EST), timezone +9  (JST)")
            return False
            
    def export_twa_data(self, output_file=None):
        """Export CSV with OSHA-compliant TWA calculations"""
        if output_file is None:
            import datetime
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            output_file = f"twa_export_{timestamp}.csv"
            
        print("\n📊 Generating OSHA-compliant 8-hour TWA export...")
        
        if self.send_command("export_twa"):
            time.sleep(2.0)  # Give more time for TWA calculations
            
            # Read TWA calculation results
            response_lines = []
            timeout = time.time() + 10
            
            while time.time() < timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)  # Show TWA calculation progress
                        response_lines.append(line)
                        if "Export file:" in line or "TWA export failed" in line:
                            break
                else:
                    time.sleep(0.1)
            
            # If export succeeded, download the TWA file
            if any("Export file:" in line for line in response_lines):
                print(f"\n📥 Downloading TWA export to {output_file}...")
                return self._download_file("/twa_export.csv", output_file)
            else:
                print("❌ TWA export failed")
                return False
        else:
            print("❌ Failed to send export_twa command")
            return False
    
    def download_log(self, output_file=None):
        """Download the CSV log file"""
        # Add timestamp to filename if using default
        if output_file is None or output_file == 'sensor_log.csv':
            import datetime
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
            output_file = f"sensor_log_{timestamp}.csv"
        
        print(f"\nDownloading log file to {output_file}...")
        
        # First, automatically generate TWA export for regulatory compliance
        twa_filename = output_file.replace('.csv', '_with_twa.csv')
        print("📊 Also generating OSHA TWA export...")
        self.export_twa_data(twa_filename)
        
        return self._download_file("/sensor_log.csv", output_file)
    
    def _download_file(self, source_path, output_file):
        """Internal method to download a file from the device"""
        # Choose the appropriate dump command based on source file
        if source_path == "/twa_export.csv":
            dump_command = "dump_twa"
        else:
            dump_command = "dump"
            
        if self.send_command(dump_command):
            time.sleep(1.0)  # Give board more time to start sending
            
            # Read the dump output
            csv_lines = []
            comment_lines = []
            
            timeout = time.time() + 10
            header_found = False
            
            while time.time() < timeout:
                if self.ser.in_waiting:
                    try:
                        # Read raw bytes first
                        raw_line = self.ser.readline()
                        # Decode to string, replacing any problematic characters
                        line = raw_line.decode('utf-8', errors='replace').strip()
                    except:
                        continue
                    
                    # Look for lines that start with [ followed by content and ]
                    if line.startswith("[") and "]" in line:
                        # Extract the part after ]
                        bracket_end = line.index("]")
                        bracket_content = line[1:bracket_end].strip()
                        csv_content = line[bracket_end + 1:].strip()
                        
                        # Capture comment lines
                        if bracket_content == "COMMENT":
                            comment_lines.append(csv_content)
                        # Capture header line
                        elif bracket_content == "HEADER" and "," in csv_content:
                            # Skip obviously corrupted headers (lots of non-printable characters)
                            non_printable_count = sum(1 for c in csv_content if ord(c) < 32 and c not in '\n\r\t')
                            if not header_found and non_printable_count < len(csv_content) * 0.2:
                                csv_lines.append(csv_content)
                                header_found = True
                        # Capture data lines (numbers only)
                        elif bracket_content.isdigit() and "," in csv_content:
                            # Skip lines with excessive non-printable characters
                            non_printable_count = sum(1 for c in csv_content if ord(c) < 32 and c not in '\n\r\t')
                            if non_printable_count < len(csv_content) * 0.2:
                                csv_lines.append(csv_content)
                    elif "Displayed" in line and "lines" in line:
                        break
                else:
                    time.sleep(0.1)
                    
            if len(csv_lines) > 0:
                # Write to file with proper newlines
                output_path = Path(output_file)
                with open(output_path, 'w', encoding='utf-8', newline='') as f:
                    # Write comment lines first
                    if comment_lines:
                        for comment in comment_lines:
                            f.write(f'# {comment}\n')
                        f.write('#\n')
                    # Write CSV data
                    f.write('\n'.join(csv_lines))
                    f.write('\n')
                    
                data_lines = len(csv_lines) - 1 if len(csv_lines) > 0 else 0  # Subtract header
                print(f"✓ Downloaded {data_lines} data rows to {output_path}")
                if comment_lines:
                    print(f"  Including {len(comment_lines)} metadata comment lines")
                print(f"  File size: {output_path.stat().st_size} bytes")
                return True
            else:
                print("✗ No CSV data received. Log file may be empty or corrupted.")
                return False
        else:
            return False
                
    def monitor(self):
        """Monitor live output from the board"""
        print("\nMonitoring live data (Press Ctrl+C to stop)...\n")
        
        try:
            while True:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                else:
                    time.sleep(0.1)
        except KeyboardInterrupt:
            print("\n\nMonitoring stopped.")
    
    def show_metadata(self):
        """Show current board metadata"""
        print("\nRetrieving metadata...")
        
        if self.send_command("metadata"):
            time.sleep(0.5)
            
            # Read response, filtering out measurement data
            timeout = time.time() + 3
            in_metadata = False
            
            while time.time() < timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Start capturing when we see the metadata header
                        if "Current Metadata" in line or "═" in line:
                            in_metadata = True
                            print(line)
                        # Stop if we see measurement indicators
                        elif "Measurement #" in line or "ENVIRONMENTAL" in line:
                            break
                        # Print metadata lines
                        elif in_metadata:
                            print(line)
                            # Stop after the final separator or tip
                            if "Tip:" in line or ("═" in line and in_metadata):
                                # Read one more line after tip
                                time.sleep(0.1)
                                if self.ser.in_waiting:
                                    extra = self.ser.readline().decode('utf-8', errors='ignore').strip()
                                    if extra:
                                        print(extra)
                                break
                else:
                    time.sleep(0.1)
        else:
            print("Failed to send metadata command")
    
    def set_metadata(self, key, value, interactive=True):
        """Set a metadata value with optional user confirmation for log clearing"""
        print(f"\nSetting metadata: {key} = {value}...")
        
        if self.send_command(f"meta {key} {value}"):
            time.sleep(0.5)
            
            # Read initial response
            lines = []
            timeout = time.time() + 5
            waiting_for_input = False
            
            while time.time() < timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                        lines.append(line)
                        
                        # Check if board is waiting for user input
                        if "Your choice:" in line or "choice:" in line.lower():
                            waiting_for_input = True
                            break
                        
                        # Check if operation completed
                        if "Metadata set:" in line or "cancelled" in line.lower() or "unchanged" in line.lower():
                            return
                else:
                    time.sleep(0.1)
            
            # If board is waiting for input, handle the interaction
            if waiting_for_input and interactive:
                choice = input().strip().lower()
                self.ser.write(f"{choice}\n".encode())
                
                # If user chose to download/dump, capture and save the CSV
                if choice == "download" or choice == "dump":
                    print("\n📥 Downloading CSV file before metadata change...")
                    time.sleep(1.5)  # Give board more time to start dumping
                    
                    # Generate timestamped filename
                    import datetime
                    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                    backup_filename = f"sensor_log_backup_{timestamp}.csv"
                    
                    # Capture CSV output using identical logic to download_log()
                    csv_lines = []
                    comment_lines = []
                    
                    dump_timeout = time.time() + 15
                    header_found = False
                    
                    while time.time() < dump_timeout:
                        if self.ser.in_waiting:
                            try:
                                # Read raw bytes first
                                raw_line = self.ser.readline()
                                # Decode to string, replacing any problematic characters
                                line = raw_line.decode('utf-8', errors='replace').strip()
                            except:
                                continue
                            
                            if line:
                                print(line)
                                
                                # Look for lines that start with [ followed by content and ]
                                if line.startswith("[") and "]" in line:
                                    # Extract the part after ]
                                    bracket_end = line.index("]")
                                    bracket_content = line[1:bracket_end].strip()
                                    csv_content = line[bracket_end + 1:].strip()
                                    
                                    # Capture comment lines
                                    if bracket_content == "COMMENT":
                                        comment_lines.append(csv_content)
                                    # Capture header line
                                    elif bracket_content == "HEADER" and "," in csv_content:
                                        # Skip obviously corrupted headers (lots of non-printable characters)
                                        non_printable_count = sum(1 for c in csv_content if ord(c) < 32 and c not in '\n\r\t')
                                        if not header_found and non_printable_count < len(csv_content) * 0.2:
                                            csv_lines.append(csv_content)
                                            header_found = True
                                    # Capture data lines (numbers only)
                                    elif bracket_content.isdigit() and "," in csv_content:
                                        # Skip lines with excessive non-printable characters
                                        non_printable_count = sum(1 for c in csv_content if ord(c) < 32 and c not in '\n\r\t')
                                        if non_printable_count < len(csv_content) * 0.2:
                                            csv_lines.append(csv_content)
                                elif "Displayed" in line and "lines" in line:
                                    break
                                
                                # Stop when we see the info message after output
                                if "CSV output complete" in line or "You can now set metadata safely." in line:
                                    break
                        else:
                            time.sleep(0.1)
                    
                    # Save the CSV file using identical format to download_log()
                    if csv_lines:
                        output_path = Path(backup_filename)
                        with open(output_path, 'w', encoding='utf-8', newline='') as f:
                            # Write comment lines first
                            if comment_lines:
                                for comment in comment_lines:
                                    f.write(f'# {comment}\n')
                                f.write('#\n')
                            # Write CSV data
                            f.write('\n'.join(csv_lines))
                            f.write('\n')
                        
                        data_lines = len(csv_lines) - 1 if len(csv_lines) > 0 else 0  # Subtract header
                        print(f"\n✅ CSV file saved to: {backup_filename}")
                        print(f"   {data_lines} data rows saved")
                        if comment_lines:
                            print(f"   Including {len(comment_lines)} metadata comment lines")
                        print(f"   File size: {output_path.stat().st_size} bytes\n")
                    else:
                        print("\n⚠ Warning: No CSV data captured (file may be empty)\n")
                    
                    # Continue reading until operation completes
                    final_timeout = time.time() + 5
                    while time.time() < final_timeout:
                        if self.ser.in_waiting:
                            line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                            if line:
                                print(line)
                                if "can now:" in line.lower() or "cancelled" in line.lower():
                                    break
                        else:
                            time.sleep(0.1)
                            if not self.ser.in_waiting:
                                break
                else:
                    # For "yes" or other responses, just read the outcome
                    time.sleep(1.0)
                    response_timeout = time.time() + 10
                    while time.time() < response_timeout:
                        if self.ser.in_waiting:
                            line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                            if line:
                                print(line)
                                if "Metadata set:" in line or "cancelled" in line.lower():
                                    break
                        else:
                            time.sleep(0.1)
                            if not self.ser.in_waiting:
                                break
        else:
            print("Failed to send meta command")
    
    def show_about(self):
        """Display project information and license"""
        print("\n" + "="*80)
        print("  SEN66-Dosimetry Project Information")
        print("="*80)
        print("\nProject: SEN66-Dosimetry")
        print("Creator: Christopher Lee")
        print("License: GNU General Public License v3.0 (GPLv3)")
        print("\nDescription:")
        print("  Advanced air quality monitoring system with the Sensirion SEN66 sensor")
        print("  Features real-time measurements, 8-hour TWA calculations, CSV logging,")
        print("  and localized timestamps with configurable UTC offset.")
        print("\nLicense Notice:")
        print("  This program is free software: you can redistribute it and/or modify")
        print("  it under the terms of the GNU General Public License as published by")
        print("  the Free Software Foundation, either version 3 of the License, or")
        print("  (at your option) any later version.")
        print("\n  This program is distributed in the hope that it will be useful,")
        print("  but WITHOUT ANY WARRANTY; without even the implied warranty of")
        print("  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the")
        print("  GNU General Public License for more details.")
        print("\n" + "="*80 + "\n")
    
    def reset_metadata(self):
        """Reset all metadata to default state"""
        print("\n⚠ WARNING: This will reset all metadata to default state!")
        print("   - Keeps: device_name, firmware_version, session_start")
        print("   - Resets: user, project, location (empty values)")
        print("   - Deletes: All other custom metadata")
        print("   - Clears: CSV log file")
        print()
        
        confirm = input("Type 'yes' to confirm: ").strip().lower()
        
        if confirm != 'yes':
            print("❌ Metadata reset cancelled.\n")
            return False
        
        # Flush any pending data
        self.flush_input_buffer()
        
        if self.send_command("resetmeta"):
            # Wait for the board to process
            time.sleep(0.5)
            
            # Send 'yes' confirmation to the board
            self.ser.write(b'yes\n')
            self.ser.flush()
            
            # Read response
            time.sleep(1.0)
            response_timeout = time.time() + 5
            while time.time() < response_timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                        if "reset to defaults" in line.lower() or "cancelled" in line.lower():
                            break
                else:
                    time.sleep(0.1)
                    if not self.ser.in_waiting:
                        break
            return True
        else:
            print("Failed to send resetmeta command")
            return False


def list_ports():
    """List all available serial ports"""
    ports = serial.tools.list_ports.comports()
    
    if not ports:
        print("No serial ports found")
        return
        
    print("\nAvailable serial ports:")
    print("-" * 60)
    for port in ports:
        print(f"  {port.device}")
        print(f"    Description: {port.description}")
        if port.manufacturer:
            print(f"    Manufacturer: {port.manufacturer}")
        print()


def interactive_mode(port=None, baudrate=115200):
    """Run CLI in interactive console mode"""
    print("\n" + "="*60)
    print("  SEN66-Dosimetry Interactive Console")
    print("="*60)
    print("\nCommands:")
    print("  status              - Show current measurement")
    print("  clear               - Clear log file")
    print("  download [file]     - Download log (default: sensor_log.csv)")
    print("  export_twa [file]   - Export 8-hour TWA calculations")
    print("  rtc status          - Show ESP32 RTC status")
    print("  rtc sync            - Synchronize ESP32 RTC")
    print("  config              - Show current configuration")
    print("  prefs <key> <value> - Set config (measurement, logging, utc)")
    print("  timezone <offset>   - Set UTC offset hours (-12 to +14)")
    print("  metadata            - Show all metadata")
    print("  meta <key> <value>  - Set metadata (user, project, location)")
    print("  resetmeta           - Reset all metadata to defaults")
    print("  monitor             - Monitor live data (Ctrl+C to stop)")
    print("  list-ports          - List available serial ports")
    print("  about               - Show project information and license")
    print("  help                - Show this help")
    print("  exit, quit          - Exit console")
    print()
    
    # Create CLI instance
    cli = SEN66CLI(port=port, baudrate=baudrate)
    connected = False
    
    while True:
        try:
            # Flush any pending serial data before prompting user
            if connected and cli.ser and cli.ser.is_open:
                cli.flush_input_buffer()
            
            # Get command
            cmd_input = input("sen66> ").strip()
            
            if not cmd_input:
                continue
                
            parts = cmd_input.split()
            cmd = parts[0].lower()
            args = parts[1:] if len(parts) > 1 else []
            
            # Handle exit commands
            if cmd in ['exit', 'quit', 'q']:
                print("\nExiting...")
                break
                
            # Handle help
            if cmd in ['help', '?']:
                print("\nCommands:")
                print("  status              - Show current measurement")
                print("  clear               - Clear log file")
                print("  download [file]     - Download log (default: sensor_log.csv)")
                print("  export_twa [file]   - Export 8-hour TWA calculations")
                print("  rtc status          - Show ESP32 RTC status")
                print("  rtc sync            - Synchronize ESP32 RTC")
                print("  config              - Show current configuration")
                print("  prefs <key> <value> - Set config (measurement, logging, utc)")
                print("  timezone <offset>   - Set UTC offset hours (-12 to +14)")
                print("  metadata            - Show all metadata")
                print("  meta <key> <value>  - Set metadata (user, project, location)")
                print("  resetmeta           - Reset all metadata to defaults")
                print("  monitor             - Monitor live data (Ctrl+C to stop)")
                print("  list-ports          - List available serial ports")
                print("  about               - Show project information and license")
                print("  help                - Show this help")
                print("  exit, quit          - Exit console")
                continue
                
            # Handle list-ports without connection
            if cmd == 'list-ports':
                list_ports()
                continue
                
            # For other commands, ensure connected
            if not connected:
                if not cli.connect():
                    print("Error: Failed to connect. Use 'list-ports' to see available ports.")
                    continue
                connected = True
                
            # Execute commands
            if cmd == 'status':
                cli.get_status()
            elif cmd == 'clear':
                cli.clear_log()
            elif cmd == 'download':
                output_file = args[0] if args else 'sensor_log.csv'
                cli.download_log(output_file)
            elif cmd == 'twa' or cmd == 'export_twa':
                output_file = args[0] if args else None
                cli.export_twa_data(output_file)
            elif cmd == 'rtc':
                if len(args) >= 1:
                    if args[0] == 'status':
                        cli.rtc_status()
                    elif args[0] == 'sync':
                        cli.rtc_sync()
                    else:
                        print("Usage: rtc <status|sync>")
                        print("  rtc status  - Show ESP32 RTC status")
                        print("  rtc sync    - Synchronize ESP32 RTC")
                else:
                    print("Usage: rtc <status|sync>")
                    print("  rtc status  - Show ESP32 RTC status")
                    print("  rtc sync    - Synchronize ESP32 RTC")
            elif cmd == 'config':
                cli.show_config()
            elif cmd == 'prefs' or cmd == 'set':  # Accept both for compatibility
                if len(args) >= 2:
                    cli.set_config(args[0], args[1])
                else:
                    print("Usage: prefs <key> <value>")
                    print("Keys: measurement, logging, utc")
                    print("Example: prefs utc -5  (for EST timezone)")
            elif cmd == 'timezone' or cmd == 'utc':
                if len(args) >= 1:
                    cli.set_timezone(args[0])
                else:
                    print("Usage: timezone <offset>")
                    print("Set UTC offset in hours (-12 to +14)")
                    print("Examples: timezone -5  (EST), timezone +9  (JST)")
            elif cmd == 'metadata':
                cli.show_metadata()
            elif cmd == 'meta':
                if len(args) >= 2:
                    cli.set_metadata(args[0], ' '.join(args[1:]))  # Join in case value has spaces
                else:
                    print("Usage: meta <key> <value>")
                    print("Common keys: user, project, location")
            elif cmd == 'resetmeta':
                cli.reset_metadata()
            elif cmd == 'about':
                cli.show_about()
            elif cmd == 'monitor':
                print("\nPress Ctrl+C to stop monitoring...\n")
                cli.monitor()
            else:
                print(f"\n❌ Unknown command: '{cmd}'\n")
                print("Available commands:")
                print("  status              - Show current measurement")
                print("  clear               - Clear log file")
                print("  download [file]     - Download log (default: sensor_log.csv)")
                print("  export_twa [file]   - Export 8-hour TWA calculations")
                print("  rtc status          - Show ESP32 RTC status")
                print("  rtc sync            - Synchronize ESP32 RTC")
                print("  config              - Show current configuration")
                print("  prefs <key> <value> - Set config (measurement, logging, utc)")
                print("  timezone <offset>   - Set UTC offset hours (-12 to +14)")
                print("  metadata            - Show all metadata")
                print("  meta <key> <value>  - Set metadata (user, project, location)")
                print("  resetmeta           - Reset all metadata to defaults")
                print("  monitor             - Monitor live data (Ctrl+C to stop)")
                print("  list-ports          - List available serial ports")
                print("  about               - Show project information and license")
                print("  help                - Show this help")
                print("  exit, quit          - Exit console")
                
        except KeyboardInterrupt:
            print("\n")
            continue
        except EOFError:
            print("\nExiting...")
            break
        except Exception as e:
            print(f"Error: {e}")
            
    if connected and cli.ser:
        cli.disconnect()


def main():
    parser = argparse.ArgumentParser(
        description="SEN66-Dosimetry CLI Tool",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                                 # Interactive console mode
  %(prog)s status                          # Show current measurement
  %(prog)s status --port COM5              # Specify port
  %(prog)s clear                           # Clear log file
  %(prog)s download --output data.csv      # Download log file
  %(prog)s export-twa                      # Export 8-hour TWA calculations
  %(prog)s rtc-status                      # Show ESP32 RTC status
  %(prog)s rtc-sync                        # Sync ESP32 RTC time
  %(prog)s timezone --offset -5            # Set timezone to UTC-5 (EST)
  %(prog)s timezone --offset +9 --port COM5 # Set timezone to UTC+9 (JST)
  %(prog)s monitor                         # Monitor live data
  %(prog)s list-ports                      # List available ports
  %(prog)s console --port COM5             # Start interactive console
        """
    )
    
    parser.add_argument('command', 
                       nargs='?',
                       choices=['status', 'clear', 'download', 'export-twa', 'monitor', 'rtc-status', 'rtc-sync', 'config', 'set', 'metadata', 'meta', 'timezone', 'utc', 'about', 'list-ports', 'console'],
                       help='Command to execute (omit for interactive mode)')
    parser.add_argument('--port', '-p',
                       help='Serial port (e.g., COM5, /dev/ttyUSB0)')
    parser.add_argument('--output', '-o',
                       default='sensor_log.csv',
                       help='Output file for download command (default: sensor_log.csv)')
    parser.add_argument('--key', '-k',
                       help='Key for set/meta commands (config: measurement, logging, utc; meta: user, project, location)')
    parser.add_argument('--value', '-v',
                       help='Value for set/meta commands')
    parser.add_argument('--offset',
                       type=int,
                       help='UTC offset in hours (-12 to +14) for timezone command')
    parser.add_argument('--baudrate', '-b',
                       type=int,
                       default=115200,
                       help='Baud rate (default: 115200)')
    
    args = parser.parse_args()
    
    # If no command given, start interactive mode
    if not args.command:
        return interactive_mode(port=args.port, baudrate=args.baudrate)
    
    # Handle console command
    if args.command == 'console':
        return interactive_mode(port=args.port, baudrate=args.baudrate)
    
    # Handle list-ports without connecting
    if args.command == 'list-ports':
        list_ports()
        return 0
        
    # Create CLI instance
    cli = SEN66CLI(port=args.port, baudrate=args.baudrate)
    
    try:
        # Connect to board
        if not cli.connect():
            return 1
            
        # Execute command
        if args.command == 'status':
            cli.get_status()
        elif args.command == 'clear':
            cli.clear_log()
        elif args.command == 'download':
            cli.download_log(args.output)
        elif args.command == 'export-twa':
            cli.export_twa_data()
        elif args.command == 'rtc-status':
            cli.rtc_status()
        elif args.command == 'rtc-sync':
            cli.rtc_sync()
        elif args.command == 'config':
            cli.show_config()
        elif args.command == 'set':
            if args.key and args.value is not None:
                cli.set_config(args.key, int(args.value))
            else:
                print("Error: set command requires --key and --value")
                print("Example: python sen66_cli.py set --key measurement --value 30")
                return 1
        elif args.command == 'metadata':
            cli.show_metadata()
        elif args.command == 'meta':
            if args.key and args.value is not None:
                cli.set_metadata(args.key, args.value)
            else:
                print("Error: meta command requires --key and --value")
                print("Example: python sen66_cli.py meta --key user --value John_Doe")
                return 1
        elif args.command == 'timezone' or args.command == 'utc':
            if args.offset is not None:
                cli.set_timezone(str(args.offset))
            else:
                print("Error: timezone command requires --offset")
                print("Example: python sen66_cli.py timezone --offset -5")
                return 1
        elif args.command == 'about':
            cli.show_about()
        elif args.command == 'monitor':
            cli.monitor()
        else:
            print(f"\n❌ Unknown command: '{args.command}'\n")
            print("Available commands:")
            print("  status              - Show current measurement")
            print("  clear               - Clear log file")
            print("  download            - Download CSV log file")
            print("  export-twa          - Export 8-hour TWA calculations")
            print("  rtc-status          - Show ESP32 RTC status")
            print("  rtc-sync            - Synchronize ESP32 RTC")
            print("  config              - Show current configuration")
            print("  set                 - Set configuration value")
            print("  timezone            - Set UTC offset (timezone)")
            print("  metadata            - Show all metadata")
            print("  meta                - Set metadata value")
            print("  monitor             - Monitor live data")
            print("  about               - Show project information and license")
            print("  list-ports          - List available serial ports")
            print("  console             - Start interactive console")
            print("\nUse --help for detailed usage information")
            return 1
            
    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
    except Exception as e:
        print(f"Error: {e}")
        return 1
    finally:
        cli.disconnect()
        
    return 0


if __name__ == '__main__':
    sys.exit(main())
