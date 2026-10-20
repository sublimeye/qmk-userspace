#!/usr/bin/env python3
"""Torn key usage logger: reads the v4 firmware's key event stream and keeps counts.

    run      background daemon (launchd runs this): connect, count, save every 5 s
    live     refreshing view of what the daemon is recording (Ctrl+C to quit)
    status   one-shot health check
    report   summary over all saved days (or --days N)
    watch    debug: print events as they arrive (stop the daemon first)

What is saved is aggregate only: per-key counts, tap/hold durations, press gaps, and
same-finger key pairs. The ordered sequence of keys is NOT stored (only same-finger
pairs are, which the finger analysis needs), so typed text and passwords cannot be
reconstructed from the files. See the privacy note in the README.
Data: ~/.local/share/torn-logger/days/YYYY-MM-DD.json and status.json.
Firmware side and packet format: keyboards/torn/keymaps/sublimeye/keylog.c.
"""
import argparse
import collections
import datetime
import fcntl
import hashlib
import json
import os
import re
import signal
import sys
import time
import traceback

VID, PID = 0x7274, 0x0001
RAW_USAGE_PAGE, RAW_USAGE = 0xFF60, 0x61
CMD = 0xE0
TAG_EVENT, TAG_PAD, TAG_HELLO = 0xA5, 0xA6, 0xA7
COLS = 12
HEARTBEAT_S = 10     # firmware stops streaming after 120 s without one
SAVE_S = 5
STATUS_S = 2
MAX_MS = 60_000      # event times wrap at 65.5 s; longer intervals are not measured
PAIR_MS = 1000       # presses further apart than this are not counted as a pair
BKSP_MS = 300        # a Backspace this soon after a tap-hold key counts as a misfire

DATA = os.path.expanduser(os.environ.get("TORN_LOGGER_DATA", "~/.local/share/torn-logger"))
DAYS = os.path.join(DATA, "days")
STATUS = os.path.join(DATA, "status.json")
KEYMAP = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "keyboards", "torn", "keymaps", "sublimeye", "keymap.c")

# LAYOUT_split_3x6_4 order -> matrix index (row * 12 + col), from keyboards/torn/keyboard.json.
LAYOUT = [r * COLS + c for r in range(3) for c in range(12)] + [3 * COLS + c for c in range(2, 10)]
# Finger per column; thumbs are one finger per hand.
FINGER = ["L5", "L5", "L4", "L3", "L2", "L2", "R2", "R2", "R3", "R4", "R5", "R5"]


def finger(pos):
    if pos // COLS == 3:
        return "LT" if pos % COLS < 6 else "RT"
    return FINGER[pos % COLS]


# ---------------------------------------------------------------- keymap labels

KC = {
    "SPC": "Space", "ESC": "Esc", "BSPC": "Bksp", "DEL": "Del", "ENT": "Enter", "TAB": "Tab",
    "LEFT": "←", "RGHT": "→", "UP": "↑", "DOWN": "↓", "HOME": "Home", "END": "End",
    "PGUP": "PgUp", "PGDN": "PgDn", "SCLN": ";", "QUOT": "'", "COMM": ",", "DOT": ".",
    "SLSH": "/", "GRV": "`", "BSLS": "\\", "LBRC": "[", "RBRC": "]", "EQL": "=", "MINS": "-",
    "LCTL": "Ctrl", "LALT": "Alt", "LSFT": "Shift", "LGUI": "Cmd",
}
SHIFTED = dict(zip("1234567890`-=[]\\;',./", "!@#$%^&*()~_+{}|:\"<>?"))
MODS = {"LCTL": "⌃", "C": "⌃", "LALT": "⌥", "A": "⌥", "LGUI": "⌘", "G": "⌘", "LSFT": "⇧", "S": "⇧", "SGUI": "⇧⌘"}
HOLD_MODS = {"LCTL_T": "Ctrl", "LALT_T": "Alt", "LSFT_T": "Shift", "LGUI_T": "Cmd",
             "RCTL_T": "Ctrl", "RALT_T": "Alt", "RSFT_T": "Shift", "RGUI_T": "Cmd"}


