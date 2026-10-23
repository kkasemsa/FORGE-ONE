#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Khantipol Kasemsant. The drawing kit (Pen, curve, the round-ended monoline lettering and the RLE
# splash encoding) follows Clipper's tools/gen_logo.py (isod89 / 3dSam, GPL-3.0), as Forge Five's logo does.
"""The FORGE ONE logo: one flame on an anvil inside the 270-degree dial (the knobs of the UI), four sequencer steps
on the anvil's face (the four tracks), a cyan spark flying off; a geometric monoline wordmark "forge one". The colours
are the FORGE palette's (tools/gen_ui_palettes.py): indigo night, paper ink, ember and gold flame, the cyan accent.

  tools/gen_fo_logo.py OUT.h          the boot splash (RLE, 16-colour palette, RGB565): FO_LOGO_*
  tools/gen_fo_logo.py --assets DIR   forge-one-logo.png, forge-one-icon.png, forge-one-splash.png (2x)
"""
import math
import sys
from pathlib import Path

from PIL import Image, ImageDraw

# the FORGE palette (gen_ui_palettes.py "FORGE"): BG, TEXT, THEME, ACCENT; the flame's gold and red
BG, PAPER, EMBER, CYAN = (10, 12, 26), (240, 232, 214), (255, 164, 56), (86, 222, 214)
GOLD, RED = (255, 222, 120), (240, 72, 40)
INKS = (PAPER, EMBER, GOLD, CYAN, RED)            # the splash's palette: BG, and each ink at 1/3, 2/3 and full over BG
XH, SW = 72, 12                                   # wordmark x-height, stroke (units: the icon is 240 x 240)
R = XH / 2 - SW / 2                               # the bowls' centre-line radius
GAP = 16


class Pen:
    """round-capped strokes and fills drawn with PIL (k: scale, ox / oy: offset)"""

    def __init__(self, d, k=1.0, ox=0.0, oy=0.0):
        self.d, self.k, self.ox, self.oy = d, k, ox, oy

    def P(self, x, y):
        return self.ox + x * self.k, self.oy + y * self.k

    def dot(self, x, y, w, col):
        (X, Y), r = self.P(x, y), w * self.k / 2
        self.d.ellipse([X - r, Y - r, X + r, Y + r], fill=col)

    def line(self, x0, y0, x1, y1, w=SW, col=PAPER):
        self.d.line([self.P(x0, y0), self.P(x1, y1)], fill=col, width=max(1, round(w * self.k)))
        self.dot(x0, y0, w, col)
        self.dot(x1, y1, w, col)

    def arc(self, cx, cy, r, a0, a1, w=SW, col=PAPER, caps=True):
        """an arc of radius r (the stroke's centre line), a0 -> a1 degrees clockwise on screen"""
        (X, Y), RR, W = self.P(cx, cy), (r + w / 2) * self.k, max(1, round(w * self.k))
        self.d.arc([X - RR, Y - RR, X + RR, Y + RR], start=a0, end=a1, fill=col, width=W)
        if caps:
            for a in (a0, a1):
                self.dot(cx + r * math.cos(math.radians(a)), cy + r * math.sin(math.radians(a)), w, col)

    def poly(self, pts, col):
        self.d.polygon([self.P(x, y) for x, y in pts], fill=col)

    def rect(self, x0, y0, x1, y1, col):
        self.d.rectangle([self.P(x0, y0), self.P(x1, y1)], fill=col)


def curve(p0, p1, bulge, n=12):
    """points from p0 to p1 bowed out to the right of the travel by bulge (a quadratic)"""
    (x0, y0), (x1, y1) = p0, p1
    mx, my = (x0 + x1) / 2, (y0 + y1) / 2
    dx, dy = x1 - x0, y1 - y0
    L = math.hypot(dx, dy)
    cx, cy = mx - dy / L * bulge * 2, my + dx / L * bulge * 2
    return [((1 - t) ** 2 * x0 + 2 * (1 - t) * t * cx + t * t * x1, (1 - t) ** 2 * y0 + 2 * (1 - t) * t * cy + t * t * y1)
            for t in (i / n for i in range(n + 1))]


