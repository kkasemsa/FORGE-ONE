/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the FORGE ONE contributors */
/* FORGE ONE 0.6: SEQ TOOLS G5 STARTER (TL_STARTER, ui_tools.c), a song starter: one press writes all four tracks, the
 * sound and the pattern of each, from one SEED. Included by ui_layer.c after ui_forge.c, whose generator it drives.
 *
 * The parts: T1 BASS, T2 LEAD, T3 PAD, T4 DRUM. 0.8: 45 starters in 13 genres (FS_GENRE, KNOB 4), 1..4 variations
 * each (black keys 1..4 while open). Each names a factory preset for each part (engine and preset by name, resolved
 * at load: tests/forge_test.c checks they all exist), a row of forge_style_t for each melodic part (fg_ovr) and a beat
 * for the drums (FORGE_BEAT or FS_BEAT_X: fg_bovr), each part's LEN, a SCALE (used when the selected track's SCALE is
 * CHR; else that one), a tempo (not with CLOCK on an external clock) and a SWG. ROOT: the selected track's.
 *
 * G5 opens it (any track; a song playing: "STOP TO EDIT"): the four tracks are copied (sound, parameters, steps, FM6
 * patch, the automation), the starter's sounds loaded and the patterns written at once, heard while playing. KNOB 1
 * SEED, 2 DENSITY (50..150 % of each part's own), 3 EVOLVE, 4 GENRE (wraps; loads its sounds); C4 / D4 reroll the
 * notes / rhythms of all four. OCT+ (or G5 again, another tool, the layer let go) applies it, OCT- puts all four
 * tracks back as they were. The one SAVE-held undo copy can hold one track only, so after STARTER it holds nothing:
 * OCT- is its undo. The mix (LEVEL PAN MUTE) stays as it was: a sound load never changes it */

enum { FP_BASS, FP_LEAD, FP_PAD, FP_DRUM, FP_N };
typedef struct { const char *eng, *preset; } fs_sound_t;
typedef struct {
    const char *name;            /* the variation (black key 1..4 picks it; its cell on the screen) */
    fs_sound_t snd[FP_N];
    forge_style_t part[3];       /* BASS LEAD PAD (name / card unused) */
    uint8_t len[FP_N], scale, bpm, beat, swing, dvel;   /* beat: FS_BEAT; swing: SWG of all four; dvel: the drums' VEL range */
} forge_starter_t;
typedef struct { const char *name, *card; uint8_t first, n; } fs_genre_t;   /* n variations from FORGE_STARTER[first] */

/* 0.8: the drum beats, FORGE's eight (FORGE_BEAT) and six of the starters' own (as forge_beat_t: core / optional hits a
 * lane, bit i = step i + 1, DENSITY's default, ratchet %) */
enum { FB_DEEPH = FS_N, FB_BREAK, FB_LOFI, FB_SYNTHW, FB_POPS, FB_ELECTRO, FB_N };
static const forge_beat_t FS_BEAT_X[FB_N - FS_N] = {
    /*          KICK    SNARE   CLAP    HATC    HATO    TOM     RIM     BELL     (core)  */
    {{0x1111, 0x0000, 0x1010, 0x0000, 0x4444, 0x0000, 0x0000, 0x0000},   /* DEEP HOUSE: four, clap 2 4, open hat off */
     {0x0080, 0x0000, 0x0000, 0xAAAA, 0x0000, 0x0000, 0x0202, 0x0000}, 70, 5},
    {{0x0405, 0x1010, 0x0000, 0x5555, 0x0000, 0x0000, 0x0000, 0x0000},   /* BREAKBEAT: kick 1, 3, 11; snare 2 4 */
     {0x0040, 0x8280, 0x0000, 0xAAAA, 0x0000, 0x0000, 0x0000, 0x0000}, 75, 10},
    {{0x0481, 0x1010, 0x0000, 0x5555, 0x0000, 0x0000, 0x0000, 0x0000},   /* LO-FI: boom bap, lazy */
     {0x0020, 0x0000, 0x0000, 0xAAAA, 0x0000, 0x0000, 0x0202, 0x0000}, 60, 0},
    {{0x0101, 0x1010, 0x1010, 0x5555, 0x0000, 0x0000, 0x0000, 0x0000},   /* SYNTHWAVE: kick 1 3, big snare 2 4 */
     {0x0808, 0x0000, 0x0000, 0xAAAA, 0x0000, 0x8000, 0x0000, 0x0000}, 75, 5},
    {{0x0901, 0x0000, 0x0000, 0x5555, 0x0000, 0x0000, 0x2424, 0x0000},   /* POPS: an early rhythm box, claves */
     {0x0000, 0x1010, 0x0000, 0xAAAA, 0x0000, 0x8000, 0x0000, 0x0101}, 60, 0},
    {{0x0841, 0x1010, 0x0000, 0x5555, 0x0000, 0x0000, 0x0000, 0x0000},   /* ELECTRO: kick 1, 7, 12 */
     {0x0004, 0x0000, 0x1010, 0xAAAA, 0x0000, 0x0080, 0x0202, 0x0000}, 70, 10},
};
static const forge_beat_t *fs_beat(uint32_t b) { return b < FS_N ? &FORGE_BEAT[b] : &FS_BEAT_X[(b - FS_N) % (FB_N - FS_N)]; }

