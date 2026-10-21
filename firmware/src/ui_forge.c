/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the FORGE ONE contributors */
/* FORGE ONE: SEQ TOOLS F5 FORGE (TL_FORGE, ui_tools.c), a seeded generator for the selected track:
 * melodies on a melodic track, beats on a drum track (0.5, forge_render_drum).
 * Included by ui_layer.c after ui_tools.c.
 *
 * F5 opens it as EUCLID opens (the undo copy is taken, one undo for all of it, and the pattern is written at once, heard
 * while playing); KNOB 1..4 SEED (0..99) DENSITY (10..100 %) EVOLVE (0..4) STYLE (8) rewrite it from the steps as it
 * opened. OCT+ (or F5 again, another tool, the layer let go) applies it, OCT- cancels it. The values stay for the next
 * one (until the power goes); the first since the power came takes a random SEED.
 *
 * What it writes: steps 1..LEN of the track (past LEN stays), only the fields a project already holds (notes, NOTE /
 * TIE / REST, accent, slide, RATCH, CHANCE or a condition, VEL by place (0.4); on a drum track the lanes and their accents):
 * no format change. The same SEED, STYLE, DENSITY,
 * EVOLVE, LEN, ROOT and SCALE always write the same steps.
 *
 * How (forge_render): a motif of the style's length (8 or 16 steps, at most LEN):
 *   pitch    a scale degree for every step from the style's contour (WALK, ARC, CHORD tones, ROOT-5TH, PEDAL, WIDE),
 *            anchored on ROOT at the style's octave (step 1 is ROOT); some become chords (3 or 4 scale notes), some jump an octave up
 *   rhythm   the style's 16-step template thinned / filled by DENSITY; step 1 always sounds; TIEs after notes
 *   copies   the motif repeated through LEN; with EVOLVE each copy after the first drifts (scale steps, from EVOLVE 3
 *            a note dropped or added); the first copy is the motif as written
 *   in-loop  with EVOLVE, ornament notes get a CHANCE (75 / 50 %) or, from EVOLVE 2, a condition (2:2 3:4 2:4 4:4),
 *            so one short pattern still changes loop to loop; ACID and TRANCE ratchet a few
 * Separate random streams for pitch, rhythm and evolution: turning DENSITY keeps the melody, EVOLVE keeps the motif.
 * The steps as it opened are kept in EUCLID's copy (eu_from: the two are never open together). */
/* FORGE_VERSION: core.h */

enum { FC_WALK, FC_ARC, FC_CHORD, FC_ROOT5, FC_PEDAL, FC_WIDE };
enum { FS_BERLIN, FS_DETROIT, FS_DUB, FS_MELODIC, FS_ACID, FS_TRANCE, FS_SYNTH, FS_AMBIENT, FS_N };
typedef struct {
    char name[8], card[7];        /* the name (the header), as the STYLE card shows it (S font: at most COL_W - 8 px) */
    uint16_t rhythm, accent;      /* bit i: step i + 1 of a 16-step bar */
    uint8_t motif, dens, base;    /* motif steps (8 / 16), DENSITY's default (%), the octave's C (MIDI) */
    uint8_t contour, span;        /* FC_*, the degrees either side of ROOT it moves in */
    uint8_t tie, chord, chord_p;  /* % a step after a note ties; chord notes (0, 3, 4) and % of notes that are one */
    uint8_t slide, octv, orn, ratch;   /* % slide, % octave jump, % ornaments (x EVOLVE / 2), % ratchet (EVOLVE > 0) */
    uint8_t vdyn;                 /* 0.4: VEL's range: beats louder, off-beat 16ths softer, a little each way */
} forge_style_t;
static const forge_style_t FORGE_STYLE[FS_N] = {
    /*                    rhythm  accent motif dens base contour   span tie chord ch% slide oct orn rat vel */
    {"BERLIN",  "BERLIN",  0xFFFF, 0x0101,  8, 85, 48, FC_WALK,   4,   0,  0,   0,  0,  15, 15,  0, 20},
    {"DETROIT", "DETROIT", 0x4949, 0x0001, 16, 70, 60, FC_WALK,   3,  20,  4,  70,  0,   5, 15,  0, 16},
    {"DUB",     "DUB",     0x4444, 0x0000, 16, 55, 48, FC_WALK,   2,  60,  3, 100,  0,   0, 20,  0, 10},
    {"MELODIC", "MELO",    0x5555, 0x0101, 16, 75, 60, FC_ARC,    7,  25,  0,   0,  0,   5, 15,  0, 18},
    {"ACID",    "ACID",    0xFFFF, 0x2224, 16, 75, 36, FC_PEDAL,  4,   0,  0,   0, 25,  30, 10, 15, 24},
    {"TRANCE",  "TRANCE",  0xFFFF, 0x1111,  8, 90, 60, FC_CHORD,  7,   0,  0,   0,  0,  10, 10, 10, 20},
    {"SYNTH",   "SYNTH",   0x5555, 0x0101,  8, 90, 48, FC_ROOT5,  7,  10,  0,   0,  0,   0,  5,  0, 16},
    {"AMBIENT", "AMBI",    0x0441, 0x0000, 16, 50, 60, FC_WIDE,   9,  80,  3,  30,  0,   0, 40,  0, 12},
};
/* 0.5: FORGE on a DRUM track, the same eight styles as beats. Each lane (KICK SNARE CLAP HATC HATO TOM RIM BELL) has
 * its core hits (kept from DENSITY 50 % up, thinned under it) and its optional hits (each kept with DENSITY's chance),
 * bit i = step i + 1 of a 16-step bar; over 75 % extra closed hats and rim ghosts come in. The low lanes (KICK SNARE
 * CLAP TOM) draw from the pitch stream (C4 rerolls them: LOW), the high ones (HATC HATO RIM BELL) from the rhythm
 * stream (D4: HATS). KICK on step 1 always; KICK accented on 1 and 9, SNARE / CLAP on 2 and 4 half the time */
