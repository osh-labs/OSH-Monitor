# Plan: Rename Project from SEN66-Dosimetry to OSH-Monitor

Complete rebranding of the project to reflect expanded scope as an Open Source Hardware platform for Occupational Safety and Health monitoring. The rename reflects the new three-library architecture where the platform layer (OSHMonitor) orchestrates sensor-specific libraries (SEN66Core, future additions) and reusable math libraries (TWACore).

## Architecture Context

**Three-Library Ecosystem:**
- **SEN66Core** (hardware abstraction) - Remains sensor-specific, no rename needed
- **TWACore** (OSHA calculations) - Sensor-agnostic, repository references need updating
- **OSHMonitor** (platform orchestration) - Main rename target, becomes sensor-agnostic

## Steps

1. **Rename platform source files** - Rename [src/SEN66Dosimetry.h](src/SEN66Dosimetry.h) → `OSHMonitor.h`, [src/SEN66Dosimetry.cpp](src/SEN66Dosimetry.cpp) → `OSHMonitor.cpp`, update all 71 class method implementations (`SEN66Dosimetry::` → `OSHMonitor::`), change header guards from `SEN66_DOSIMETRY_H` → `OSH_MONITOR_H`, update ESP32 NVS namespace keys from `"sen66"` → `"osh-mon"` and `"sen66-meta"` → `"osh-meta"`

2. **Update firmware and examples** - Modify [src/main.cpp](src/main.cpp) to include `OSHMonitor.h` and instantiate `OSHMonitor` class, update UI banner strings to "OSH-Monitor Air Quality System", apply same changes to [examples/BasicLogger/BasicLogger.ino](examples/BasicLogger/BasicLogger.ino)

3. **Update platform library configuration** - Update [library.json](library.json) name to `"OSH-Monitor"`, description emphasizing platform architecture, update headers field to `"OSHMonitor.h"`, change repository URLs from `SEN66-Dosimetry` to `OSH-Monitor`, update keywords to include "osh", "platform", "sensor-agnostic"

4. **Update sensor library references** - In [lib/SEN66Core/library.json](lib/SEN66Core/library.json): update homepage/repository URLs from `SEN66-Dosimetry` to `OSH-Monitor` (SEN66Core lives in OSH-Monitor repo but remains sensor-specific), keep library name as `"SEN66Core"` unchanged

5. **Update math library references** - In [lib/TWACore/library.json](lib/TWACore/library.json): update homepage/repository URLs from `SEN66-Dosimetry` to `OSH-Monitor`, keep library name as `"TWACore"` unchanged

6. **Update CLI tool** - Rename [sen66_cli.py](sen66_cli.py) → `osh_cli.py`, update all project references in docstrings/UI strings to "OSH-Monitor", update banner to "OSH-Monitor Interactive Console", update [CLI_README.md](CLI_README.md) title to "OSH-Monitor CLI Tool" and update all references

7. **Rebrand main documentation** - Update [README.md](README.md) title to "OSH-Monitor" with subtitle "Open-Source Occupational Safety & Health Monitoring Platform", add architecture section explaining three-library design, update code examples to use `OSHMonitor` class, update API references, emphasize sensor-agnostic platform design

8. **Update supporting documentation** - Modify [CHANGELOG.md](CHANGELOG.md) project name and add section for architecture refactoring, update [CONTRIBUTING.md](CONTRIBUTING.md) with OSH-Monitor name, update [context.md](context.md) to reflect platform architecture, update [RTC_INTEGRATION.md](RTC_INTEGRATION.md) title to "ESP32-S3 RTC Integration for OSH-Monitor", update [BUILD_SUMMARY.md](BUILD_SUMMARY.md) project name

9. **Update documentation library references** - In [docs/TWACore-Developer-Guide.md](docs/TWACore-Developer-Guide.md): update project references and repository URLs, in [docs/CLI-Quickstart.md](docs/CLI-Quickstart.md): update tool name to `osh_cli.py` and project references, in [docs/SEN66Core-Test-Report.md](docs/SEN66Core-Test-Report.md): update repository URLs but keep SEN66Core context

10. **Rename workspace folder and GitHub repository** - Rename local workspace folder from `SEN66-Dosimetry` → `OSH-Monitor`, coordinate GitHub repository rename from `duluthmachineworks/SEN66-Dosimetry` to `duluthmachineworks/OSH-Monitor` to update all clone URLs and references

## Scope Boundaries

**Components that remain sensor-specific (NO rename):**
- `lib/SEN66Core/` library name and all internal code
- `lib/SEN66Core/examples/BasicSEN66/` example sketch names
- SEN66-specific function names like `readFullData()`, data structures like `SEN66RawData`

**Components that become platform-generic (YES rename):**
- Platform class: `SEN66Dosimetry` → `OSHMonitor`
- Platform files: `SEN66Dosimetry.h/.cpp` → `OSHMonitor.h/.cpp`
- CLI tool: `sen66_cli.py` → `osh_cli.py`
- Repository: `SEN66-Dosimetry` → `OSH-Monitor`
- All user-facing strings, banners, documentation titles

## CSV Header Branding Decision

Generated CSV comment headers should show:
```
# OSH-Monitor Air Quality Data Log
# Sensor: SEN66 (via SEN66Core v1.0.0)
```
This reflects platform architecture while maintaining sensor traceability.

## Further Considerations

**Legacy Support:** Force clean break - no backward compatibility shims. Users must update includes, class names, and CLI commands. Clear migration guide in CHANGELOG.md.

**Future Extensibility:** Rename sets foundation for adding BME680Core, SCD40Core, etc. Platform code (OSHMonitor) remains unchanged when adding sensors.


