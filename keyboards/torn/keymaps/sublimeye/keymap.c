// Torn keymap, recovered 2026-10-19 from the firmware on the keyboard itself.
// The original source was lost. The keymap table below was decoded from the chip's
// flash dump (backup/torn-2026-10-19-flash.hex, offset 0x6C) and compiles to the
// same bytes. Since then: Colemak layer removed, outer right column cleared,
// Home/PgUp/PgDn/End on Nav, one-key input-source switch, Num key fixed.
// v3: Caps Word, Redo on Nav, Flow Tap, Speculative Hold, outer keys for Superwhisper
// (F1), screenshot (F12) and emoji picker; ' is back on the right outer column.
// v3.3: Caps Word moved to the Caps Lock spot, left of A; top-left is free.
// Language key: tap switches input source, hold for Num. Superwhisper (F1) moved next to Space; outer right thumb free.
// v4: key usage stream for the Mac logger (keylog.c, logger/).
#include QMK_KEYBOARD_H
#include "keylog.h"

enum torn_layers { _QWERTY, _SYM, _NAV, _NUM, _MEDIA, _FN };

// Tap: Ctrl+Space (macOS input-source switch). Hold: _NUM. LT can only tap a plain
// key, so KC_SPC is a placeholder; process_record_user sends Ctrl+Space instead.
#define LANG_NUM LT(_NUM, KC_SPC)

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
[_QWERTY] = LAYOUT_split_3x6_4(
    _______,          KC_Q,             KC_W,             KC_E,             KC_R,             KC_T,               KC_Y,             KC_U,             KC_I,             KC_O,             KC_P,             KC_F12,
    CW_TOGG,          LCTL_T(KC_A),     LALT_T(KC_S),     LSFT_T(KC_D),     LGUI_T(KC_F),     KC_G,               KC_H,             RGUI_T(KC_J),     RSFT_T(KC_K),     RALT_T(KC_L),     RCTL_T(KC_SCLN),  KC_QUOT,
    _______,          KC_Z,             KC_X,             KC_C,             KC_V,             KC_B,               KC_N,             KC_M,             KC_COMM,          KC_DOT,           KC_SLSH,          C(G(KC_SPC)),
                                        MO(_MEDIA),       HYPR(XXXXXXX),    LT(_SYM, KC_ESC), LANG_NUM,           MO(_FN),          LT(_NAV, KC_SPC), KC_F1,            _______
),
[_SYM] = LAYOUT_split_3x6_4(
    _______,          LSFT(KC_1),       LSFT(KC_2),       LSFT(KC_3),       LSFT(KC_4),       LSFT(KC_5),         LSFT(KC_6),       LSFT(KC_7),       LSFT(KC_8),       LSFT(KC_EQL),     _______,          _______,
    _______,          LSFT(KC_GRV),     LSFT(KC_BSLS),    KC_LBRC,          KC_EQL,           _______,            LSFT(KC_LBRC),    LSFT(KC_9),       LSFT(KC_QUOT),    KC_MINS,          KC_QUOT,          _______,
    _______,          KC_GRV,           _______,          KC_RBRC,          LSFT(KC_MINS),    _______,            LSFT(KC_RBRC),    LSFT(KC_0),       LSFT(KC_COMM),    LSFT(KC_DOT),     KC_BSLS,          _______,
                                        _______,          _______,          _______,          _______,            _______,          _______,          _______,          _______
),
[_NAV] = LAYOUT_split_3x6_4(
    _______,          KC_HOME,          KC_PGUP,          KC_PGDN,          KC_END,           _______,            _______,          LALT(KC_LEFT),    KC_UP,            LALT(KC_RGHT),    SGUI(KC_GRV),     _______,
    _______,          KC_LCTL,          KC_LALT,          KC_LSFT,          KC_LGUI,          _______,            KC_TAB,           KC_LEFT,          KC_DOWN,          KC_RGHT,          KC_ENT,           _______,
    _______,          LGUI(KC_Z),       LGUI(KC_X),       LGUI(KC_C),       LGUI(KC_V),       SGUI(KC_Z),         LGUI(KC_BSPC),    KC_BSPC,          LALT(KC_BSPC),    KC_DEL,           LALT(KC_DEL),     LCTL(KC_K),
                                        _______,          _______,          _______,          _______,            _______,          _______,          _______,          _______
),
[_NUM] = LAYOUT_split_3x6_4(
    _______,          _______,          _______,          _______,          _______,          _______,            LSFT(KC_EQL),     KC_7,             KC_8,             KC_9,             LSFT(KC_8),       _______,
    _______,          KC_LCTL,          KC_LALT,          KC_LSFT,          KC_LGUI,          _______,            KC_MINS,          KC_4,             KC_5,             KC_6,             KC_0,             _______,
    _______,          _______,          _______,          _______,          _______,          _______,            KC_EQL,           KC_1,             KC_2,             KC_3,             LSFT(KC_5),       _______,
                                        _______,          _______,          _______,          _______,            _______,          _______,          _______,          _______
),
[_MEDIA] = LAYOUT_split_3x6_4(
    _______,          _______,          KC_MPRV,          KC_MNXT,          KC_MUTE,          _______,            _______,          _______,          _______,          _______,          _______,          _______,
    _______,          _______,          KC_VOLD,          KC_VOLU,          KC_MPLY,          _______,            _______,          _______,          _______,          _______,          _______,          _______,
    _______,          KC_BRID,          KC_BRIU,          KC_SCRL,          KC_PAUS,          _______,            _______,          _______,          _______,          _______,          _______,          _______,
                                        _______,          _______,          _______,          _______,            _______,          _______,          _______,          _______
),
[_FN] = LAYOUT_split_3x6_4(
    _______,          _______,          _______,          _______,          _______,          _______,            _______,          KC_F1,            KC_F2,            KC_F3,            KC_F4,            _______,
    _______,          KC_LCTL,          KC_LALT,          KC_LSFT,          KC_LGUI,          _______,            _______,          KC_F5,            KC_F6,            KC_F7,            KC_F8,            _______,
    _______,          _______,          _______,          _______,          _______,          _______,            _______,          KC_F9,            KC_F10,           KC_F11,           KC_F12,           _______,
                                        _______,          _______,          _______,          _______,            _______,          _______,          _______,          _______
),
};
// clang-format on


