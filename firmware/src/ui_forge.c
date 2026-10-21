/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the Felucca-FORGE contributors */
/* Felucca-FORGE: SEQ TOOLS F5 FORGE (TL_FORGE, ui_tools.c), a seeded motif generator for the selected melodic track.
 * Included by ui_layer.c after ui_tools.c.
 *
 * F5 opens it as EUCLID opens (the undo copy is taken, one undo for all of it, and the pattern is written at once, heard
 * while playing); KNOB 1..4 SEED (0..99) DENSITY (10..100 %) EVOLVE (0..4) STYLE (8) rewrite it from the steps as it
 * opened. OCT+ (or F5 again, another tool, the layer let go) applies it, OCT- cancels it. The values stay for the next
 * one (until the power goes); the first since the power came takes a random SEED.
 *
 * What it writes: steps 1..LEN of the track (past LEN stays), only the fields a project already holds (notes, NOTE /
 * TIE / REST, accent, slide, RATCH, CHANCE or a condition; VEL 96): no format change. The same SEED, STYLE, DENSITY,
 * EVOLVE, LEN, ROOT and SCALE always write the same steps.
 *
 * How (forge_render): a motif of the style's length (8 or 16 steps, at most LEN):
 *   pitch    a scale degree for every step from the style's contour (WALK, ARC, CHORD tones, ROOT-5TH, PEDAL, WIDE),
 *            anchored on ROOT at the style's octave; some become chords (3 or 4 scale notes), some jump an octave up
 *   rhythm   the style's 16-step template thinned / filled by DENSITY; step 1 always sounds; TIEs after notes
 *   copies   the motif repeated through LEN; with EVOLVE each copy after the first drifts (scale steps, from EVOLVE 3
 *            a note dropped or added); the first copy is the motif as written
 *   in-loop  with EVOLVE, ornament notes get a CHANCE (75 / 50 %) or, from EVOLVE 2, a condition (2:2 3:4 2:4 4:4),
 *            so one short pattern still changes loop to loop; ACID and TRANCE ratchet a few
 * Separate random streams for pitch, rhythm and evolution: turning DENSITY keeps the melody, EVOLVE keeps the motif.
 * The steps as it opened are kept in EUCLID's copy (eu_from: the two are never open together). */
#define FORGE_VERSION "0.2"

enum { FC_WALK, FC_ARC, FC_CHORD, FC_ROOT5, FC_PEDAL, FC_WIDE };
enum { FS_BERLIN, FS_DETROIT, FS_DUB, FS_MELODIC, FS_ACID, FS_TRANCE, FS_SYNTH, FS_AMBIENT, FS_N };
typedef struct {
    char name[8], card[7];        /* the name (the header), as the STYLE card shows it (S font: at most COL_W - 8 px) */
    uint16_t rhythm, accent;      /* bit i: step i + 1 of a 16-step bar */
    uint8_t motif, dens, base;    /* motif steps (8 / 16), DENSITY's default (%), the octave's C (MIDI) */
    uint8_t contour, span;        /* FC_*, the degrees either side of ROOT it moves in */
    uint8_t tie, chord, chord_p;  /* % a step after a note ties; chord notes (0, 3, 4) and % of notes that are one */
    uint8_t slide, octv, orn, ratch;   /* % slide, % octave jump, % ornaments (x EVOLVE / 2), % ratchet (EVOLVE > 0) */
} forge_style_t;
static const forge_style_t FORGE_STYLE[FS_N] = {
    /*                    rhythm  accent motif dens base contour   span tie chord ch% slide oct orn rat */
    {"BERLIN",  "BERLIN",  0xFFFF, 0x0101,  8, 85, 48, FC_WALK,   4,   0,  0,   0,  0,  15, 15,  0},
    {"DETROIT", "DETROIT", 0x4949, 0x0001, 16, 70, 60, FC_WALK,   3,  20,  4,  70,  0,   5, 15,  0},
    {"DUB",     "DUB",     0x4444, 0x0000, 16, 55, 48, FC_WALK,   2,  60,  3, 100,  0,   0, 20,  0},
    {"MELODIC", "MELO",    0x5555, 0x0101, 16, 75, 60, FC_ARC,    7,  25,  0,   0,  0,   5, 15,  0},
    {"ACID",    "ACID",    0xFFFF, 0x2224, 16, 75, 36, FC_PEDAL,  4,   0,  0,   0, 25,  30, 10, 15},
    {"TRANCE",  "TRANCE",  0xFFFF, 0x1111,  8, 90, 60, FC_CHORD,  7,   0,  0,   0,  0,  10, 10, 10},
    {"SYNTH",   "SYNTH",   0x5555, 0x0101,  8, 90, 48, FC_ROOT5,  7,  10,  0,   0,  0,   0,  5,  0},
    {"AMBIENT", "AMBI",    0x0441, 0x0000, 16, 50, 60, FC_WIDE,   9,  80,  3,  30,  0,   0, 40,  0},
};
static const char H_FORGE[] = "[K1]SEED[K3]EVOLVE[K4]STYLE";   /* (the footer holds three; DENSITY says itself) */

