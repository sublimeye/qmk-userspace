#pragma once

// Queue a key event for the Mac logger (see keylog.c). Call first in process_record_user.
void keylog_record(uint16_t keycode, keyrecord_t *record);