def flame(pen, cx, base, half, top, lean, col):
    """a flame: from its foot (base, half wide) each side swells out to a shoulder, then draws in to the tip (top),
    which leans right by lean; the right side a little fuller (the draught)"""
    h = base - top
    l, r, tip = (cx - half, base), (cx + half, base), (cx + lean, top)
    ls, rs = (cx - half * 1.08, base - h * 0.38), (cx + half * 1.12, base - h * 0.42)
    side_l = curve(l, ls, -half * 0.30, 8) + curve(ls, tip, half * 0.16, 12)[1:]     # (curve: + bows right of travel)
    side_r = curve(tip, rs, half * 0.12, 12) + curve(rs, r, -half * 0.32, 8)[1:]
    pen.poly(side_l + side_r[1:], col)


def icon(pen, ox, oy):
    cx, cy = ox + 120, oy + 120
    pen.arc(cx, cy, 100, 135, 405)                                   # the dial (Clipper's, SLOOP's, Forge Five's)
    # the anvil: face, waist, foot; the horn to the left
    face, waist, foot = cy + 34, cy + 56, cy + 74
    pen.poly([(cx - 46, face), (cx + 52, face), (cx + 52, face + 14), (cx + 30, face + 20), (cx + 18, waist),
              (cx + 34, foot), (cx - 34, foot), (cx - 18, waist), (cx - 30, face + 20), (cx - 46, face + 14)], PAPER)
    pen.poly([(cx - 46, face)] + curve((cx - 46, face), (cx - 78, face + 2), 0)[1:] +
             curve((cx - 78, face + 2), (cx - 46, face + 14), 3)[1:], PAPER)          # the horn
    for k in range(4):                                               # four steps cut in the face: the four tracks
        x = cx - 38 + k * 22
        pen.rect(x, face + 4, x + 15, face + 10, BG if k != 0 else EMBER)
    # one flame: red, ember, gold, from the outside in, leaning a little with the draught
    base = face - 2
    flame(pen, cx + 2, base, 30, cy - 78, 10, RED)
    flame(pen, cx + 3, base, 23, cy - 56, 8, EMBER)
    flame(pen, cx + 4, base, 13, cy - 26, 5, GOLD)
    # the spark: a small cyan diamond flying off up to the right, clear of the flame, and its trail
    sx, sy = cx + 58, cy - 58
    pen.poly([(sx, sy - 9), (sx + 6, sy), (sx, sy + 9), (sx - 6, sy)], CYAN)
    pen.line(sx - 18, sy + 18, sx - 11, sy + 11, w=4, col=CYAN)


# ---- the wordmark "forge one" (Forge Five's lettering: round bowls of the x-height, round-ended strokes) ----------
def _f(pen, x, oy, base, w):
    rh, top = R * 0.62, oy - 40
    pen.line(x + w / 2, top + rh + w / 2, x + w / 2, base - w / 2)
    pen.arc(x + w / 2 + rh, top + rh + w / 2, rh, 180, 275)
    pen.line(x - 4, oy + w / 2, x + w + 14, oy + w / 2)
    return w + 18


def word(pen, ox, oy):
    x, mid, base, w = ox, oy + XH / 2, oy + XH, SW
    x += _f(pen, x, oy, base, w) + GAP                               # f
    pen.arc(x + XH / 2, mid, R, 0, 360, caps=False)                  # o
    x += XH + GAP
    pen.line(x + w / 2, oy + w / 2, x + w / 2, base - w / 2)         # r
    pen.arc(x + w / 2 + R, mid, R, 180, 290)
    x += w / 2 + R * 1.55 + GAP
    pen.arc(x + XH / 2, mid, R, 0, 360, caps=False)                  # g
    pen.line(x + XH - w / 2, mid, x + XH - w / 2, base + 24)
    pen.arc(x + XH / 2, base + 24, R, 0, 150)
    x += XH + GAP
    pen.arc(x + XH / 2, mid, R, 40, 360)                             # e
    pen.line(x + w / 2, mid, x + XH - w / 2, mid)
    x += XH + GAP * 2.6                                              # the word space
    pen.arc(x + XH / 2, mid, R, 0, 360, caps=False, col=EMBER)       # o: "one" in ember
    x += XH + GAP
    pen.line(x + w / 2, oy + w / 2, x + w / 2, base - w / 2, col=EMBER)   # n: a stem, the arch, the right stem
    pen.arc(x + XH / 2, mid, R, 180, 360, caps=False, col=EMBER)
    pen.line(x + XH - w / 2, mid, x + XH - w / 2, base - w / 2, col=EMBER)
    x += XH + GAP
    pen.arc(x + XH / 2, mid, R, 40, 360, col=EMBER)                  # e
    pen.line(x + w / 2, mid, x + XH - w / 2, mid, col=EMBER)
    return x + XH - ox


def word_width():
    return word(Pen(ImageDraw.Draw(Image.new("RGB", (1, 1)))), 0, 0)