static struct { uint8_t on, set, seed, dens, evolve, style, trk; } fg;
static track_t *fg_track(void) { return &trk[fg.trk % NTRK]; }

/* xorshift32 streams of their own (libc.c rng() stays the panel's) */
static uint32_t fg_r;
static void fg_seed(uint32_t salt)
{
    fg_r = ((uint32_t)fg.seed + 1u) * 2654435761u ^ ((uint32_t)fg.style + 1u) * 40503u ^ salt * 0x9E3779B9u;
    if (!fg_r)
        fg_r = 0x1234567u;
}
static uint32_t fg_rnd(uint32_t n)
{
    fg_r ^= fg_r << 13;
    fg_r ^= fg_r >> 17;
    fg_r ^= fg_r << 5;
    return n ? fg_r % n : 0u;
}
static int fg_pct(uint32_t p) { return fg_rnd(100) < p; }

/* the track's scale notes from an octave under the style's to two over it (deg), ROOT's place in it (*a0) and the
 * notes an octave holds (*oct) */
#define FG_NDEG 40u
static uint32_t fg_degrees(const track_t *t, const forge_style_t *st, uint8_t *deg, uint32_t *a0, uint32_t *oct)
{
    uint32_t n = 0, mask = scale_mask(t), x;
    int32_t root = (int32_t)st->base + t->p[P_ROOT] % 12;
    *oct = (uint32_t)__builtin_popcount(mask & 0xFFFu);
    *oct = *oct ? *oct : 12u;
    *a0 = 0;
    for (x = (uint32_t)clamp(root - 12, 12, 115); x <= (uint32_t)clamp(root + 24, 12, 115) && n < FG_NDEG; x++)
        if (tl_in_scale(t, (int32_t)x)) {
            if ((int32_t)x == root)
                *a0 = n;
            deg[n++] = (uint8_t)x;
        }
    return n;
}

/* one motif step: sounds, its degree (index into deg), chord notes (1, 3, 4), octave (0, +1), flags */
typedef struct { uint8_t on, idx, n, flags; int8_t oct; } fg_step_t;