typedef struct { uint16_t core[NLANE], opt[NLANE]; uint8_t dens, rat; } forge_beat_t;
static const forge_beat_t FORGE_BEAT[FS_N] = {
    /*          KICK    SNARE   CLAP    HATC    HATO    TOM     RIM     BELL     (core)  */
    {{0x1111, 0x0000, 0x0000, 0x5555, 0x0000, 0x0000, 0x0000, 0x0000},   /* BERLIN */
     {0x0000, 0x0000, 0x0000, 0xAAAA, 0x4040, 0x0000, 0x2222, 0x0000}, 70, 10},
    {{0x1111, 0x0000, 0x1010, 0x5555, 0x4444, 0x0000, 0x0000, 0x0000},   /* DETROIT */
     {0x0400, 0x0000, 0x0000, 0xAAAA, 0x0000, 0x0080, 0x0202, 0x0000}, 70, 10},
    {{0x0101, 0x0000, 0x0000, 0x5555, 0x0000, 0x0000, 0x1010, 0x0000},   /* DUB */
     {0x0400, 0x0000, 0x0000, 0xAAAA, 0x0040, 0x0000, 0x0202, 0x0000}, 55, 5},
    {{0x1111, 0x0000, 0x1010, 0x4444, 0x0000, 0x0000, 0x0000, 0x0000},   /* MELODIC */
     {0x0000, 0x0000, 0x0000, 0xBBBB, 0x0404, 0x8000, 0x0000, 0x0000}, 70, 10},
    {{0x1111, 0x1010, 0x0000, 0xAAAA, 0x0000, 0x0000, 0x0000, 0x0000},   /* ACID */
     {0x0800, 0x0000, 0x1010, 0x5555, 0x4444, 0x0000, 0x2002, 0x0000}, 75, 20},
    {{0x1111, 0x0000, 0x1010, 0x5555, 0x4444, 0x0000, 0x0000, 0x0000},   /* TRANCE */
     {0x0000, 0x0000, 0x0000, 0xAAAA, 0x0000, 0x0000, 0x8888, 0x0000}, 85, 15},
    {{0x0041, 0x1010, 0x0000, 0x5555, 0x0000, 0x0000, 0x0000, 0x0000},   /* SYNTH */
     {0x0800, 0x0000, 0x1010, 0xAAAA, 0x4000, 0x8000, 0x0000, 0x0000}, 70, 5},
    {{0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000},   /* AMBIENT */
     {0x0100, 0x0000, 0x0000, 0x5555, 0x0400, 0x2000, 0x0440, 0x0010}, 45, 0},
};
#define FG_LOW_LANES (1u << DV_KICK | 1u << DV_SNARE | 1u << DV_CLAP | 1u << DV_TOM)
static const char H_FORGE[] = "[K1]SEED[K3]EVOLVE[K4]STYLE";   /* (the footer holds three; DENSITY says itself, C4 / D4 the grid) */