def render(kind, W, H, k, ss=4):
    im = Image.new("RGB", (W * ss, H * ss), BG)
    d = ImageDraw.Draw(im)
    K = k * ss
    if kind == "h":
        icon(Pen(d, K, 10 * K, (H * ss - 240 * K) / 2), 0, 0)
        word(Pen(d, K, 270 * K, H * ss / 2 - 36 * K), 0, 0)
    elif kind == "v":                                # the splash: the icon large, the wordmark under the dial's gap
        KI, KW = ICON_K * ss, WORD_K * ss
        icon(Pen(d, KI, (W * ss - 240 * KI) / 2, -6 * KI), 0, 0)
        word(Pen(d, KW, (W * ss - word_width() * KW) / 2, WORD_Y * ss), 0, 0)
    else:
        icon(Pen(d, K, (W * ss - 240 * K) / 2, (H * ss - 240 * K) / 2), 0, 0)
    return im.resize((W, H), Image.LANCZOS)


# the splash: SPLASH_W x SPLASH_H at the top of the screen; the firmware draws the version and the credits under it
SPLASH_W, SPLASH_H = 240, 168
SPLASH_K = 1.0                                     # (render's k: unused by "v", which has its own two scales)
ICON_K, WORD_K = 0.56, 0.31                        # the icon 134 px, the wordmark ~202 px wide
WORD_Y = 122                                       # the wordmark's x-height top (px): the f's ascender 12 px above, clear of the anvil
assert word_width() * WORD_K <= SPLASH_W - 24, "the wordmark must fit the splash"
assert WORD_Y + (XH + 24 + R + SW) * WORD_K <= SPLASH_H, "the wordmark's g must fit the splash"


def rgb565(c):
    r, g, b = c
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def palette():
    cols = [BG]
    for c in INKS:
        cols += [tuple(round(BG[j] + (c[j] - BG[j]) * f) for j in range(3)) for f in (1 / 3, 2 / 3, 1.0)]
    assert len(cols) == 16
    return cols


def quantise(im):
    cols, src = palette(), im.tobytes()
    px, cache = [], {}
    for i in range(0, len(src), 3):
        p = (src[i], src[i + 1], src[i + 2])
        if p not in cache:
            cache[p] = min(range(16), key=lambda k: sum((p[j] - cols[k][j]) ** 2 for j in range(3)))
        px.append(cache[p])
    return cols, px


def splash_header(path):
    cols, px = quantise(render("v", SPLASH_W, SPLASH_H, SPLASH_K))
    rle = bytearray()
    i = 0
    while i < len(px):
        j = i
        while j < len(px) and px[j] == px[i] and j - i < 16:
            j += 1
        rle.append(((j - i - 1) << 4) | px[i])
        i = j
    L = ["/* generated by tools/gen_fo_logo.py: the FORGE ONE boot splash (runs of (run - 1) << 4 | colour) */",
         "#pragma once", "#include <stdint.h>",
         f"#define FO_LOGO_W {SPLASH_W}", f"#define FO_LOGO_H {SPLASH_H}",
         "static const uint16_t FO_LOGO_PAL[16] = {" + ", ".join(f"0x{rgb565(c):04X}" for c in cols) + "};",
         f"static const uint8_t FO_LOGO_RLE[{len(rle)}] = {{"]
    for k in range(0, len(rle), 24):
        L.append("    " + ", ".join(str(b) for b in rle[k:k + 24]) + ",")
    L.append("};")
    Path(path).write_text("\n".join(L) + "\n")
    print(f"logo: FORGE ONE splash {SPLASH_W}x{SPLASH_H}, {len(rle)} B RLE -> {path}")


def assets(out):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    W = int(270 + word_width() + 20)
    render("h", W * 2, 520, 2.0).save(out / "forge-one-logo.png")
    render("i", 512, 512, 512 / 240).save(out / "forge-one-icon.png")
    cols, px = quantise(render("v", SPLASH_W, SPLASH_H, SPLASH_K))   # (as the device shows it: the 16 colours)
    im = Image.new("RGB", (SPLASH_W, SPLASH_H))
    im.putdata([cols[p] for p in px])
    im.resize((SPLASH_W * 2, SPLASH_H * 2), Image.NEAREST).save(out / "forge-one-splash.png")
    print(f"logo: assets in {out}")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--assets":
        assets(sys.argv[2])
    elif len(sys.argv) == 2:
        splash_header(sys.argv[1])
    else:
        sys.exit(__doc__)