/* the parts' rows (forge_style_t): rhythm accent motif dens base contour span tie chord ch% slide oct orn rat vel */
#define R(rh, ac, mo, de, ba, co, sp, ti, ch, cp, sl, oc, orn, ra, ve) {"", "", rh, ac, mo, de, ba, co, sp, ti, ch, cp, sl, oc, orn, ra, ve}
/* basses */
#define B_PED16 R(0xFFFF, 0x0101,  8,  90, 36, FC_PEDAL, 3,   0, 0,   0,  0, 25, 10,  0, 18)   /* a 16th sequence, octaves */
#define B_OFF   R(0x4444, 0x0000, 16, 100, 36, FC_PEDAL, 2,   0, 0,   0,  0,  0,  0,  0, 10)   /* the off-beat */
#define B_OFF5  R(0x4444, 0x0000, 16,  85, 36, FC_ROOT5, 3,  20, 0,   0,  0, 10, 10,  0, 14)
#define B_ROLL  R(0xEEEE, 0x2222, 16,  85, 36, FC_ROOT5, 3,   0, 0,   0,  0,  5, 10,  0, 16)   /* rolling 16ths */
#define B_8TH5  R(0x5555, 0x0101, 16,  90, 36, FC_ROOT5, 3,   0, 0,   0,  0, 20,  5,  0, 14)
#define B_SUB   R(0x4545, 0x0001, 16,  60, 36, FC_PEDAL, 2,  50, 0,   0,  0,  0, 10,  0,  8)   /* sparse, held */
#define B_DRONE R(0x0001, 0x0000, 16, 100, 36, FC_PEDAL, 2, 100, 0,   0,  0,  0,  0,  0,  6)
#define B_ACID  R(0xFFFF, 0x2224, 16,  75, 36, FC_PEDAL, 4,   0, 0,   0, 25, 30, 10, 15, 24)
#define B_ACID2 R(0xFFFF, 0x2224, 16,  85, 36, FC_WALK,  5,   0, 0,   0, 30, 35, 10, 15, 26)
#define B_DEEPH R(0x2484, 0x0004, 16,  90, 36, FC_ROOT5, 3,  10, 0,   0,  0, 10, 10,  0, 12)
#define B_BREAK R(0x0409, 0x0001, 16,  95, 36, FC_PEDAL, 3,  20, 0,   0, 10, 10, 10,  0, 16)
#define B_LOFI  R(0x0481, 0x0001, 16,  95, 36, FC_ROOT5, 3,  30, 0,   0,  0,  0, 10,  0, 10)
#define B_OCT16 R(0xFFFF, 0x0101,  8,  95, 36, FC_PEDAL, 2,   0, 0,   0,  0, 50,  0,  0, 14)   /* octave 16ths */
#define B_ELEC  R(0x4A49, 0x0001, 16,  90, 36, FC_ROOT5, 3,  10, 0,   0, 10, 15, 10,  0, 16)
/* leads */
#define L_HISEQ R(0xFFFF, 0x0101, 16,  55, 60, FC_ARC,   5,  30, 0,   0,  0, 10, 15,  0, 16)
#define L_STAB4 R(0x4949, 0x0001, 16,  70, 60, FC_WALK,  3,  20, 4,  70,  0,  5, 15,  0, 16)
#define L_STAB3 R(0x4444, 0x0000, 16,  55, 48, FC_WALK,  2,  60, 3, 100,  0,  0, 20,  0, 10)
#define L_HOUSE R(0x0909, 0x0101, 16,  80, 60, FC_WALK,  2,  10, 4, 100,  0,  0, 10,  0, 16)   /* piano / organ stabs */
#define L_ARC8  R(0x5555, 0x0101, 16,  75, 60, FC_ARC,   7,  25, 0,   0,  0,  5, 15,  0, 18)
#define L_ARP8  R(0x5555, 0x0101,  8,  60, 60, FC_CHORD, 7,   0, 0,   0,  0, 10, 10,  0, 14)
#define L_ARP16 R(0xFFFF, 0x1111,  8,  90, 60, FC_CHORD, 7,   0, 0,   0,  0, 10, 10, 10, 20)
#define L_WALK  R(0xFFFF, 0x0101, 16,  80, 60, FC_WALK,  5,   0, 0,   0,  0, 10, 15, 10, 20)
#define L_RIFF5 R(0x5555, 0x0101,  8,  90, 48, FC_ROOT5, 7,  10, 0,   0,  0,  0,  5,  0, 16)
#define L_SPARS R(0x0441, 0x0000, 16,  50, 60, FC_WIDE,  9,  80, 3,  30,  0,  0, 40,  0, 12)
#define L_SLOW  R(0x1111, 0x0000, 16,  75, 60, FC_ARC,   7,  60, 0,   0, 20,  0, 15,  0, 14)   /* long, gliding */
#define L_DOTS  R(0x4040, 0x0000, 16,  60, 60, FC_WIDE,  7,  10, 0,   0,  0,  0, 30,  0, 12)   /* a note here and there */
#define L_PLUCK R(0xFFFF, 0x0101, 16,  70, 60, FC_WALK,  4,   0, 0,   0,  0, 15, 15,  0, 18)
#define L_RIDE  R(0x5555, 0x1111, 16,  85, 60, FC_ARC,   7,  15, 0,   0,  0, 10, 10,  0, 18)   /* uplifting 8ths */
/* pads */
#define P_HALF  R(0x0101, 0x0000, 16, 100, 48, FC_WALK,  3,  95, 3, 100,  0,  0,  0,  0,  6)   /* a chord a half bar */
#define P_HALF4 R(0x0101, 0x0000, 16, 100, 48, FC_WALK,  2,  60, 4, 100,  0,  0, 10,  0,  8)
#define P_BAR   R(0x0001, 0x0000, 16, 100, 48, FC_WALK,  2,  98, 3, 100,  0,  0,  0,  0,  6)   /* a chord a bar */
#define P_WIDE  R(0x0001, 0x0000, 16, 100, 48, FC_WIDE,  3, 100, 3, 100,  0,  0,  0,  0,  6)
#define P_STAB4 R(0x4949, 0x0001, 16,  70, 48, FC_WALK,  2,  20, 4,  80,  0,  0, 10,  0, 14)
#define P_KEYS  R(0x0909, 0x0101, 16,  80, 48, FC_WALK,  2,  30, 4, 100,  0,  0, 10,  0, 14)

