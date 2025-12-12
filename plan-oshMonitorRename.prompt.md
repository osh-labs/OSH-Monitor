# Plan: Rename Project from SEN66-Dosimetry to OSH-Monitor

Complete rebranding of the project to reflect expanded scope as an Open Source Hardware platform for Occupational Safety and Health monitoring. The rename encompasses ~200+ changes across 17 files including source code, documentation, configuration, and examples.

## Steps

1. **Rename core source files** - Rename [src/SEN66Dosimetry.h](src/SEN66Dosimetry.h) → `OSHMonitor.h`, [src/SEN66Dosimetry.cpp](src/SEN66Dosimetry.cpp) → `OSHMonitor.cpp`, update all 71 class method implementations (`SEN66Dosimetry::` → `OSHMonitor::`), change header guards, update ESP32 NVS namespace keys from `"sen66"` → `"osh-mon"` and `"sen66-meta"` → `"osh-meta"`

2. **Update firmware and examples** - Modify [src/main.cpp](src/main.cpp) to include `OSHMonitor.h` and instantiate `OSHMonitor` class, update UI banner strings, apply same changes to [examples/BasicLogger/BasicLogger.ino](examples/BasicLogger/BasicLogger.ino)

3. **Rebrand documentation and configuration** - Update [README.md](README.md) title/subtitle emphasizing "Open-Source Occupational Safety & Health Monitor", modify code examples and API references, update [library.json](library.json) name/description/headers/URLs, update [lib/TWACore/library.json](lib/TWACore/library.json) homepage/repository references

4. **Update CLI tool** - Rename [sen66_cli.py](sen66_cli.py) → `osh_cli.py`, update all project references in docstrings/UI strings, update [CLI_README.md](CLI_README.md) title and references

5. **Update supporting documentation** - Modify [CHANGELOG.md](CHANGELOG.md), [CONTRIBUTING.md](CONTRIBUTING.md), [context.md](context.md), [RTC_INTEGRATION.md](RTC_INTEGRATION.md), and [BUILD_SUMMARY.md](BUILD_SUMMARY.md) to replace project name and GitHub URLs, update repository references from `duluthmachineworks/SEN66-Dosimetry` to `duluthmachineworks/OSH-Monitor`

6. **Rename workspace folder and GitHub repository** - Rename local workspace folder from `SEN66-Dosimetry` → `OSH-Monitor`, coordinate GitHub repository rename to update all clone URLs and references

7. **CSV Header Branding**: Generated CSV files will show "OSH-Monitor System"—acceptable, or keep sensor-specific branding in output files?
## Further Considerations

8. **Legacy Support**:  force clean break


