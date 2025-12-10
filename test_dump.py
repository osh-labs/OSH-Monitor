import serial
import time

# Connect to board
ser = serial.Serial('COM5', 115200, timeout=1)
time.sleep(2)

# Clear any existing output
ser.reset_input_buffer()

# Send dump command
print("Sending 'dump' command...")
ser.write(b'dump\n')
time.sleep(1)

# Read raw response
print("\n=== RAW OUTPUT (first 500 bytes) ===")
raw_data = ser.read(500)
print(repr(raw_data[:500]))

print("\n=== DECODED OUTPUT (first 20 lines) ===")
ser.reset_input_buffer()
ser.write(b'dump\n')
time.sleep(1)

for i in range(20):
    if ser.in_waiting:
        line = ser.readline()
        print(f"Line {i}: {repr(line)}")
    else:
        break

ser.close()
