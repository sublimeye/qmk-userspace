#!/bin/bash
# READ-ONLY. -n = avrdude never writes. Also tracks whether RESET alone works.
prev=$(ioreg -c IOHIDDevice -r -l 2>/dev/null | grep -c '"Product" = "Torn"')
echo "start: torn HID nodes = $prev"
for i in $(seq 1 200); do
  now=$(ioreg -c IOHIDDevice -r -l 2>/dev/null | grep -c '"Product" = "Torn"')
  if [ "$now" != "$prev" ]; then
    echo "[poll $i] Torn HID nodes changed: $prev -> $now  (board re-enumerated: RESET is working)"
    prev=$now
  fi
  if ioreg -p IOUSB -l 2>/dev/null | grep -qi usbasp; then
    echo "[poll $i] *** USBasp VISIBLE IN USB TREE ***"
  fi
  if avrdude -c usbasp -p m328p -n 2>&1 | grep -qi "signature"; then
    echo "=== BOOTLOADER DETECTED on poll $i ==="
    avrdude -c usbasp -p m328p -n 2>&1 | grep -iE "signature|processor|Error|warning" | head -8
    exit 0
  fi
  sleep 3
done
echo "=== NOT DETECTED after ~10 min ==="
exit 1
