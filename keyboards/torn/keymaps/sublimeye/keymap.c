// Torn keymap, recovered 2026-10-19 from the firmware on the keyboard itself.
// The original source was lost. The keymap table below was decoded from the chip's
// flash dump (backup/torn-2026-10-19-flash.hex, offset 0x6C) and compiles to the
// same bytes. Since then: Colemak layer removed, outer right column cleared,
// Home/PgUp/PgDn/End on Nav, one-key input-source switch, Num key fixed.
#include QMK_KEYBOARD_H

enum torn_layers { _QWERTY, _SYM, _NAV, _NUM, _MEDIA, _FN };

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
[_QWERTY] = LAYOUT_split_3x6_4(
    KC_CAPS,          KC_Q,             KC_W,             KC_E,             KC_R,             KC_T,               KC_Y,             KC_U,             KC_I,             KC_O,             KC_P,             _______,
    _______,          LCTL_T(KC_A),     LALT_T(KC_S),     LSFT_T(KC_D),     LGUI_T(KC_F),     KC_G,               KC_H,             RGUI_T(KC_J),     RSFT_T(KC_K),     RALT_T(KC_L),     RCTL_T(KC_SCLN),  _______,
    _______,          KC_Z,             KC_X,             KC_C,             KC_V,             KC_B,               KC_N,             KC_M,             KC_COMM,          KC_DOT,           KC_SLSH,          _______,
                                        MO(_MEDIA),       HYPR(XXXXXXX),    LT(_SYM, KC_ESC), LCTL(KC_SPC),       MO(_FN),          LT(_NAV, KC_SPC), MO(_NUM),         _______
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
    _______,          LGUI(KC_Z),       LGUI(KC_X),       LGUI(KC_C),       LGUI(KC_V),       _______,            LGUI(KC_BSPC),    KC_BSPC,          LALT(KC_BSPC),    KC_DEL,           LALT(KC_DEL),     LCTL(KC_K),
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


// Hold both thumb layer keys (Esc -> _SYM, Space -> _NAV) to get _NUM.
// Only drop _NUM if this function turned it on, so the dedicated MO(_NUM)
// thumb key also works (update_tri_layer_state() would cancel it at once).
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