/* SCALE numbers (seq.c SCALE_MASK): 1 MAJ, 2 MIN, 3 DOR, 7 HARM, 8 PHRY, 9 LYD. Each: its four sounds; BASS LEAD PAD rows;
 * LEN T1..T4, SCALE, BPM, beat, SWG, the drums' VEL range. At most one heavy engine (GRAIN, PHYS) a starter */
#define SND(a, b, c, d, e, f, g, h) {{a, b}, {c, d}, {e, f}, {g, h}}
static const forge_starter_t FORGE_STARTER[] = {
    /* BERLIN (0.6's BERLIN first) */
    {"SEQ",   SND("ANALOG", "SQR BASS", "TRIO", "ARP LEAD", "GRAIN", "CLOUD PAD", "DRUM", "DEEP KIT"),
     {B_PED16, L_HISEQ, P_HALF}, {16, 16, 32, 16}, 2, 120, FS_BERLIN, 0, 20},
    {"DRIFT", SND("LOFI", "WAVE BASS", "PHASE", "RESO", "ANALOG", "STRINGS", "DRUM", "PERC KIT"),
     {B_PED16, L_ARC8, P_BAR}, {16, 32, 32, 16}, 2, 108, FS_AMBIENT, 0, 12},
    {"CHOIR", SND("FM6", "FM BASS", "TRIO", "SYNC LEAD", "VOICE", "CHOIR AAH", "DRUM", "DEEP KIT"),
     {B_PED16, L_SLOW, P_BAR}, {16, 32, 64, 16}, 7, 116, FS_BERLIN, 0, 16},
    {"ARP",   SND("TRIO", "FAT BASS", "LOFI", "STEP LEAD", "ANALOG", "SOFT PAD", "DRUM", "DRUM KIT"),
     {B_PED16, L_ARP16, P_HALF}, {16, 16, 32, 16}, 3, 124, FS_BERLIN, 0, 20},
    /* DETROIT */
    {"STABS", SND("FM6", "FM BASS", "ANALOG", "PWM STR", "FM6", "TINE EP", "DRUM", "TIGHT KIT"),
     {B_OFF5, L_STAB4, P_HALF4}, {16, 16, 32, 16}, 3, 126, FS_DETROIT, 0, 16},
    {"STRNG", SND("ANALOG", "SUB BASS", "ANALOG", "STRINGS", "SAMPLE", "PIANO", "DRUM", "DRUM KIT"),
     {B_OFF, L_HOUSE, P_STAB4}, {16, 16, 16, 16}, 2, 128, FS_DETROIT, 0, 16},
    {"ELECT", SND("TRIO", "FAT BASS", "LOFI", "PULSE LD", "PHASE", "STRING", "DRUM", "TIGHT KIT"),
     {B_ELEC, L_WALK, P_BAR}, {16, 16, 32, 16}, 8, 125, FB_ELECTRO, 0, 18},
    {"DEEP",  SND("FM6", "FM BASS", "TRIO", "ARP LEAD", "ANALOG", "SOFT PAD", "DRUM", "DEEP KIT"),
     {B_ROLL, L_ARP8, P_HALF}, {16, 16, 32, 16}, 2, 124, FS_MELODIC, 0, 14},
    /* DUB TECHNO (0.6's DUB first) */
    {"CHORD", SND("ANALOG", "SUB BASS", "PHASE", "STRING", "GRAIN", "SHIMMER", "DRUM", "DEEP KIT"),
     {B_SUB, L_STAB3, P_BAR}, {16, 16, 32, 16}, 3, 118, FS_DUB, 0, 10},
    {"DEEP",  SND("VOICE", "WOW BASS", "FM6", "PAD", "GRAIN", "CLOUD PAD", "DRUM", "DEEP KIT"),
     {B_SUB, L_STAB3, P_WIDE}, {16, 16, 64, 16}, 2, 120, FS_DUB, 0, 10},
    {"MINI",  SND("ANALOG", "SUB BASS", "PHASE", "RESO", "ANALOG", "SOFT PAD", "DRUM", "TIGHT KIT"),
     {B_SUB, L_DOTS, P_BAR}, {16, 16, 32, 16}, 3, 124, FS_DUB, 0, 12},
    {"ECHO",  SND("ANALOG", "SUB BASS", "FM6", "TINE EP", "GRAIN", "FROZEN", "DRUM", "PERC KIT"),
     {B_SUB, L_STAB3, P_WIDE}, {16, 32, 64, 16}, 2, 116, FS_DUB, 0, 8},
    /* MELODIC TECHNO (0.6's MELODIC first) */
    {"ARC",   SND("TRIO", "FAT BASS", "ANALOG", "SAW LEAD", "FM6", "PAD", "DRUM", "DRUM KIT"),
     {B_ROLL, L_ARC8, P_HALF}, {16, 32, 32, 16}, 2, 122, FS_MELODIC, 0, 18},
    {"ARP",   SND("ANALOG", "SQR BASS", "TRIO", "SYNC LEAD", "ANALOG", "STRINGS", "DRUM", "TIGHT KIT"),
     {B_ROLL, L_ARP16, P_HALF}, {16, 16, 32, 16}, 2, 124, FS_MELODIC, 0, 18},
    {"PIANO", SND("FM6", "FM BASS", "SAMPLE", "PIANO", "GRAIN", "CLOUD PAD", "DRUM", "DEEP KIT"),
     {B_8TH5, L_ARC8, P_BAR}, {16, 32, 64, 16}, 3, 122, FS_MELODIC, 0, 16},
    {"PLUCK", SND("TRIO", "FAT BASS", "ANALOG", "PLUCK", "VOICE", "CHOIR AAH", "DRUM", "DRUM KIT"),
     {B_ROLL, L_PLUCK, P_HALF}, {16, 16, 32, 16}, 2, 123, FS_MELODIC, 0, 18},
    /* ACID HOUSE */
    {"CLASS", SND("ANALOG", "ACID", "SAMPLE", "PIANO", "ANALOG", "SOFT PAD", "DRUM", "DRUM KIT"),
     {B_ACID, L_HOUSE, P_BAR}, {16, 16, 32, 16}, 2, 124, FB_DEEPH, 10, 20},
    {"JACK",  SND("ANALOG", "ACID", "ANALOG", "RAVE", "VOICE", "CHOIR AAH", "DRUM", "TIGHT KIT"),
     {B_ACID, L_STAB4, P_BAR}, {16, 16, 32, 16}, 3, 122, FB_DEEPH, 15, 20},
    {"FLOOR", SND("ANALOG", "ACID", "LOFI", "ARP 8BIT", "PHASE", "ORGAN", "DRUM", "DRUM KIT"),
     {B_ACID2, L_ARP8, P_HALF}, {16, 16, 32, 16}, 2, 128, FS_ACID, 0, 24},
    /* ACID TECHNO (0.6's ACID first) */
    {"ROLL",  SND("ANALOG", "ACID", "LOFI", "ARP 8BIT", "ANALOG", "SOFT PAD", "DRUM", "TIGHT KIT"),
     {B_ACID, L_ARP8, P_BAR}, {16, 16, 32, 16}, 2, 128, FS_ACID, 0, 24},
    {"WARE",  SND("ANALOG", "ACID", "LOFI", "STEP LEAD", "ANALOG", "PWM STR", "DRUM", "DRUM KIT"),
     {B_ACID2, L_DOTS, P_STAB4}, {16, 16, 16, 16}, 2, 135, FS_ACID, 0, 24},
    {"HARD",  SND("ANALOG", "ACID", "FM6", "BRASS", "PHASE", "WIRE", "DRUM", "TIGHT KIT"),
     {B_ACID2, L_STAB4, P_BAR}, {16, 16, 32, 16}, 8, 140, FS_ACID, 0, 26},
    /* TRANCE (0.6's TRANCE first) */
    {"GATE",  SND("ANALOG", "SUB BASS", "TRIO", "ARP LEAD", "ANALOG", "STRINGS", "DRUM", "DRUM KIT"),
     {B_OFF, L_ARP16, P_HALF}, {16, 16, 32, 16}, 2, 138, FS_TRANCE, 0, 20},
    {"LIFT",  SND("ANALOG", "SUB BASS", "ANALOG", "SAW LEAD", "VOICE", "CHOIR AAH", "DRUM", "DRUM KIT"),
     {B_OFF, L_RIDE, P_HALF}, {16, 32, 32, 16}, 2, 138, FS_TRANCE, 0, 20},
    {"PSY",   SND("ANALOG", "SQR BASS", "TRIO", "RING BELL", "PHASE", "RESO", "DRUM", "TIGHT KIT"),
     {B_ROLL, L_WALK, P_BAR}, {16, 16, 32, 16}, 8, 142, FS_TRANCE, 0, 22},
    /* DEEP HOUSE */
    {"KEYS",  SND("TRIO", "FAT BASS", "FM6", "TINE EP", "ANALOG", "SOFT PAD", "DRUM", "DEEP KIT"),
     {B_DEEPH, L_HOUSE, P_BAR}, {16, 16, 32, 16}, 3, 122, FB_DEEPH, 15, 14},
    {"ORGAN", SND("VOICE", "WOW BASS", "WHEEL", "JAZZ PERC", "ANALOG", "STRINGS", "DRUM", "DRUM KIT"),
     {B_DEEPH, L_HOUSE, P_HALF}, {16, 16, 32, 16}, 2, 120, FB_DEEPH, 20, 14},
    {"VOCAL", SND("ANALOG", "SUB BASS", "VOICE", "VOX LEAD", "FM6", "PAD", "DRUM", "DEEP KIT"),
     {B_DEEPH, L_SLOW, P_HALF}, {16, 32, 32, 16}, 2, 121, FB_DEEPH, 10, 14},
    /* SYNTHWAVE (0.6's SYNTH as NIGHT) */
    {"DRIVE", SND("ANALOG", "SQR BASS", "ANALOG", "SAW LEAD", "ANALOG", "PWM STR", "DRUM", "DRUM KIT"),
     {B_OCT16, L_ARC8, P_HALF}, {16, 32, 32, 16}, 2, 110, FB_SYNTHW, 0, 16},
    {"NIGHT", SND("TRIO", "FAT BASS", "ANALOG", "BRASS", "FM6", "PAD", "DRUM", "DRUM KIT"),
     {B_8TH5, L_RIFF5, P_HALF}, {16, 16, 32, 16}, 2, 98, FB_SYNTHW, 0, 16},
    {"8BIT",  SND("LOFI", "WAVE BASS", "LOFI", "PULSE LD", "TRIO", "CHIP CHOIR", "DRUM", "TIGHT KIT"),
     {B_OCT16, L_WALK, P_HALF}, {16, 16, 32, 16}, 1, 118, FB_SYNTHW, 0, 16},
    /* BREAKBEAT */
    {"AMEN",  SND("ANALOG", "SUB BASS", "FM6", "BRASS", "ANALOG", "STRINGS", "DRUM", "TIGHT KIT"),
     {B_BREAK, L_STAB4, P_BAR}, {16, 16, 32, 16}, 2, 134, FB_BREAK, 10, 22},
    {"NUSK",  SND("ANALOG", "ACID", "LOFI", "STEP LEAD", "PHASE", "STRING", "DRUM", "DRUM KIT"),
     {B_ACID, L_DOTS, P_BAR}, {16, 16, 32, 16}, 8, 135, FB_BREAK, 5, 22},
    {"ELECT", SND("TRIO", "FAT BASS", "TRIO", "SYNC LEAD", "FM6", "PAD", "DRUM", "TIGHT KIT"),
     {B_ELEC, L_RIFF5, P_HALF}, {16, 16, 32, 16}, 2, 128, FB_ELECTRO, 5, 20},
    /* AMBIENT (one heavy engine each: 0.6's AMBIENT's KALIMBA is now FM6 BELL beside the CLOUD PAD) */
    {"CLOUD", SND("ANALOG", "SUB BASS", "FM6", "BELL", "GRAIN", "CLOUD PAD", "DRUM", "PERC KIT"),
     {B_DRONE, L_SPARS, P_WIDE}, {32, 32, 64, 16}, 9, 80, FS_AMBIENT, 0, 12},
    {"DRONE", SND("PHYS", "DRONE STRING", "SAMPLE", "FLUTE", "ANALOG", "SOFT PAD", "DRUM", "PERC KIT"),
     {B_DRONE, L_SPARS, P_WIDE}, {64, 32, 64, 16}, 3, 72, FS_AMBIENT, 0, 10},
    {"GLASS", SND("ANALOG", "SUB BASS", "PHYS", "MARIMBA", "VOICE", "CHOIR AAH", "DRUM", "PERC KIT"),
     {B_DRONE, L_ARC8, P_WIDE}, {32, 32, 64, 16}, 9, 84, FS_AMBIENT, 0, 10},
    {"RAIN",  SND("FM6", "FM BASS", "TRIO", "RING BELL", "GRAIN", "FROZEN", "DRUM", "PERC KIT"),
     {B_DRONE, L_DOTS, P_WIDE}, {32, 32, 64, 16}, 2, 76, FS_AMBIENT, 0, 10},
    /* LO-FI */
    {"PIANO", SND("ANALOG", "SUB BASS", "SAMPLE", "PIANO", "ANALOG", "SOFT PAD", "DRUM", "DEEP KIT"),
     {B_LOFI, L_HOUSE, P_BAR}, {16, 16, 32, 16}, 3, 84, FB_LOFI, 25, 12},
    {"TAPE",  SND("TRIO", "FAT BASS", "FM6", "TINE EP", "WHEEL", "SOFT FLUTE", "DRUM", "DRUM KIT"),
     {B_LOFI, L_HOUSE, P_BAR}, {16, 16, 32, 16}, 2, 88, FB_LOFI, 20, 12},
    {"SAX",   SND("ANALOG", "SUB BASS", "SAMPLE", "SAX", "FM6", "PAD", "DRUM", "PERC KIT"),
     {B_LOFI, L_SLOW, P_BAR}, {16, 32, 32, 16}, 3, 80, FB_LOFI, 22, 12},
    /* JARRE */
    {"OXY",   SND("ANALOG", "SQR BASS", "ANALOG", "SAW LEAD", "ANALOG", "STRINGS", "DRUM", "PERC KIT"),
     {B_8TH5, L_SLOW, P_BAR}, {16, 32, 64, 16}, 2, 104, FB_POPS, 0, 10},
    {"EQUI",  SND("LOFI", "WAVE BASS", "TRIO", "ARP LEAD", "VOICE", "CHOIR AAH", "DRUM", "TIGHT KIT"),
     {B_PED16, L_ARP16, P_BAR}, {16, 16, 64, 16}, 2, 120, FB_ELECTRO, 0, 14},
    {"FIELD", SND("FM6", "FM BASS", "PHASE", "BRASS", "ANALOG", "PWM STR", "DRUM", "DRUM KIT"),
     {B_8TH5, L_STAB4, P_HALF}, {16, 16, 32, 16}, 3, 112, FB_POPS, 0, 12},
    {"ZOO",   SND("VOICE", "WOW BASS", "VOICE", "VOX LEAD", "GRAIN", "SHIMMER", "DRUM", "TIGHT KIT"),
     {B_ELEC, L_DOTS, P_WIDE}, {16, 16, 64, 16}, 8, 118, FB_ELECTRO, 0, 16},
};
#define FS_NSTART (sizeof FORGE_STARTER / sizeof FORGE_STARTER[0])
/* the genres (K4): their full names (the message), the card, the variations (black keys 1..4) */
static const fs_genre_t FS_GENRE[] = {
    {"BERLIN", "BERLIN", 0, 4}, {"DETROIT", "DETROIT", 4, 4}, {"DUB TECHNO", "DUB", 8, 4},
    {"MELODIC", "MELO", 12, 4}, {"ACID HOUSE", "ACID H", 16, 3}, {"ACID TECHNO", "ACID T", 19, 3},
    {"TRANCE", "TRANCE", 22, 3}, {"DEEP HOUSE", "DEEP H", 25, 3}, {"SYNTHWAVE", "SYNTH", 28, 3},
    {"BREAKBEAT", "BREAKS", 31, 3}, {"AMBIENT", "AMBI", 34, 4}, {"LO-FI", "LO-FI", 38, 3}, {"JARRE", "JARRE", 41, 4},
};
#define FS_NGENRE (sizeof FS_GENRE / sizeof FS_GENRE[0])
static const char *const FP_ROLE[FP_N] = {"BASS", "LEAD", "PAD", "DRUM"};
static const char H_STARTER[] = "[K4]GENRE[KEYS]BLACK 1-4: VARIATION";   /* (the second row: two fit) */

