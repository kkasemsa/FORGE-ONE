#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 the FORGE ONE contributors
"""The keyboard maps of the guides: one picture a layer (docs/keymaps/*.png), the FM-1's 27 keys (F3..G5: 16 white,
11 black) each labelled with what it does there, how to get into the layer, and what the knobs and buttons do.
The tables here follow the firmware (ui_layer.c's layer comments, ui_tools.c, ui_forge.c, ui_starter.c, ui_song.c);
tests/forge_test.c's maps are the reference for SONG.

  gen_keymaps.py [OUTDIR]     (default docs/keymaps)

A key's label: up to three lines; "D4: .." and "FX: .." lines are drawn as tags (what the key does with D4 or FX
held). Colours by kind: track (mute, solo, focus), effect, tool, sound, part / scene, setting.
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
FONT = ROOT / "assets/fonts/InterTight[wght].ttf"
OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "docs/keymaps"

WHITE = ["F3", "G3", "A3", "B3", "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5", "D5", "E5", "F5", "G5"]
BLACK = ["F#3", "G#3", "A#3", "C#4", "D#4", "F#4", "G#4", "A#4", "C#5", "D#5", "F#5"]
# a black key sits after white key index: F#3 after F3 (0), G#3 after G3 (1), ...
BLACK_AFTER = [0, 1, 2, 4, 5, 7, 8, 9, 11, 12, 14]

KIND = {   # fill, ink
    "trk": ("#2f6fd1", "#ffffff"),    # tracks: mute, solo, focus, lock
    "fx": ("#e8892b", "#1a1a1a"),     # effects
    "tool": ("#2c9d5c", "#ffffff"),   # pattern tools, rerolls
    "snd": ("#8a4fd0", "#ffffff"),    # sounds, engines, presets
    "part": ("#d14a7a", "#ffffff"),   # parts, scenes, songs
    "set": ("#5d6b7a", "#ffffff"),    # settings, tempo, modes
    "note": ("#ececec", "#333333"),   # plays a note
    None: (None, None),
}

# ---------------------------------------------------------------------------------------------- the layers ---
# each: title, how to enter, white {key: (kind, label)}, black {key: (kind, label)}, knobs [4], notes [lines]
LAYERS = {
    "play": dict(
        title="Playing (no layer)",
        enter="Nothing held: the keys play the selected track (ALGORITHM picks it). OCT+ / OCT- move the octave.",
        white={k: ("note", k) for k in WHITE}, black={k: ("note", k) for k in BLACK},
        knobs=["the page's", "the page's", "the page's", "the page's"],
        notes=["SELECT: tempo (MENU > CONTROL > SELECT can make it PAGE, MODW or VEL)",
               "PRESETS: the selected track's sound    ALGORITHM: the selected track (T1..T4)"]),
    "fx": dict(
        title="FX layer: performance effects",
        enter="Hold FX (double-tap FX: PERFORM). With MENU > CONTROL > FX LATCH ON the keys toggle.",
        white={"F3": ("fx", "REPEAT 1/8"), "G3": ("fx", "REPEAT 1/16"), "A3": ("fx", "REPEAT 1/32"),
               "B3": ("fx", "REVERSE"), "C4": ("fx", "LPF"), "D4": ("fx", "HPF"), "E4": ("fx", "TAPE STOP"),
               "F4": ("fx", "FREEZE"), "G4": ("fx", "OCT UP"), "A4": ("fx", "OCT DN"), "B4": ("fx", "FLANGER"),
               "C5": ("fx", "PHASER"), "D5": ("fx", "REV THROW"), "E5": ("fx", "ECHO THROW"),
               "F5": ("fx", "CUTOFF THROW"), "G5": ("fx", "DIST THROW")},
        black={"F#3": ("trk", "MUTE T1 (held)"), "G#3": ("trk", "MUTE T2 (held)"), "A#3": ("trk", "MUTE T3 (held)"),
               "C#4": ("trk", "MUTE T4 (held)")},
        knobs=["FILTER", "CRUSH", "THROW", "DEPTH"],
        notes=["Each white key holds its effect while pressed. A white key held + PRESETS: give it another effect",
               "(DUCK too); + EDIT: its default back. FX + OCT-: every effect off."]),
    "perform": dict(
        title="PERFORM (FX locked)",
        enter="Double-tap FX. Tap FX again to leave.",
        white=None,   # (as the FX layer's)
        black={"F#3": ("trk", "MUTE T1\nOCT+: SOLO"), "G#3": ("trk", "MUTE T2\nOCT+: SOLO"),
               "A#3": ("trk", "MUTE T3\nOCT+: SOLO"), "C#4": ("trk", "MUTE T4\nOCT+: SOLO"),
               "D#4": ("part", "SCENE 1\n(hold: store)"), "F#4": ("part", "SCENE 2\n(hold: store)"),
               "G#4": ("part", "SCENE 3\n(hold: store)"), "A#4": ("part", "SCENE 4\n(hold: store)"), "C#5": ("set", "QUANT\nBAR / NOW"),
               "D#5": ("fx", "DUCK\non / off"), "F#5": ("set", "CLEAR\nall on, all off")},
        knobs=["T1 LEVEL", "T2 LEVEL", "T3 LEVEL", "T4 LEVEL"],
        notes=["Mutes and solos are latched (press again to undo); OCT+ held + black 1-4: SOLO. Scenes: tap recalls,",
               "hold 0.6 s stores. White keys: the FX layer's effects. OCT+ tap: the knobs FILTER CRUSH THROW DEPTH and",
               "back. OCT-: every effect and macro off."]),
    "glo": dict(
        title="GLO layer: the four tracks",
        enter="Hold GLO (double-tap: locked open).",
        white={"F3": ("trk", "SOLO T1 (held)"), "G3": ("trk", "SOLO T2 (held)"), "A3": ("trk", "SOLO T3 (held)"),
               "B3": ("trk", "SOLO T4 (held)"), "C4": ("trk", "UNMUTE ALL"), "D4": ("set", "MIDI LEARN"),
               "E4": ("set", "FILL (held)"), "F4": ("set", "TAP tempo")},
        black={"F#3": ("trk", "MUTE T1"), "G#3": ("trk", "MUTE T2"), "A#3": ("trk", "MUTE T3"),
               "C#4": ("trk", "MUTE T4")},
        knobs=["T1 LEVEL", "T2 LEVEL", "T3 LEVEL", "T4 LEVEL"],
        notes=["GLO + PLAY: restart from the top.  GLO + SELECT: tempo (even with BPM LOCK).",
               "Black keys: latched mutes (lit = sounding)."]),
    "scl": dict(
        title="SCL layer: key and scale",
        enter="Hold SCL (double-tap: locked open).",
        white={k: ("set", f"ROOT {k[:-1]}") for k in WHITE}, black={k: ("set", f"ROOT {k[:-1]}") for k in BLACK},
        knobs=["ROOT", "SCL", "CHRD", "VOIC"],
        notes=["Any key: its note name becomes the selected track's ROOT. The LEDs: the root lit, the scale's notes",
               "breathing."]),
    "edit": dict(
        title="EDIT layer: engines",
        enter="Hold EDIT (double-tap: locked open). A tap of EDIT opens the EDIT pages (EDIT 1: KIT on a DRUM track).",
        white={"F3": ("snd", "ANALOG"), "G3": ("snd", "FM6"), "A3": ("snd", "PHASE"), "B3": ("snd", "LOFI"),
               "C4": ("snd", "SAMPLE"), "D4": ("snd", "VOICE"), "E4": ("snd", "TRIO"), "F4": ("snd", "WHEEL"),
               "G4": ("snd", "GRAIN"), "A4": ("snd", "PHYS"), "B4": ("snd", "NOISE"), "C5": ("snd", "SLICE"),
               "D5": ("snd", "DRUM"), "E5": ("set", "INIT\n(asks)"), "F5": ("set", "CLEAN INIT\n(asks)")},
        black={b: ("trk", f"DRUM: mute\n{n}") for b, n in zip(BLACK[:8], ["KICK", "SNARE", "CLAP", "HATCL", "HATOP",
                                                                         "TOM", "RIM", "BELL"])},
        knobs=["ENG", "No. (sound)", "FAV", ""],
        notes=["A white key loads that engine on the selected track (the steps stay; SAVE held undoes).",
               "Black keys 1-8 on a DRUM track: its lanes' mutes, live only."]),
    "seq": dict(
        title="SEQ TOOLS: pattern tools",
        enter="Hold SEQ (double-tap: locked open). On any page.",
        white={"F3": ("tool", "CLEAR"), "G3": ("tool", "REVERSE"), "A3": ("tool", "SHIFT <"),
               "B3": ("tool", "SHIFT >"), "C4": ("tool", "RANDOM"), "D4": ("tool", "COOK"),
               "E4": ("tool", "BEAT\n(DRUM)"), "F4": ("tool", "EUCLID"), "G4": ("tool", "lane CLEAR\n(DRUM)"),
               "A4": ("tool", "lane REV\n(DRUM)"), "B4": ("tool", "lane FILL\n(DRUM)"), "C5": ("tool", "lane RND\n(DRUM)"),
               "D5": ("tool", "DOUBLE"), "E5": ("tool", "HALF"), "F5": ("snd", "FORGE"), "G5": ("part", "STARTER")},
        black={b: ("trk", f"lane {n}\n(DRUM)") for b, n in zip(BLACK[:8], ["KICK", "SNARE", "CLAP", "HATCL", "HATOP",
                                                                          "TOM", "RIM", "BELL"])},
        knobs=["LEN", "DIV", "SWING", "GATE"],
        notes=["Each tool is one undo (SAVE held); OCT- puts the track back as the layer opened. Not while a SONG plays.",
               "EUCLID open: K1 HITS, K2 STEPS, K3 ROT; OCT+ keeps it, OCT- cancels."]),
    "forge": dict(
        title="FORGE (SEQ + F5)",
        enter="In SEQ TOOLS press F5. OCT+ (or F5 again) keeps the pattern, OCT- puts the track back.",
        white={"C4": ("tool", "NOTES\n(DRUM: low)"), "D4": ("tool", "RHYTHM\n(DRUM: hats)"),
               "F5": ("snd", "FORGE\nkeep")},
        black={},
        knobs=["SEED", "DENSITY", "EVOLVE", "STYLE"],
        notes=["C4: new notes (the rhythm kept), D4: a new rhythm; on a DRUM track C4 the low lanes, D4 the hats.",
               "A synth track: a melody; a bass, a pad: their own kind of line. A DRUM track: a beat.",
               "Another SEQ tool key keeps the FORGE pattern, then runs that tool."]),
    "starter": dict(
        title="STARTER (SEQ + G5)",
        enter="In SEQ TOOLS press G5. OCT+ (or G5 again) keeps all four tracks, OCT- puts them back.",
        white={"C4": ("tool", "new NOTES\nall four"), "D4": ("tool", "new RHYTHMS\nall four"),
               "G5": ("part", "STARTER\nkeep")},
        black={"F#3": ("part", "VARIATION 1"), "G#3": ("part", "VARIATION 2"), "A#3": ("part", "VARIATION 3"),
               "C#4": ("part", "VARIATION 4"), "F#5": ("part", "SONG\n(spec v0.7)")},
        knobs=["SEED", "DENSITY", "EVOLVE", "GENRE"],
        notes=["T1 BASS, T2 LEAD, T3 PAD, T4 DRUM: a sound and a pattern each, from one seed; 13 genres on K4."]),
    "song": dict(
        title="SONG: spec v0.8 (FORGE ONE 0.17)",
        enter="Hold SEQ 0.6 s (the last song), or STARTER + F#5. It stays open: SEQ again leaves. OCT+ GENERATE writes A-D.",
        white={"F3": ("trk", "FOCUS T1\nD4: reroll\nFX: mute"), "G3": ("trk", "FOCUS T2\nD4: reroll\nFX: mute"),
               "A3": ("trk", "FOCUS T3\nD4: reroll\nFX: mute"), "B3": ("trk", "FOCUS T4\nD4: reroll\nFX: mute"),
               "C4": ("tool", "new NOTES\nD4: FILL"), "D4": ("set", "ALT\nheld: LEVELS"),
               "E4": ("fx", "REPEAT\n1/16"), "F4": ("fx", "LPF"), "G4": ("fx", "HPF"), "A4": ("fx", "TAPE\nSTOP"),
               "B4": ("fx", "REV\nTHROW"), "C5": ("fx", "ECHO\nTHROW"), "D5": ("part", "SCENE 1\nhold: store"),
               "E5": ("part", "SCENE 2\nhold: store"), "F5": ("set", "TAP\nhold: CLICK"), "G5": ("part", "STARTER\nkeep + out")},
        black={"F#3": ("part", "PART A\nhold: FAV1\nD4: load"), "G#3": ("part", "PART B\nhold: FAV2\nD4: load"),
               "A#3": ("part", "PART C\nhold: FAV3\nD4: load"), "C#4": ("part", "PART D\nhold: FAV4\nD4: load"),
               "D#4": ("trk", "LOCK T1"), "F#4": ("trk", "LOCK T2"), "G#4": ("trk", "LOCK T3"), "A#4": ("trk", "LOCK T4"),
               "C#5": ("fx", "CLEAR"), "D#5": ("fx", "DUCK"), "F#5": ("part", "SONG off\nD4: CHAIN")},
        knobs=["SEED / focus 1 / D4: T1 LVL", "DENSITY / focus 2 / D4: T2", "EVOLVE / focus 3 / D4: T3", "GENRE / focus 4 / D4: T4"],
        notes=["SELECT: tempo (FX: x5).  ALGORITHM: key (FX: scale; D4: CHAIN bars).  PRESETS: every sound of the focus",
               "track's category (T4: the drum list), at the bar; FX + PRESETS: the next category.  OCT- twice: UNDO.",
               "D4 + C4 held: a live FILL on T4; tapped: a full-bar FILL at the bar end.  Every action: a box for 1 s."]),
    "rec": dict(
        title="REC layer: recording settings",
        enter="Hold REC (a tap of REC still arms the track).",
        white={"F3": ("tool", "CLEAR\ntrack"), "G3": ("set", "CLICK\nOFF REC ON"), "A3": ("set", "COUNT-IN\nOFF 1 2"),
               "B3": ("set", "CLICK LEVEL\nLOW MID HIGH")},
        black={},
        knobs=["CLICK", "COUNT-IN", "LEVEL", ""],
        notes=["F3 clears the selected track's steps and automation (one undo, SAVE held)."]),
    "grid": dict(
        title="DRUM grid (SEQ > STEP on a DRUM track)",
        enter="Select the DRUM track, tap SEQ: the STEP page is a grid.",
        white={k: ("note", f"step {i + 1}") for i, k in enumerate(WHITE)},
        black={**{b: ("trk", f"lane\n{n}") for b, n in zip(BLACK[:8], ["KICK", "SNARE", "CLAP", "HATCL", "HATOP",
                                                                        "TOM", "RIM", "BELL"])},
               "C#5": ("set", "ACCENT"), "D#5": ("set", "PAGE <"), "F#5": ("set", "PAGE >")},
        knobs=["the page's", "", "", ""],
        notes=["A white key toggles the selected lane's hit on that step (16 a page); black keys 1-8 pick the lane."]),
}
LAYERS["perform"]["white"] = LAYERS["fx"]["white"]

# ------------------------------------------------------------------------------------------------- drawing ---
SC = 2                                   # drawn at 2x, saved at 1x (crisp text)
W = 1600
KW, KH = 170, 316                        # a white key (two rows: F3..E4, then F4..G5, so a page's width holds them large)
BW, BH = 112, 168                        # a black key
X0 = 32
ROWS = [(0, 7), (7, 16)]                 # white key ranges of the two rows


def font(size, weight):
    f = ImageFont.truetype(str(FONT), size * SC)
    try:
        f.set_variation_by_axes([weight])
    except Exception:
        pass
    return f


F_T, F_E, F_K, F_L, F_S, F_N = font(36, 700), font(21, 500), font(24, 650), font(17, 600), font(18, 600), font(20, 500)


def wrap(d, text, f, maxw):
    lines = []
    for para in text.split("\n"):
        cur = ""
        for w in para.split(" "):
            t = (cur + " " + w).strip()
            if d.textlength(t, font=f) <= maxw * SC or not cur:
                cur = t
            else:
                lines.append(cur)
                cur = w
        lines.append(cur)
    return lines


CUT = []                                 # labels that did not fit (the build fails on one)


def label(d, x, y, w, text, kind, f, maxh):
    """the label's lines centred in w from y: a plain line in the kind's ink, D4: / FX: lines as dark tags"""
    fill, ink = KIND[kind]
    yy = y
    for raw in text.split("\n"):
        tag = raw.startswith(("D4:", "FX:", "OCT+:"))
        f2 = F_S if tag else f
        for ln in wrap(d, raw, f2, w - 8):
            lw = d.textlength(ln, font=f2) / SC
            h = f2.size // SC + 7
            if yy + h > y + maxh:
                CUT.append(text.replace("\n", " / "))
                return
            lx = x + (w - lw) / 2
            if tag:
                d.rounded_rectangle([(lx - 4) * SC, yy * SC, (lx + lw + 4) * SC, (yy + h - 1) * SC], radius=5 * SC,
                                    fill="#222831")
                d.text((lx * SC, (yy + 2) * SC), ln, font=f2, fill="#ffd27a")
            else:
                d.text((lx * SC, (yy + 2) * SC), ln, font=f2, fill=ink if fill else "#444")
            yy += h + (2 if tag else 0)


def draw(name, L):
    notes = L.get("notes", [])
    y0 = 130
    H = y0 + 2 * (KH + 30) + 140 + 30 * (len(notes) + 1)
    im = Image.new("RGB", (W * SC, H * SC), "#f7f7f5")
    d = ImageDraw.Draw(im)
    d.text((X0 * SC, 22 * SC), L["title"], font=F_T, fill="#1b1b1b")
    for i, ln in enumerate(wrap(d, "How to get here: " + L["enter"], F_E, W - 2 * X0)):
        d.text((X0 * SC, (78 + 28 * i) * SC), ln, font=F_E, fill="#444")
    for r, (w0, w1) in enumerate(ROWS):
        Y0 = y0 + r * (KH + 30)
        for i in range(w0, w1):                         # the white keys
            k = WHITE[i]
            x = X0 + (i - w0) * KW
            kind, text = L["white"].get(k, (None, "")) if L["white"] else (None, "")
            d.rounded_rectangle([x * SC, Y0 * SC, (x + KW - 5) * SC, (Y0 + KH) * SC], radius=10 * SC, fill="#ffffff",
                                outline="#9a9a9a", width=2 * SC)
            if kind and kind != "note":
                d.rounded_rectangle([(x + 6) * SC, (Y0 + BH + 6) * SC, (x + KW - 11) * SC, (Y0 + KH - 28) * SC],
                                    radius=7 * SC, fill=KIND[kind][0])
            label(d, x + 6, Y0 + BH + 10, KW - 17, text, kind or "note", F_K, KH - BH - 40)
            kl = d.textlength(k, font=F_S) / SC
            d.text(((x + (KW - 5 - kl) / 2) * SC, (Y0 + KH - 25) * SC), k, font=F_S, fill="#777")
        for b, k in enumerate(BLACK):                   # the black keys of this row
            a = BLACK_AFTER[b]
            if not (w0 <= a < w1 - 1):
                continue
            x = X0 + (a - w0 + 1) * KW - BW // 2 - 3
            kind, text = L["black"].get(k, (None, "")) if L["black"] else (None, "")
            d.rounded_rectangle([x * SC, Y0 * SC, (x + BW) * SC, (Y0 + BH) * SC], radius=8 * SC, fill="#262626")
            if kind:
                fill, ink = KIND[kind]
                d.rounded_rectangle([(x + 5) * SC, (Y0 + 28) * SC, (x + BW - 5) * SC, (Y0 + BH - 7) * SC],
                                    radius=6 * SC, fill=fill if kind != "note" else "#3a3a3a")
                label(d, x + 5, Y0 + 32, BW - 10, text, kind if kind != "note" else None, F_S, BH - 40)
            kl = d.textlength(k, font=F_S) / SC
            d.text(((x + (BW - kl) / 2) * SC, (Y0 + 5) * SC), k, font=F_S, fill="#c8c8c8")
    y = y0 + 2 * (KH + 30) + 4                           # the knobs
    d.text((X0 * SC, y * SC), "KNOBS", font=F_L, fill="#666")
    for i, kn in enumerate(L["knobs"]):
        x = X0 + 100 + i * 370
        d.ellipse([x * SC, (y - 2) * SC, (x + 28) * SC, (y + 26) * SC], outline="#555", width=2 * SC)
        d.text(((x + 8) * SC, (y + 1) * SC), f"{i + 1}", font=F_S, fill="#555")
        d.text(((x + 38) * SC, (y + 1) * SC), "K%d  %s" % (i + 1, kn or "-"), font=F_N, fill="#1b1b1b")
    y += 48                                              # the legend
    x = X0
    for kind, name_ in (("trk", "tracks"), ("fx", "effects"), ("tool", "tools / rerolls"), ("snd", "sounds"),
                        ("part", "parts / scenes"), ("set", "settings")):
        d.rounded_rectangle([x * SC, y * SC, (x + 20) * SC, (y + 20) * SC], radius=4 * SC, fill=KIND[kind][0])
        d.text(((x + 27) * SC, y * SC), name_, font=F_S, fill="#555")
        x += 47 + int(d.textlength(name_, font=F_S) / SC)
    d.rounded_rectangle([x * SC, y * SC, (x + 64) * SC, (y + 22) * SC], radius=5 * SC, fill="#222831")
    d.text(((x + 7) * SC, (y + 1) * SC), "D4: ..", font=F_S, fill="#ffd27a")
    d.text(((x + 74) * SC, y * SC), "with that key / button held", font=F_S, fill="#555")
    y += 40
    for n in wrap(d, " ".join(notes), F_N, W - 2 * X0):
        d.text((X0 * SC, y * SC), n, font=F_N, fill="#333")
        y += 30
    im = im.resize((W, H), Image.LANCZOS)
    OUT.mkdir(parents=True, exist_ok=True)
    im.save(OUT / f"{name}.png", optimize=True)


if __name__ == "__main__":
    for n, L in LAYERS.items():
        draw(n, L)
    if CUT:
        sys.exit("labels cut: " + "; ".join(CUT))
    print(f"wrote {len(LAYERS)} keyboard maps to {OUT}")