def label(kc):
    """Short label for a keymap.c keycode expression."""
    kc = kc.strip()
    if kc in ("_______", "XXXXXXX", "KC_NO", "KC_TRNS"):
        return ""
    if kc == "LANG_NUM":
        return "Lang"
    if kc == "CW_TOGG":
        return "CapsWord"
    m = re.fullmatch(r"(\w+)\((.*)\)", kc)
    if m:
        fn, arg = m.groups()
        if fn in HOLD_MODS:
            return label(arg)
        if fn == "LT":
            return label(arg.split(",", 1)[1])
        if fn == "MO":
            return arg.lstrip("_")
        if fn == "HYPR":
            return "Hyper"
        if fn in ("LSFT", "S"):
            inner = label(arg)
            return SHIFTED.get(inner, "⇧" + inner)
        if fn in MODS:
            return MODS[fn] + label(arg)
        return kc
    if kc.startswith("KC_"):
        name = kc[3:]
        return KC.get(name, name)
    return kc


def hold_label(kc):
    m = re.fullmatch(r"(\w+)\((.*)\)", kc.strip())
    if not m:
        return None
    fn, arg = m.groups()
    if fn in HOLD_MODS:
        return HOLD_MODS[fn]
    if fn == "LT":
        return arg.split(",")[0].strip().lstrip("_")
    return None


def strip_comments(src):
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    return "\n".join(re.sub(r"//.*", "", line) for line in src.splitlines())


class Keymap:
    def __init__(self, path=KEYMAP):
        raw = open(path).read()
        self.sha = hashlib.sha1(raw.encode()).hexdigest()[:10]
        src = strip_comments(raw)
        self.names = [n.strip().lstrip("_") for n in re.search(r"enum torn_layers \{([^}]*)\}", src).group(1).split(",") if n.strip()]
        defines = dict(re.findall(r"#define (\w+) (LT\([^)]*\))", src))
        self.layers = []
        for body in re.findall(r"\] = LAYOUT_split_3x6_4\((.*?)\n\)", src, re.S):
            keys = split_args(body)
            if len(keys) != len(LAYOUT):
                raise ValueError(f"layer has {len(keys)} keys, expected {len(LAYOUT)}")
            keys = [defines.get(k, k) if k != "LANG_NUM" else "LT(_NUM, LANG_NUM)" for k in keys]
            self.layers.append({pos: kc for pos, kc in zip(LAYOUT, keys)})
        if not self.layers:
            raise ValueError("no layers parsed from keymap.c")

    def layer_of(self, pos, state):
        """Layer whose keycode a press used: highest active layer that isn't transparent."""
        for layer in range(len(self.layers) - 1, -1, -1):
            if (state | 1) >> layer & 1 and self.layers[layer].get(pos, "_______") != "_______":
                return layer
        return 0

    def keycode(self, pos, layer):
        return self.layers[layer].get(pos, "XXXXXXX") if layer < len(self.layers) else "XXXXXXX"

    def hold_layer(self, pos, layer):
        """Layer a layer-tap at this key activates on hold, or -1."""
        m = re.match(r"LT\((\w+),", self.keycode(pos, layer))
        if m and m.group(1).lstrip("_") in self.names:
            return self.names.index(m.group(1).lstrip("_"))
        return -1

    def name(self, pos, layer, kind="k"):
        kc = self.keycode(pos, layer)
        if kc == "LT(_NUM, LANG_NUM)":
            return "Lang" if kind != "h" else "Lang→NUM"
        if kind == "h":
            return f"{label(kc)}→{hold_label(kc)}"
        text = label(kc) or f"({pos // COLS},{pos % COLS})"
        return text if layer == 0 else f"{self.names[layer]}:{text}"


def split_args(body):
    out, depth, cur = [], 0, ""
    for ch in body:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


