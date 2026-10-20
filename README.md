# qmk-userspace

QMK [External Userspace](https://docs.qmk.fm/newbs_external_userspace) for my
[Torn](https://github.com/qmk/qmk_firmware/tree/master/keyboards/torn) split keyboard
(ATmega328P, USBaspLoader bootloader). Keymap: `keyboards/torn/keymaps/sublimeye`.

## Build

Needs Docker and a checkout of `qmk/qmk_firmware` (`$QMK_HOME`):

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
| QWERTY | default | Home-row mods: Ctrl/Alt/Shift/Cmd on A S D F and J K L ; |
| SYM | hold Esc (left thumb) | Symbols |
| NAV | hold Space (right thumb) | Arrows, word jumps, Home/PgUp/PgDn/End on Q W E R, deletes on N M , . / |
| NUM | hold Num key, or Esc + Space together | Numpad on the right hand |
| MEDIA | hold left outer thumb | Volume, play, prev/next, brightness |
| FN | hold right inner thumb | F1–F12 |

Thumb row, outer to inner, then inner to outer:

```
LEFT:   MEDIA  Hyper  Esc/SYM  Lang (Ctrl+Space)  |  FN  Space/NAV  NUM  (empty)  :RIGHT
```

## Mac setup the keymap relies on

- **Language key** sends Ctrl+Space: enable System Settings → Keyboard → Keyboard
  Shortcuts → Input Sources → "Select the previous input source".
- **Nav deletes in iTerm** (Settings → Keys → Key Bindings):
  ⌘⌫ → Send Hex Codes `0x15`; ⌥⌦ → Send Escape Sequence `d`.
- zsh: `bindkey '^[[3;3~' kill-word` (Option+Fwd-Delete deletes the next word).

## Changelog

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