static uint32_t fg_reflect(int32_t i, int32_t lo, int32_t hi)
{
    if (hi <= lo)
        return (uint32_t)lo;
    while (i < lo || i > hi)
        i = i < lo ? 2 * lo - i : 2 * hi - i;
    return (uint32_t)i;
}
/* the degree of every step of the motif, from the contour (pitch stream) */
static void fg_pitch(const forge_style_t *st, fg_step_t *m, uint32_t ml, uint32_t nd, uint32_t a0, uint32_t oct)
{
    static const int8_t STEP[8] = {-2, -1, -1, 0, 0, 1, 1, 2};
    static const uint8_t ARP[4][4] = {{0, 2, 4, 7}, {7, 4, 2, 0}, {0, 4, 2, 7}, {0, 2, 0, 4}};
    int32_t lo = (int32_t)a0 - st->span, hi = (int32_t)a0 + st->span, cur = (int32_t)a0;
    uint32_t i, arp, peak;
    lo = lo < 0 ? 0 : lo;
    hi = hi >= (int32_t)nd ? (int32_t)nd - 1 : hi;
    fg_seed(1);
    arp = fg_rnd(4);
    peak = ml / 2u + fg_rnd(ml / 4u + 1u);              /* ARC: where it turns */
    for (i = 0; i < ml; i++) {
        int32_t d = 0;
        switch (st->contour) {
        case FC_WALK:
            if (i % 4u == 0u && fg_pct(30))
                cur = (int32_t)a0;
            else
                cur += STEP[fg_rnd(8)];
            break;
        case FC_ARC: {
            int32_t tgt = (int32_t)a0 + (int32_t)(i <= peak ? st->span * i / (peak ? peak : 1u)
                                                            : st->span * (ml - i) / (ml - peak));
            cur += (tgt > cur) - (tgt < cur) + (fg_pct(25) ? STEP[fg_rnd(8)] : 0);
            break;
        }
        case FC_CHORD:                                  /* the scale's triad + its octave, an arpeggio */
            d = ARP[arp][i % 4u];
            cur = (int32_t)a0 + (d == 7 ? (int32_t)oct : d) + (i % 8u >= 4u && fg_pct(40) ? 2 : 0);
            break;
        case FC_ROOT5:                                  /* root, fifth, octave (a third now and then) */
            d = (int32_t)((i % 2u) ? (fg_pct(50) ? oct : 4u) : (fg_pct(15) ? 2u : 0u));
            cur = (int32_t)a0 + d;
            break;
        case FC_PEDAL:                                  /* every other note the root, the others anywhere near */
            cur = i % 2u == 0u && fg_pct(70) ? (int32_t)a0 : (int32_t)a0 + (int32_t)fg_rnd((uint32_t)st->span * 2u + 1u) - st->span;
            break;
        default:                                        /* FC_WIDE: fourths and fifths, a wide range */
            cur += (fg_rnd(2) ? 1 : -1) * (int32_t)(3u + fg_rnd(2));
            break;
        }
        if (!i)                                         /* (step 1: the root) */
            cur = (int32_t)a0;
        m[i].idx = (uint8_t)fg_reflect(cur, st->contour == FC_CHORD || st->contour == FC_ROOT5 ? 0 : lo,
                                       st->contour == FC_CHORD || st->contour == FC_ROOT5 ? (int32_t)nd - 1 : hi);
        cur = m[i].idx;
        m[i].oct = (int8_t)(i && fg_pct(st->octv) ? 1 : 0);   /* (octave jumps go up only) */
        m[i].n = (uint8_t)(st->chord && fg_pct(st->chord_p) ? st->chord : 1u);
    }
}
/* which steps sound, accents, slides, ties (rhythm stream; flags: SF_ACCENT, SF_SLIDE; on 2 = a TIE) */
static void fg_rhythm(const forge_style_t *st, fg_step_t *m, uint32_t ml)
{
    uint32_t i, d = fg.dens;
    fg_seed(2);
    for (i = 0; i < ml; i++) {
        uint32_t tpl = (st->rhythm >> (i % 16u)) & 1u;
        m[i].flags = 0;
        m[i].on = (uint8_t)(!i || (tpl ? fg_pct(d + 20u > 100u ? 100u : d + 20u) : fg_pct(d > 75u ? (d - 75u) * 2u : 0u)));
        if (!m[i].on && i && m[i - 1].on && fg_pct(st->tie))
            m[i].on = 2;                                /* (a TIE: it holds the note before it) */
        if (m[i].on == 1u && ((st->accent >> (i % 16u)) & 1u))
            m[i].flags |= SF_ACCENT;
    }
    for (i = 0; i + 1u < ml; i++)                       /* a slide into the next note */
        if (m[i].on == 1u && m[i + 1].on == 1u && fg_pct(st->slide))
            m[i].flags |= SF_SLIDE;
}
/* motif step f (its copy k: drifted by d degrees) into step s */
static void fg_write(const track_t *t, step_t *s, const fg_step_t *f, int32_t d, const uint8_t *deg, uint32_t nd)
{
    uint32_t k, x;
    memset(s, 0, sizeof *s);                            /* (all of it: no CHANCE, nudge or note left from before) */
    s->time = ST_REST;
    if (!f->on)
        return;
    if (f->on == 2u) {
        s->time = ST_TIE;
        return;
    }
    s->time = ST_NOTE;
    s->vel = 96;
    s->flags = f->flags;
    for (k = 0; k < f->n && k < 4u; k++) {
        x = fg_reflect((int32_t)f->idx + d + 2 * (int32_t)k, 0, (int32_t)nd - 1);
        s->note[k] = (uint8_t)clamp((int32_t)deg[x] + 12 * f->oct, 24, 108);
    }
    s->n = (uint8_t)k;
    (void)t;
}

