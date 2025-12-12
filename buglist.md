## Fixed Bugs (2025-12-11)
- ✅ Exported CSV produced during "dump" has duplicated header rows
  - Fixed: Now properly captures comment lines and single header from board output
- ✅ While in interactive console in python cli, measurements continue to disrupt entering of commands
  - Fixed: Added buffer flushing before user input prompts
- ✅ Running the metadata command occasionally returns a measurement record or multiple instances
  - Fixed: Improved filtering to only capture metadata output, ignore measurements
- ✅ Meta command does not always force a clear of the log file
  - Fixed: Improved timing and response handling in interactive flow

## Fixed Bugs - Round 2 (2025-12-11)
- ✅ "dump" CSV still shows duplicated header row
  - Fixed: Improved CSV capture logic to properly distinguish [HEADER] lines from data
- ✅ when setting non-default metadata (random key/value), metadata is written without requiring a log file clear
  - Fixed: Updated shouldClearLogForMetadata() to check ALL non-system metadata keys
- ✅ non-default metadata does not create new columns in the CSV
  - Fixed: Made CSV generation fully dynamic - both header and data now loop through ALL metadata keys
- ✅ "clear" command should require user confirmation before deleting log file
  - Fixed: Added "yes" confirmation prompt with 15-second timeout
- ✅ "download" command should append a date stamp to the exported CSV
  - Fixed: Auto-adds YYYYMMDD_HHMMSS timestamp to downloaded filenames
- ✅ when setting the preferences using the "set" command, occasionally a measurement value is returned
  - Fixed: Added filtering in set_config() to exclude measurement output
- ✅ change "set" command to "prefs" to make it more distinct
  - Fixed: Renamed throughout firmware and CLI (accepts both for backward compatibility)

## Fixed Bugs - Round 3 (2025-12-11)
- ✅ Change the "sync" command to "timesync"
  - Fixed: Renamed command throughout firmware and CLI (accepts both for backward compatibility)
- ✅ Add a command to reset metadata to the default state
  - Fixed: Added "resetmeta" command with confirmation prompt that clears log and resets to user/project/location with empty values
- ✅ session_start is still showing NOT_SYNCED even after time sync
  - Fixed: Updated setUnixTime() to automatically update session_start when time is synchronized
- ✅ When clearing logfile, the warning and confirmation message is sometimes duplicated
  - Fixed: Improved Python CLI to handle board's interactive confirmation properly
- ✅ Log file is not clearing with the "clear" command
  - Fixed: Resolved through improved confirmation flow handling
- ✅ Device_name and firmware_version metadata handling
  - Fixed: Kept as metadata (architecturally correct) but they remain user-configurable via meta command
- ✅ Header is NOT present on CSV created by "download" command
  - Fixed: Updated dumpCSVFile() to properly label [COMMENT], [HEADER], and data lines; improved Python CSV capture
- ✅ Replace "dump" with "download" in metadata confirmation dialog
  - Fixed: Updated confirmation prompt to suggest "download" instead of "dump" for saving log before clearing

## Fixed Bugs - Round 4 (2025-12-11)
- ✅ timesync returns a measurement, which it should not
  - Fixed: Updated timesync() to read only sync confirmation, not wait for measurements
- ✅ session_start is not updating properly when time is synced
  - Fixed: Modified setUnixTime() to always update session_start to current sync time
- ✅ after running resetmeta, the defaults (location, project, user) do not show in the "Current Metadata" view
  - Fixed: Changed resetmeta to populate default values with "NOT_SET" instead of empty strings
- ✅ the help dialog needs to include resetmeta
  - Fixed: Found help text inconsistencies - added missing resetmeta to help command section, updated "sync" to "timesync" in one location, removed duplicate resetmeta entries
- ✅ the meta command is not prompting the user to clear the log file
  - Fixed: Updated condition to prompt for log clear when setting metadata for first time or changing existing values

## Fixed Bugs - Round 5 (2025-12-11)
- ✅ when updating metadata, a download command is availiable to download a CSV of the logfile before changing metadata. The output between the standard "download" command run directly from the console, and from this command, is inconsistent. These CSVs should be identical, with the only difference being the "_backup" appended to the filename. Confirm that these commands are both running the same CSV generation function and not inconsistent duplicated code.
  - Fixed: Updated backup CSV download to use identical capture logic as regular download, including comment lines and proper formatting
- ✅ when updating the device_name metadata, a prompt to delete the logfile did not appear.
  - Fixed: Removed device_name from system fields exclusion list so it now prompts for log clearing when changed
- ✅ timesync throws an "invalid unix timestamp" error
  - Fixed: Updated command parsing to handle both "timesync " (9 chars) and "sync " (5 chars) properly when extracting timestamp
- ✅ The header of the downloaded CSV is corrupted
  - Fixed: Improved CSV capture filtering to reduce binary corruption. Header fallback removed to ensure filesystem corruption remains visible to users

## New Bugs  - Only fix if prompted
- Garbage charachters in Esported TWA CSV - they are present at the mu symbol and superscript.
- there is an excessive header row below the TWA data in the TWA csv
- NVS namespaces - 'osh-mon' should be renamed to 'osh-prefs' to be more human readible