/* the four tracks as STARTER opened (OCT-): in the pool, as FORGE's eu_from */
static struct {
    int16_t p[NTRK][P_COUNT];
    step_t step[NTRK][NSTEP];
    uint8_t fm6[NTRK][FP_SIZE + 1u];
    uint8_t eng[NTRK], preset[NTRK], user[NTRK], fm6_slot[NTRK];
    motion_store_t mo;
    int16_t bpm;
} fs_from __attribute__((section(".pool")));
static struct { uint8_t on, set, genre, var, dens, root, scale; } fs;   /* dens: % of each part's own; var: of the genre */
static uint32_t fs_idx(void) { return (FS_GENRE[fs.genre % FS_NGENRE].first + fs.var) % FS_NSTART; }

/* engine and factory preset by name (0: found; the test checks every one is) */
static int fs_find(const fs_sound_t *s, uint32_t *e, uint32_t *k)
{
    for (*e = 0; *e < NENGINES; (*e)++)
        if (eng_ok(*e) && str_eq(ENGINES[*e]->name, s->eng))
            for (*k = 0; *k < ENGINES[*e]->npresets; (*k)++)
                if (preset_orig(ENGINES[*e], *k) == *k && str_eq(ENGINES[*e]->presets[*k].name, s->preset))
                    return 0;
    return 1;
}
static void fs_sounds(void)                             /* the starter's four sounds */
{
    uint32_t i, e, k;
    for (i = 0; i < FP_N; i++)
        if (!fs_find(&FORGE_STARTER[fs_idx()].snd[i], &e, &k)) {
            track_t *t = &trk[i];
            if (t->eng_req % NENGINES != e)
                set_engine_of(t, e);
            apply_preset_to(t, k);
        }
}
/* the patterns: each part's LEN and DIV 1/16, the key, then FORGE's generator with the part's row and stream */
static void fs_render(void)
{
    const forge_starter_t *s = &FORGE_STARTER[fs_idx()];
    uint8_t trk0 = fg.trk, style0 = fg.style, dens0 = fg.dens;
    uint32_t i;
    fg_bovr = fs_beat(s->beat);
    fg_bvd = s->dvel;
    for (i = 0; i < FP_N; i++) {
        track_t *t = &trk[i];
        uint32_t own = i < FP_DRUM ? s->part[i].dens : fg_bovr->dens;
        t->p[P_SLEN] = s->len[i];
        t->p[P_SDIV] = 2;
        t->p[P_SSWING] = s->swing;
        t->p[P_ROOT] = fs.root;
        t->p[P_SCALE] = fs.scale ? fs.scale : s->scale;
        fg.trk = (uint8_t)i;
        fg.style = (uint8_t)fs_idx();                   /* (the seed's streams: each starter its own) */
        fg.dens = (uint8_t)clamp((int32_t)(own * fs.dens / 100u), 10, 100);
        fg_role = (uint8_t)(i + 1u);
        fg_ovr = i < FP_DRUM ? &s->part[i] : 0;
        forge_render();
    }
    fg_role = 0;
    fg_ovr = 0;
    fg_bovr = 0;
    fg.trk = trk0;
    fg.style = style0;
    fg.dens = dens0;
}
static void fs_tempo(void)
{
    if (!song.g[G_CLOCK])
        song.g[G_BPM] = (int16_t)clamp(FORGE_STARTER[fs_idx()].bpm, 40, 240);
}
static void fs_say(void)                                /* "DUB DEEP 42": the genre's card, the variation, the seed */
{
    char b[24], n[4];
    str_cpy(b, FS_GENRE[fs.genre % FS_NGENRE].card, sizeof b);
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), FORGE_STARTER[fs_idx()].name, sizeof b - str_len(b));
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    fmt_int(n, fg.seed);
    str_cpy(b + str_len(b), n, sizeof b - str_len(b));
    ui_message(b);
}
static void fs_open(void)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        memcpy(fs_from.p[i], trk[i].p, sizeof fs_from.p[i]);
        memcpy(fs_from.step[i], trk[i].step, sizeof fs_from.step[i]);
        memcpy(fs_from.fm6[i], fm6_patch[i], sizeof fs_from.fm6[i]);
        fs_from.eng[i] = trk[i].eng_req;
        fs_from.preset[i] = trk[i].preset;
        fs_from.user[i] = trk[i].user;
        fs_from.fm6_slot[i] = fm6_slot[i];
    }
    fs_from.mo = motion;
    fs_from.bpm = song.g[G_BPM];
    fs.root = (uint8_t)(TSEL->p[P_ROOT] % 12);          /* the selected track's key */
    fs.scale = (uint8_t)TSEL->p[P_SCALE];               /* (CHR: the starter's own) */
    if (!fs.set) {                                      /* (the first since the power came: FORGE's SEED) */
        fs.set = 1;
        fs.genre = 0;                                   /* (BERLIN SEQ) */
        fs.var = 0;
        fs.dens = 100;
        if (!fg.set) {
            fg.set = 1;
            fg.seed = (uint8_t)(rng() % 100u);
            fg.style = FS_BERLIN;
            fg.evolve = 1;
            fg.dens = FORGE_STYLE[FS_BERLIN].dens;
        }
    }
    fg.nvar = fg.rvar = 0;
    fs.on = 1;
    fs_sounds();
    fs_tempo();
    fs_render();
    undo.trk = 0;                                       /* (the loads took a one-track copy: OCT- is the undo) */
    fs_say();
}
static void fs_knob(uint32_t k, int32_t s)
{
    if (k == 0u) {
        fg.seed = (uint8_t)(((int32_t)fg.seed + s % 100 + 100) % 100);
        fg.nvar = fg.rvar = 0;
    } else if (k == 1u) {
        fs.dens = (uint8_t)clamp((int32_t)fs.dens + 5 * s, 50, 150);
    } else if (k == 2u) {
        fg.evolve = (uint8_t)clamp((int32_t)fg.evolve + s, 0, 4);
    } else {
        fs.genre = (uint8_t)(((int32_t)fs.genre + s % (int32_t)FS_NGENRE + (int32_t)FS_NGENRE) % (int32_t)FS_NGENRE);
        fs.var = 0;                                     /* (a new genre: its first variation) */
        fg.nvar = fg.rvar = 0;
        fs_sounds();                                    /* (a new starter: its sounds and tempo) */
        fs_tempo();
        undo.trk = 0;
    }
    fs_render();
    fs_say();
}
/* 0.8: black key v (1..4) while open: the genre's variation v, its sounds, tempo and patterns */
static void fs_variation(uint32_t v)
{
    if (v >= FS_GENRE[fs.genre % FS_NGENRE].n)
        return;
    fs.var = (uint8_t)v;
    fg.nvar = fg.rvar = 0;
    fs_sounds();
    fs_tempo();
    undo.trk = 0;
    fs_render();
    fs_say();
}
static void fs_reroll(uint32_t rhythm)                  /* C4 / D4: the notes / the rhythms of all four */
{
    char b[24], n[4];
    if (rhythm)
        fg.rvar = (uint8_t)((fg.rvar + 1u) % 100u);
    else
        fg.nvar = (uint8_t)((fg.nvar + 1u) % 100u);
    fs_render();
    str_cpy(b, rhythm ? "NEW RHYTHMS " : "NEW NOTES ", sizeof b);
    fmt_int(n, rhythm ? fg.rvar : fg.nvar);
    str_cpy(b + str_len(b), n, sizeof b - str_len(b));
    ui_message(b);
}
/* OCT+ (apply: emptied steps lose their locks) or OCT- (cancel: the four tracks as they were, sound and all) */
static void fs_close(int apply)
{
    uint32_t i;
    if (!fs.on)
        return;
    fs.on = 0;
    if (apply) {
        for (i = 0; i < NTRK; i++)
            tl_unlock(&trk[i], tl_used_of(fs_from.step[i], NSTEP), NSTEP);
        return;
    }
    fm1_irq_off();                                      /* (as undo_swap: the audio ISR never sees half a sound) */
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        panic_req |= (uint8_t)(1u << i);
        t->eng_req = fs_from.eng[i];
        t->preset = fs_from.preset[i];
        t->user = fs_from.user[i];
        memcpy(t->p, fs_from.p[i], sizeof t->p);
        memcpy(t->step, fs_from.step[i], sizeof t->step);
    }
    motion = fs_from.mo;
    song.g[G_BPM] = fs_from.bpm;
    fm1_irq_on();
    for (i = 0; i < NTRK; i++) {                        /* (FM6: the patch as it was; one never changed stays as */
        if (memcmp(fm6_patch[i], fs_from.fm6[i], FP_SIZE)) {   /* it is: fm6_set_patch sanitizes) */
            fm6_set_patch(i, fs_from.fm6[i]);
            fm6_own_ok &= (uint8_t)~(1u << i);
        }
        fm6_slot[i] = fs_from.fm6_slot[i];
    }
    undo.trk = 0;
    sync_reload = 1;
}
static uint32_t fs_sig(void)
{
    return fs.on ? 17u + fg.seed * 13u + fs.dens * 1031u + fg.evolve * 65537u + (fs.genre * 4u + fs.var) * 524287u + fg.nvar * 97u +
                   fg.rvar * 7919u : 0u;
}

