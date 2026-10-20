#pragma once

// Tap-hold settings, ported 1:1 from the 2021 firmware (see git history).
// TAPPING_TERM read from the chip; the rest determined by behavioural tests.
#define TAPPING_TERM 200
#define PERMISSIVE_HOLD
// 2021 TAPPING_FORCE_HOLD: tap-then-hold is a hold, never auto-repeat.
#define QUICK_TAP_TERM 0
// 2021 IGNORE_MOD_TAP_INTERRUPT is gone: its behaviour has been the default since 2023-05.
// RETRO_TAPPING stays off.

// VIA stores 4 layers by default; this keymap has 7 (7 x 48 keys x 2 B = 672 B of the 1 KB EEPROM).
#define DYNAMIC_KEYMAP_LAYER_COUNT 7
