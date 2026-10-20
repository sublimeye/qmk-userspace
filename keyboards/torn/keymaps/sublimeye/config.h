#pragma once

// Tap-hold settings, ported 1:1 from the 2021 firmware (see git history).
// TAPPING_TERM read from the chip; the rest determined by behavioural tests.
#define TAPPING_TERM 200
#define PERMISSIVE_HOLD
// 2021 TAPPING_FORCE_HOLD: tap-then-hold is a hold, never auto-repeat.
#define QUICK_TAP_TERM 0
// 2021 IGNORE_MOD_TAP_INTERRUPT is gone: its behaviour has been the default since 2023-05.
// RETRO_TAPPING stays off.
