# OSH-Monitor CLI Tool

A Python command-line tool for interacting with the OSH-Monitor air quality monitor.

## Installation

```bash
pip install pyserial
```

## Usage

### List Available Ports
```bash
python osh_cli.py list-ports
```

### View Current Status
Get the most recent measurement from the sensor:
```bash
python osh_cli.py status

# Specify port
python osh_cli.py status --port COM5
```

### Download Log File
Download the CSV log file from the board:
```bash
python osh_cli.py download

# Specify output file
python osh_cli.py download --output my_data.csv

# Specify port and output
python osh_cli.py download --port COM5 --output data.csv
```

### Clear Log File
Clear the CSV log file on the board:
```bash
python osh_cli.py clear --port COM5
```

### Monitor Live Data
Continuously monitor live output from the board:
```bash
python osh_cli.py monitor --port COM5
```
Press `Ctrl+C` to stop monitoring.

## Command Reference

| Command | Description | Options |
|---------|-------------|---------|
| `list-ports` | List all available serial ports | - |
| `status` | Show the most recent measurement | `--port` |
| `clear` | Clear the log file on the board | `--port` |
| `download` | Download CSV log file | `--port`, `--output` |
| `monitor` | Monitor live sensor data | `--port` |

## Options

- `--port`, `-p`: Serial port (e.g., COM5, /dev/ttyUSB0)
  - If not specified, the tool attempts to auto-detect the board
- `--output`, `-o`: Output file path for download command (default: sensor_log.csv)
- `--baudrate`, `-b`: Serial baud rate (default: 115200)

## Examples

```bash
# Auto-detect board and show status
python osh_cli.py status

# Download data with custom filename
python osh_cli.py download --output "air_quality_$(date +%Y%m%d).csv"

# Clear log on specific port
python osh_cli.py clear --port COM6

# Monitor with auto-detection
python osh_cli.py monitor
```

## Requirements

- Python 3.6+
- pyserial library
- OSH-Monitor board connected via USB

## Troubleshooting

**Port access denied:**
- On Windows: Close any serial monitors (PlatformIO, Arduino IDE)
- On Linux: Add user to dialout group: `sudo usermod -a -G dialout $USER`
- On macOS: Check port permissions

**Port not detected:**
- Use `list-ports` to see available ports
- Manually specify port with `--port` option
- Check USB cable connection

**No data received:**
- Ensure board is powered and running firmware
- Wait for measurement cycle (20 seconds in debug mode)
- Check baud rate matches board configuration (115200)
