#pragma once

// Tap-hold settings. TAPPING_TERM read from the chip; the three flags below were
// determined by behavioural tests on the keyboard (2026-10-19). RETRO_TAPPING is off.
#define TAPPING_TERM 200
#define IGNORE_MOD_TAP_INTERRUPT
#define PERMISSIVE_HOLD
#define TAPPING_FORCE_HOLD