/* the open STARTER: the genre's variations (black keys 1..4, the one now selected), then a row a track: T1..T4, the
 * preset, the first 16 steps (a note or hit filled, a TIE half) */
static void layer_starter(void)
{
    const fs_genre_t *g = &FS_GENRE[fs.genre % FS_NGENRE];
    uint32_t i, j, e, k;
    for (i = 0; i < g->n && i < 4u; i++) {
        int32_t x = 6 + 58 * (int32_t)i;               /* (a cell a black key, F#3 G#3 A#3 C#4 in order) */
        uint16_t f = i == fs.var ? T_THEME : T_RAISE, ink = i == fs.var ? T_SURF : T_TEXT;
        cv_rrect(x, 4, 54, 16, 3, f, T_SURF);
        GFX_HOOK_ALIGN(x, 4, x + 54, 20, AL_HV, "starter variation cell centred");
        cv_text_in(x, 4 + CAP_IN(S, 16), 54, &AF_S, FORGE_STARTER[g->first + i].name, ink, f);
    }
    for (i = 0; i < FP_N; i++) {
        const track_t *t = &trk[i];
        int32_t y = 24 + 19 * (int32_t)i;
        char n[3] = {'T', (char)('1' + i), 0};
        const fs_sound_t *sd = &FORGE_STARTER[fs_idx()].snd[i];
        GFX_HOOK_ALIGN(0, y, 0, y + 17, AL_V, "starter row text on its middle");
        cv_text_on(8, y + CAP_IN(S, 17), &AF_S, n, T_MID, T_SURF);
        GFX_HOOK_ALIGN(0, y, 0, y + 17, AL_V, "starter row text on its middle");
        cv_text_on(26, y + CAP_IN(S, 17), &AF_S, fs_find(sd, &e, &k) ? FP_ROLE[i] : sd->preset, T_TEXT, T_SURF);
        for (j = 0; j < 16u; j++) {
            const step_t *s = &t->step[j];
            int on = drum_track(t) ? step_lanes(s) != 0u : s->time == ST_NOTE && s->n;
            int32_t x = 122 + 7 * (int32_t)j;
            uint16_t c = j >= tl_len(t) ? T_SURF : on ? T_THEME : s->time == ST_TIE ? T_MID : T_RAISE;
            cv_rect(x, y + 2, 6, 13, c);
        }
    }
    {   /* the rerolls on the keys, under the rows */
        char a[16], b[16], n[4];
        str_cpy(a, "C4 NOTES ", sizeof a);
        fmt_int(n, fg.nvar);
        str_cpy(a + str_len(a), n, sizeof a - str_len(a));
        str_cpy(b, "D4 RHYTHM ", sizeof b);
        fmt_int(n, fg.rvar);
        str_cpy(b + str_len(b), n, sizeof b - str_len(b));
        cv_text_on(10, 100 + CAP_IN(S, 18), &AF_S, a, fg.nvar ? T_THEME : T_MID, T_SURF);
        cv_text_r(230, 100 + CAP_IN(S, 18), &AF_S, b, fg.rvar ? T_THEME : T_MID, T_SURF);
    }
}
static void starter_cards(void)
{
    char val[8];
    fmt_int(val, fg.seed);
    draw_column(0, "SEED", val, "", VAL(0u), fg.seed * 1000 / 99, ICON_PROB);
    fmt_int(val, fs.dens);
    draw_column(1, "DENSITY", val, "%", VAL(1u), (fs.dens - 50) * 10, ICON_STEPS);
    fmt_int(val, fg.evolve);
    draw_column(2, "EVOLVE", val, "", fg.evolve ? VAL(2u) : T_DIM, fg.evolve * 250, ICON_W_SH);
    draw_column(3, "GENRE", FS_GENRE[fs.genre % FS_NGENRE].card, "", VAL(3u), fs.genre * 1000 / (FS_NGENRE - 1u),
                ICON_X_SONG);
}
