#!/usr/bin/env python3
"""
SEN66-Dosimetry CLI Tool
A command-line interface for interacting with the SEN66 air quality monitor

Usage:
    python sen66_cli.py                               # Interactive console mode
    python sen66_cli.py console [--port COM5]         # Interactive console mode
    python sen66_cli.py status [--port COM5]
    python sen66_cli.py clear [--port COM5]
    python sen66_cli.py download [--port COM5] [--output data.csv]
    python sen66_cli.py sync [--port COM5]
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
        
    def send_command(self, command):
        """Send a command to the board"""
        if not self.ser or not self.ser.is_open:
            print("Error: Not connected")
            return False
            
        try:
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
        print("\nClearing log file...")
        
        if self.send_command("clear"):
            time.sleep(0.5)
            response = self.read_until_prompt(timeout=2)
            for line in response:
                print(line)
        else:
            print("Failed to send clear command")
    
    def sync_time(self):
        """Synchronize the board's time with current Unix timestamp"""
        import time as time_module
        
        unix_time = int(time_module.time())
        print(f"\nSynchronizing board time to Unix timestamp: {unix_time}")
        
        if self.send_command(f"sync {unix_time}"):
            time.sleep(0.5)
            response = self.read_until_prompt(timeout=2)
            for line in response:
                print(line)
        else:
            print("Failed to send sync command")
    
    def show_config(self):
        """Show current board configuration"""
        print("\nRetrieving configuration...")
        
        if self.send_command("config"):
            time.sleep(0.5)
            response = self.read_until_prompt(timeout=2)
            for line in response:
                print(line)
        else:
            print("Failed to send config command")
    
    def set_config(self, key, value):
        """Set a configuration value"""
        print(f"\nSetting {key} to {value}...")
        
        if self.send_command(f"set {key} {value}"):
            time.sleep(0.5)
            response = self.read_until_prompt(timeout=2)
            for line in response:
                print(line)
        else:
            print("Failed to send set command")
            
    def download_log(self, output_file):
        """Download the CSV log file"""
        print(f"\nDownloading log file to {output_file}...")
        
        if self.send_command("dump"):
            time.sleep(1.0)  # Give board more time to start sending
            
            # Known CSV header from firmware
            csv_header = "timestamp,temperature,humidity,vocIndex,noxIndex,pm1_0,pm2_5,pm4_0,pm10,co2,dewPoint,heatIndex,absoluteHumidity,twa_pm1_0,twa_pm2_5,twa_pm4_0,twa_pm10"
            
            # Read the dump output
            csv_lines = [csv_header]  # Start with header
            
            timeout = time.time() + 10
            while time.time() < timeout:
                if self.ser.in_waiting:
                    try:
                        # Read raw bytes first
                        raw_line = self.ser.readline()
                        # Decode to string, replacing any problematic characters
                        line = raw_line.decode('utf-8', errors='replace').strip()
                    except:
                        continue
                    
                    # Look for lines that start with [ followed by numbers and ]
                    # Format is like: [   1] or [ 123]
                    if line.startswith("[") and "]" in line:
                        # Extract the part after ]
                        bracket_end = line.index("]")
                        bracket_content = line[1:bracket_end].strip()
                        csv_content = line[bracket_end + 1:].strip()
                        
                        # Only process if bracket contains a number (data lines, not HEADER)
                        # and CSV content looks valid (has commas, no control characters)
                        if bracket_content.isdigit() and "," in csv_content:
                            # Check for control characters (except \r and \n which we've stripped)
                            if not any(ord(c) < 32 for c in csv_content):
                                csv_lines.append(csv_content)
                    elif "Displayed" in line and "lines" in line:
                        break
                else:
                    time.sleep(0.1)
                    
            if len(csv_lines) > 1:  # More than just the header
                # Write to file with proper newlines
                output_path = Path(output_file)
                with open(output_path, 'w', encoding='utf-8', newline='') as f:
                    f.write('\n'.join(csv_lines))
                    f.write('\n')
                    
                data_lines = len(csv_lines) - 1  # Subtract header
                print(f"✓ Downloaded {data_lines} data rows ({len(csv_lines)} total lines) to {output_path}")
                print(f"  File size: {output_path.stat().st_size} bytes")
            else:
                print("✗ No CSV data received. Log file may be empty or corrupted.")
                
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
    print("  sync                - Synchronize time with PC")
    print("  config              - Show current configuration")
    print("  set <key> <value>   - Set config (measurement, logging)")
    print("  monitor             - Monitor live data (Ctrl+C to stop)")
    print("  list-ports          - List available serial ports")
    print("  help                - Show this help")
    print("  exit, quit          - Exit console")
    print()
    
    # Create CLI instance
    cli = SEN66CLI(port=port, baudrate=baudrate)
    connected = False
    
    while True:
        try:
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
                print("  sync                - Synchronize time with PC")
                print("  config              - Show current configuration")
                print("  set <key> <value>   - Set config (measurement, logging)")
                print("  monitor             - Monitor live data (Ctrl+C to stop)")
                print("  list-ports          - List available serial ports")
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
            elif cmd == 'sync':
                cli.sync_time()
            elif cmd == 'config':
                cli.show_config()
            elif cmd == 'set':
                if len(args) >= 2:
                    cli.set_config(args[0], args[1])
                else:
                    print("Usage: set <key> <value>")
                    print("Keys: measurement, logging")
            elif cmd == 'monitor':
                print("\nPress Ctrl+C to stop monitoring...\n")
                cli.monitor()
            else:
                print(f"\n❌ Unknown command: '{cmd}'\n")
                print("Available commands:")
                print("  status              - Show current measurement")
                print("  clear               - Clear log file")
                print("  download [file]     - Download log (default: sensor_log.csv)")
                print("  sync                - Synchronize time with PC")
                print("  config              - Show current configuration")
                print("  set <key> <value>   - Set config (measurement, logging)")
                print("  monitor             - Monitor live data (Ctrl+C to stop)")
                print("  list-ports          - List available serial ports")
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
  %(prog)s sync                            # Sync time with PC
  %(prog)s monitor                         # Monitor live data
  %(prog)s list-ports                      # List available ports
  %(prog)s console --port COM5             # Start interactive console
        """
    )
    
    parser.add_argument('command', 
                       nargs='?',
                       choices=['status', 'clear', 'download', 'monitor', 'sync', 'config', 'set', 'list-ports', 'console'],
                       help='Command to execute (omit for interactive mode)')
    parser.add_argument('--port', '-p',
                       help='Serial port (e.g., COM5, /dev/ttyUSB0)')
    parser.add_argument('--output', '-o',
                       default='sensor_log.csv',
                       help='Output file for download command (default: sensor_log.csv)')
    parser.add_argument('--key', '-k',
                       help='Config key for set command (measurement, logging)')
    parser.add_argument('--value', '-v',
                       type=int,
                       help='Config value for set command (seconds)')
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
        elif args.command == 'sync':
            cli.sync_time()
        elif args.command == 'config':
            cli.show_config()
        elif args.command == 'set':
            if args.key and args.value is not None:
                cli.set_config(args.key, args.value)
            else:
                print("Error: set command requires --key and --value")
                print("Example: python sen66_cli.py set --key measurement --value 30")
                return 1
        elif args.command == 'monitor':
            cli.monitor()
        else:
            print(f"\n❌ Unknown command: '{args.command}'\n")
            print("Available commands:")
            print("  status              - Show current measurement")
            print("  clear               - Clear log file")
            print("  download            - Download CSV log file")
            print("  sync                - Synchronize time with PC")
            print("  config              - Show current configuration")
            print("  set                 - Set configuration value")
            print("  monitor             - Monitor live data")
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
