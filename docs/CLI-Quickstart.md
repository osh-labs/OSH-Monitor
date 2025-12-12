# OSH-Monitor CLI Quickstart

A concise guide to installing, connecting, and using the OSH-Monitor Python CLI to interact with the ESP32-S3 SEN66 firmware.

---

## Prerequisites

- Python 3.8+ installed
- Windows: COM port access to the ESP32-S3 device
- Install pyserial:

```bash
pip install pyserial
```

---

## Finding Your Port

List available ports, then pick your device (e.g., `COM5`):

```bash
python osh_cli.py list-ports
```

Auto-detection is built-in, but specifying `--port` is more reliable when multiple devices are connected.

---

## Quick Commands

- Show measurement status:
```bash
python osh_cli.py status --port COM5
```

- Check storage capacity:
```bash
python osh_cli.py console --port COM5
> storage
```

- Download CSV log (timestamped filename):
```bash
python osh_cli.py download --output sensor_log.csv --port COM5
```

- Export OSHA 8-hour TWA (and download):
```bash
python osh_cli.py export-twa --port COM5
```

- Set timezone to UTC-5:
```bash
python osh_cli.py timezone --offset -5 --port COM5
```

- Monitor live stream:
```bash
python osh_cli.py monitor --port COM5
```

- Interactive console (REPL):
```bash
python osh_cli.py
# or
python osh_cli.py console --port COM5
```

---

## Typical Workflows

### Compliance Export + Log Retrieval
1. Generate TWA export:
```bash
python osh_cli.py export-twa --port COM5
```
2. Download sensor log (auto-generates TWA alongside):
```bash
python osh_cli.py download --output sensor_log.csv --port COM5
```

### RTC Synchronization
- Show RTC status:
```bash
python osh_cli.py rtc-status --port COM5
```
- Sync device RTC to host time:
```bash
python osh_cli.py rtc-sync --port COM5
```

---

## Command → Device Protocol Mapping

The CLI talks to the firmware over serial using human-readable commands.

- `status` → Reads recent measurement block from stream (no command sent)
- `storage` → Sends `storage` and prints filesystem capacity, usage, and estimated remaining hours
- `clear` → Sends `clear`, shows device warning, relays user confirmation (`yes/no`)
- `download` → Sends `dump` to stream `/sensor_log.csv` and writes a local file
- `export-twa` → Sends `export_twa` to compute; then `dump_twa` to fetch `/twa_export.csv`
- `rtc-status` → Sends `rtc status` and prints structured RTC info
- `rtc-sync` → Sends `rtc sync <unix_time>` with host timestamp
- `config` → Sends `config` and prints current settings
- `set` → Sends `prefs <key> <value>` (e.g., `measurement`, `logging`, `utc`)
- `timezone`/`utc` → Validates offset [-12,+14], then `prefs utc <offset>`
- `metadata` → Sends `metadata` and prints all metadata
- `meta` → Sends `meta <key> <value>`; handles device prompts (download/dump/clear)
- `resetmeta` → Sends `resetmeta`, confirms `yes`, clears metadata and log
- `monitor` → Streams lines from device until Ctrl+C
- `about`/`list-ports`/`console` → Local-only helpers

---

## Connection, Port Detection, and Buffering

- **Auto-detection:** Scans ports for descriptions containing "USB" or "ESP32"; otherwise uses the first port.
- **Buffer hygiene:** Before sending any command, the CLI flushes the input buffer to avoid mixing ongoing measurements with the command response.
- **Response parsing:** Time-bounded read loops collect lines, separate comments from CSV, detect headers, and look for success markers (e.g., `Export file:`).

---

## Expanded Usage Examples

- List ports and connect for status:
```bash
python osh_cli.py list-ports
python osh_cli.py status --port COM5
```

- Download logs and auto-generate TWA alongside:
```bash
python osh_cli.py download --output sensor_log.csv --port COM5
```

- Generate dedicated TWA export:
```bash
python osh_cli.py export-twa --port COM5
```

- Set timezone (validation in [-12, +14]):
```bash
python osh_cli.py timezone --offset -5 --port COM5
```

