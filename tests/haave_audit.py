#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Dominik Siemon
"""HAAVE's audit, read: tests/haave_audit.c's renders -> each world's sounds beside its pad, in dB, on headphones (the
whole range) / through a small speaker (a 4th-order high-pass at 300 Hz: the FM-1's own, roughly).

A held sound (pad, bass, voice, shimmer, SYNTH, BASS) is its median 400 ms from 1 s on, past its attack (an attack alone
is not the sound); a struck one (the layers' notes, BELLS, the GONG, the sprinkles) its loudest 400 ms. A little more
weight above 1.5 kHz, as the ear has. Marked: ! more than 12 dB under the pad through the small speaker, !! 18.

  python3 tests/haave_audit.py OUT_DIR
"""
import sys
from pathlib import Path

import numpy as np
from scipy.signal import butter, sosfilt

FS = 44100
SEG = 5 * FS                                         # (haave_audit.c AUDIT_MS)
ROOT = Path(__file__).resolve().parents[1]
ELEM = ["pad", "bass", "vox", "arp", "mel", "shim", "SYNTH", "BELLS", "GONG", "BASS"]
HELD = {"pad", "bass", "vox", "shim", "SYNTH", "BASS"}
HP = butter(2, 30, "hp", fs=FS, output="sos")
SPEAKER = butter(4, 300, "hp", fs=FS, output="sos")
EAR = butter(1, 1500, "hp", fs=FS, output="sos")


def worlds():
    """the worlds' names and sprinkles, in order (haave_ui.c HAAVE_WORLDS, haave_spr.c HAAVE_SPR)"""
    import re
    ui = (ROOT / "firmware/src/haave_ui.c").read_text()
    spr = (ROOT / "firmware/src/haave_spr.c").read_text()
    names = re.findall(r'^\s+\{"([A-Z]+)", \d+, \d, \d+, \{', ui, re.M)
    rows = re.findall(r"^\s+\{(HS_\w+(?:, HS_\w+){5})\},", spr, re.M)
    return [(n, [h.strip()[3:].lower() for h in r.split(",")]) for n, r in zip(names, rows)]


def level(y, held):
    y = y + 0.6 * sosfilt(EAR, y)
    w = int(0.4 * FS)
    c = np.concatenate(([0.0], np.cumsum(y.astype(np.float64) ** 2)))
    s = (c[w:] - c[:-w])[::441] / w
    if held:
        s = s[FS // 441:int(FS * 4.5) // 441]
        return 10 * np.log10(np.median(s) + 1e-15)
    return 10 * np.log10(s.max() + 1e-15)


def main(d):
    for i, (name, spr) in enumerate(worlds()):
        x = np.fromfile(Path(d) / f"a{i:02d}.raw", dtype=np.float32).reshape(-1, 2).mean(axis=1)
        line, pad = f"{name:8s}", None
        for k, e in enumerate(ELEM + spr):
            y = x[k * SEG:(k + 1) * SEG]
            f, s = level(sosfilt(HP, y), e in HELD), level(sosfilt(SPEAKER, y), e in HELD)
            if pad is None:
                pad = (f, s)
                line += f" pad {f:5.1f}/{s:5.1f} |"
                continue
            mark = " !!" if s < pad[1] - 18 else " !" if s < pad[1] - 12 else ""
            line += f" {e} {f - pad[0]:+3.0f}/{s - pad[1]:+3.0f}{mark} |"
        print(line)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
