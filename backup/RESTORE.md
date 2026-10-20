# Panic button — restore the Torn to its 2026-10-19 firmware

This is the rollback path. If a new build feels wrong, bricks a layer, or won't type,
this puts the keyboard back to exactly how it was on 2026-10-19.

## 1. Enter the bootloader

Hold **BOOT**, tap **RESET**, release **BOOT**.

The keyboard stops acting as a keyboard while in bootloader mode — use the laptop
keyboard. Plug the Torn **directly into the Mac**, not through a hub or KVM: it is a
low-speed V-USB device and flashing through a hub is unreliable.

Confirm it's there (read-only, `-n` means no writes):

```bash
avrdude -v -c usbasp -p m328p -n
```

You should see a device signature. If not, you are not in the bootloader — do not continue.

## 2. Write the firmware back

```bash
cd ~/projects/qmk-userspace
avrdude -p m328p -c usbasp -U flash:w:backup/torn-2026-10-19-flash.hex:i
```

Unplug and replug. You should be back on the 2021 keymap.

## 3. EEPROM (only if something is still off)

The EEPROM holds persistent state such as the default layer. Restoring flash alone is
usually enough. Only if behaviour is still wrong:

```bash
avrdude -p m328p -c usbasp -U eeprom:w:backup/torn-2026-10-19-eeprom.hex:i
```

Alternatively, clear it and let QMK repopulate defaults rather than restoring it.

## What's in this directory

| File | What it is |
|---|---|
| `torn-2026-10-19-flash.hex` | **Authoritative.** Byte-exact dump read off the chip — literally the firmware that was running. |
| `torn-2026-10-19-eeprom.hex` | EEPROM dump from the same session. |
| `decoded-keymap-from-chip.txt` | Human-readable decode of the keymap table in the flash dump (7 layers). |

The dump is the only copy of the original firmware: its source was lost (the
`sublimeye/qmk_firmware` fork's `kinesish` keymap turned out to be someone else's,
not what was on the chip). A 2021-toolchain rebuild of the recovered keymap is not
byte-identical to the dump — the original was compiled with a different avr-gcc — so
there is no secondary restore image. **Restore from the dump.**

## Hardware facts worth not re-deriving

- MCU `atmega328p`, bootloader USBaspLoader (`rtitmuss/USBaspLoader`), V-USB (no native USB).
- avrdude programmer is `usbasp`; the part is `m328p`.
- Identify the board when running normally:
  `ioreg -c IOHIDDevice -r -l | grep -i -A5 torn` → VID `0x7274` / PID `0x0001` / `richard.titmuss`.
