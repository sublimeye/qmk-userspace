#pragma once

// Tap-hold settings, ported 1:1 from the 2021 firmware (see git history).
// TAPPING_TERM read from the chip; the rest determined by behavioural tests.
#define TAPPING_TERM 200
#define PERMISSIVE_HOLD
// 2021 TAPPING_FORCE_HOLD: tap-then-hold is a hold, never auto-repeat.
#define QUICK_TAP_TERM 0
// 2021 IGNORE_MOD_TAP_INTERRUPT is gone: its behaviour has been the default since 2023-05.
// RETRO_TAPPING stays off.
// v3: typing-speed taps stay taps (thumb layer keys exempt, see keymap.c).
#define FLOW_TAP_TERM 150
// v3: Shift/Ctrl home-row mods apply on press, so Shift-click works without waiting.
#define SPECULATIVE_HOLD

// VIA stores 4 layers by default; this keymap has 6 (6 x 48 keys x 2 B = 576 B of the 1 KB EEPROM).
#define DYNAMIC_KEYMAP_LAYER_COUNT 6
