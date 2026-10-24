# qmk-userspace

QMK [External Userspace](https://docs.qmk.fm/newbs_external_userspace) for my
[Torn](https://github.com/qmk/qmk_firmware/tree/master/keyboards/torn) split keyboard
(ATmega328P, USBaspLoader bootloader). Keymap: `keyboards/torn/keymaps/sublimeye`.

## Build

Needs Docker and a checkout of `qmk/qmk_firmware` (`$QMK_HOME`; built against commit `180f394b`):

```sh
docker run --rm -v "$QMK_HOME":/qmk_firmware -v "$PWD":/userspace -w /userspace \
  ghcr.io/qmk/qmk_cli:latest sh -c \
  'qmk config user.qmk_home=/qmk_firmware user.overlay_dir=/userspace >/dev/null && qmk compile -kb torn -km sublimeye'
```

Output: `torn_sublimeye.hex` (gitignored).

## Flash

1. Plug the board straight into the Mac (no hub).
2. Hold **BOOT**, tap **RESET**, release **BOOT**. `backup/detect-bootloader.sh` (read-only) confirms it is visible.
3. `avrdude -p m328p -c usbasp -U flash:w:torn_sublimeye.hex:i`
4. If `keymap.c` changed, also blank the EEPROM (still in the bootloader):
   `head -c 1024 /dev/zero | LC_ALL=C tr '\0' '\377' > blank.bin && avrdude -p m328p -c usbasp -D -U eeprom:w:blank.bin:r`
5. Tap **RESET** to start the new firmware.

VIA keeps its keymap in EEPROM and only reloads it from `keymap.c` when the build
**date** differs from the last one (QMK compares yy-mm-dd, not the time). Two builds
on the same day keep the old keymap, hence step 4. Blanking the EEPROM also drops any
VIA edits, so permanent changes go in `keymap.c`. To go back to the original 2021
firmware, see [`backup/RESTORE.md`](backup/RESTORE.md).

## Layers

| Layer | How | What |
|---|---|---|
| QWERTY | default | Home-row mods: Ctrl/Alt/Shift/Cmd on A S D F and J K L ;. Left of A (the Caps Lock spot): Caps Word; above it, F12 (screenshot). Right outer column: [ (х on the Russian layout), ', emoji picker |
| SYM | hold Esc (left thumb) | Symbols |
| NAV | hold Space (right thumb) | Arrows, word jumps, Home/PgUp/PgDn/End on Q W E R, deletes on N M , . / |
| NUM | hold the language key (left inner thumb), or Esc + Space together | Numpad on the right hand |
| MEDIA | hold left outer thumb | Volume, play, prev/next, brightness |
| FN | hold right inner thumb | F1–F12 |

Thumb row, outer to inner, then inner to outer:

```
LEFT:   MEDIA  Hyper  Esc/SYM  Lang/NUM (tap Ctrl+Space)  |  FN  Space/NAV  F1 (Superwhisper)  (free)  :RIGHT
```

## Key usage logger (v4)

The firmware can stream every key event (which physical key, which layer, tap or hold,
press/release time) to the Mac over VIA's raw HID channel; `logger/torn_logger.py`
counts them. Only aggregates are saved: per-key counts, tap/hold lengths, press gaps,
and **same-finger** key pairs (which the finger analysis needs). The full ordered
sequence of keystrokes is never stored, so typed text and passwords cannot be
reconstructed from the files.
The stream is off at power-up; the script turns it on and repeats that every 10 s;
the firmware stops sending 2 minutes after the last repeat.

Caveat: while the keyboard is plugged in, any program running as you can send the same
raw HID command and read the live key stream (it maps to letters via the VIA keymap).
That is a property of the raw HID channel, not just this logger; macOS asks for no
extra permission because the interface is vendor-defined. The exposure is only while
something is actively streaming. `logger/uninstall.sh` stops the daemon; unplugging the
board ends it entirely.

```sh
logger/install.sh        # venv + launchd agent (starts at login) + `torn-logger` command
torn-logger live         # refreshing view: daemon/keyboard/firmware health, today's top keys
torn-logger status       # one-shot health check (exit code 0 = all good)
torn-logger report       # summary of all days; --days 7 for the last week
torn-logger watch        # debug: print raw events (stop the daemon first, see below)
logger/uninstall.sh      # stop and remove the agent; data is kept
```

Data: `~/.local/share/torn-logger/days/YYYY-MM-DD.json`, daemon log `daemon.log` there.
Stop/start the daemon: `launchctl bootout gui/$(id -u)/com.sublimeye.torn-logger` /
`logger/install.sh`. VIA (usevia.app) can't connect while the daemon runs: it has the
raw HID channel open.

## Mac setup the keymap relies on

- **Language key** sends Ctrl+Space: enable System Settings → Keyboard → Keyboard
  Shortcuts → Input Sources → "Select next source in Input menu" (Ctrl+Space).
- **Nav deletes in iTerm** (Settings → Keys → Key Bindings):
  ⌘⌫ → Send Hex Codes `0x15`; ⌥⌦ → Send Escape Sequence `d`.
- zsh: `bindkey '^[[3;3~' kill-word` (Option+Fwd-Delete deletes the next word).

## Changelog

### v4.1 (2026-10-23)

- `[` is back on the top-right key: on the Russian layout it types х.
- Screenshot (F12) moved from there to the free top-left key, above Caps Word.
- Flashed 2026-10-23.

### v4 (2026-10-20)

- Key usage stream for the Mac logger (`keylog.c`, `logger/`). One 8-byte packet per
  key event, sent from the main loop only when the USB endpoint is free; typing never
  waits. Off unless the logger asks for it.
- Packets are padded up to a 32-byte HID report (macOS won't deliver a short one).
- Flashed and verified streaming 2026-10-20.

### v3.3 (2026-10-20)

- Caps Word moved from top-left to the Caps Lock spot (left of A). Top-left is free.
- Language key (left inner thumb): tap still switches input source, hold is NUM. Numbers
  now take one thumb on the opposite hand. Esc + Space still works.
- Superwhisper (F1) moved from the outer right thumb to the key next to Space (old NUM
  key, switch now replaced). Outer right thumb is free.

### v3.2 (2026-10-20)

- NAV: with Cmd held, the Home key (Q) types Q, so Cmd+Tab → Q quits the selected app
  while Space is still held.

### v3.1 (2026-10-19)

- Flow Tap no longer applies to the Shift home-row keys (D, K): with it, a capital typed
  within 150 ms of the previous key came out lowercase ("and I" → "and di").

### v3 (2026-10-19)

- Caps Word replaces Caps Lock (top-left).
- NAV: Redo (Cmd+Shift+Z) on B, next to undo/cut/copy/paste.
- Flow Tap (`FLOW_TAP_TERM 150`): fast typing never triggers home-row mods. The Space
  and Esc thumb keys are exempt so they can be held right after a word.
- Speculative Hold: Shift/Ctrl home-row mods apply on press (Shift-click works).
- Right outer column: F12 (macOS area screenshot to clipboard), `'` back, Ctrl+Cmd+Space
  (emoji picker). Right outer thumb: F1 (Superwhisper). Same keys as the laptop.

### v2 (2026-10-19)

- Removed the Colemak-DH layer and its toggle (right outer thumb is now empty).
- Cleared `[` and `'` from the right outer column (both are on SYM).
- NAV: Q = Home, W = PgUp, E = PgDn, R = End.
- NAV, NUM, FN: A is a plain Ctrl, like the Alt/Shift/Cmd next to it (it used to
  fall through to the base layer's Ctrl-on-hold, with its 200 ms delay).
- Left inner thumb (previously unused): Ctrl+Space, macOS input-source switch.
- Fixed the dedicated NUM thumb key, which never worked: the tri-layer code switched
  NUM off unless Esc and Space were both held.
- VIA layer count 7 → 6.

### v1 (2026-10-19)

The 2021 firmware, rebuilt. The original source was lost, so the keymap was decoded
from the chip's flash and ported to current QMK with the same behaviour: same keys
on all layers, Esc + Space tri-layer to NUM, `TAPPING_TERM 200`, `PERMISSIVE_HOLD`,
`QUICK_TAP_TERM 0`. Added VIA.