/* the track's steps 1..LEN from the steps as it opened (eu_from) and fg's values */
static void forge_render(void)
{
    track_t *t = fg_track();
    const forge_style_t *st = &FORGE_STYLE[fg.style % FS_N];
    uint8_t deg[FG_NDEG];
    fg_step_t m[16];
    uint32_t len = tl_len(t), ml = st->motif < len ? st->motif : len, nd, a0, oct, i, k, e = fg.evolve;
    int32_t drift = 0;
    memcpy(t->step, eu_from, sizeof t->step);
    nd = fg_degrees(t, st, deg, &a0, &oct);
    if (!nd)
        return;
    fg_pitch(st, m, ml, nd, a0, oct);
    fg_rhythm(st, m, ml);
    fg_seed(3);                                         /* (the evolution's stream) */
    for (i = 0; i < len; i++) {
        fg_step_t f = m[i % ml];
        step_t *s = &t->step[i];
        k = i / ml;                                     /* the copy */
        if (i % ml == 0u)
            drift = k && e && fg_pct(e * 15u) ? (fg_rnd(2) ? 1 : -1) : drift;   /* (a copy may sit a step away) */
        if (k && e) {
            if (f.on == 1u && fg_pct(e * 8u + (k - 1u) * e * 4u))
                f.idx = (uint8_t)fg_reflect((int32_t)f.idx + (fg_rnd(2) ? 1 : -1), 0, (int32_t)nd - 1);
            if (e >= 3u && i % ml && fg_pct(e * 3u))    /* a note dropped, or one in a rest */
                f.on = f.on == 1u ? 0u : f.on == 0u ? 1u : f.on;
        }
        fg_write(t, s, &f, k ? drift : 0, deg, nd);
        if (e && s->time == ST_NOTE && i % ml && fg_pct(st->orn * e / 2u)) {
            static const uint8_t COND[4] = {2, 8, 7, 9};   /* 2:2 3:4 2:4 4:4 (core.h: 1:2 2:2 1:3 2:3 3:3 1:4 2:4 3:4 4:4) */
            if (e >= 2u && fg_rnd(2))
                step_set_chance_v(s, 100u + COND[fg_rnd(4)]);
            else
                step_set_chance_v(s, fg_rnd(2) ? 75u : 50u);
        }
        if (e && s->time == ST_NOTE && fg_pct(st->ratch))
            step_set_ratchet(s, 2u + fg_rnd(2));
    }
    for (i = 0; i < len; i++) {                         /* a TIE with nothing to hold (its note dropped): a REST */
        const step_t *p = i ? &t->step[i - 1] : 0;
        if (t->step[i].time == ST_TIE && (!p || !((p->time == ST_NOTE && p->n) || p->time == ST_TIE)))
            step_clear(&t->step[i]);
    }
}