// Flow Tap: a tap-hold key pressed within FLOW_TAP_TERM of the previous key is a tap,
// so home-row mods don't fire while typing fast. Not for the thumb layer keys, so
// Space can still be held for Nav straight after a word, and not for Shift, so a
// capital right after a space or mid-word ("and I", camelCase) isn't typed as "di".
uint16_t get_flow_tap_term(uint16_t keycode, keyrecord_t *record, uint16_t prev_keycode) {
    switch (keycode) {
        case LT(_NAV, KC_SPC):
        case LT(_SYM, KC_ESC):
        case LANG_NUM:
        case LSFT_T(KC_D):
        case RSFT_T(KC_K):
            return 0;
    }
    return is_flow_tap_key(keycode) && is_flow_tap_key(prev_keycode) ? FLOW_TAP_TERM : 0;
}

// Cmd+Tab is Cmd + Space(Nav)+H. With Space still held, Q is Home, so quitting an app
// from the switcher would send Cmd+Home. While Cmd is held, Nav's Home types Q instead.
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    static bool home_is_q = false;
    keylog_record(keycode, record);
    if (keycode == LANG_NUM && record->tap.count) {
        if (record->event.pressed) {
            tap_code16(C(KC_SPC));
        }
        return false;
    }
    if (keycode == KC_HOME) {
        if (record->event.pressed) {
            home_is_q = get_mods() & MOD_MASK_GUI;
        }
        if (home_is_q) {
            if (record->event.pressed) {
                register_code(KC_Q);
            } else {
                unregister_code(KC_Q);
            }
            return false;
        }
    }
    return true;
}

// Hold both thumb layer keys (Esc -> _SYM, Space -> _NAV) to get _NUM.
// Only drop _NUM if this function turned it on, so holding the language key
// also works (update_tri_layer_state() would cancel it at once).
layer_state_t layer_state_set_user(layer_state_t state) {
    static bool tri_num = false;
    const layer_state_t both = ((layer_state_t)1 << _SYM) | ((layer_state_t)1 << _NAV);
    if ((state & both) == both) {
        tri_num = true;
        return state | ((layer_state_t)1 << _NUM);
    }
    if (tri_num) {
        tri_num = false;
        state &= ~((layer_state_t)1 << _NUM);
    }
    return state;
}
