// Torn keymap, recovered 2026-10-19 from the firmware on the keyboard itself.
// The original source was lost. The keymap table below was decoded from the chip's
// flash dump (backup/torn-2026-10-19-flash.hex, offset 0x6C) and compiles to the
// same bytes. Layer 0 is the boot default; the right-most thumb key toggles
// QWERTY <-> Colemak-DH via DF().
#include QMK_KEYBOARD_H

enum torn_layers { _QWERTY, _COLEMAK, _SYM, _NAV, _NUM, _MEDIA, _FN };

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
[_QWERTY] = LAYOUT_split_3x6_4(
    KC_CAPS,          KC_Q,             KC_W,             KC_E,             KC_R,             KC_T,               KC_Y,             KC_U,             KC_I,             KC_O,             KC_P,             KC_LBRC,
    _______,          LCTL_T(KC_A),     LALT_T(KC_S),     LSFT_T(KC_D),     LGUI_T(KC_F),     KC_G,               KC_H,             RGUI_T(KC_J),     RSFT_T(KC_K),     RALT_T(KC_L),     RCTL_T(KC_SCLN),  KC_QUOT,
    _______,          KC_Z,             KC_X,             KC_C,             KC_V,             KC_B,               KC_N,             KC_M,             KC_COMM,          KC_DOT,           KC_SLSH,          _______,
                                        MO(_MEDIA),       HYPR(XXXXXXX),    LT(_SYM, KC_ESC), _______,            MO(_FN),          LT(_NAV, KC_SPC), MO(_NUM),         DF(_COLEMAK)
),
[_COLEMAK] = LAYOUT_split_3x6_4(
    KC_CAPS,          KC_Q,             KC_W,             KC_F,             KC_P,             KC_B,               KC_J,             KC_L,             KC_U,             KC_Y,             KC_SCLN,          KC_LBRC,
    _______,          LCTL_T(KC_A),     LALT_T(KC_R),     LSFT_T(KC_S),     LGUI_T(KC_T),     KC_G,               KC_M,             RGUI_T(KC_N),     RSFT_T(KC_E),     RALT_T(KC_I),     RCTL_T(KC_O),     KC_QUOT,
    _______,          KC_Z,             KC_X,             KC_C,             KC_D,             KC_V,               KC_K,             KC_H,             KC_COMM,          KC_DOT,           KC_SLSH,          _______,
                                        MO(_MEDIA),       HYPR(XXXXXXX),    LT(_SYM, KC_ESC), _______,            MO(_FN),          LT(_NAV, KC_SPC), MO(_NUM),         DF(_QWERTY)
),
[_SYM] = LAYOUT_split_3x6_4(
    _______,          LSFT(KC_1),       LSFT(KC_2),       LSFT(KC_3),       LSFT(KC_4),       LSFT(KC_5),         LSFT(KC_6),       LSFT(KC_7),       LSFT(KC_8),       LSFT(KC_EQL),     _______,          _______,
    _______,          LSFT(KC_GRV),     LSFT(KC_BSLS),    KC_LBRC,          KC_EQL,           _______,            LSFT(KC_LBRC),    LSFT(KC_9),       LSFT(KC_QUOT),    KC_MINS,          KC_QUOT,          _______,
    _______,          KC_GRV,           _______,          KC_RBRC,          LSFT(KC_MINS),    _______,            LSFT(KC_RBRC),    LSFT(KC_0),       LSFT(KC_COMM),    LSFT(KC_DOT),     KC_BSLS,          _______,
                                        _______,          _______,          _______,          _______,            _______,          _______,          _______,          _______
),
[_NAV] = LAYOUT_split_3x6_4(
    _______,          _______,          _______,          _______,          _______,          _______,            _______,          LALT(KC_LEFT),    KC_UP,            LALT(KC_RGHT),    SGUI(KC_GRV),     _______,
    _______,          _______,          KC_LALT,          KC_LSFT,          KC_LGUI,          _______,            KC_TAB,           KC_LEFT,          KC_DOWN,          KC_RGHT,          KC_ENT,           _______,
    _______,          LGUI(KC_Z),       LGUI(KC_X),       LGUI(KC_C),       LGUI(KC_V),       _______,            LGUI(KC_BSPC),    KC_BSPC,          LALT(KC_BSPC),    KC_DEL,           LALT(KC_DEL),     LCTL(KC_K),
                                        _______,          _______,          _______,          _______,            _______,          _______,          _______,          _______
),
[_NUM] = LAYOUT_split_3x6_4(
    _______,          _______,          _______,          _______,          _______,          _______,            LSFT(KC_EQL),     KC_7,             KC_8,             KC_9,             LSFT(KC_8),       _______,
    _______,          _______,          KC_LALT,          KC_LSFT,          KC_LGUI,          _______,            KC_MINS,          KC_4,             KC_5,             KC_6,             KC_0,             _______,
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
    _______,          _______,          KC_LALT,          KC_LSFT,          KC_LGUI,          _______,            _______,          KC_F5,            KC_F6,            KC_F7,            KC_F8,            _______,
    _______,          _______,          _______,          _______,          _______,          _______,            _______,          KC_F9,            KC_F10,           KC_F11,           KC_F12,           _______,
                                        _______,          _______,          _______,          _______,            _______,          _______,          _______,          _______
),
};
// clang-format on


// Hold both thumb layer keys (Esc -> _SYM, Space -> _NAV) to get _NUM.
// Recovered from the chip: layer_state_set at dump offset 0x1D80 sets bit 4
// when bits 2 and 3 are both on, and clears it otherwise.
layer_state_t layer_state_set_user(layer_state_t state) {
    return update_tri_layer_state(state, _SYM, _NAV, _NUM);
}