static void forge_say(void)                             /* "FORGE 42 BERLIN" */
{
    char b[24], n[4];
    str_cpy(b, "FORGE ", sizeof b);
    fmt_int(n, fg.seed);
    str_cpy(b + str_len(b), n, sizeof b - str_len(b));
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), FORGE_STYLE[fg.style % FS_N].name, sizeof b - str_len(b));
    ui_message(b);
}
static void forge_open(track_t *t)
{
    tl_undo_take(t);
    memcpy(eu_from, t->step, sizeof eu_from);
    fg.trk = (uint8_t)trk_index(t);
    if (!fg.set) {                                      /* (the first since the power came) */
        fg.set = 1;
        fg.seed = (uint8_t)(rng() % 100u);
        fg.style = FS_BERLIN;
        fg.evolve = 1;
        fg.dens = FORGE_STYLE[FS_BERLIN].dens;
    }
    fg.on = 1;
    forge_render();
    forge_say();
}
static void forge_knob(uint32_t k, int32_t s)
{
    if (k == 0u)
        fg.seed = (uint8_t)(((int32_t)fg.seed + s % 100 + 100) % 100);
    else if (k == 1u)
        fg.dens = (uint8_t)clamp((int32_t)fg.dens + 5 * s, 10, 100);
    else if (k == 2u)
        fg.evolve = (uint8_t)clamp((int32_t)fg.evolve + s, 0, 4);
    else {
        fg.style = (uint8_t)clamp((int32_t)fg.style + s, 0, FS_N - 1);
        fg.dens = FORGE_STYLE[fg.style].dens;            /* (each style's own) */
    }
    forge_render();
    forge_say();
}
/* OCT+ (apply: the emptied steps lose their locks) or OCT- (cancel: the steps of the copy back) */
static void forge_close(int apply)
{
    track_t *t;
    uint32_t len;
    if (!fg.on)
        return;
    t = fg_track();
    len = tl_len(t);
    fg.on = 0;
    if (apply) {
        tl_unlock(t, tl_used_of(eu_from, len), len);
        return;
    }
    memcpy(t->step, eu_from, sizeof t->step);
}
static uint32_t forge_sig(void)
{
    return fg.on ? 11u + fg.seed * 13u + fg.dens * 1031u + fg.evolve * 65537u + fg.style * 524287u : 0u;
}

/* the open FORGE's pattern: a cell a step, 16 a row (up to 4 rows): a NOTE filled with a mark at its pitch, a TIE
 * half-filled, a REST empty */
static void layer_forge(void)
{
    const track_t *t = fg_track();
    uint32_t len = tl_len(t), i, rows = (len + 15u) / 16u, lo = 127, hi = 0;
    int32_t h = rows == 1u ? 60 : rows == 2u ? 40 : rows == 3u ? 28 : 20;
    for (i = 0; i < len; i++)
        if (t->step[i].time == ST_NOTE && t->step[i].n) {
            lo = t->step[i].note[0] < lo ? t->step[i].note[0] : lo;
            hi = t->step[i].note[0] > hi ? t->step[i].note[0] : hi;
        }
    for (i = 0; i < len; i++) {
        const step_t *s = &t->step[i];
        int32_t x = 8 + 14 * (int32_t)(i % 16u), y = 6 + (h + 2) * (int32_t)(i / 16u);
        int note = s->time == ST_NOTE && s->n;
        cv_rrect(x, y, 12, h, 2, note ? T_THEME : s->time == ST_TIE ? T_MID : T_RAISE, T_SURF);
        if (note) {
            int32_t span = hi > lo ? (int32_t)(hi - lo) : 1, ty = y + h - 5 - (int32_t)(s->note[0] - lo) * (h - 8) / span;
            cv_rect(x + 2, ty, 8, 3, T_SURF);
        }
    }
}
static void forge_cards(void)
{
    char val[8];
    fmt_int(val, fg.seed);
    draw_column(0, "SEED", val, "", VAL(0u), fg.seed * 1000 / 99, ICON_PROB);
    fmt_int(val, fg.dens);
    draw_column(1, "DENSITY", val, "%", VAL(1u), fg.dens * 10, ICON_STEPS);
    fmt_int(val, fg.evolve);
    draw_column(2, "EVOLVE", val, "", fg.evolve ? VAL(2u) : T_DIM, fg.evolve * 250, ICON_W_SH);
    draw_column(3, "STYLE", FORGE_STYLE[fg.style % FS_N].card, "", VAL(3u), fg.style * 1000 / (FS_N - 1), ICON_PULSE);
}

/* F5: open (a melodic track: tl_cell); the caller applied any EUCLID or FORGE open before */
static void forge_key(track_t *t)
{
    forge_open(t);
}
