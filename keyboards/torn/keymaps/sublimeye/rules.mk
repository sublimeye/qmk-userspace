# Board defaults (OLED, bongocat, WPM, encoder) are kept: they are present in the chip dump.
# EXTRAKEY: the keyboard exposes a System Control HID interface, so this was on.
EXTRAKEY_ENABLE = yes

# Edit keys live with the VIA app (usevia.app). VIA keeps the keymap in EEPROM;
# keymap.c is only the default it starts from (and resets to).
VIA_ENABLE = yes