static struct { uint8_t on, set, seed, dens, evolve, style, trk, nvar, rvar, drum; } fg;   /* nvar rvar (0.4): C4 / D4 rerolls; drum (0.5): the last open was a DRUM track */
static track_t *fg_track(void) { return &trk[fg.trk % NTRK]; }

/* xorshift32 streams of their own (libc.c rng() stays the panel's): 1 pitch, 2 rhythm, 3 evolution, 4 (0.4)
 * velocity, in a state of its own (fg_vr) so the others draw as in 0.3. 0.4: C4 / D4 while open reroll the pitch /
 * rhythm stream alone (fg.nvar, fg.rvar: 0 = as 0.3, so a seed with both 0 keeps its 0.3 notes and rhythm) */
static uint32_t fg_r, fg_vr;
static uint32_t fg_start(uint32_t salt)
{
    uint32_t r = ((uint32_t)fg.seed + 1u) * 2654435761u ^ ((uint32_t)fg.style + 1u) * 40503u ^ salt * 0x9E3779B9u ^
                 (salt == 1u ? fg.nvar * 0x85EBCA6Bu : salt == 2u ? fg.rvar * 0xC2B2AE35u : 0u);
    return r ? r : 0x1234567u;
}
static void fg_seed(uint32_t salt) { fg_r = fg_start(salt); }
static uint32_t fg_xs(uint32_t *r, uint32_t n)
{
    *r ^= *r << 13;
    *r ^= *r >> 17;
    *r ^= *r << 5;
    return n ? *r % n : 0u;
}
static uint32_t fg_rnd(uint32_t n) { return fg_xs(&fg_r, n); }
static int fg_pct(uint32_t p) { return fg_rnd(100) < p; }

/* the track's scale notes from an octave under the style's to two over it (deg) and ROOT's place in it (*a0). Every
 * SCALE holds its root (SCALE_MASK bit 0, tested over all of them), so ROOT is found; were it not, the in-scale note
 * nearest to it (0.5.1) */