- Use interactive console:
```bash
python osh_cli.py
# or
python osh_cli.py console --port COM5
```

---

## Onboarding Notes for New Developers

- The CLI maintains connection state and flushes buffers between commands to keep responses clean.
- Error handling is built-in for missing `pyserial`, invalid timezone offsets, busy ports, and empty dumps.
- File outputs use timestamped defaults: `sensor_log_YYYYMMDD_HHMMSS.csv`, `twa_export_YYYYMMDD_HHMMSS.csv`.
- When running `download`, a companion TWA file `<output>_with_twa.csv` is also produced for compliance workflows.

---

## Output Files

- Sensor log downloads default to `sensor_log_YYYYMMDD_HHMMSS.csv` (if `--output` omitted or equals `sensor_log.csv`).
- TWA exports default to `twa_export_YYYYMMDD_HHMMSS.csv`.
- When using `download`, the CLI also produces a TWA file named `<output>_with_twa.csv`.

---

## Onboarding Tips

- If ports are busy, unplug/replug and re-run `list-ports`.
- Prefer specifying `--port` explicitly when multiple serial devices are connected.
- Always set `timezone` before logging for correct localized timestamps.
- For config changes (e.g., measurement or logging intervals), use:
```bash
python osh_cli.py set --key measurement --value 60 --port COM5
python osh_cli.py set --key logging --value 60 --port COM5
```

---

## Troubleshooting

- **pyserial missing:** Install with `pip install pyserial`.
- **Permission errors:** Close other serial monitors (VS Code Serial, Arduino IDE) before running the CLI.
- **No data in downloads:** Ensure logging is enabled and the device has produced at least one measurement.
- **Timezone errors:** Offset must be integer in [-12, +14]; examples: `-5` (EST), `+9` (JST).

---

## Interactive Console Commands

Within the console (`osh_cli.py` or `console`), supported commands include:

`status`, `storage`, `clear`, `download [file]`, `export_twa [file]`, `rtc status`, `rtc sync`, `config`, `prefs <key> <value>`, `timezone <offset>`, `metadata`, `meta <key> <value>`, `resetmeta`, `monitor`, `list-ports`, `about`, `help`, `exit`.

This console flushes the buffer between prompts to keep responses clean and predictable.

---

## Compact Command Reference

| Command | Description | Example |
|---|---|---|
| `list-ports` | List available serial ports | `python osh_cli.py list-ports` |
| `status` | Show latest measurement block | `python osh_cli.py status --port COM5` |
| `storage` | Show filesystem storage statistics | `python osh_cli.py console --port COM5` then `storage` |
| `download` | Download `/sensor_log.csv` to local file | `python osh_cli.py download --output sensor_log.csv --port COM5` |
| `export-twa` | Generate and download 8-hr TWA export | `python osh_cli.py export-twa --port COM5` |
| `timezone` / `utc` | Set UTC offset [-12,+14] | `python osh_cli.py timezone --offset -5 --port COM5` |
| `rtc-status` | Show ESP32 RTC status | `python osh_cli.py rtc-status --port COM5` |
| `rtc-sync` | Sync RTC to host Unix time | `python osh_cli.py rtc-sync --port COM5` |
| `config` | Show current configuration | `python osh_cli.py config --port COM5` |
| `set` | Set config value via `prefs <key> <value>` (keys: measurement, logging, utc, storage_warning) | `python osh_cli.py set --key logging --value 60 --port COM5` |
| `metadata` | Show all metadata | `python osh_cli.py metadata --port COM5` |
| `meta` | Set metadata value | `python osh_cli.py meta --key project --value Demo --port COM5` |
| `resetmeta` | Reset metadata to defaults and clear log | `python osh_cli.py resetmeta --port COM5` |
| `monitor` | Stream live output until Ctrl+C | `python osh_cli.py monitor --port COM5` |
| `console` | Start interactive console (REPL) | `python osh_cli.py console --port COM5` |
| `about` | Show project and license info | `python osh_cli.py about` |

Note: In interactive console, the equivalent command for `export-twa` is `export_twa`.