# ---------------------------------------------------------------- packets

def parse_chunk(c):
    """One 8-byte packet -> dict, or None if it isn't a valid logger packet."""
    if len(c) != 8 or c[0] not in (TAG_EVENT, TAG_PAD, TAG_HELLO):
        return None
    if (sum(c[:7]) & 0xFF) ^ 0x5A != c[7]:
        return None
    p = {"tag": c[0], "seq": c[5], "drops": c[6]}
    if c[0] == TAG_EVENT:
        p.update(pos=c[1] & 0x3F, pressed=bool(c[1] & 0x40), tap=bool(c[1] & 0x80),
                 state=c[2] & 0x3F, taphold=bool(c[2] & 0x40), time=c[3] | c[4] << 8)
    elif c[0] == TAG_HELLO:
        p.update(version=c[1], rows=c[2], cols=c[3])
    return p


def bin10(ms):
    return str(min(ms, 990) // 10 * 10)


# ---------------------------------------------------------------- daemon

def new_day(date, sha):
    return {"date": date, "keymaps": [sha] if sha else [], "presses": 0, "events": 0,
            "lost": 0, "dropped": 0, "connected_s": 0, "pairs_total": 0,
            "press": {}, "sfb": {}, "tap_ms": {}, "hold_ms": {}, "gap_ms": {},
            "bksp_after": {}, "hours": [0] * 24}


def inc(d, k, n=1):
    d[k] = d.get(k, 0) + n


def save_json(path, obj):
    tmp = f"{path}.{os.getpid()}.tmp"
    with open(tmp, "w") as f:
        json.dump(obj, f, separators=(",", ":"))
    os.replace(tmp, path)


def load_day(path, date, sha):
    """Load a day file, or start fresh. A corrupt file is set aside, never fatal."""
    if not os.path.exists(path):
        return new_day(date, sha)
    try:
        d = json.load(open(path))
        if isinstance(d, dict) and "press" in d:
            for k, v in new_day(date, sha).items():  # backfill fields added since the file was written
                d.setdefault(k, v)
            return d
        raise ValueError("not a day file")
    except Exception:
        try:
            os.replace(path, path + ".corrupt")
        except OSError:
            pass
        return new_day(date, sha)


class Counter:
    """Turns the event stream into day counts. Holds only the last press in memory."""

    def __init__(self, keymap):
        self.set_keymap(keymap)
        self.day = None
        self.reset_transient()

    def set_keymap(self, keymap):
        self.km = keymap
        self.bksp = {(pos, layer) for layer, grid in enumerate(keymap.layers)
                     for pos, kc in grid.items() if "BSPC" in kc}

    def reset_transient(self):
        """Forget in-flight presses (call on reconnect or a sequence gap)."""
        self.down = {}       # pos -> (device time, host time, kind)
        self.prev = None     # (key id, pos, device time, host time) of the last press
        self.pending = None  # (key id, host time, hold layer) of the last tap-hold press
        self.recent = getattr(self, "recent", collections.deque())

    def load(self, date):
        self.day = load_day(os.path.join(DAYS, date + ".json"), date, self.km.sha)
        if self.km.sha not in self.day["keymaps"]:
            self.day["keymaps"].append(self.km.sha)

    def save(self):
        if self.day:
            os.makedirs(DAYS, exist_ok=True)
            save_json(os.path.join(DAYS, self.day["date"] + ".json"), self.day)

    def _today(self):
        """Ensure self.day is for the current date; roll over and reset state at midnight."""
        today = datetime.date.today().isoformat()
        if not self.day or self.day["date"] != today:
            self.save()
            self.load(today)
            self.reset_transient()
        return self.day

    def note_lost(self, n):
        if n:
            inc(self._today(), "lost", n)

    def note_drops(self, n):
        if n:
            inc(self._today(), "dropped", n)

    def add_connected(self, seconds):
        self._today()["connected_s"] += seconds

    def event(self, e, now):
        d = self._today()
        d["events"] += 1
        pos, t = e["pos"], e["time"]
        kind = ("t" if e["tap"] else "h") if e["taphold"] else "k"
        if not e["pressed"]:
            if pos in self.down:
                pt, phost, pkind = self.down.pop(pos)
                ms = (t - pt) & 0xFFFF
                if pkind != "k" and now - phost < MAX_MS / 1000:
                    inc(d["tap_ms" if pkind == "t" else "hold_ms"].setdefault(str(pos), {}), bin10(ms))
            return
        layer = self.km.layer_of(pos, e["state"])
        key = f"{pos}:{layer}"
        d["presses"] += 1
        d["hours"][datetime.datetime.now().hour] += 1
        inc(d["press"], f"{key}:{kind}")
        self.down[pos] = (t, now, kind)
        self.recent.append(now)
        # Did a Backspace follow the previous tap-hold key quickly, and not via its own
        # hold layer? That is a proxy for the key firing the wrong way.
        if self.pending:
            pkey, ptime, phl = self.pending
            if now - ptime <= BKSP_MS / 1000:
                via_hold = (pos, layer) in self.bksp and layer == phl
                hit = (pos, layer) in self.bksp and not via_hold
                inc(d["bksp_after"], f"{pkey}:{'bksp' if hit else 'other'}")
            self.pending = None
        if kind != "k":
            self.pending = (f"{pos}:{kind}", now, self.km.hold_layer(pos, layer))
        if self.prev:
            pkey, ppos, ptime, phost = self.prev
            if now - phost <= PAIR_MS / 1000:
                d["pairs_total"] += 1
                gap = (t - ptime) & 0xFFFF
                if now - phost < MAX_MS / 1000:
                    inc(d["gap_ms"], bin10(gap))
                if ppos != pos and finger(ppos) == finger(pos):  # same finger = SFB
                    inc(d["sfb"], f"{pkey}>{key}")
        self.prev = (key, pos, t, now)

    def rate(self, now):
        while self.recent and now - self.recent[0] > 60:
            self.recent.popleft()
        return len(self.recent)


def find_raw():
    import hid
    for d in hid.enumerate(VID, PID):
        if d["usage_page"] == RAW_USAGE_PAGE and d["usage"] == RAW_USAGE:
            return d["path"]
    return None


def send(h, on):
    if h.write([0x00, CMD, 1 if on else 0] + [0] * 30) < 0:
        raise OSError("write failed")


def log(msg):
    print(f"{datetime.datetime.now():%Y-%m-%d %H:%M:%S} {msg}", flush=True)


def run(args):
    import hid
    os.makedirs(DAYS, exist_ok=True)
    lock = open(os.path.join(DATA, "daemon.lock"), "w")
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except OSError:
        sys.exit("another torn_logger daemon is already running")
    try:
        c = Counter(Keymap())
    except Exception as err:
        sys.exit(f"cannot parse keymap.c: {err}")
    c.load(datetime.date.today().isoformat())
    st = {"pid": os.getpid(), "started": time.time(), "connected": False, "firmware": "unknown",
          "last_event": None, "last_hello": None, "lost": 0, "dropped": 0}
    h = None
    stop = []
    signal.signal(signal.SIGTERM, lambda *_: stop.append(1))
    signal.signal(signal.SIGINT, lambda *_: stop.append(1))
    last_seq = last_drops = None
    next_beat = next_save = next_status = 0.0
    conn_tick = None
    while not stop:
        now = time.time()
        try:
            if h is None:
                path = find_raw()
                if path is None:
                    st.update(connected=False, firmware="keyboard not found")
                    raise OSError("not found")
                h = hid.device()
                h.open_path(path)
                st.update(connected=True, firmware="waiting for hello", last_hello=None)
                last_seq = last_drops = None
                next_beat = 0
                conn_tick = now
                c.reset_transient()
            if now >= next_beat:
                send(h, True)
                next_beat = now + HEARTBEAT_S
            data = h.read(64, 500)
            now = time.time()
            for i in range(0, len(data) - 7, 8):
                chunk = data[i:i + 8]
                if chunk[0] == 0xFF and chunk[1] == 0x01 and i == 0:
                    st["firmware"] = "no logger in firmware (VIA answered): flash v4"
                    continue
                p = parse_chunk(chunk)
                if p is None:
                    continue
                if last_seq is not None:
                    gap = (p["seq"] - last_seq - 1) & 0xFF
                    if gap:
                        c.note_lost(gap)
                        st["lost"] += gap
                        c.reset_transient()  # a lost event could be a release; don't pair across it
                last_seq = p["seq"]
                if last_drops is not None:
                    n = (p["drops"] - last_drops) & 0xFF
                    c.note_drops(n)
                    st["dropped"] += n
                last_drops = p["drops"]
                if p["tag"] == TAG_HELLO:
                    st.update(firmware=f"logger v{p['version']}", last_hello=now)
                elif p["tag"] == TAG_EVENT:
                    c.event(p, now)
                    st["last_event"] = now
        except OSError as err:
            if h is not None:
                try:
                    h.close()
                except Exception:
                    pass
                h = None
                st.update(connected=False, firmware=f"disconnected ({err})")
            conn_tick = None
            time.sleep(2)
        except Exception:
            log("error in daemon loop:\n" + traceback.format_exc())
            time.sleep(2)
        now = time.time()
        if conn_tick is not None and st.get("connected"):
            try:
                c.add_connected(now - conn_tick)
            except Exception:
                pass
            conn_tick = now
        if now >= next_save:
            c.save()
            next_save = now + SAVE_S
        if now >= next_status:
            st.update(updated=now, rate_per_min=c.rate(now))
            save_json(STATUS, st)
            next_status = now + STATUS_S
    if h is not None:
        try:
            send(h, False)
            h.close()
        except Exception:
            pass
    c.save()
    st.update(connected=False, firmware="daemon stopped", updated=time.time(), pid=None)
    save_json(STATUS, st)


# ---------------------------------------------------------------- views

def ago(t):
    if not t:
        return "never"
    s = int(time.time() - t)
    return f"{s}s ago" if s < 120 else f"{s // 60}m ago" if s < 7200 else f"{s // 3600}h ago"


def read_status():
    try:
        return json.load(open(STATUS))
    except Exception:
        return None


def health():
    """(ok, lines) for live/status."""
    st = read_status()
    if st is None:
        return False, ["daemon has never run, or status is unreadable (logger/install.sh)"]
    alive = False
    if st.get("pid"):
        try:
            os.kill(st["pid"], 0)
            alive = True
        except OSError:
            pass
    fresh = time.time() - st.get("updated", 0) < 10
    hello = bool(st.get("last_hello")) and time.time() - st["last_hello"] < 3 * HEARTBEAT_S
    ok = alive and fresh and st.get("connected") and hello
    lines = [
        f"daemon      {'running' if alive and fresh else 'NOT RUNNING'} (pid {st.get('pid')})",
        f"keyboard    {'connected' if st.get('connected') else 'not connected'}, {st.get('firmware')}",
        f"firmware    {'answering' if hello else 'NOT answering'} (last hello {ago(st.get('last_hello'))})",
        f"last key    {ago(st.get('last_event'))}, {st.get('rate_per_min', 0)} presses in the last minute",
        f"lost        {st.get('lost', 0)} packets, {st.get('dropped', 0)} queue drops since daemon start",
    ]
    return ok, lines


def load_days(n=None):
    files = sorted(f for f in os.listdir(DAYS) if f.endswith(".json")) if os.path.isdir(DAYS) else []
    if n:
        files = files[-n:]
    out = []
    for f in files:
        try:
            out.append(json.load(open(os.path.join(DAYS, f))))
        except Exception:
            pass
    return out


def merge(days):
    tot = new_day("", "")
    tot["keymaps"] = []
    for d in days:
        for k in ("presses", "events", "lost", "dropped", "connected_s", "pairs_total"):
            tot[k] += d.get(k, 0)
        for k in ("press", "sfb", "gap_ms", "bksp_after"):
            for kk, v in d.get(k, {}).items():
                inc(tot[k], kk, v)
        for k in ("tap_ms", "hold_ms"):
            for pos, hist in d.get(k, {}).items():
                for b, v in hist.items():
                    inc(tot[k].setdefault(pos, {}), b, v)
        tot["hours"] = [a + b for a, b in zip(tot["hours"], d.get("hours", [0] * 24))]
        for s in d.get("keymaps", []):
            if s not in tot["keymaps"]:
                tot["keymaps"].append(s)
    return tot


def top_keys(km, d, n):
    by_key = collections.Counter()
    for k, v in d["press"].items():
        pos, layer, kind = k.split(":")
        by_key[km.name(int(pos), int(layer), "k" if kind == "t" else kind)] += v
    return by_key.most_common(n)


def live(args):
    km = Keymap()
    try:
        while True:
            ok, lines = health()
            today = datetime.date.today().isoformat()
            days = [d for d in load_days(1) if d["date"] == today]
            d = days[0] if days else new_day(today, "")
            out = ["\033[H\033[J" + f"Torn logger  {datetime.datetime.now():%H:%M:%S}   {'OK' if ok else 'CHECK BELOW'}", ""]
            out += lines
            out += ["", f"today       {d['presses']} presses, {d['lost']} lost, {d['dropped']} dropped", ""]
            out += [f"  {v:6}  {k}" for k, v in top_keys(km, d, 12)]
            print("\n".join(out), flush=True)
            time.sleep(1)
    except KeyboardInterrupt:
        pass


def status(args):
    ok, lines = health()
    print("\n".join(lines))
    sys.exit(0 if ok else 1)


def pct(hist, q):
    """Upper edge of the 10 ms bin that holds the q-quantile (so the value is <= result)."""
    total = sum(hist.values())
    if not total:
        return None
    acc = 0
    for b in sorted(hist, key=int):
        acc += hist[b]
        if acc >= q * total:
            return int(b) + 10
    return None


def report(args):
    km = Keymap()
    days = [d for d in load_days(args.days) if d.get("presses")]
    if not days:
        sys.exit("no data yet")
    d = merge(days)
    p = d["presses"] or 1
    cov = d["connected_s"] / 3600
    print(f"{len(days)} days ({days[0]['date']} .. {days[-1]['date']}), {d['presses']} presses, "
          f"{cov:.1f} h recorded, {d['lost']} packets lost, {d['dropped']} dropped")
    if set(d["keymaps"]) - {km.sha}:
        print("note: some days were recorded with a different keymap.c; labels use the current file.")
        print("      if keys were edited in VIA (not keymap.c), labels can be wrong and this can't tell.")

    print("\nLayers")
    layers = collections.Counter()
    for k, v in d["press"].items():
        layers[km.names[int(k.split(":")[1])]] += v
    for name, v in layers.most_common():
        print(f"  {name:8} {v / p:6.1%}")

    print("\nTop keys")
    for k, v in top_keys(km, d, 25):
        print(f"  {v:7}  {v / p:6.1%}  {k}")

    print("\nTap-hold keys: taps, holds, tap length <=p50/p90/p99 ms, taps >= 150 ms, Backspace next")
    print("(tap length is the signal for TAPPING_TERM; with Flow Tap on, true holds can't be < 150 ms)")
    th = collections.defaultdict(lambda: {"t": 0, "h": 0})
    for k, v in d["press"].items():
        pos, layer, kind = k.split(":")
        if kind in "th":
            th[int(pos)][kind] += v
    for pos, n in sorted(th.items(), key=lambda x: -x[1]["t"] - x[1]["h"]):
        hist = d["tap_ms"].get(str(pos), {})
        q = [pct(hist, x) for x in (0.5, 0.9, 0.99)]
        long_ = sum(v for b, v in hist.items() if int(b) >= 150) / (sum(hist.values()) or 1)
        bk = {kind: d["bksp_after"].get(f"{pos}:{kind}:bksp", 0) / (n[kind] or 1) for kind in "th"}
        print(f"  {km.name(pos, 0):5} {n['t']:7} {n['h']:6}   {q[0]}/{q[1]}/{q[2]}   {long_:5.1%}   "
              f"after tap {bk['t']:5.1%}  after hold {bk['h']:5.1%}   (hold = {km.name(pos, 0, 'h')})")

    gaps = d["gap_ms"]
    g = sum(gaps.values()) or 1
    print(f"\nPress-to-press gaps (all keys): {sum(v for b, v in gaps.items() if int(b) < 150) / g:.1%} "
          f"under 150 ms, median <= {pct(gaps, 0.5)} ms")

    print("\nSame-finger pairs (different keys, same finger)")
    sfb = collections.Counter()
    for k, v in d["sfb"].items():
        a, b = k.split(">")
        pa, la = map(int, a.split(":"))
        pb, lb = map(int, b.split(":"))
        sfb[f"{km.name(pa, la)} > {km.name(pb, lb)}"] += v
    print(f"  {sum(sfb.values()) / (d['pairs_total'] or 1):.1%} of all key pairs")
    for k, v in sfb.most_common(12):
        print(f"  {v:7}  {k}")

    print("\nRarely used (fewer than 0.05% of presses)")
    used = collections.Counter()
    for k, v in d["press"].items():
        pos, layer, _ = k.split(":")
        used[(int(pos), int(layer))] += v
    rare = []
    for layer, grid in enumerate(km.layers):
        for pos, kc in grid.items():
            if kc not in ("_______", "XXXXXXX") and used[(pos, layer)] < 0.0005 * p:
                rare.append(f"{km.name(pos, layer)} ({used[(pos, layer)]})")
    print("  " + ", ".join(rare))


def watch(args):
    import hid
    km = Keymap()
    path = find_raw()
    if not path:
        sys.exit("keyboard not found")
    h = hid.device()
    try:
        h.open_path(path)
    except OSError:
        sys.exit("cannot open the keyboard: is the daemon running? "
                 "(launchctl bootout gui/$UID/com.sublimeye.torn-logger)")
    send(h, True)
    beat = time.time()
    print("press keys; Ctrl+C to stop. Nothing is saved.")
    try:
        while True:
            if time.time() - beat > HEARTBEAT_S:
                send(h, True)
                beat = time.time()
            data = h.read(64, 500)
            for i in range(0, len(data) - 7, 8):
                p = parse_chunk(data[i:i + 8])
                if p is None:
                    continue
                if p["tag"] == TAG_HELLO:
                    print(f"  hello: logger v{p['version']}, matrix {p['rows']}x{p['cols']}, seq {p['seq']}, drops {p['drops']}")
                elif p["tag"] == TAG_EVENT:
                    layer = km.layer_of(p["pos"], p["state"])
                    kind = ("t" if p["tap"] else "h") if p["taphold"] else "k"
                    show = kind if p["pressed"] else "k"
                    print(f"  {'down' if p['pressed'] else 'up  '}  {km.name(p['pos'], layer, show):14} "
                          f"pos {p['pos']:2}  layers {p['state']:06b}  "
                          f"{'tap ' if kind == 't' else 'hold' if kind == 'h' else '    '}"
                          f"  t={p['time']:5}  seq {p['seq']:3}  drops {p['drops']}")
    except KeyboardInterrupt:
        pass
    finally:
        try:
            send(h, False)
            h.close()
        except Exception:
            pass


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("run", "live", "status", "watch"):
        sub.add_parser(name)
    r = sub.add_parser("report")
    r.add_argument("--days", type=int, help="only the last N days")
    args = ap.parse_args()
    {"run": run, "live": live, "status": status, "report": report, "watch": watch}[args.cmd](args)


if __name__ == "__main__":
    main()