#define FG_NDEG 40u
static uint32_t fg_degrees(const track_t *t, const forge_style_t *st, uint8_t *deg, uint32_t *a0)
{
    uint32_t n = 0, x, best = 99;
    int32_t root = (int32_t)st->base + t->p[P_ROOT] % 12;
    *a0 = 0;
    for (x = (uint32_t)clamp(root - 12, 12, 115); x <= (uint32_t)clamp(root + 24, 12, 115) && n < FG_NDEG; x++)
        if (tl_in_scale(t, (int32_t)x)) {
            uint32_t d = (uint32_t)((int32_t)x > root ? (int32_t)x - root : root - (int32_t)x);
            if (d < best) {
                best = d;
                *a0 = n;
            }
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
/* 0.3: chords and chord tones by interval, so they hold in every scale (CHR, the pentatonics, BLUES, WHOLE ..), not
 * by counting scale notes (which made C D E a "triad" in CHR). The third, fifth and seventh above note n: the first of
 * the intervals the scale holds (a seven-note scale holds exactly one of each: the scale's own chord); none of them in
 * the scale: the first snapped down onto it, but above floor (the chord's note under it), else up */
static const int8_t FG_IV3[2] = {3, 4}, FG_IV5[3] = {7, 6, 8}, FG_IV7[3] = {10, 11, 9};
static int32_t fg_tone(const track_t *t, int32_t n, const int8_t *iv, uint32_t niv, int32_t floor)
{
    uint32_t i;
    int32_t x;
    for (i = 0; i < niv; i++)
        if (tl_in_scale(t, n + iv[i]))
            return n + iv[i];
    for (x = n + iv[0]; x > floor && !tl_in_scale(t, x); x--)
        ;
    if (x <= floor)
        for (x = n + iv[0]; x < n + 12 && !tl_in_scale(t, x); x++)
            ;
    return x;
}
static uint32_t fg_idx_of(const uint8_t *deg, uint32_t nd, int32_t note)   /* the first scale note at or over note */
{
    uint32_t i;
    for (i = 0; i + 1u < nd && deg[i] < note; i++)
        ;
    return i;
}
/* the degree of every step of the motif, from the contour (pitch stream) */
static void fg_pitch(const track_t *t, const forge_style_t *st, fg_step_t *m, uint32_t ml, const uint8_t *deg,
                     uint32_t nd, uint32_t a0)
{
    static const int8_t STEP[8] = {-2, -1, -1, 0, 0, 1, 1, 2};
    static const uint8_t ARP[4][4] = {{0, 1, 2, 3}, {3, 2, 1, 0}, {0, 2, 1, 3}, {0, 1, 0, 2}};   /* (into tone[]) */
    int32_t lo = (int32_t)a0 - st->span, hi = (int32_t)a0 + st->span, cur = (int32_t)a0, r = deg[a0], tone[5];
    uint32_t i, arp, peak;
    tone[0] = r;                                        /* the root's chord: root, third, fifth, octave, its third */
    tone[1] = fg_tone(t, r, FG_IV3, 2, r);
    tone[2] = fg_tone(t, r, FG_IV5, 3, tone[1]);
    tone[3] = r + 12;
    tone[4] = tone[1] + 12;
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
        case FC_CHORD:                                  /* the root's triad + its octave, an arpeggio (a tone up now and then) */
            d = ARP[arp][i % 4u] + (i % 8u >= 4u && fg_pct(40) ? 1 : 0);
            cur = (int32_t)fg_idx_of(deg, nd, tone[d]);
            break;
        case FC_ROOT5:                                  /* root, fifth, octave (a third now and then) */
            d = (i % 2u) ? (fg_pct(50) ? 3 : 2) : (fg_pct(15) ? 1 : 0);
            cur = (int32_t)fg_idx_of(deg, nd, tone[d]);
            break;
        case FC_PEDAL:                                  /* every other note the root, the others anywhere near */
            cur = i % 2u == 0u && fg_pct(70) ? (int32_t)a0 : (int32_t)a0 + (int32_t)fg_rnd((uint32_t)st->span * 2u + 1u) - st->span;
            break;
        default:                                        /* FC_WIDE: fourths and fifths (by interval), a wide range */
            d = (fg_rnd(2) ? 1 : -1) * (fg_rnd(2) ? 5 : 7);
            cur = (int32_t)fg_idx_of(deg, nd, deg[fg_reflect(cur, 0, (int32_t)nd - 1)] + d);
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
/* motif step f (its copy k: drifted by d degrees) into step s; a chord: its third, fifth, seventh by interval */
static void fg_write(const track_t *t, step_t *s, const fg_step_t *f, int32_t d, const uint8_t *deg, uint32_t nd)
{
    static const int8_t *const IV[4] = {0, FG_IV3, FG_IV5, FG_IV7};
    static const uint8_t NIV[4] = {0, 2, 3, 3};
    uint32_t k;
    int32_t n0, x;
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
    n0 = clamp((int32_t)deg[fg_reflect((int32_t)f->idx + d, 0, (int32_t)nd - 1)] + 12 * f->oct, 24, 108);
    s->note[0] = (uint8_t)n0;
    for (k = 1; k < f->n && k < 4u; k++) {
        x = fg_tone(t, n0, IV[k], NIV[k], s->note[k - 1]);
        if (x > 108)                                    /* (no note over the top: a smaller chord) */
            break;
        s->note[k] = (uint8_t)x;
    }
    s->n = (uint8_t)k;
}

/* 0.5: a DRUM track's steps 1..LEN (forge_render decides): the motif is one 16-step bar (or LEN), hits per lane by
 * DENSITY, then copies through LEN as the melodic one: with EVOLVE the later copies move a hat or two, add a ghost
 * snare / rim with a CHANCE, drop or add a kick off the beat (3 and up), ratchet hats (the style's rate); with EVOLVE
 * 2 and up the last copy ends in a fill: the last two or three steps a snare roll on a 4:4 condition (every 4th loop) */
static void forge_render_drum(track_t *t, uint32_t len)
{
    const forge_beat_t *bt = &FORGE_BEAT[fg.style % FS_N];
    uint8_t hit[16], acc[16], vel[16];
    uint32_t ml = len < 16u ? len : 16u, i, l, k, e = fg.evolve, d = fg.dens, copies = (len + ml - 1u) / ml, g;
    memset(hit, 0, sizeof hit);
    memset(acc, 0, sizeof acc);
    for (l = 0; l < NLANE; l++) {                       /* the lanes, each its own state off its group's stream */
        fg_r = fg_start((FG_LOW_LANES >> l) & 1u ? 1u : 2u) ^ (l + 1u) * 0x9E3779B1u;
        fg_r = fg_r ? fg_r : 0x1234567u;
        for (i = 0; i < ml; i++) {
            uint32_t core = (bt->core[l] >> i) & 1u, opt = (bt->opt[l] >> i) & 1u, on;
            on = core ? (d >= 50u || fg_pct(d * 2u)) : opt ? fg_pct(d) : 0u;
            if (!on && d > 75u && (l == DV_HATC || l == DV_RIM) && (i & 1u))
                on = fg_pct((d - 75u) * 2u);           /* (over 75 %: 16th hats and rim ghosts) */
            if (l == DV_KICK && !i)
                on = 1;
            if (on)
                hit[i] |= (uint8_t)(1u << l);
            if (on && ((l == DV_KICK && (i == 0u || i == 8u)) ||
                       ((l == DV_SNARE || l == DV_CLAP) && (i == 4u || i == 12u) && fg_pct(50))))
                acc[i] |= (uint8_t)(1u << l);
        }
    }
    fg_vr = fg_start(4);                                /* VEL by place, as the melodic one */
    for (i = 0; i < ml; i++) {
        int32_t v = FORGE_STYLE[fg.style % FS_N].vdyn, w = i % 4u == 0u ? 2 : i % 2u == 0u ? 0 : -1;
        vel[i] = (uint8_t)clamp(96 + w * v / 4 + (int32_t)fg_xs(&fg_vr, (uint32_t)v / 2u + 1u) - v / 4, 40, 124);
    }
    fg_seed(3);                                         /* (the evolution's stream) */
    for (i = 0; i < len; i++) {
        step_t s;
        uint32_t h = hit[i % ml], a = acc[i % ml], fill = 0;
        k = i / ml;
        if (k && e) {
            if (fg_pct(e * 6u + (k - 1u) * e * 3u))      /* a closed hat moved on or off */
                h ^= 1u << DV_HATC;
            if (e >= 3u && (i % 4u) && fg_pct(e * 2u))  /* a kick off the beat dropped or added */
                h ^= 1u << DV_KICK;
        }
        if (e >= 2u && ml >= 4u && k + 1u == copies && i % ml >= ml - (ml > 8u ? 3u : 2u) && fg_pct(60)) {
            h = 1u << DV_SNARE;                         /* the fill: a snare roll, every 4th loop (never on step 1 */
            a = 0;                                      /* of a copy: LEN 2 / 3 have none, 0.5.1) */
            fill = 1;
        }
        memset(&s, 0, sizeof s);
        s.time = ST_REST;
        if (!h && e && (i % ml) && (i & 1u) && fg_pct(e * 6u)) {   /* an empty off-beat: a ghost snare / rim with */
            h = 1u << (fg_rnd(2) ? DV_SNARE : DV_RIM);  /* a CHANCE (its own step: nothing else is thinned) */
            s.time = ST_NOTE;
            s.hit = (uint8_t)h;
            s.vel = (uint8_t)(vel[i % ml] - 20u);
            step_set_chance_v(&s, fg_rnd(2) ? 75u : 50u);
        } else if (h) {
            s.time = ST_NOTE;
            s.hit = (uint8_t)h;
            s.acc = (uint8_t)(a & h);
            s.vel = vel[i % ml];
            if (fill) {
                step_set_ratchet(&s, 2u + fg_rnd(2));
                step_set_chance_v(&s, 100u + 9u);       /* 4:4 */
            } else if (e && (h & (1u << DV_HATC)) && (i & 1u) && fg_pct(bt->rat)) {
                step_set_ratchet(&s, 2u);               /* (a ratcheted hat: the whole step's hits) */
            }
        }
        g = motion_guard();
        t->step[i] = s;
        motion_unguard(g);
    }
}

/* the track's steps 1..LEN from fg's values (the steps as it opened: eu_from, for a cancel). 0.3: each step is made
 * whole here and then put in place with the audio ISR held off (motion_guard), one at a time: the sequencer, which
 * reads the steps in the ISR while playing, sees a step as it was or as it is, never half of one (up to 0.2 the steps
 * were put back as they opened and then written over in place, so a turn while playing could sound a step between) */
static void forge_render(void)
{
    track_t *t = fg_track();
    const forge_style_t *st = &FORGE_STYLE[fg.style % FS_N];
    uint8_t deg[FG_NDEG], vel[16];
    fg_step_t m[16];
    uint32_t len = tl_len(t), ml = st->motif < len ? st->motif : len, nd, a0, i, k, e = fg.evolve, g, held = 0;
    int32_t drift = 0;
    if (drum_track(t)) {                                /* (0.5) */
        forge_render_drum(t, len);
        return;
    }
    nd = fg_degrees(t, st, deg, &a0);
    if (!nd)
        return;
    fg_pitch(t, st, m, ml, deg, nd, a0);
    fg_rhythm(st, m, ml);
    fg_seed(3);                                         /* (the evolution's stream) */
    fg_vr = fg_start(4);                                /* 0.4: VEL by place, beats louder, a little each way (an */
    for (i = 0; i < ml; i++) {                          /* accent plays 127 anyway); one per motif step */
        int32_t v = st->vdyn, w = i % 4u == 0u ? 2 : i % 2u == 0u ? 0 : -1;
        vel[i] = (uint8_t)clamp(96 + w * v / 4 + (int32_t)fg_xs(&fg_vr, (uint32_t)v / 2u + 1u) - v / 4, 40, 124);
    }
    for (i = 0; i < len; i++) {
        fg_step_t f = m[i % ml];
        step_t s;
        k = i / ml;                                     /* the copy */
        if (i % ml == 0u)
            drift = k && e && fg_pct(e * 15u) ? (fg_rnd(2) ? 1 : -1) : drift;   /* (a copy may sit a step away) */
        if (k && e) {
            if (f.on == 1u && fg_pct(e * 8u + (k - 1u) * e * 4u))
                f.idx = (uint8_t)fg_reflect((int32_t)f.idx + (fg_rnd(2) ? 1 : -1), 0, (int32_t)nd - 1);
            if (e >= 3u && i % ml && fg_pct(e * 3u))    /* a note dropped, or one in a rest */
                f.on = f.on == 1u ? 0u : f.on == 0u ? 1u : f.on;
        }
        fg_write(t, &s, &f, k ? drift : 0, deg, nd);
        if (s.time == ST_NOTE)                          /* 0.4: VEL by place in the motif (copies alike) */
            s.vel = vel[i % ml];
        if (e && s.time == ST_NOTE && i % ml && fg_pct(st->orn * e / 2u)) {
            static const uint8_t COND[4] = {2, 8, 7, 9};   /* 2:2 3:4 2:4 4:4 (core.h: 1:2 2:2 1:3 2:3 3:3 1:4 2:4 3:4 4:4) */
            if (e >= 2u && fg_rnd(2))
                step_set_chance_v(&s, 100u + COND[fg_rnd(4)]);
            else
                step_set_chance_v(&s, fg_rnd(2) ? 75u : 50u);
        }
        if (e && s.time == ST_NOTE && fg_pct(st->ratch))
            step_set_ratchet(&s, 2u + fg_rnd(2));
        if (s.time == ST_TIE && !held) {                /* a TIE with nothing to hold (its note dropped): a REST */
            memset(&s, 0, sizeof s);
            s.time = ST_REST;
        }
        held = (s.time == ST_NOTE && s.n) || s.time == ST_TIE;
        g = motion_guard();
        t->step[i] = s;
        motion_unguard(g);
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
    if (drum_track(t) != fg.drum) {                     /* (0.5: a DRUM track's DENSITY is the beat's) */
        fg.drum = (uint8_t)drum_track(t);
        fg.dens = fg.drum ? FORGE_BEAT[fg.style % FS_N].dens : FORGE_STYLE[fg.style % FS_N].dens;
    }
    fg.on = 1;
    forge_render();
    forge_say();
}
static void forge_knob(uint32_t k, int32_t s)
{
    if (k == 0u) {
        fg.seed = (uint8_t)(((int32_t)fg.seed + s % 100 + 100) % 100);
        fg.nvar = fg.rvar = 0;                          /* (a new seed: its own notes and rhythm) */
    }
    else if (k == 1u)
        fg.dens = (uint8_t)clamp((int32_t)fg.dens + 5 * s, 10, 100);
    else if (k == 2u)
        fg.evolve = (uint8_t)clamp((int32_t)fg.evolve + s, 0, 4);
    else {
        fg.style = (uint8_t)(((int32_t)fg.style + s % FS_N + FS_N) % FS_N);   /* (0.5.1: wraps, as SEED) */
        fg.nvar = fg.rvar = 0;                          /* (a new style: its own notes and rhythm, as a new seed) */
        fg.dens = drum_track(fg_track()) ? FORGE_BEAT[fg.style].dens : FORGE_STYLE[fg.style].dens;   /* (each style's own) */
    }
    forge_render();
    forge_say();
}
/* 0.4: C4 (RANDOM) while open: new notes on the same rhythm; D4 (COOK): a new rhythm on the same notes (the pitch of
 * each step stays; with EVOLVE the later copies may drift otherwise). Each press the next of 100 */
static void forge_reroll(uint32_t rhythm)
{
    char b[24], n[4];
    if (rhythm)
        fg.rvar = (uint8_t)((fg.rvar + 1u) % 100u);
    else
        fg.nvar = (uint8_t)((fg.nvar + 1u) % 100u);
    forge_render();
    str_cpy(b, drum_track(fg_track()) ? (rhythm ? "NEW HATS " : "NEW LOW ") : rhythm ? "NEW RHYTHM " : "NEW NOTES ", sizeof b);
    fmt_int(n, rhythm ? fg.rvar : fg.nvar);
    str_cpy(b + str_len(b), n, sizeof b - str_len(b));
    ui_message(b);
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
    {
        uint32_t g = motion_guard();                    /* (0.3: as forge_render, never half of the steps) */
        memcpy(t->step, eu_from, sizeof t->step);
        motion_unguard(g);
    }
}
static uint32_t forge_sig(void)
{
    return fg.on ? 11u + fg.seed * 13u + fg.dens * 1031u + fg.evolve * 65537u + fg.style * 524287u + fg.nvar * 97u +
                   fg.rvar * 7919u : 0u;
}

/* the open FORGE's pattern: a cell a step, 16 a row (up to 4 rows): a NOTE filled with a mark at its pitch, a TIE
 * half-filled, a REST empty */
static void layer_forge(void)
{
    const track_t *t = fg_track();
    uint32_t len = tl_len(t), i, rows = (len + 15u) / 16u, lo = 127, hi = 0;
    int drum = drum_track(t);
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
        if (drum) {                                     /* 0.5: a strip a lane, KICK at the foot, an accent brighter */
            static const uint8_t COL[NLANE] = {0, 0, 0, 1, 1, 0, 1, 1}, ROW[NLANE] = {0, 1, 2, 0, 1, 3, 2, 3};
            int32_t two = (h - 2) / NLANE < 3, sh = (h - 2) / (two ? NLANE / 2 : NLANE), l;   /* 0.5.1: LEN 33..64 */
            uint32_t m = step_lanes(s), ac = step_accents(s);   /* two columns (KICK SNARE CLAP TOM | HATS RIM BELL) */
            cv_rrect(x, y, 12, h, 2, T_RAISE, T_SURF);
            for (l = 0; l < NLANE; l++)
                if ((m >> l) & 1u)
                    cv_rect(two ? x + 2 + 5 * COL[l] : x + 2, y + h - 1 - sh * ((two ? ROW[l] : l) + 1), two ? 3 : 8,
                            sh - (sh > 2), (ac >> l) & 1u ? T_TEXT : T_THEME);
            continue;
        }
        cv_rrect(x, y, 12, h, 2, note ? T_THEME : s->time == ST_TIE ? T_MID : T_RAISE, T_SURF);
        if (note) {
            int32_t span = hi > lo ? (int32_t)(hi - lo) : 1, ty = y + h - 5 - (int32_t)(s->note[0] - lo) * (h - 8) / span;
            cv_rect(x + 2, ty, 8, 3, T_SURF);
        }
    }
    {   /* 0.4: the rerolls on the keys, under the steps */
        char a[16], b[16], n[4];
        str_cpy(a, drum ? "C4 LOW " : "C4 NOTES ", sizeof a);
        fmt_int(n, fg.nvar);
        str_cpy(a + str_len(a), n, sizeof a - str_len(a));
        str_cpy(b, drum ? "D4 HATS " : "D4 RHYTHM ", sizeof b);
        fmt_int(n, fg.rvar);
        str_cpy(b + str_len(b), n, sizeof b - str_len(b));
        cv_text_on(10, 100 + CAP_IN(S, 18), &AF_S, a, fg.nvar ? T_THEME : T_MID, T_SURF);
        cv_text_r(230, 100 + CAP_IN(S, 18), &AF_S, b, fg.rvar ? T_THEME : T_MID, T_SURF);
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
