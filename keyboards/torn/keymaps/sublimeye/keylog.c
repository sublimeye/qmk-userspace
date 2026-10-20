// Key usage stream for logger/torn_logger.py (v4).
//
// Every key event this firmware processes (after tap/hold is decided) is sent to the
// Mac as one 8-byte packet on the VIA raw HID endpoint. Nothing is stored on the
// keyboard. Packets are sent from the main loop only when the endpoint is free, so
// typing never waits on USB; if the queue fills, events are dropped and counted.
//
// Streaming is off at power-up. The Mac script turns it on with the raw HID command
// [KEYLOG_CMD, 1] and repeats it as a heartbeat; without one for KEYLOG_TIMEOUT the
// stream stops, so keys are never sent when nothing is listening.
//
// Packet: tag, key, layer, time lo, time hi, seq, drops, checksum
//   key:   bits 0-5 row * MATRIX_COLS + col, bit 6 pressed, bit 7 resolved as tap
//   layer: bits 0-5 layer_state, bit 6 tap-hold key
//   time:  record->event.time, the press/release time in ms (wraps every 65 s)
//   seq:   +1 per packet, so the Mac can count lost ones; drops: queue overflows
// macOS delivers a HID input report only once it fills to its 32-byte max (4 packets)
// or ends on a short packet; empirically the short packet is not honoured here, so we
// pad the current report with filler packets up to the 4-packet boundary. That flushes
// the trailing 1-3 events of a burst at once. A lost packet only regroups packets into
// reports; every packet is self-describing (seq + checksum), so the Mac never misreads.
#include QMK_KEYBOARD_H
#include <usbdrv/usbdrv.h>
#include "keylog.h"

#define KEYLOG_CMD 0xE0
#define KEYLOG_VERSION 1
#define KEYLOG_QUEUE 16
#define KEYLOG_TIMEOUT 120000

_Static_assert(DYNAMIC_KEYMAP_LAYER_COUNT <= 6, "layer bits 6-7 are reused as flags in the packet");

enum { TAG_EVENT = 0xA5, TAG_PAD = 0xA6, TAG_HELLO = 0xA7 };

typedef struct {
    uint8_t  key;
    uint8_t  layer;
    uint16_t time;
} keylog_event_t;

static keylog_event_t queue[KEYLOG_QUEUE];
static uint8_t        head, len;
static uint8_t        seq, drops;
static uint8_t        in_report; // packets put into the current 32-byte host report (0..3)
static bool           on, hello;
static uint32_t       heartbeat;

void keylog_record(uint16_t keycode, keyrecord_t *record) {
    keypos_t k = record->event.key;
    if (!on || record->event.type != KEY_EVENT || k.row >= MATRIX_ROWS || k.col >= MATRIX_COLS) {
        return;
    }
    if (len == KEYLOG_QUEUE) {
        drops++;
        return;
    }
    bool            taphold = IS_QK_MOD_TAP(keycode) || IS_QK_LAYER_TAP(keycode);
    keylog_event_t *e       = &queue[(head + len++) % KEYLOG_QUEUE];
    e->key                  = (k.row * MATRIX_COLS + k.col) | (record->event.pressed ? 0x40 : 0) | (taphold && record->tap.count ? 0x80 : 0);
    e->layer                = (layer_state & 0x3F) | (taphold ? 0x40 : 0);
    e->time                 = record->event.time;
}

void housekeeping_task_user(void) {
    if (on && timer_elapsed32(heartbeat) > KEYLOG_TIMEOUT) {
        on  = false;
        len = 0;
    }
    if (!usbConfiguration || !usbInterruptIsReady4()) {
        return;
    }
    uint8_t p[8] = {0};
    if (on && hello) {
        hello = false;
        p[0]  = TAG_HELLO;
        p[1]  = KEYLOG_VERSION;
        p[2]  = MATRIX_ROWS;
        p[3]  = MATRIX_COLS;
    } else if (on && len) {
        keylog_event_t *e = &queue[head];
        head              = (head + 1) % KEYLOG_QUEUE;
        len--;
        p[0] = TAG_EVENT;
        p[1] = e->key;
        p[2] = e->layer;
        p[3] = e->time & 0xFF;
        p[4] = e->time >> 8;
    } else if (in_report) {
        // Nothing to send but a report is half-filled: pad it up to 32 bytes so the
        // host delivers the events already in it.
        p[0] = TAG_PAD;
    } else {
        return;
    }
    p[5] = seq++;
    p[6] = drops;
    for (uint8_t i = 0; i < 7; i++) {
        p[7] += p[i];
    }
    p[7] ^= 0x5A;
    usbSetInterrupt4(p, 8);
    in_report = (in_report + 1) & 3;
}

// [KEYLOG_CMD, 1] turns the stream on (and answers with a hello packet), [KEYLOG_CMD, 0]
// turns it off. No VIA reply is sent: it would take 4 packets of the same endpoint.
bool via_command_kb(uint8_t *data, uint8_t length) {
    if (data[0] != KEYLOG_CMD) {
        return false;
    }
    on        = data[1];
    hello     = on;
    heartbeat = timer_read32();
    if (!on) {
        len = 0;
    }
    return true;
}
