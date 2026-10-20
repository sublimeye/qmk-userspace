#!/bin/sh
# Stop and remove the launchd agent. Recorded data in ~/.local/share/torn-logger is kept.
LABEL=com.sublimeye.torn-logger
launchctl bootout "gui/$(id -u)/$LABEL" 2>/dev/null || true
rm -f "$HOME/Library/LaunchAgents/$LABEL.plist"
# keep ~/.local/bin/torn-logger: `torn-logger report` still works on the saved data.
echo "stopped and removed; data + the torn-logger command are kept"
