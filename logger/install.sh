#!/bin/sh
# Install the Torn key logger as a login item (launchd agent) and start it.
# Data and the Python venv live in ~/.local/share/torn-logger; this repo holds the script.
set -e
DATA="$HOME/.local/share/torn-logger"
LABEL=com.sublimeye.torn-logger
PLIST="$HOME/Library/LaunchAgents/$LABEL.plist"
SCRIPT="$(cd "$(dirname "$0")" && pwd)/torn_logger.py"

mkdir -p "$DATA"
if [ ! -x "$DATA/venv/bin/python" ]; then
  python3 -m venv "$DATA/venv"
fi
"$DATA/venv/bin/python" -m pip install -q --disable-pip-version-check hidapi 2>/dev/null ||
  uv pip install -q --python "$DATA/venv/bin/python" hidapi

cat > "$PLIST" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key><string>$LABEL</string>
  <key>ProgramArguments</key>
  <array>
    <string>$DATA/venv/bin/python</string>
    <string>-I</string>
    <string>$SCRIPT</string>
    <string>run</string>
  </array>
  <key>RunAtLoad</key><true/>
  <key>KeepAlive</key><true/>
  <key>ThrottleInterval</key><integer>10</integer>
  <key>ProcessType</key><string>Adaptive</string>
  <key>StandardOutPath</key><string>$DATA/daemon.log</string>
  <key>StandardErrorPath</key><string>$DATA/daemon.log</string>
</dict>
</plist>
PLIST

# Short command: torn-logger live | status | report | watch
mkdir -p "$HOME/.local/bin"
printf '#!/bin/sh\nexec "%s" -I "%s" "$@"\n' "$DATA/venv/bin/python" "$SCRIPT" > "$HOME/.local/bin/torn-logger"
chmod +x "$HOME/.local/bin/torn-logger"

launchctl bootout "gui/$(id -u)/$LABEL" 2>/dev/null || true
# bootout returns before the old agent is gone; bootstrap fails with error 5 until it is.
for _ in 1 2 3 4 5 6 7 8 9 10; do
  launchctl print "gui/$(id -u)/$LABEL" >/dev/null 2>&1 || break
  sleep 1
done
launchctl bootstrap "gui/$(id -u)" "$PLIST"
echo "installed; check with: torn-logger status   (or torn-logger live)"
