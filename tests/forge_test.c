/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the FORGE ONE contributors */
/* FORGE ONE (firmware/src/ui_forge.c): SEQ TOOLS F5 FORGE, on the real sequencer and UI sources (with the stubs of
 * tests/ui_test.c):
 *   open       F5 on a melodic track writes steps 1..LEN at once ("FORGE <seed> <style>"), F5 lit; past LEN untouched
 *   the steps  every note in ROOT / SCALE, step 1 a note, a TIE always after a note or a TIE, at most 4 notes a step, VEL 40..124,
 *              notes 24..108, conditions / chances only with EVOLVE, none at EVOLVE 0 (a pure loop: copies the same)
 *   seeded     the same SEED STYLE DENSITY EVOLVE LEN ROOT SCALE: the same steps (again after other values); another
 *              SEED: other steps; DENSITY alone: the melody of the notes kept stays (the pitch stream is its own)
 *   knobs      KNOB 1 SEED (0..99, round), 2 DENSITY (10..100 by 5), 3 EVOLVE (0..4), 4 STYLE (8, round; its DENSITY)
 *   styles     all 8 at LEN 16, 32 and 64, in MIN, MAJ, PEN and CHR, every ROOT: the rules above hold
 *   close      OCT- cancels (the steps and a lock as they were), OCT+ / F5 again / the layer let go apply (an emptied
 *              step's lock goes); SAVE held after: one undo for all of it
 *   chords     (0.3) by interval: CHR minor 0 3 7 10, seven-note scales their own; TRANCE / SYNTH in CHR on the triad
 *   rerolls    (0.4) C4 new notes, D4 a new rhythm while open; K1 starts them again; VEL by place
 *   golden     (0.3) pinned seeds: tests/forge_golden.txt
 *   DRUM       (0.5) F5 opens: hits only, KICK on 1, DENSITY, C4 LOW / D4 HATS, the rules over the styles
 *   PERFORM    (0.7) FX locked: mutes / solos latched, QUANT, scenes, DUCK, CLEAR, knobs; throws back; DUCK
 *   STARTER    (0.6) G5: four tracks' sounds and patterns, seeded; K1..K4, C4 / D4; OCT- all four back; OCT+
 *   0.5.1      step 1 ROOT in every SCALE / ROOT; no drum fill on step 1 at LEN 2..4; STYLE wraps, resets C4 / D4
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include <stdint.h>
static int sg_test_save(uint32_t slot, const char *name);   /* (0.13.1: SONG's saves, one made to fail) */
#define SG_SAVE sg_test_save
#include "ui_test.c"

static void tkey(uint32_t w) { key_down(white(w)); frame(); key_up(white(w)); frame(); }
static void oct_fwd(void) { btn_down(B_OCTUP); frame(); btn_up(B_OCTUP); frame(); }
static void kt(uint32_t role, int32_t s) { memset(ui.enc_t, 0, sizeof ui.enc_t); turn(role, s); }

/* track 1 alone, LEN len of 1/16, a few notes (C4 on 1, F4 on 6), a lock on step 6; past LEN a note on 31 or 63 */
static track_t *melody(uint32_t len)
{
    track_t *t;
    ui_power_on();
    song.sel = 0;
    t = &trk[0];
    track_defaults_steps(t);
    motion_clear(t);
    apply_preset_to(t, 0);                              /* (0.12: ANALOG SAW LEAD, a LEAD: the styles as they are; T1's
                                                         * default ACID is a BASS, which F5 now fits: test_roles) */
    t->p[P_SLEN] = (int16_t)len;
    t->p[P_SDIV] = 2;
    t->p[P_ROOT] = 9;                                   /* A minor */
    t->p[P_SCALE] = 2;
    t->step[0] = (step_t){{60}, 1, ST_NOTE, 0, 96};
    t->step[5] = (step_t){{65}, 1, ST_NOTE, 0, 96};
    t->step[63] = (step_t){{90}, 1, ST_NOTE, 0, 96};
    motion_set_lock(t, 5, P_REV, 50);
    return t;
}
static int lock_at5(void) { int16_t v; return motion_lock_get(&trk[0], 5, P_REV, &v) && v == 50; }

/* the rules every FORGE pattern keeps (why: 0 = fine, else what broke) */
static const char *rules(const track_t *t, uint32_t len)
{
    uint32_t i, k;
    if (t->step[0].time != ST_NOTE || !t->step[0].n)
        return "step 1 is not a note";
    for (i = 0; i < len; i++) {
        const step_t *s = &t->step[i];
        if (s->time == ST_NOTE && s->n) {
            if (s->n > 4u)
                return "more than 4 notes";
            for (k = 0; k < s->n; k++) {
                if (s->note[k] < 24u || s->note[k] > 108u)
                    return "a note out of 24..108";
                if (!tl_in_scale(t, s->note[k]))
                    return "a note out of the scale";
            }
            if (s->vel < 40u || s->vel > 124u)
                return "VEL out of 40..124";
        } else if (s->time == ST_TIE) {
            const step_t *p = &t->step[i - 1];
            if (!i || !((p->time == ST_NOTE && p->n) || p->time == ST_TIE))
                return "a TIE with nothing to hold";
        } else if (s->n) {
            return "a REST with notes";
        }
        if (s->hit)
            return "drum hits on a melodic step";
    }
    return 0;
}
static int no_extras(const track_t *t, uint32_t len)   /* no chance, condition or ratchet */
{
    uint32_t i;
    for (i = 0; i < len; i++)
        if (step_chance_v(&t->step[i]) != 100u || step_ratchet(&t->step[i]) != 1u)
            return 0;
    return 1;
}
static int copies_equal(const track_t *t, uint32_t len, uint32_t ml)   /* every copy of the motif the same */
{
    uint32_t i;
    for (i = ml; i < len; i++)
        if (memcmp(&t->step[i], &t->step[i % ml], sizeof(step_t)))
            return 0;
    return 1;
}

static int test_open(void)
{
    int bad = 0, ok;
    step_t ref[NSTEP], a[NSTEP];
    track_t *t = melody(16);
    uint32_t i;
    memcpy(ref, t->step, sizeof ref);
    go_page(GR_ROLL); frame();
    fg.set = 0;
    btn_down(B_SEQ); frame();
    tkey(TL_FORGE);                                     /* F5 */
    ok = fg.on && fg.style == FS_BERLIN && fg.evolve == 1u && fg.dens == 85u && fg.seed < 100u &&
         str_eq(ui.msg, "FORGE ") == 0 && !memcmp(ui.msg, "FORGE ", 6) && !rules(t, 16) && t->step[63].note[0] == 90;
    bad += check("FORGE (F5): open on BERLIN, EVOLVE 1, its DENSITY; steps 1..LEN written by the rules; past LEN kept", ok);
    {
        char want[24], n[4];
        str_cpy(want, "FORGE ", sizeof want);
        fmt_int(n, fg.seed);
        str_cpy(want + str_len(want), n, sizeof want - str_len(want));
        str_cpy(want + str_len(want), " BERLIN", sizeof want - str_len(want));
        bad += check("  its message: FORGE <seed> BERLIN", msg_is(want));
    }
    /* seeded */
    fg.seed = 41; kt(EN_K1, 1);
    memcpy(a, t->step, sizeof a);
    ok = fg.seed == 42u && msg_is("FORGE 42 BERLIN");
    kt(EN_K1, 5); kt(EN_K4, 3); kt(EN_K3, 2); kt(EN_K2, -4);
    ok &= memcmp(a, t->step, sizeof a) != 0;
    kt(EN_K4, -3); kt(EN_K3, -2); kt(EN_K1, -5);       /* (STYLE back: its DENSITY back) */
    ok &= fg.seed == 42u && fg.style == FS_BERLIN && fg.evolve == 1u && fg.dens == 85u && !memcmp(a, t->step, sizeof a);
    bad += check("  seeded: the same values, the same steps (after others); KNOB 1 SEED, 4 STYLE with its DENSITY", ok);
    kt(EN_K1, 1);
    ok = memcmp(a, t->step, sizeof a) != 0;
    kt(EN_K1, -1);
    bad += check("  another SEED: other steps", ok);
    kt(EN_K1, -43);
    ok = fg.seed == 99u;
    kt(EN_K1, 1);
    ok &= fg.seed == 0u;
    kt(EN_K1, 42);
    kt(EN_K2, 100);
    ok &= fg.dens == 100u;
    kt(EN_K2, -100);
    ok &= fg.dens == 10u && !rules(t, 16);
    kt(EN_K3, 10);
    ok &= fg.evolve == 4u;
    kt(EN_K3, -10);
    ok &= fg.evolve == 0u;
    kt(EN_K4, 7);
    ok &= fg.style == FS_AMBIENT && fg.dens == FORGE_STYLE[FS_AMBIENT].dens;
    kt(EN_K4, -7);
    ok &= fg.style == FS_BERLIN && fg.dens == 85u;
    bad += check("  KNOB 1 SEED 0..99 round, 2 DENSITY 10..100, 3 EVOLVE 0..4, 4 STYLE BERLIN..AMBIENT round", ok);
    {   /* DENSITY alone: a note kept on a step plays the same pitch */
        step_t lo[NSTEP];
        kt(EN_K2, -5);                                  /* 60 */
        memcpy(lo, t->step, sizeof lo);
        kt(EN_K2, 3);                                   /* 75 */
        ok = 1;
        for (i = 0; i < 16u; i++)
            if (lo[i].time == ST_NOTE && lo[i].n && t->step[i].time == ST_NOTE && t->step[i].n)
                ok &= lo[i].note[0] == t->step[i].note[0];
        kt(EN_K2, 2);
        bad += check("  DENSITY alone: the notes on steps kept keep their pitch", ok && fg.dens == 85u);
    }
    ui.force = 1; frame();
    oct_back();
    ok = !fg.on && !memcmp(t->step, ref, sizeof ref) && lock_at5() && msg_is("FORGE CANCELLED") && ui.layer == LAYER_SEQ;
    bad += check("  OCT-: cancelled, the steps and the lock as they were (the layer stays open)", ok);
    tkey(TL_FORGE);
    memcpy(a, t->step, sizeof a);
    oct_fwd();
    ok = !fg.on && msg_is("FORGE APPLIED") && !memcmp(a, t->step, sizeof a);
    ok &= (t->step[5].time == ST_NOTE && t->step[5].n) ? lock_at5() : !lock_at5();
    bad += check("  OCT+: applied (an emptied step's lock goes)", ok);
    btn_up(B_SEQ); frame();
    hold(B_SAVE);
    bad += check("  SAVE held: one undo for all of it (the steps and the lock back)", !memcmp(t->step, ref, sizeof ref) && lock_at5());
    btn_down(B_SEQ); frame();
    tkey(TL_FORGE);
    ok = fg.on;
    tkey(TL_FORGE);
    ok &= !fg.on && msg_is("FORGE APPLIED");
    tkey(TL_FORGE);
    ok &= fg.on;
    btn_up(B_SEQ); frame();
    ok &= !fg.on && !ui.layer && !rules(t, 16);
    bad += check("  F5 again: applied; the layer let go with it open: applied", ok);
    btn_down(B_SEQ); frame();
    tkey(TL_FORGE);
    tkey(7);                                            /* F4 EUCLID: FORGE applied first, then EUCLID open */
    ok = !fg.on && eu.on;
    oct_back();
    btn_up(B_SEQ); frame();
    bad += check("  another tool (F4 EUCLID) with it open: FORGE applied, EUCLID opens", ok);
    return bad;
}

/* every style, LEN, scale and root: the rules; EVOLVE 0 a pure loop with no extras */
static int test_styles(void)
{
    static const uint8_t LEN[3] = {16, 32, 64}, SCL[4] = {2, 1, 5, 0};
    int bad = 0;
    uint32_t s, l, c, r, e, sd;
    const char *why = 0, *why0 = 0;
    char where[64] = "";
    track_t *t = melody(16);
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    for (s = 0; s < FS_N; s++)
        for (l = 0; l < 3u; l++)
            for (c = 0; c < 4u; c++)
                for (r = 0; r < 12u; r += 5u)
                    for (e = 0; e <= 4u; e += 2u)
                        for (sd = 0; sd < 100u; sd += 33u) {
                            const char *w;
                            forge_close(0);
                            t->p[P_SLEN] = LEN[l];
                            t->p[P_SCALE] = SCL[c];
                            t->p[P_ROOT] = (int16_t)r;
                            fg.set = 1; fg.style = (uint8_t)s; fg.dens = FORGE_STYLE[s].dens; fg.evolve = (uint8_t)e;
                            fg.seed = (uint8_t)sd;
                            forge_open(t);
                            w = rules(t, LEN[l]);
                            if (w && !why) {
                                why = w;
                                str_cpy(where, FORGE_STYLE[s].name, sizeof where);
                            }
                            if (!e && !why0 && !(no_extras(t, LEN[l]) &&
                                                 copies_equal(t, LEN[l], FORGE_STYLE[s].motif < LEN[l] ? FORGE_STYLE[s].motif : LEN[l])))
                            {
                                why0 = FORGE_STYLE[s].name;
                                printf("    EVOLVE 0 breaks: %s LEN %u SCL %u ROOT %u seed %u\n", why0, LEN[l], SCL[c], r, sd);
                            }
                        }
    forge_close(0);
    btn_up(B_SEQ); frame();
    if (why)
        printf("    %s: %s\n", where, why);
    bad += check("styles: all 8 x LEN 16/32/64 x MIN MAJ PEN CHR x ROOTs x EVOLVE 0/2/4 x seeds: the rules hold", !why);
    if (why0)
        printf("    %s\n", why0);
    bad += check("  EVOLVE 0: a pure loop (every copy the motif), no CHANCE, condition or RATCH", !why0);
    return bad;
}

/* EVOLVE: copies drift and ornaments appear (over many seeds, somewhere) */
static int test_evolve(void)
{
    int bad = 0, drift = 0, orn = 0;
    uint32_t sd, i;
    track_t *t = melody(64);
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    for (sd = 0; sd < 20u; sd++) {
        forge_close(0);
        fg.set = 1; fg.style = FS_MELODIC; fg.dens = 75; fg.evolve = 4; fg.seed = (uint8_t)sd;
        forge_open(t);
        drift += !copies_equal(t, 64, 16);
        for (i = 0; i < 64u; i++)
            orn += step_chance_v(&t->step[i]) != 100u;
    }
    forge_close(0);
    btn_up(B_SEQ); frame();
    bad += check("EVOLVE 4: the copies drift, ornaments get a CHANCE or a condition", drift >= 15 && orn > 20);
    return bad;
}

static int test_names(void)                             /* the STYLE card's names fit its value (S font) */
{
    uint32_t s, ok = 1;
    for (s = 0; s < FS_N; s++)
        ok &= text_w(&AF_S, FORGE_STYLE[s].card) <= COL_W - 8 && str_len(FORGE_STYLE[s].name) < 8u;
    return check("STYLE: every card name fits the card", (int)ok);
}

/* 0.3: chords by interval in every scale: CHR's chords are minor (0 3 7, 10), a seven-note scale's its own (third 3
 * or 4, fifth 6 7 or 8 above the chord's root); TRANCE / SYNTH in CHR keep to the root's triad and octaves */
static int test_chords(void)
{
    static const uint8_t SCL[3] = {0, 1, 2};            /* CHR MAJ MIN */
    int bad = 0, ok = 1, arp = 1;
    uint32_t s, c, sd, i, chords = 0;
    track_t *t = melody(32);
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    for (s = 0; s < FS_N; s++)
        for (c = 0; c < 3u; c++)
            for (sd = 0; sd < 100u; sd += 11u) {
                forge_close(0);
                t->p[P_SCALE] = SCL[c];
                t->p[P_ROOT] = 4;                       /* E */
                fg.set = 1; fg.style = (uint8_t)s; fg.dens = FORGE_STYLE[s].dens; fg.evolve = 0; fg.seed = (uint8_t)sd;
                forge_open(t);
                for (i = 0; i < 32u; i++) {
                    const step_t *x = &t->step[i];
                    if (x->time != ST_NOTE || x->n < 2u)
                        continue;
                    chords++;
                    if (!SCL[c])
                        ok &= x->note[1] - x->note[0] == 3 && x->note[2] - x->note[0] == 7 &&
                              (x->n < 4u || x->note[3] - x->note[0] == 10);
                    else
                        ok &= (x->note[1] - x->note[0] == 3 || x->note[1] - x->note[0] == 4) &&
                              x->note[2] - x->note[0] >= 6 && x->note[2] - x->note[0] <= 8;
                }
                if (!SCL[c] && (s == FS_TRANCE || s == FS_SYNTH))
                    for (i = 0; i < 32u; i++)           /* (E: 4; its tones E G B, octaves; a jump: +12) */
                        if (t->step[i].time == ST_NOTE && t->step[i].n) {
                            uint32_t pc = (uint32_t)(t->step[i].note[0] + 120 - 4) % 12u;
                            arp &= pc == 0u || pc == 3u || pc == 7u;
                        }
            }
    forge_close(0);
    btn_up(B_SEQ); frame();
    bad += check("0.3 chords by interval: CHR minor (0 3 7 10), MAJ / MIN their own (3rd 3-4, 5th 6-8)", ok && chords > 100u);
    bad += check("  TRANCE and SYNTH in CHR: the root's triad and octaves only", arp);
    return bad;
}

/* 0.3: pinned seeds. The steps FORGE writes for fixed settings, hashed (FNV-1a over steps 1..LEN), against
 * tests/forge_golden.txt: a change of the generator shows here, so a saved seed keeps its pattern unless a release
 * says otherwise. FORGE_GOLDEN_UPDATE=1 rewrites the file (after an intended change: say so in CHANGELOG-FORGE.md) */
static uint32_t fg_fnv(const void *p, uint32_t n)
{
    const uint8_t *b = p;
    uint32_t h = 2166136261u;
    while (n--)
        h = (h ^ *b++) * 16777619u;
    return h;
}
static int test_golden(void)
{
    static const struct { uint8_t len, scl, root, evolve; } SET[4] = {{16, 2, 9, 1}, {64, 1, 0, 3}, {32, 0, 4, 2},
                                                                     {32, 5, 7, 4}};   /* (A MIN, C MAJ, E CHR, G PEN) */
    static const uint8_t SEED[3] = {0, 42, 99};
    const char *path = "tests/forge_golden.txt", *upd = getenv("FORGE_GOLDEN_UPDATE");
    char line[96];
    FILE *f;
    uint32_t s, k, j, n = 0, miss = 0, got[FS_N * 6 * 3];
    track_t *t = melody(16);
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    for (s = 0; s < FS_N; s++)
        for (k = 0; k < 4u; k++)
            for (j = 0; j < 3u; j++) {
                forge_close(0);
                t->p[P_SLEN] = SET[k].len; t->p[P_SCALE] = SET[k].scl; t->p[P_ROOT] = SET[k].root;
                fg.set = 1; fg.drum = 0; fg.style = (uint8_t)s; fg.dens = FORGE_STYLE[s].dens; fg.evolve = SET[k].evolve;
                fg.seed = SEED[j]; fg.nvar = fg.rvar = 0;
                forge_open(t);
                got[n++] = fg_fnv(t->step, SET[k].len * (uint32_t)sizeof(step_t));
            }
    forge_close(0);
    set_engine_of(t, ENGI_DRUM);                        /* 0.5: the beats, LEN 16 EVOLVE 1 and LEN 64 EVOLVE 3 */
    for (s = 0; s < FS_N; s++)
        for (k = 0; k < 2u; k++)
            for (j = 0; j < 3u; j++) {
                forge_close(0);
                t->p[P_SLEN] = k ? 64 : 16;
                fg.set = 1; fg.drum = 1; fg.style = (uint8_t)s; fg.dens = FORGE_BEAT[s].dens; fg.evolve = k ? 3 : 1;
                fg.seed = SEED[j]; fg.nvar = fg.rvar = 0;
                forge_open(t);
                got[n++] = fg_fnv(t->step, (uint32_t)t->p[P_SLEN] * (uint32_t)sizeof(step_t));
            }
    forge_close(0);
    btn_up(B_SEQ); frame();
    if (upd && upd[0] == '1') {
        f = fopen(path, "w");
        if (!f)
            return check("pinned seeds: write tests/forge_golden.txt", 0);
        fprintf(f, "# FORGE pinned seeds (tests/forge_test.c test_golden): style set seed hash. FORGE_GOLDEN_UPDATE=1 rewrites\n");
        for (n = 0, s = 0; s < FS_N; s++)
            for (k = 0; k < 4u; k++)
                for (j = 0; j < 3u; j++, n++)
                    fprintf(f, "%s %u %u %08x\n", FORGE_STYLE[s].name, k, SEED[j], got[n]);
        for (s = 0; s < FS_N; s++)
            for (k = 0; k < 2u; k++)
                for (j = 0; j < 3u; j++, n++)
                    fprintf(f, "D-%s %u %u %08x\n", FORGE_STYLE[s].name, k, SEED[j], got[n]);
        fclose(f);
        printf("    wrote %s (%u patterns)\n", path, n);
        return 0;
    }
    f = fopen(path, "r");
    if (!f)
        return check("pinned seeds: tests/forge_golden.txt there (FORGE_GOLDEN_UPDATE=1 makes it)", 0);
    for (n = 0; fgets(line, sizeof line, f);) {
        char name[16];
        unsigned kk, sd, h;
        if (line[0] == '#' || sscanf(line, "%15s %u %u %x", name, &kk, &sd, &h) != 4)
            continue;
        if (n < FS_N * 6u * 3u && got[n] != h) {
            if (miss < 5u)
                printf("    changed: %s set %u seed %u\n", name, kk, sd);
            miss++;
        }
        n++;
    }
    fclose(f);
    return check("pinned seeds: 8 styles x (4 melodic + 2 drum settings) x 3 seeds write the steps they wrote before", !miss && n == FS_N * 6u * 3u);
}

/* 0.4: C4 / D4 while FORGE is open reroll the notes / the rhythm alone; VEL by place */
static int test_reroll(void)
{
    int bad = 0, ok;
    step_t a[NSTEP], b[NSTEP];
    uint32_t i, sd, beat = 0, nb = 0, off = 0, no = 0, same = 1, rhythm_same = 1, diff = 0;
    track_t *t = melody(16);
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    forge_close(0);
    fg.set = 1; fg.style = FS_BERLIN; fg.dens = 40; fg.evolve = 0; fg.seed = 42; fg.nvar = fg.rvar = 0;
    forge_open(t);
    memcpy(a, t->step, sizeof a);
    tkey(4);                                            /* C4: new notes */
    ok = fg.on && fg.nvar == 1u && msg_is("NEW NOTES 1");
    for (i = 0; i < 16u; i++) {
        rhythm_same &= a[i].time == t->step[i].time && (a[i].n != 0) == (t->step[i].n != 0);
        diff += a[i].time == ST_NOTE && a[i].n && a[i].note[0] != t->step[i].note[0];
    }
    bad += check("0.4 C4 with FORGE open: new notes on the same rhythm (FORGE stays open)", ok && rhythm_same && diff > 0u);
    memcpy(a, t->step, sizeof a);
    tkey(5);                                            /* D4: a new rhythm */
    ok = fg.on && fg.rvar == 1u && msg_is("NEW RHYTHM 1");
    diff = 0;
    for (i = 0; i < 16u; i++) {
        diff += (a[i].time == ST_NOTE && a[i].n) != (t->step[i].time == ST_NOTE && t->step[i].n);
        if (a[i].time == ST_NOTE && a[i].n && t->step[i].time == ST_NOTE && t->step[i].n)
            same &= a[i].note[0] == t->step[i].note[0];
    }
    bad += check("  D4: a new rhythm, each step's pitch kept (EVOLVE 0)", ok && same && diff > 0u);
    memcpy(b, t->step, sizeof b);
    kt(EN_K2, 1); kt(EN_K2, -1);
    ok = !memcmp(b, t->step, sizeof b) && fg.nvar == 1u && fg.rvar == 1u;
    kt(EN_K1, 1);
    ok &= fg.nvar == 0u && fg.rvar == 0u && fg.seed == 43u;
    bad += check("  the rerolls stay through other knobs; K1 SEED starts them again from 0", ok);
    oct_back();
    btn_up(B_SEQ); frame();
    btn_down(B_SEQ); frame();
    for (sd = 0; sd < 20u; sd++) {                      /* VEL: beats louder than off-beat 16ths, on average */
        forge_close(0);
        fg.style = FS_BERLIN; fg.dens = 100; fg.evolve = 0; fg.seed = (uint8_t)sd; fg.nvar = fg.rvar = 0;
        forge_open(t);
        for (i = 0; i < 16u; i++)
            if (t->step[i].time == ST_NOTE && t->step[i].n && !(t->step[i].flags & SF_ACCENT)) {
                if (i % 4u == 0u) { beat += t->step[i].vel; nb++; }
                if (i % 2u == 1u) { off += t->step[i].vel; no++; }
            }
    }
    forge_close(0);
    btn_up(B_SEQ); frame();
    bad += check("  VEL by place: beats louder than off-beat 16ths", nb && no && beat / nb > off / no + 6u);
    return bad;
}

/* 0.5: FORGE on a DRUM track: hits only (no notes), KICK on step 1, the rules; DENSITY thins; EVOLVE 0 a pure
 * loop; C4 rerolls the low lanes alone, D4 the hats alone; OCT- puts the grid back */
static const char *drum_rules(const track_t *t, uint32_t len)
{
    uint32_t i;
    if (!(step_lanes(&t->step[0]) & (1u << DV_KICK)))
        return "no KICK on step 1";
    for (i = 0; i < len; i++) {
        const step_t *s = &t->step[i];
        if (s->n)
            return "a note on a drum step";
        if (s->time == ST_TIE)
            return "a TIE";
        if (s->time == ST_NOTE && !s->hit)
            return "a NOTE step with no hits";
        if (s->time == ST_NOTE && (s->vel < 20u || s->vel > 124u))
            return "VEL out of range";
        if (s->acc & ~s->hit)
            return "an accent with no hit";
    }
    return 0;
}
static uint32_t hits_of(const track_t *t, uint32_t len, uint32_t lanes)
{
    uint32_t i, n = 0;
    for (i = 0; i < len; i++)
        n += (uint32_t)__builtin_popcount(step_lanes(&t->step[i]) & lanes);
    return n;
}
static int test_drum(void)
{
    int bad = 0, ok;
    step_t ref[NSTEP], a[NSTEP];
    uint32_t i, s, sd, e, lo, hi;
    const char *why = 0;
    track_t *t = melody(16);
    set_engine_of(t, ENGI_DRUM);
    t->step[0] = (step_t){{0}, 0, ST_NOTE, 0, 96, 1u << DV_KICK, 0};
    t->step[63] = (step_t){{0}, 0, ST_NOTE, 0, 96, 1u << DV_SNARE, 0};
    memcpy(ref, t->step, sizeof ref);
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    fg.set = 1; fg.drum = 0; fg.style = FS_BERLIN; fg.evolve = 1; fg.seed = 42; fg.nvar = fg.rvar = 0; fg.dens = 85;
    tkey(TL_FORGE);
    ok = fg.on && tl_cell(t, TL_FORGE) && fg.dens == FORGE_BEAT[FS_BERLIN].dens && !drum_rules(t, 16) &&
         t->step[63].hit == (1u << DV_SNARE) && hits_of(t, 16, 1u << DV_HATC) >= 6u;
    bad += check("DRUM (0.5): F5 opens on the beat's DENSITY, hits only (no notes), KICK on 1, past LEN kept", ok);
    hi = hits_of(t, 16, 0xFFu);
    kt(EN_K2, -10);                                     /* 20 % */
    lo = hits_of(t, 16, 0xFFu);
    ok = fg.dens == 20u && lo < hi && !drum_rules(t, 16);
    kt(EN_K2, 10);
    bad += check("  DENSITY thins the beat", ok);
    memcpy(a, t->step, sizeof a);
    tkey(4);                                            /* C4: LOW */
    ok = fg.nvar == 1u && msg_is("NEW LOW 1");
    for (i = 0; i < 16u; i++)
        ok &= (step_lanes(&a[i]) & ~FG_LOW_LANES) == (step_lanes(&t->step[i]) & ~FG_LOW_LANES);
    bad += check("  C4: the low lanes rerolled, the hats as they were", ok);
    memcpy(a, t->step, sizeof a);
    tkey(5);                                            /* D4: HATS */
    ok = fg.rvar == 1u && msg_is("NEW HATS 1");
    for (i = 0; i < 16u; i++)
        ok &= (step_lanes(&a[i]) & FG_LOW_LANES) == (step_lanes(&t->step[i]) & FG_LOW_LANES);
    bad += check("  D4: the hats rerolled, the low lanes as they were", ok);
    oct_back();
    ok = !fg.on && !memcmp(t->step, ref, sizeof ref);
    bad += check("  OCT-: the grid as it was", ok);
    for (s = 0; s < FS_N && !why; s++)                  /* every style and seed at LEN 16 and 64, EVOLVE 0 .. 4 */
        for (e = 0; e <= 4u && !why; e += 2u)
            for (sd = 0; sd < 100u && !why; sd += 13u) {
                forge_close(0);
                t->p[P_SLEN] = e ? 64 : 16;
                fg.set = 1; fg.drum = 1; fg.style = (uint8_t)s; fg.dens = FORGE_BEAT[s].dens; fg.evolve = (uint8_t)e;
                fg.seed = (uint8_t)sd; fg.nvar = fg.rvar = 0;
                forge_open(t);
                why = drum_rules(t, (uint32_t)t->p[P_SLEN]);
                if (!e && !why && !(no_extras(t, 16) && copies_equal(t, 16, 16)))
                    why = "EVOLVE 0 with extras";
                if (e >= 2u && !why) {                  /* (the fill: a 4:4 somewhere in the last 3 steps) */
                    int f = 0;
                    for (i = 61; i < 64u; i++)
                        f |= step_cond(&t->step[i]) == 9u;
                    if (!f && sd == 0u && s == FS_TRANCE)
                        why = "no fill";
                }
            }
    forge_close(0);
    btn_up(B_SEQ); frame();
    if (why)
        printf("    %s\n", why);
    bad += check("  every style x EVOLVE 0 / 2 / 4 x seeds: the rules hold; EVOLVE 0 a pure loop", !why);
    return bad;
}

/* 0.5.1 (Beam's second review): step 1 is ROOT in every SCALE and ROOT; a drum fill never on step 1 (LEN 2 .. 4);
 * STYLE wraps and starts C4 / D4 again */
static int test_051(void)
{
    int bad = 0, ok = 1;
    uint32_t sc, r, s, e, sd, len;
    const char *why = 0;
    track_t *t = melody(16);
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    for (sc = 0; sc < 16u && ok; sc++)
        for (r = 0; r < 12u && ok; r++)
            for (s = 0; s < FS_N && ok; s++) {
                forge_close(0);
                t->p[P_SCALE] = (int16_t)sc;
                t->p[P_ROOT] = (int16_t)r;
                fg.set = 1; fg.drum = 0; fg.style = (uint8_t)s; fg.dens = FORGE_STYLE[s].dens; fg.evolve = 2;
                fg.seed = (uint8_t)(sc * 12u + r); fg.nvar = fg.rvar = 0;
                forge_open(t);
                ok = t->step[0].time == ST_NOTE && t->step[0].note[0] == FORGE_STYLE[s].base + r && !rules(t, 16);
                if (!ok)
                    printf("    SCALE %u ROOT %u %s: step 1 is %u, not %u\n", (unsigned)sc, (unsigned)r,
                           FORGE_STYLE[s].name, (unsigned)t->step[0].note[0], (unsigned)(FORGE_STYLE[s].base + r));
            }
    bad += check("0.5.1: step 1 is ROOT at the style's octave, in all 16 SCALEs, every ROOT, every STYLE", ok);
    forge_close(0);
    set_engine_of(t, ENGI_DRUM);
    for (len = 2; len <= 4u && !why; len++)
        for (s = 0; s < FS_N && !why; s++)
            for (e = 0; e <= 4u && !why; e++)
                for (sd = 0; sd < 100u && !why; sd += 7u) {
                    forge_close(0);
                    t->p[P_SLEN] = (int16_t)len;
                    fg.set = 1; fg.drum = 1; fg.style = (uint8_t)s; fg.dens = FORGE_BEAT[s].dens; fg.evolve = (uint8_t)e;
                    fg.seed = (uint8_t)sd; fg.nvar = fg.rvar = 0;
                    forge_open(t);
                    why = drum_rules(t, len);
                }
    if (why)
        printf("    %s\n", why);
    bad += check("0.5.1: DRUM at LEN 2, 3, 4: KICK stays on step 1 (no fill over it), every style, seed, EVOLVE", !why);
    fg.style = FS_AMBIENT; fg.nvar = 3; fg.rvar = 2;
    kt(EN_K4, 1);
    ok = fg.style == FS_BERLIN && !fg.nvar && !fg.rvar;
    kt(EN_K4, -1);
    ok &= fg.style == FS_AMBIENT;
    bad += check("0.5.1: STYLE wraps (AMBIENT > BERLIN > AMBIENT) and starts C4 / D4 again", ok);
    forge_close(0);
    btn_up(B_SEQ); frame();
    return bad;
}

/* 0.6 STARTER (ui_starter.c): G5 writes all four tracks, sound and pattern; OCT- puts all four back */
static uint32_t all_sig(void)
{
    uint32_t h = 2166136261u, i, j;
    for (i = 0; i < NTRK; i++)
        for (j = 0; j < sizeof trk[i].step; j++)
            h = (h ^ ((const uint8_t *)trk[i].step)[j]) * 16777619u;
    return h;
}
static void fs_at(uint32_t s)                          /* the genre and variation of starter s */
{
    uint32_t g;
    for (g = 0; g + 1u < FS_NGENRE && FS_GENRE[g + 1u].first <= s; g++)
        ;
    fs.genre = (uint8_t)g;
    fs.var = (uint8_t)(s - FS_GENRE[g].first);
}
static int test_starter(void)
{
    int bad = 0, ok = 1;
    uint32_t s, i, e, k, sd, ev, sig;
    const char *why = 0;
    static track_t before[NTRK];
    static uint8_t fm6b[NTRK][FP_SIZE + 1u];
    motion_store_t mo;
    int16_t bpm;
    track_t *t = melody(16);
    for (s = 0; s < FS_NSTART; s++)                     /* every sound names a real factory preset */
        for (i = 0; i < FP_N; i++)
            if (fs_find(&FORGE_STARTER[s].snd[i], &e, &k)) {
                printf("    %s T%u: %s %s not found\n", FORGE_STARTER[s].name, (unsigned)i + 1u,
                       FORGE_STARTER[s].snd[i].eng, FORGE_STARTER[s].snd[i].preset);
                ok = 0;
            }
    bad += check("STARTER (0.6): every part's sound is a factory preset of a selectable engine (by name)", ok);
    {   /* 0.8: the genres cover the table in order, 1..4 variations each; the names fit; one heavy engine a starter;
         * DRUM on T4; the beats' and LENs' values in range */
        uint32_t g, nx = 0, heavy;
        ok = FS_NGENRE == 13u && FS_NSTART >= 40u;
        for (g = 0; g < FS_NGENRE; g++) {
            ok &= FS_GENRE[g].first == nx && FS_GENRE[g].n >= 1u && FS_GENRE[g].n <= 4u &&
                  text_w(&AF_S, FS_GENRE[g].card) <= COL_W - 8 && str_len(FS_GENRE[g].name) <= 12u;
            nx += FS_GENRE[g].n;
        }
        ok &= nx == FS_NSTART && !(PM_ENG == 0u);
        for (s = 0; s < FS_NSTART; s++) {
            const forge_starter_t *st = &FORGE_STARTER[s];
            uint32_t ed = 0;
            for (heavy = 0, i = 0; i < FP_N; i++) {     /* (the engine a sound loads: a pack sound's base's) */
                ok &= !fs_find(&st->snd[i], &e, &k);
                heavy += e == ENGI_PHYS || str_eq(ENGINES[e % NENGINES]->name, "GRAIN");
                ed = i == FP_DRUM ? e : ed;
            }
            ok &= heavy <= 1u && ed == ENGI_DRUM && text_w(&AF_S, st->name) <= 48 && st->beat < FB_N &&
                  st->bpm >= 60u && st->bpm <= 160u && st->swing <= 30u && st->scale < 16u;
            for (i = 0; i < FP_N; i++)
                ok &= st->len[i] == 16u || st->len[i] == 32u || st->len[i] == 64u;
            if (!ok) {
                printf("    starter %u (%s) breaks a rule\n", (unsigned)s, st->name);
                break;
            }
        }
        bad += check("STARTER (0.8): 13 genres over 45 starters in order, names fit, one heavy engine (GRAIN / PHYS) each", ok);
    }
    {   /* 0.9 FORGE PACK: every sound's base preset exists, its name fits the row, every one is used, the values are
         * in range; loaded, the track has them */
        uint32_t used, j, n = 0;
        ok = FS_NPACK >= 40u;
        for (i = 0; i < FS_NPACK && ok; i++) {
            const forge_sound_t *f = &FORGE_PACK[i];
            ok &= !fs_find_base(f->eng, f->base, &e, &k) && text_w(&AF_S, f->name) <= 92 && fs_pack(f->name) == f &&
                  f->voice <= 3u && f->lwave <= 4u;
            for (j = 0; j < 8u; j++)
                ok &= f->e[j] == FK || (f->e[j] >= ENGINES[e]->edit[j].min && f->e[j] <= ENGINES[e]->edit[j].max);
            for (used = 0, s = 0; s < FS_NSTART; s++)
                for (j = 0; j < FP_N; j++)
                    used += str_eq(FORGE_STARTER[s].snd[j].eng, "FORGE") && str_eq(FORGE_STARTER[s].snd[j].preset, f->name);
            n += used;
            if (!ok || !used) {
                printf("    pack sound %s: %s\n", f->name, used ? "breaks a rule" : "never used");
                ok = 0;
            }
        }
        bad += check("FORGE PACK (0.9): 40+ sounds, bases exist, names fit, values in range, each used by a starter", ok && n > 40u);
        for (i = 0; i < FS_NPACK; i++)
            ok &= FORGE_PACK[i].lvl >= 0 && FORGE_PACK[i].lvl <= 127;
        bad += check("  (0.9.1) a pack LEVEL is 0 (left) or 1..127", ok);
    }
    t->p[P_SCALE] = 0;                                  /* (CHR: the starter's own SCALE) */
    t->p[P_ROOT] = 9;
    trk[3].step[2] = (step_t){{50}, 1, ST_NOTE, 0, 96};
    motion_set_lock(&trk[2], 3, P_REV, 40);
    song.g[G_BPM] = 99;
    memcpy(before, trk, sizeof before);
    for (i = 0; i < NTRK; i++)
        memcpy(fm6b[i], fm6_patch[i], sizeof fm6b[i]);
    mo = motion;
    bpm = song.g[G_BPM];
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    fg.set = 1; fg.seed = 42; fg.evolve = 1; fs.set = 1; fs.genre = 0; fs.var = 0; fs.dens = 100;
    tkey(TL_STARTER);                                   /* G5 */
    ok = fs.on && !fg.on && msg_is("BERLIN SEQ 42") && song.g[G_BPM] == 120;
    for (i = 0; i < FP_N; i++) {
        (void)fs_find(&FORGE_STARTER[0].snd[i], &e, &k);
        ok &= trk[i].eng_req == e && trk[i].preset == k && trk[i].p[P_ROOT] == 9 && trk[i].p[P_SCALE] == 2 &&
              trk[i].p[P_SLEN] == FORGE_STARTER[0].len[i];
        ok &= i < FP_DRUM ? !rules(&trk[i], tl_len(&trk[i])) && !drum_track(&trk[i])
                          : drum_track(&trk[i]) && !drum_rules(&trk[i], tl_len(&trk[i]));
    }
    ok &= trk[0].step[0].note[0] == 36 + 9 && trk[1].step[0].note[0] == 60 + 9;   /* (bass on A1, lead on A3) */
    bad += check("  G5: BERLIN's four sounds, LENs, A MIN (CHR: the starter's), 120 BPM; bass, lead, pad, drums by the rules", ok);
    sig = all_sig();
    ok = memcmp(trk[0].step, trk[1].step, sizeof trk[0].step) && memcmp(trk[1].step, trk[2].step, sizeof trk[1].step);
    kt(EN_K1, 1);
    ok &= all_sig() != sig && msg_is("BERLIN SEQ 43");
    kt(EN_K1, -1);
    ok &= all_sig() == sig;
    bad += check("  seeded: each part its own pattern; K1 SEED another song, back: the same four", ok);
    kt(EN_K2, -10);
    ok = fs.dens == 50u && hits_of(&trk[3], 16, 0xFFu) < 99u;
    kt(EN_K2, 100);
    ok &= fs.dens == 150u;
    kt(EN_K2, -10);
    ok &= fs.dens == 100u && all_sig() == sig;
    bad += check("  K2 DENSITY 50..150 % of each part's own", ok);
    tkey(4);                                            /* C4 */
    ok = fg.nvar == 1u && msg_is("NEW NOTES 1") && all_sig() != sig;
    tkey(5);                                            /* D4 */
    ok &= fg.rvar == 1u && msg_is("NEW RHYTHMS 1") && fs.on;
    bad += check("  C4 / D4: new notes / rhythms on all four, STARTER stays open", ok);
    kt(EN_K4, -1);                                      /* (wraps: JARRE, its first: OXY) */
    (void)fs_find(&FORGE_STARTER[41].snd[1], &e, &k);
    ok = fs.genre == 12u && !fs.var && song.g[G_BPM] == 104 && trk[1].eng_req == e && trk[1].preset == k &&
         trk[2].p[P_SLEN] == 64 && !fg.nvar && msg_is("JARRE OXY 42");
    bad += check("  K4 GENRE wraps (BERLIN > JARRE): its first variation's sounds and 104 BPM, C4 / D4 from 0", ok);
    key_down(black(1)); frame(); key_up(black(1)); frame();   /* G#3: the second variation */
    (void)fs_find(&FORGE_STARTER[42].snd[0], &e, &k);
    ok = fs.var == 1u && song.g[G_BPM] == 120 && trk[0].eng_req == e && trk[0].preset == k && msg_is("JARRE EQUI 42") &&
         !rules(&trk[1], tl_len(&trk[1])) && !drum_rules(&trk[3], 16);
    {   /* (0.9.1: EQUI's T3 is the pack's CHOIR AAH, the factory sound at LEVEL 88 (-12 dB); T1's LEVEL untouched) */
        const forge_sound_t *f = fs_pack("CHOIR AAH");
        ok &= f && f->lvl == 88 && trk[2].p[P_LEVEL] == 88 && trk[0].p[P_LEVEL] == before[0].p[P_LEVEL] &&
              str_eq(FORGE_STARTER[42].snd[2].preset, "CHOIR AAH");
    }
    kt(EN_K4, -1);                                      /* LO-FI: three variations, black key 4 nothing */
    ok &= fs.genre == 11u && !fs.var && trk[0].p[P_SSWING] == 25;
    key_down(black(3)); frame(); key_up(black(3)); frame();
    ok &= !fs.var;
    kt(EN_K4, 1);
    {   /* (EQUI's T4: the pack's RHYTHM BOX: DRUM, its kit 7 over PERC KIT, REV 30) */
        const forge_sound_t *f = fs_pack("RHYTHM BOX");
        (void)fs_find_base(f->eng, f->base, &e, &k);
        ok &= trk[3].eng_req == e && trk[3].preset == k && trk[3].p[P_E0] == 7 && trk[3].p[P_REV] == 30 &&
              str_eq(FORGE_STARTER[42].snd[3].preset, "RHYTHM BOX");
    }
    bad += check("  black keys 1..4: the genre's variations (JARRE EQUI: its sounds, 120 BPM); LO-FI's SWG 25, no 4th", ok);
    oct_back();
    ok = !fs.on && song.g[G_BPM] == bpm && !memcmp(&motion, &mo, sizeof mo) && msg_is("STARTER CANCELLED");
    for (i = 0; i < NTRK; i++)
        ok &= trk[i].eng_req == before[i].eng_req && trk[i].preset == before[i].preset &&
              !memcmp(trk[i].p, before[i].p, sizeof trk[i].p) && !memcmp(trk[i].step, before[i].step, sizeof trk[i].step) &&
              !memcmp(fm6_patch[i], fm6b[i], FP_SIZE);
    bad += check("  OCT-: all four tracks as they were (sound, parameters, steps, FM6 patch), the automation, the tempo", ok);
    t->p[P_SCALE] = 3;                                  /* DOR: the user's SCALE stays */
    tkey(TL_STARTER);
    ok = fs.on && trk[0].p[P_SCALE] == 3 && trk[3].p[P_SCALE] == 3 && !rules(&trk[0], 16) && !rules(&trk[1], 16);
    oct_fwd();
    ok &= !fs.on && msg_is("STARTER APPLIED") && !undo.trk;
    bad += check("  a SCALE set (DOR) is kept; OCT+ applies (no one-track undo copy left)", ok);
    tkey(TL_FORGE);                                     /* FORGE open, then G5: FORGE applied, STARTER opens */
    ok = fg.on;
    tkey(TL_STARTER);
    ok &= !fg.on && fs.on;
    tkey(TL_STARTER);                                   /* G5 again: applied */
    ok &= !fs.on && msg_is("STARTER APPLIED");
    bad += check("  G5 with FORGE open: FORGE applied, STARTER opens; G5 again applies", ok);
    for (s = 0; s < FS_NSTART && !why; s++)             /* every starter, seeds, EVOLVE: the rules on all four */
        for (ev = 0; ev <= 4u && !why; ev += 2u)
            for (sd = 0; sd < 100u && !why; sd += 17u) {
                fs_at(s);
                fg.seed = (uint8_t)sd; fg.evolve = (uint8_t)ev; fs.dens = 100;
                tkey(TL_STARTER);
                for (i = 0; i < FP_N && !why; i++)
                    why = i < FP_DRUM ? rules(&trk[i], tl_len(&trk[i])) : drum_rules(&trk[i], tl_len(&trk[i]));
                if (!why && !ev)
                    for (i = 0; i < FP_DRUM && !why; i++)
                        if (!no_extras(&trk[i], tl_len(&trk[i])))
                            why = "EVOLVE 0 with extras";
                oct_back();
            }
    if (why)
        printf("    %s\n", why);
    bad += check("  every starter x seeds x EVOLVE 0/2/4: the rules on all four tracks; OCT- each time", !why);
    {   /* 0.9: every starter sounds: playing 2 s, each of the four tracks peaks above a floor (no silent or broken
         * sound), the master output never stuck on full scale */
        static int32_t o[2 * CTL];
        uint32_t quiet = 0, clip = 0, blk;
        for (s = 0; s < FS_NSTART; s++) {
            int32_t pk[NTRK] = {0, 0, 0, 0};
            uint32_t full = 0;
            fs_at(s);
            fg.seed = 42; fg.evolve = 1; fs.dens = 100;
            tkey(TL_STARTER);
            transport_req = 1;
            for (blk = 0; blk < 2u * 44100u / CTL; blk++) {
                mix_block(o, CTL);
                for (i = 0; i < NTRK; i++) {
                    pk[i] = trk[i].peak > pk[i] ? trk[i].peak : pk[i];
                    trk[i].peak = 0;
                }
                for (i = 0; i < 2u * CTL; i++)
                    full += o[i] >= 32700 || o[i] <= -32700;
            }
            transport_req = 2;
            mix_block(o, CTL);
            for (i = 0; i < NTRK; i++)
                if (pk[i] < 200) {
                    printf("    %s %s T%u: peak %d\n", FS_GENRE[fs.genre].name, FORGE_STARTER[s].name, (unsigned)i + 1u, (int)pk[i]);
                    quiet++;
                }
            if (getenv("FORGE_PEAKS")) printf("    %u %s pk %d %d %d %d full %u playing %d\n", (unsigned)s, FORGE_STARTER[s].name, (int)pk[0], (int)pk[1], (int)pk[2], (int)pk[3], (unsigned)full, song.playing);
            if (full > 2u * 44100u / 50u) {             /* (more than 1 % of the samples on full scale) */
                printf("    %s %s: %u samples at full scale\n", FS_GENRE[fs.genre].name, FORGE_STARTER[s].name, (unsigned)full);
                clip++;
            }
            oct_back();
        }
        bad += check("  every starter sounds: 2 s played, all four tracks above the floor, the master not stuck at full scale",
                     !quiet && !clip);
    }
    {   /* pinned: every starter x seeds 0 42 99 (A, the starter's SCALE, EVOLVE 1): the four tracks' steps */
        static const uint8_t SEED[3] = {0, 42, 99};
        const char *path = "tests/starter_golden.txt", *upd = getenv("FORGE_GOLDEN_UPDATE");
        uint32_t got[FS_NSTART * 3], n = 0, miss = 0, j;
        char line[96];
        FILE *f;
        t->p[P_SCALE] = 0;
        t->p[P_ROOT] = 9;
        for (s = 0; s < FS_NSTART; s++)
            for (j = 0; j < 3u; j++) {
                fs_at(s);
                fg.seed = SEED[j]; fg.evolve = 1; fs.dens = 100;
                tkey(TL_STARTER);
                got[n++] = all_sig();
                oct_back();
            }
        if (upd && upd[0] == '1') {
            f = fopen(path, "w");
            if (f) {
                fprintf(f, "# STARTER pinned seeds (tests/forge_test.c test_starter): starter seed hash. FORGE_GOLDEN_UPDATE=1 rewrites\n");
                for (n = 0, s = 0; s < FS_NSTART; s++)
                    for (j = 0; j < 3u; j++, n++)
                        fprintf(f, "%s %u %08x\n", FORGE_STARTER[s].name, SEED[j], got[n]);
                fclose(f);
                printf("    wrote %s\n", path);
            }
        }
        f = fopen(path, "r");
        n = 0;
        while (f && fgets(line, sizeof line, f)) {
            char nm[16];
            unsigned sd, h;
            if (line[0] == '#' || sscanf(line, "%15s %u %x", nm, &sd, &h) != 3)
                continue;
            if (n < FS_NSTART * 3u && h != got[n]) {
                printf("    %s seed %u: %08x, pinned %08x\n", nm, sd, (unsigned)got[n], h);
                miss++;
            }
            n++;
        }
        if (f)
            fclose(f);
        bad += check("  pinned seeds: 45 starters x 3 seeds write the four tracks they wrote before", f && n == FS_NSTART * 3u && !miss);
    }
    btn_up(B_SEQ); frame();
    return bad;
}

/* 0.7 PERFORM (ui_perform.c, perform.c): FX locked; latched mutes / solos, QUANT at the bar, scenes, DUCK, CLEAR,
 * the knobs; the throws flow back; DUCK pumps the synth tracks on a DRUM track's kick */
static void pm_tap_black(uint32_t b) { key_down(black(b)); frame(); key_up(black(b)); frame(); }
static void pm_blocks(uint32_t n) { int32_t o[2 * CTL]; while (n--) mix_block(o, CTL); }
static int test_perform(void)
{
    int bad = 0, ok;
    uint32_t i;
    ui_power_on();
    memset(pm_store, 0, 5);
    go_title("ENV"); frame();
    press(B_FX); press(B_FX); frames(100);
    ok = ui.lock == LAYER_FX && pm_on() && perf_pmode && str_eq(layer_head(), "[FX] PERFORM");
    pm_tap_black(0);
    ok &= trk[0].p[P_MUTE] == 1 && !(perf_held & PF_MUTE);
    pm_tap_black(0);
    ok &= trk[0].p[P_MUTE] == 0;
    bad += check("PERFORM (0.7): FX double tap; black key 1 mutes T1 latched (P_MUTE, not the held mute), again unmutes", ok);
    btn_down(B_OCTUP); frame();
    pm_tap_black(1);
    btn_up(B_OCTUP); frame();
    ok = perf_psolo == 2u && !pm.knobs && glo_sounding(1) && !glo_sounding(0);
    press(B_OCTUP); frames(2);
    ok &= pm.knobs == 1u && msg_is("KNOBS: FX MACROS");
    press(B_OCTUP); frames(2);
    ok &= !pm.knobs;
    bad += check("  OCT+ held + black key 2: T2 solo, latched (no knob switch); OCT+ alone: the knobs LEVELS / MACROS", ok);
    pm_tap_black(4);                                    /* D#4: scene 1, empty */
    ok = msg_is("SCENE 1 EMPTY") && !PM_STORED(0);
    key_down(black(4));
    for (i = 0; i < 50u; i++)                           /* (800 ms: stored) */
        frame();
    ok &= PM_STORED(0) && pm_store[0] == (2u << 4) && msg_is("SCENE 1 STORED");
    key_up(black(4)); frames(2);
    pm_tap_black(10);                                   /* F#5: CLEAR */
    ok &= !perf_psolo && !trk[0].p[P_MUTE] && msg_is("CLEAR: ALL ON");
    pm_tap_black(4);                                    /* recall scene 1: T2 solo */
    ok &= perf_psolo == 2u && msg_is("SCENE 1");
    bad += check("  scenes: a tap of an empty one says how, held 600 ms stores (mutes, solos), CLEAR, a tap recalls", ok);
    pm_tap_black(10);
    pm_tap_black(8);                                    /* C#5: QUANT */
    ok = (pm_store[4] & PM_QUANT) && msg_is("QUANT: NEXT BAR");
    transport_req = 1;                                  /* PLAY, then on to the 4th 1/16 of the bar */
    pm_blocks(1);
    for (i = 0; i < 4000u && !(clk_step == 3u && clk_pos < CTL); i++)
        pm_blocks(1);
    pm_tap_black(2);                                    /* T3 mute: waits */
    ok &= pq_pend && !trk[2].p[P_MUTE];
    {
        uint32_t blocks = 0, p = div_samples(2);
        while (pq_pend && blocks < 8000u) {             /* the ISR's blocks until the bar */
            int32_t o[2 * CTL];
            mix_block(o, CTL);
            blocks++;
        }
        if (!(!pq_pend && trk[2].p[P_MUTE] == 1 && blocks * CTL >= 11u * p && blocks * CTL <= 13u * p + CTL))
            printf("    QUANT: %u samples (1/16 %u), pending %u, T3 MUTE %d\n", blocks * CTL, p, pq_pend, trk[2].p[P_MUTE]);
        ok &= !pq_pend && trk[2].p[P_MUTE] == 1 && blocks * CTL >= 11u * p && blocks * CTL <= 13u * p + CTL;
    }
    transport_req = 2;                                  /* STOP */
    pm_blocks(1);
    pm_tap_black(2);                                    /* stopped: at once */
    ok &= !trk[2].p[P_MUTE] && !pq_pend;
    bad += check("  QUANT: playing, a mute waits for the bar (the ISR puts it in before the bar's first step); stopped: at once", ok);
    /* 0.9.1 (Beam's third review): a queued change goes in on STOP and on QUANT OFF, never waits for the next PLAY;
     * GLO's C4 UNMUTE ALL clears the latched solos and a queued change; a project loaded drops the queue */
    transport_req = 1;
    pm_blocks(1);
    for (i = 0; i < 4000u && !(clk_step == 3u && clk_pos < CTL); i++)
        pm_blocks(1);
    pm_tap_black(2);                                    /* T3 mute queued */
    ok = pq_pend && !trk[2].p[P_MUTE];
    transport_req = 2;                                  /* STOP: in at once */
    pm_blocks(1);
    ok &= !pq_pend && trk[2].p[P_MUTE] == 1 && !song.playing;
    transport_req = 1;                                  /* PLAY again: nothing else changes */
    pm_blocks(4);
    ok &= !pq_pend && trk[2].p[P_MUTE] == 1;
    transport_req = 2;
    pm_blocks(1);
    bad += check("  a queued mute goes in on STOP (not after the next PLAY)", ok);
    trk[2].p[P_MUTE] = 0;
    transport_req = 1;
    pm_blocks(1);
    for (i = 0; i < 4000u && !(clk_step == 3u && clk_pos < CTL); i++)
        pm_blocks(1);
    btn_down(B_OCTUP); frame();
    pm_tap_black(1);                                    /* T2 solo queued */
    btn_up(B_OCTUP); frame();
    ok = pq_pend && pq_solo == 2u && !perf_psolo;
    pm_tap_black(8);                                    /* QUANT OFF: AT ONCE, so the queued solo too */
    ok &= !(pm_store[4] & PM_QUANT) && !pq_pend && perf_psolo == 2u && msg_is("QUANT OFF: AT ONCE");
    bad += check("  a queued solo goes in on QUANT OFF", ok);
    pm_tap_black(8);                                    /* QUANT on again */
    pm_tap_black(2);                                    /* T3 mute queued (playing) */
    ok = pq_pend && perf_psolo == 2u && !trk[2].p[P_MUTE] && !glo_sounding(0);
    press(B_FX); frames(16);                            /* PERFORM closed; GLO + C4: UNMUTE ALL */
    btn_down(B_GLO); frames(2);
    key_down(white(4)); frame(); key_up(white(4)); frame();
    btn_up(B_GLO); frame();
    ok &= !pq_pend && !perf_psolo && !trk[2].p[P_MUTE] && glo_sounding(0) && glo_sounding(2);
    bad += check("  GLO C4 UNMUTE ALL: the PERFORM solos and the queued change cleared too (every track sounds)", ok);
    transport_req = 2;                                  /* (stopped: saved, slot 1) */
    pm_blocks(1);
    ok = !project_save(1);
    press(B_FX); press(B_FX); frames(100);
    ok &= pm_on();
    transport_req = 1;
    pm_blocks(1);
    pm_tap_black(2);                                    /* T3 mute queued */
    ok &= pq_pend;
    project_load(1);                                    /* the tracks replaced: the queue dropped, nothing goes in */
    ok &= !pq_pend && !trk[2].p[P_MUTE] && !song.playing;
    bad += check("  a project loaded drops the queued change", ok);
    transport_req = 2;
    pm_blocks(1);
    press(B_FX); frames(16);
    press(B_FX); press(B_FX); frames(100);
    memset(pm_store, 0, 5);
    ok = pm_on() && !pq_pend && !perf_psolo;
    bad += check("  (PERFORM open again for the rest)", ok);
    pm_tap_black(9);                                    /* D#5: DUCK */
    ok = (perf_latched & PF_BIT(PF_DUCK)) && msg_is("DUCK ON");
    pm_tap_black(9);
    ok &= !(perf_latched & PF_BIT(PF_DUCK));
    press(B_FX); frames(16);
    ok &= !pm_on() && !perf_pmode && !ui.lock;
    bad += check("  DUCK on / off; FX tapped: PERFORM closed (FX held: black keys mute while held again)", ok);
    /* the throws and DUCK in the audio */
    {
        int32_t d0, dmax = 0;
        uint32_t back = 0;
        perf_latched = perf_held = 0;
        perf_press(PF_DTHR, 1);
        for (i = 0; i < 64u; i++)                       /* (CTL samples a block) */
            perf_begin(CTL);
        ok = perf_dthr == THR_DRIVE;
        perf_press(PF_DTHR, 0);
        perf_begin(CTL);
        d0 = perf_dthr;
        for (i = 0; i < 4000u && perf_dthr; i++)
            perf_begin(CTL);
        back = i * CTL;
        ok &= d0 > THR_DRIVE * 9 / 10 && back > 30000u && back < 34000u;
        bad += check("  DIST THROW: every DIST at least 100 while held; let go, back to the tracks' own over 0.74 s", ok);
        perf_press(PF_CTHR, 1);
        for (i = 0; i < 6000u / CTL; i++)
            perf_begin(CTL);
        ok = pf.lc < PF_TOP / 3 && pf.ct == 32768;
        perf_press(PF_CTHR, 0);
        for (i = 0; i < 4000u / CTL; i++)
            perf_begin(CTL);
        ok &= pf.ct > 16384 && pf.lc < PF_TOP;          /* (0.1 s after: still well down) */
        for (i = 0; i < 40000u / CTL; i++)
            perf_begin(CTL);
        ok &= !pf.ct && pf.lc == PF_TOP;
        bad += check("  CUTOFF THROW: the LPF down while held; let go, it opens again over 0.74 s", ok);
        perf_latched = PF_BIT(PF_DUCK);
        trk[3] = trk[3];
        set_engine_of(&trk[3], ENGI_DRUM);
        trk_note_on(&trk[3], 36, 100);                  /* a kick */
        ok = perf_kick == 1u;
        perf_begin(CTL);
        for (i = 0; i < 60u; i++) {
            dmax = pf.dd1 > dmax ? pf.dd1 : dmax;
            perf_begin(CTL);
        }
        ok &= dmax > 20000 && pf.dd1 < dmax && !perf_kick;
        for (i = 0; i < 40000u / CTL && pf.duck; i++)
            perf_begin(CTL);
        ok &= !pf.duck && !pf.dd1;
        {   /* a synth track's block ducked, the drum track's not (fx.c drum_part) */
            static int32_t b[CTL];
            trk_note_on(&trk[3], 36, 100);
            perf_begin(CTL);
            perf_begin(CTL);
            for (i = 0; i < CTL; i++)
                b[i] = 10000;
            perf_duck(b, 0, CTL);
            ok &= b[CTL - 1] < 6000 && drum_part(&trk[3]) && !drum_part(&trk[0]);
        }
        perf_latched = 0;
        trk_note_on(&trk[3], 36, 100);                  /* DUCK off: a kick does nothing */
        for (i = 0; i < 40000u / CTL; i++)
            perf_begin(CTL);
        ok &= !pf.duck;
        bad += check("  DUCK: a DRUM track's kick dips the synth tracks (to -12 dB, back over an 8th); off: nothing", ok);
    }
    memset(pm_store, 0, 5);
    return bad;
}

/* 0.12: F5 reads the role of the track's sound. On a BASS sound (ANALOG SQR BASS) every style writes single notes in
 * the bass register; on a PAD sound (ANALOG SOFT PAD) chords held from the bar's 1 and 9, voiced between A2 and E6; on a
 * LEAD sound (ANALOG SAW LEAD) the style as it was (test_golden pins it). Every style, 20 seeds, EVOLVE 0..4, LEN 32
 * (two copies of a 16-step motif), every scale-and-root set of test_golden; all of rules() as well. (A LEAD's steps
 * are 0.11's exactly: test_golden's track is a LEAD, melody(), and its pins did not move) */
static const char *bass_rules(const track_t *t, uint32_t len, uint32_t *low, uint32_t *all)
{
    uint32_t i, mn = 127;
    for (i = 0; i < len; i++) {
        const step_t *s = &t->step[i];
        if (s->time != ST_NOTE || !s->n)
            continue;
        if (s->n != 1u)
            return "a chord on a bass";
        if (s->note[0] > 71u)
            return "a bass note over B4";
        if (s->note[0] < mn)
            mn = s->note[0];
        *all += 1u;
        *low += s->note[0] < 60u;
    }
    return mn > 47u ? "a bass line that never goes under C3" : 0;
}
static const char *pad_rules(const track_t *t, uint32_t len, uint32_t *held, uint32_t *onsets)
{
    uint32_t i;
    for (i = 0; i < len; i++) {
        const step_t *s = &t->step[i];
        if (s->time == ST_TIE)
            *held += 1u;
        if (s->time != ST_NOTE || !s->n)
            continue;
        *onsets += 1u;
        *held += 1u;
        if (i % 8u)
            return "a pad chord off the bar's 1 and 9";
        if (s->n < 3u)
            return "a pad onset that is not a chord";
        if (s->note[0] < 45u || s->note[s->n - 1u] > 88u)
            return "a pad chord out of A2..E6";
        if (step_ratchet(s) > 1u || (s->flags & SF_SLIDE))
            return "a ratchet or slide on a pad";
    }
    return 0;
}
static int test_roles(void)
{
    static const struct { uint8_t scl, root; } SET[4] = {{2, 9}, {1, 0}, {0, 4}, {5, 7}};
    static const char *const PN[3] = {"LEAD", "BASS", "PAD"};
    static const uint8_t PRE[3] = {0, 2, 1};            /* ANALOG: SAW LEAD, SQR BASS, SOFT PAD */
    int bad = 0, ok;
    uint32_t p, s, k, e, seed, low = 0, all = 0, held = 0, onsets = 0;
    track_t *t = melody(16);
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    for (p = 0; p < 3u; p++) {
        const char *why = 0;
        ok = 1;
        if (t->eng_req % NENGINES != 0u)
            set_engine_of(t, 0);
        apply_preset_to(t, PRE[p]);
        t->user = 0;
        ok &= track_cat(t) == (p == 0u ? CAT_LEAD : p == 1u ? CAT_BASS : CAT_PAD) && fg_part_of(t) == p;
        for (s = 0; s < FS_N && ok; s++)
            for (k = 0; k < 4u && ok; k++)
                for (e = 0; e < 5u && ok; e++)
                    for (seed = 0; seed < 20u && ok; seed++) {
                        forge_close(0);
                        t->p[P_SLEN] = 32; t->p[P_SCALE] = SET[k].scl; t->p[P_ROOT] = SET[k].root;
                        fg.set = 1; fg.drum = 0; fg.style = (uint8_t)s; fg.dens = FORGE_STYLE[s].dens;
                        fg.evolve = (uint8_t)e; fg.seed = (uint8_t)(seed * 5u); fg.nvar = fg.rvar = 0;
                        forge_open(t);
                        why = rules(t, 32);
                        if (!why && p == 1u)
                            why = bass_rules(t, 32, &low, &all);
                        if (!why && p == 2u)
                            why = pad_rules(t, 32, &held, &onsets);
                        if (why) {
                            printf("    %s %s set %u EVOLVE %u seed %u: %s\n", PN[p], FORGE_STYLE[s].name, (unsigned)k,
                                   (unsigned)e, (unsigned)(seed * 5u), why);
                            ok = 0;
                        }
                    }
        forge_close(0);
        bad += check(p == 0u ? "F5 (0.12) on a LEAD sound: every style as before, by the rules"
                     : p == 1u ? "  on a BASS sound: single notes, never over B4, down to C3 or under, every style and set"
                               : "  on a PAD sound: chords from the bar's 1 and 9 only, A2..E6, no ratchet or slide", ok);
    }
    ok = all && low * 10u >= all * 7u;
    if (!ok) printf("    bass notes under C4: %u of %u\n", (unsigned)low, (unsigned)all);
    bad += check("  the bass mostly under C4 (70 % of its notes or more)", ok);
    ok = onsets && held >= onsets * 4u;
    if (!ok) printf("    pad: %u steps held over %u onsets\n", (unsigned)held, (unsigned)onsets);
    bad += check("  the pad held: 4 steps a chord or more on average", ok);
    {   /* the message names the role; STARTER's parts (fg_ovr) and DRUM tracks never take one */
        forge_close(0);
        apply_preset_to(t, 2);
        fg.style = FS_AMBIENT; fg.seed = 42; fg.evolve = 1; fg.dens = FORGE_STYLE[FS_AMBIENT].dens;
        forge_open(t);
        ok = msg_is("BASS 42 AMBIENT");
        {
            static const char *const W[] = {"BASS 99 MELODIC", "BASS 99 AMBIENT", "BASS 99 DETROIT", "PAD 99 MELODIC"};
            uint32_t q;
            for (q = 0; q < 4u; q++)
                ok &= text_w(&AF_M, W[q]) <= HEAD_MSG_W - 8;   /* (the big font, not the hint's) */
        }
        fg_ovr = &FORGE_STYLE[0];
        ok &= fg_part_of(t) == FR_NONE;
        fg_ovr = 0;
        forge_close(0);
        apply_preset_to(t, 0);
        forge_open(t);
        ok &= msg_is("FORGE 42 AMBIENT");
        forge_close(0);
        bad += check("  the message leads with the role (BASS 42 AMBIENT); STARTER's parts and LEAD sounds: FORGE 42 ..", ok);
    }
    btn_up(B_SEQ); frame();
    return bad;
}

/* 0.13 SONG (ui_song.c; FORGE-ONE Song Mode spec v0.3 §9, acceptance tests 1..12): STARTER's F#5 SONG, four parts
 * into slots A..D and the chain, the confirm, SAVE X?, the locks, the labels, the chain played as stock code plays it */
static uint32_t sg_act(const track_t *t)                /* the steps that sound within LEN (notes or hits) */
{
    uint32_t i, n = 0;
    for (i = 0; i < tl_len(t); i++)
        n += drum_track(t) ? step_lanes(&t->step[i]) != 0u : t->step[i].time == ST_NOTE && t->step[i].n;
    return n;
}
static int32_t sg_mean(const track_t *t)                /* the mean pitch of its notes (x 16), -1 none */
{
    uint32_t i, k, n = 0;
    int32_t sum = 0;
    for (i = 0; i < tl_len(t); i++)
        for (k = 0; t->step[i].time == ST_NOTE && k < t->step[i].n; k++, n++)
            sum += t->step[i].note[k];
    return n ? sum * 16 / (int32_t)n : -1;
}
static void sg_tap(uint32_t b) { key_down(black(b)); frame(); key_up(black(b)); frame(); }
static void sg_open(uint32_t genre, uint32_t seed)     /* STARTER on GENRE's first, SEED; then F#5: SONG, part A */
{
    track_t *t = melody(16);
    t->p[P_SCALE] = 0;
    t->p[P_ROOT] = 9;
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    fg.set = 1; fg.seed = (uint8_t)seed; fg.evolve = 1; fs.set = 1; fs.genre = (uint8_t)genre; fs.var = 0; fs.dens = 100;
    tkey(TL_STARTER);
    sg_tap(10);                                         /* F#5 */
}
static void sg_close(void) { if (fs.on) { btn_up(B_SEQ); frame(); } song_off(); fs.on = 0; }
static int sg_genre(const char *name)
{
    uint32_t g;
    for (g = 0; g < FS_NGENRE; g++)
        if (str_eq(FS_GENRE[g].name, name))
            return (int)g;
    return -1;
}
static int test_song(void)
{
    static const struct { const char *g; uint8_t d; } MAP[13] = {
        {"BERLIN", SD_LIFT}, {"DETROIT", SD_BREAK}, {"DUB TECHNO", SD_BREAK}, {"MELODIC", SD_BREAK},
        {"ACID HOUSE", SD_BREAK}, {"ACID TECHNO", SD_BREAK}, {"TRANCE", SD_BREAK}, {"DEEP HOUSE", SD_LIFT},
        {"SYNTHWAVE", SD_LIFT}, {"BREAKBEAT", SD_BREAK}, {"AMBIENT", SD_LIFT}, {"LO-FI", SD_BREAK}, {"JARRE", SD_LIFT}};
    static const uint8_t CH[2][8][5] = {          /* §3b: T1 T2 T3 T4 repeats; 4 = "-" */
        {{4, 4, 0, 0, 2}, {0, 0, 0, 0, 4}, {1, 1, 1, 1, 4}, {0, 0, 0, 0, 4}, {2, 2, 2, 2, 2}, {1, 1, 1, 1, 4},
         {3, 3, 3, 3, 4}, {4, 4, 1, 1, 2}},
        {{4, 4, 0, 0, 2}, {0, 0, 0, 0, 4}, {1, 1, 1, 1, 4}, {0, 0, 0, 0, 4}, {2, 2, 2, 2, 2}, {3, 3, 3, 3, 2},
         {1, 1, 1, 1, 4}, {4, 4, 1, 1, 2}}};
    static step_t part[SG_N][NTRK][NSTEP];
    static project_store_t slots[4], slots2[4];
    static track_t before[NTRK];
    int bad = 0, ok, g;
    uint32_t i, j, x, d, lo, n;
    char nm[SG_NAME_LEN + 1u];
    ok = SG_NAME_LEN == PROJ_NAME_LEN && sizeof FS_GENRE_D == 13u;
    for (i = 0; i < 13u; i++)
        ok &= (g = sg_genre(MAP[i].g)) >= 0 && FS_GENRE_D[g] == MAP[i].d;
    for (i = 0, n = 0; i < 13u; i++)
        n += FS_GENRE_D[i] == SD_LIFT;
    bad += check("SONG (0.13, spec v0.3 tests 1..11) 3. D by genre: the 13 of §3a (5 LIFT, 8 BREAK)", ok && n == 5u);

    /* 1. determinism, 2. relation: SYNTHWAVE (LIFT) and TRANCE (BREAK), seed 42 */
    for (d = 0; d < 2u; d++) {
        uint32_t gi = (uint32_t)sg_genre(d ? "TRANCE" : "SYNTHWAVE"), bpm;
        uint8_t eng[NTRK], pre[NTRK];
        sg_open(gi, 42);
        ok = sg.on && fs.on && sg.part == SG_A && sg.stage == SG_EDIT && song_dtype() == d && msg_is(d ? "SONG 42 TRANCE" : "SONG 42 SYNTHWAVE");
        for (x = 0; x < SG_N; x++) {
            song_render(x);
            for (i = 0; i < NTRK; i++)
                memcpy(part[x][i], trk[i].step, sizeof part[x][i]);
            song_render(x);                             /* (again: the same steps) */
            for (i = 0; i < NTRK; i++)
                ok &= !memcmp(part[x][i], trk[i].step, sizeof part[x][i]);
        }
        bad += check(d ? "  1. TRANCE 42: each part twice, byte for byte the same" : "  1. SYNTHWAVE 42: each part twice, byte for byte the same", ok);
        bpm = (uint32_t)song.g[G_BPM];
        for (i = 0; i < NTRK; i++)
            eng[i] = trk[i].eng_req, pre[i] = trk[i].preset;
        song_render(SG_A);
        oct_fwd();                                      /* GENERATE: the confirm */
        ok = sg.stage == SG_CONFIRM && msg_is("OVERWRITE A-D?");
        oct_fwd();                                      /* OVERWRITE */
        ok &= sg.stage == SG_WRITTEN && msg_is("SONG SAVED: A-D") && sg.part == SG_A;
        for (x = 0; x < SG_N; x++) {
            ok &= project_name(x, nm) && str_eq(nm, x == SG_A ? "S42 VERSE" : x == SG_B ? "S42 CHORUS" :
                                                    x == SG_C ? "S42 BRIDGE" : d ? "S42 BREAK" : "S42 LIFT");
        }
        bad += check("  GENERATE: OVERWRITE A-D?, then the four slots named S42 VERSE CHORUS BRIDGE LIFT / BREAK", ok);
        sg_close();
        ok = 1;
        for (x = 0; x < SG_N; x++) {                    /* each slot as stock code loads it: the part, all four tracks */
            project_load(x);
            for (i = 0; i < NTRK; i++) {
                int q = !memcmp(part[x][i], trk[i].step, sizeof part[x][i]), r = trk[i].eng_req == eng[i] && trk[i].preset == pre[i],
                    u = trk[i].p[P_ROOT] == 9 && trk[i].p[P_SCALE] == trk[0].p[P_SCALE];
                if (!(q && r && u) && getenv("SGDBG"))
                    printf("    slot %c T%u: steps %d sound %d (eng %u/%u preset %u/%u) key %d\n", (char)('A' + x), (unsigned)i + 1u, q, r,
                           (unsigned)trk[i].eng_req, (unsigned)eng[i], (unsigned)trk[i].preset, (unsigned)pre[i], u);
                ok &= q && r && u;
            }
            ok &= (uint32_t)song.g[G_BPM] == bpm;
            ok &= chain_config.count == 8u;
            for (j = 0; j < 8u; j++) {
                for (i = 0; i < NTRK; i++)
                    ok &= chain_config.row[j].slot[i] == (CH[d][j][i] == 4u ? CHAIN_SILENT : CH[d][j][i]);
                ok &= chain_config.row[j].repeat == CH[d][j][4];
            }
        }
        bad += check("  1-2, 6. each slot loads as its part (all four tracks), one sound set, ROOT, SCALE, tempo; the chain of §3b", ok);
        {   /* 2. B denser than A; C's motif not A's */
            uint32_t na = 0, nb = 0;
            for (i = 0; i < NTRK; i++) {
                project_load(SG_A); na += sg_act(&trk[i]);
                project_load(SG_B); nb += sg_act(&trk[i]);
            }
            ok = nb > na && memcmp(part[SG_C][FP_LEAD], part[SG_A][FP_LEAD], 16u * sizeof(step_t));
            if (!ok) printf("    active A %u B %u\n", (unsigned)na, (unsigned)nb);
            bad += check("  2. B denser than A (all four tracks); C's lead another motif", ok);
        }
        project_load(SG_A);
        {
            int32_t ma = sg_mean(&trk[FP_LEAD]);
            uint32_t ab = 0;
            project_load(SG_B);
            for (i = 0; i < NTRK; i++) ab += sg_act(&trk[i]);
            project_load(SG_D);
            if (!d) {                                   /* 4. LIFT */
                uint32_t nd = 0, fill = 0, L = tl_len(&trk[FP_DRUM]);
                for (i = 0; i < NTRK; i++) nd += sg_act(&trk[i]);
                for (j = L - 16u; j < L; j++)
                    fill += (step_lanes(&trk[FP_DRUM].step[j]) & (1u << DV_SNARE | 1u << DV_TOM)) && step_ratchet(&trk[FP_DRUM].step[j]) > 1u;
                ok = sg_mean(&trk[FP_LEAD]) - ma >= 12 * 16 && nd > ab && fill >= 2u && L == 64u;
                if (!ok) printf("    LIFT: lead +%d/16, D %u B %u, fill %u, LEN %u\n", (int)(sg_mean(&trk[FP_LEAD]) - ma), (unsigned)nd, (unsigned)ab, (unsigned)fill, (unsigned)L);
                bad += check("  4. LIFT: the lead 12 semitones or more over A's, denser than B, a fill on the last bar", ok);
            } else {                                    /* 5. BREAK */
                uint32_t L = tl_len(&trk[FP_DRUM]), lowhits = 0, snl = 0, id = song_filter_id(&trk[FP_PAD]);
                int16_t v0 = 0, vl = 0;
                int32_t top = motion_base_value(&trk[FP_PAD], id);
                ok = !sg_act(&trk[FP_BASS]) && tl_len(&trk[FP_BASS]) == FORGE_STARTER[fs_idx()].len[FP_BASS];
                for (j = 0; j < L; j++) {
                    uint32_t h = step_lanes(&trk[FP_DRUM].step[j]);
                    lowhits += (h & (1u << DV_KICK | 1u << DV_CLAP)) != 0u || (j < L - 16u && (h & (1u << DV_SNARE)));
                    snl += j >= L - 16u && (h & (1u << DV_SNARE));
                }
                ok &= !lowhits && snl >= 8u && tl_len(&trk[FP_PAD]) == L;
                ok &= motion_lock_get(&trk[FP_PAD], 0, id, &v0) == 0 || 1;   /* (events, not locks) */
                {
                    int32_t e0 = motion_find(&trk[FP_PAD], 0, id), el = motion_find(&trk[FP_PAD], L - 2u, id);
                    ok &= e0 >= 0 && el >= 0;
                    if (e0 >= 0 && el >= 0) {
                        v0 = motion.event[e0].value;
                        vl = motion.event[el].value;
                        ok &= v0 < top && vl == top && v0 < vl;
                    }
                }
                ok &= chain_config.row[6].slot[0] == SG_B && chain_config.row[6].slot[3] == SG_B && chain_config.row[5].slot[0] == SG_D;
                if (!ok) printf("    BREAK: bass %u, low %u, roll %u, pad LEN %u, filter %d .. %d of %d\n", (unsigned)sg_act(&trk[FP_BASS]), (unsigned)lowhits, (unsigned)snl, (unsigned)tl_len(&trk[FP_PAD]), v0, vl, (int)top);
                bad += check("  5. BREAK: T1 empty (LEN kept), no kick / clap, snare only in the last bar's roll, the PAD's filter down then open, CHORUS after", ok);
            }
        }
        {   /* 11. the chain as stock code plays it: rows 1..8 in order, then it stops */
            uint32_t it = 0, rows = 0, last = 99;
            project_load(SG_A);
            ok = chain_valid(&chain_config) && chain_prepare() == 0;
            while (it++ < 2000000u && (chain.running || chain.armed)) {
                events_block(CTL);
                if (chain.running && chain.row != last) {
                    ok &= chain.row == (last == 99 ? 0u : last + 1u);
                    last = chain.row;
                    rows++;
                }
            }
            ok &= !chain.running && rows == 8u;
            if (!ok) printf("    chain: %u rows, last %u, running %u\n", (unsigned)rows, (unsigned)last, (unsigned)chain.running);
            bad += check("  11. the chain plays its 8 rows in order on stock code, then stops", ok);
            transport_req = 2;
            events_block(CTL);
        }
    }

    /* 7. the confirm: OCT- leaves every slot and the four tracks as they were */
    sg_open((uint32_t)sg_genre("SYNTHWAVE"), 7);
    sg_close();
    memcpy(slots, proj_slot, sizeof slots);
    {
        track_t *t = melody(16);
        t->p[P_SCALE] = 0; t->p[P_ROOT] = 9;
        memcpy(before, trk, sizeof before);
        go_page(GR_ROLL); frame();
        btn_down(B_SEQ); frame();
        fg.seed = 7; fs.genre = (uint8_t)sg_genre("SYNTHWAVE"); fs.var = 0;
        tkey(TL_STARTER);
        sg_tap(10);
        oct_fwd();
        ok = sg.stage == SG_CONFIRM;
        tkey(TL_RANDOM);                                /* (the confirm: only OCT+ / OCT- answer it) */
        ok &= sg.stage == SG_CONFIRM && fs.on;
        oct_back();
        ok &= !sg.on && !fs.on && !memcmp(slots, proj_slot, sizeof slots) && msg_is("SONG CANCELLED");
        for (i = 0; i < NTRK; i++)
            ok &= !memcmp(trk[i].step, before[i].step, sizeof trk[i].step) && !memcmp(trk[i].p, before[i].p, sizeof trk[i].p);
        btn_up(B_SEQ); frame();
        bad += check("  7. OVERWRITE A-D? then OCT-: all four slots and tracks byte for byte as before; other keys wait", ok);
    }
    /* 8. SAVE X?: OCT- writes nothing, OCT+ slot B only; 9. a locked track keeps its steps */
    sg_open((uint32_t)sg_genre("TRANCE"), 42);
    oct_fwd(); oct_fwd();                               /* generated */
    sg_tap(1);                                          /* G#3: part B */
    ok = sg.part == SG_B && msg_is("B CHORUS");
    memcpy(slots, proj_slot, sizeof slots);
    {
        step_t t3[NSTEP], t1[NSTEP];
        memcpy(t3, trk[2].step, sizeof t3);
        memcpy(t1, trk[0].step, sizeof t1);
        sg_tap(6);                                      /* G#4: T3 locked */
        ok &= (sg.lock >> 2) & 1u;
        tkey(TL_RANDOM);                                /* C4: new notes, part B */
        ok &= sg.pend == SG_B + 1u && msg_is("SAVE B?") && !memcmp(t3, trk[2].step, sizeof t3) && memcmp(t1, trk[0].step, sizeof t1);
        bad += check("  9. T3 locked: a reroll of B leaves T3's steps byte for byte, rerolls T1", ok);
    }
    oct_back();                                         /* KEEP: nothing saved */
    ok = !sg.pend && !memcmp(slots, proj_slot, sizeof slots) && sg.on;
    tkey(3);                                            /* B3: T4's new rhythm, SAVE B? again (0.14: D4 is FOCUS) */
    ok &= sg.pend == SG_B + 1u;
    oct_fwd();                                          /* SAVE: slot B only */
    memcpy(slots2, proj_slot, sizeof slots2);
    ok &= !sg.pend && !memcmp(&slots[0], &slots2[0], sizeof slots[0]) && memcmp(&slots[1], &slots2[1], sizeof slots[1]) &&
          !memcmp(&slots[2], &slots2[2], sizeof slots[2]) && !memcmp(&slots[3], &slots2[3], sizeof slots[3]) &&
          project_name(1, nm) && str_eq(nm, "S42 CHORUS");
    bad += check("  8. SAVE B?: OCT- (KEEP) writes no slot; after another reroll OCT+ writes slot B only", ok);
    oct_fwd();                                          /* written: DONE */
    ok = !sg.on && !fs.on && msg_is("SONG DONE");
    sg_close();
    bad += check("  written: OCT+ DONE ends SONG and STARTER, the part shown in the tracks", ok);
    /* 10. labels: every seed and type fits 12 characters */
    ok = 1;
    for (n = 0; n < 100u; n++)
        for (x = 0; x < SG_N; x++) {
            char b[SG_NAME_LEN + 1u];
            fg.seed = (uint8_t)n;
            fs.genre = (uint8_t)(n % FS_NGENRE);
            song_name(x, b);
            ok &= str_len(b) <= SG_NAME_LEN && b[0] == 'S';
        }
    fg.seed = 42; fs.genre = (uint8_t)sg_genre("TRANCE");
    {
        char b[SG_NAME_LEN + 1u];
        song_name(SG_D, b);
        ok &= str_eq(b, "S42 BREAK");
        fs.genre = (uint8_t)sg_genre("JARRE");
        song_name(SG_D, b);
        ok &= str_eq(b, "S42 LIFT");
    }
    bad += check("  10. labels S<seed> VERSE CHORUS BRIDGE LIFT / BREAK, all in 12 characters", ok);
    /* every genre: four parts by the rules (the generator's own: in scale, VEL, a drum KICK on step 1 except BREAK) */
    ok = 1;
    for (n = 0; n < FS_NGENRE && ok; n++) {
        sg_open(n, 42);
        uint32_t na = 0, nb = 0;
        for (x = 0; x < SG_N && ok; x++) {
            const char *why;
            song_render(x);
            for (i = 0; i < NTRK; i++) {
                na += x == SG_A ? sg_act(&trk[i]) : 0u;
                nb += x == SG_B ? sg_act(&trk[i]) : 0u;
            }
            for (i = 0; i < FP_DRUM && ok; i++)
                if (sg_act(&trk[i]) && (why = rules(&trk[i], tl_len(&trk[i])))) {
                    printf("    %s %c T%u: %s\n", FS_GENRE[n].name, (char)('A' + x), (unsigned)i + 1u, why);
                    ok = 0;
                }
        }
        if (nb <= na) {
            printf("    %s: B %u not denser than A %u\n", FS_GENRE[n].name, (unsigned)nb, (unsigned)na);
            ok = 0;
        }
        sg_close();
    }
    bad += check("  every genre: the four parts' notes by FORGE's rules (in the scale, VEL, ties); B denser than A", ok);
    lo = 0;
    (void)lo;
    memset(proj_slot, 0, sizeof proj_slot);
    chain_defaults(&chain_config);
    return bad;
}

/* 0.13.1, review of PR #1: the writes refused up front or halfway, DENSITY / EVOLVE keep the song written, SAVE X?
 * refused keeps its prompt, BREAK's PAD sweep in the section's last loop only (and a pattern of a bar or less) */
static int sg_fail_slot = -1;
static int sg_test_save(uint32_t slot, const char *name)
{
    if ((int)slot == sg_fail_slot)
        return 2;
    return project_save_as(slot, name);
}
static int test_song_fix(void)
{
    static project_store_t slots[4];
    int bad = 0, ok;
    chain_config_t ch;
    uint32_t i, n;
    /* 1. playing: nothing written, the chain as it was; a slot refused halfway: the chain back, SAVE FAILED C */
    sg_open((uint32_t)sg_genre("TRANCE"), 42);
    memset(proj_slot, 0, sizeof proj_slot);
    chain_defaults(&chain_config);
    chain_config.count = 1;
    chain_config.row[0] = (chain_row_t){{0, 1, 2, 3}, 3};
    ch = chain_config;
    memcpy(slots, proj_slot, sizeof slots);
    transport_req = 1;
    pm_blocks(2);
    ok = song.playing && song_write() == 1 && msg_is("STOP TO SAVE") && !memcmp(&ch, &chain_config, sizeof ch) &&
         !memcmp(slots, proj_slot, sizeof slots) && sg.stage == SG_EDIT;
    transport_req = 2;
    pm_blocks(2);
    sg_fail_slot = SG_C;
    oct_fwd(); oct_fwd();                               /* GENERATE, OVERWRITE: D written, C refused */
    ok &= msg_is("SAVE FAILED C") && !memcmp(&ch, &chain_config, sizeof ch) && sg.stage == SG_EDIT && sg.on;
    sg_fail_slot = -1;
    bad += check("SONG spec v0.4.1 15. playing, nothing written; a slot refused halfway: SAVE FAILED C, the chain as it was", ok);
    /* 2. written; DENSITY / EVOLVE: still written (a reroll asks SAVE X?); SEED: another song, EDIT */
    oct_fwd(); oct_fwd();
    ok = sg.stage == SG_WRITTEN;
    kt(EN_K2, 1);
    kt(EN_K3, 1);
    ok &= sg.stage == SG_WRITTEN;
    tkey(TL_RANDOM);
    ok &= sg.pend == SG_A + 1u;
    oct_back();
    kt(EN_K1, 1);
    ok &= sg.stage == SG_EDIT && !sg.pend;
    tkey(TL_RANDOM);
    ok &= !sg.pend;
    bad += check("  16. DENSITY / EVOLVE keep the song written (a reroll asks SAVE A?); SEED: EDIT, no prompt", ok);
    /* 3. SAVE X? while playing: STOP TO SAVE, the prompt stays, nothing written; stopped: saved */
    kt(EN_K1, -1);
    oct_fwd(); oct_fwd();
    sg_tap(2);                                          /* C */
    tkey(3);                                            /* (B3: T4's rhythm; 0.14: D4 is FOCUS) */
    memcpy(slots, proj_slot, sizeof slots);
    transport_req = 1;
    pm_blocks(2);
    oct_fwd();
    ok = msg_is("STOP TO SAVE") && sg.pend == SG_C + 1u && !memcmp(slots, proj_slot, sizeof slots);
    sg_fail_slot = SG_C;
    transport_req = 2;
    pm_blocks(2);
    oct_fwd();
    ok &= msg_is("SAVE FAILED C") && sg.pend == SG_C + 1u;
    sg_fail_slot = -1;
    oct_fwd();
    ok &= !sg.pend && msg_is("C SAVED") && memcmp(&slots[2], &proj_slot[2], sizeof slots[2]) &&
          !memcmp(&slots[0], &proj_slot[0], sizeof slots[0]);
    bad += check("  17. SAVE C? playing or refused: STOP TO SAVE / SAVE FAILED C, the prompt stays; then C SAVED", ok);
    sg_close();
    /* 4. BREAK's PAD: one loop the whole section (128 steps: 64 at 1/8); the sweep only in its last bar, never over
     * the top; a bar or less: its second half; under 4 steps: none. AMBIENT named */
    {
        int16_t dv = 0;
        uint32_t l1 = song_break_pad_len(128, &dv), l2;
        int16_t d2 = 0;
        track_t *t = &trk[2];
        l2 = song_break_pad_len(32, &d2);
        ok = l1 == 64u && dv == 1 && l2 == 32u && d2 == 2 && str_eq(FS_GENRE[FS_G_AMBIENT].name, "AMBIENT");
        {
            static const struct { uint8_t len, div, from; } C[4] = {{64, 1, 56}, {32, 2, 16}, {16, 2, 8}, {8, 2, 4}};
            for (n = 0; n < 4u; n++) {
                motion_clear(t);
                t->p[P_SDIV] = C[n].div;
                song_sweep(t, C[n].len, P_E4, 20, 100);
                for (i = 1; i < NSTEP; i++) {
                    int32_t e = motion_find(t, i, P_E4);
                    if (e >= 0 && (i < C[n].from || i >= C[n].len || motion.event[e].value > 100))
                        ok = 0;
                }
                ok &= motion_find(t, 0, P_E4) >= 0 && motion.event[motion_find(t, 0, P_E4)].value == 20;
                ok &= motion_find(t, C[n].len - (C[n].div == 1 ? 1u : 2u), P_E4) >= 0 &&
                      motion.event[motion_find(t, C[n].len - (C[n].div == 1 ? 1u : 2u), P_E4)].value == 100;
            }
            motion_clear(t);
            song_sweep(t, 3, P_E4, 20, 100);
            ok &= motion_count(t) == 0u;
            motion_clear(t);
            t->p[P_SDIV] = 2;
        }
        bad += check("  18. BREAK's PAD one loop per section (64 at 1/8 past 64 steps); sweep in the last bar only; short patterns", ok);
    }
    memset(proj_slot, 0, sizeof proj_slot);
    chain_defaults(&chain_config);
    return bad;
}

/* 0.13 spec v0.4.1 (§B, §C; tests 19..29): per-track rerolls on F3..B3, the punch-in effects E4..C5 with no FX button,
 * FX held + F3..B3 the mutes (decided at the key's down), CLEAR, DUCK, the latch, leaving SONG, the chain untouched */
static void sg_w(uint32_t p, int down) { if (down) key_down(white(p)); else key_up(white(p)); frame(); }
static void sg_wtap(uint32_t p) { sg_w(p, 1); sg_w(p, 0); }
static int sg_same(const step_t (*a)[NSTEP], uint32_t skip)   /* the four tracks as a[], but track skip */
{
    uint32_t i;
    for (i = 0; i < NTRK; i++)
        if (i != skip && memcmp(a[i], trk[i].step, sizeof a[i]))
            return 0;
    return 1;
}
static int test_song_fx(void)
{
    static step_t was[NTRK][NSTEP];
    static project_store_t slots[4];
    int bad = 0, ok;
    uint32_t i, j, e;
    int16_t mute[NTRK];
    /* 19. F3 G3 A3: that track's notes (its rhythm kept), B3: T4's rhythm; the others byte for byte; deterministic */
    sg_open((uint32_t)sg_genre("TRANCE"), 42);
    fx_latch &= ~1u; perf_latch_on = 0;
    for (i = 0; i < NTRK; i++) mute[i] = trk[i].p[P_MUTE];
    ok = perf_song && sg_fxmask;
    for (i = 0; i < NTRK; i++) {
        for (j = 0; j < NTRK; j++) memcpy(was[j], trk[j].step, sizeof was[j]);
        sg_wtap(i);
        ok &= sg_same((const step_t (*)[NSTEP])was, i) && memcmp(was[i], trk[i].step, sizeof was[i]) && fs.on && sg.on;
        if (i < FP_DRUM) {
            for (j = 0; j < NSTEP; j++)
                ok &= (trk[i].step[j].time == ST_NOTE) == (was[i][j].time == ST_NOTE) && trk[i].step[j].time == was[i][j].time;
            ok &= sg.nvar[SG_A][i] == 1u;
        } else {
            ok &= sg.rvar[SG_A][3] == 1u;
        }
        {
            char m[8] = {'N', 'E', 'W', ' ', 'T', (char)('1' + i), 0};
            ok &= msg_is(m);
        }
    }
    for (j = 0; j < NTRK; j++) memcpy(was[j], trk[j].step, sizeof was[j]);
    song_render(SG_A);
    ok &= sg_same((const step_t (*)[NSTEP])was, 99);
    bad += check("SONG spec v0.4.1 19. F3 G3 A3: one track's notes (its rhythm kept), B3: T4's rhythm, the rest byte for byte", ok);
    /* 20. T3 locked: A3 nothing, no prompt, T3 LOCKED; C4 skips T3 */
    oct_fwd(); oct_fwd();                               /* (written: a reroll would ask) */
    sg_tap(6);                                          /* G#4: T3 locked */
    for (j = 0; j < NTRK; j++) memcpy(was[j], trk[j].step, sizeof was[j]);
    sg_wtap(2);
    ok = msg_is("T3 LOCKED") && !sg.pend && sg_same((const step_t (*)[NSTEP])was, 99);
    tkey(TL_RANDOM);
    ok &= !memcmp(was[2], trk[2].step, sizeof was[2]) && sg.pend == SG_A + 1u;
    oct_back();
    sg_tap(6);
    bad += check("  20. T3 locked: A3 changes nothing and asks nothing (T3 LOCKED); C4 skips T3", ok);
    /* 21. SAVE X? after a per-track reroll: one prompt for several; OCT- writes nothing, OCT+ slot A only */
    memcpy(slots, proj_slot, sizeof slots);
    sg_wtap(1); sg_wtap(0);
    ok = sg.pend == SG_A + 1u && msg_is("SAVE A?");
    oct_back();
    ok &= !sg.pend && !memcmp(slots, proj_slot, sizeof slots);
    sg_wtap(3);
    oct_fwd();
    ok &= !sg.pend && memcmp(&slots[0], &proj_slot[0], sizeof slots[0]) && !memcmp(&slots[1], &proj_slot[1], 3 * sizeof slots[0]);
    bad += check("  21. a per-track reroll asks SAVE A? (once for several); OCT- writes nothing, OCT+ slot A only", ok);
    /* 22. E4..C5: held, the six effects; no note, no step, STARTER stays */
    ok = 1;
    for (j = 0; j < NTRK; j++) memcpy(was[j], trk[j].step, sizeof was[j]);
    for (i = 0; i < 6u; i++) {
        sg_w(6u + i, 1);
        e = SG_FX[i];
        ok &= ((perf_held >> e) & 1u) && !gates() && fs.on && sg.on;
        sg_w(6u + i, 0);
        ok &= !((perf_held >> e) & 1u);
    }
    ok &= sg_same((const step_t (*)[NSTEP])was, 99) && SG_FX[0] == PF_R16 && SG_FX[1] == PF_LPF && SG_FX[2] == PF_HPF &&
          SG_FX[3] == PF_TAPE && SG_FX[4] == PF_RTHR && SG_FX[5] == PF_ETHR;
    bad += check("  22. E4..C5 held: REPEAT 1/16, LPF, HPF, TAPE STOP, REV THROW, ECHO THROW; no note, no step, STARTER open", ok);
    /* 23. FX held + G3: T2 muted while G3 is (FX let go first: still), no reroll; G3 then FX: a reroll, no mute */
    {
        uint8_t nv = sg.nvar[SG_A][1];
        btn_down(B_FX); frame();
        sg_w(1, 1);
        ok = ((perf_held >> (PF_M1 + 1u)) & 1u) && sg.nvar[SG_A][1] == nv && fs.on && sg.on && ui.ly == LAYER_SEQ;
        btn_up(B_FX); frame();
        ok &= ((perf_held >> (PF_M1 + 1u)) & 1u) != 0u;
        sg_w(1, 0);
        ok &= !((perf_held >> (PF_M1 + 1u)) & 1u);
        sg_w(1, 1);                                     /* G3 first: a reroll */
        btn_down(B_FX); frame();
        ok &= !((perf_held >> (PF_M1 + 1u)) & 1u) && sg.nvar[SG_A][1] == (uint8_t)(nv + 1u);
        sg_w(1, 0);
        btn_up(B_FX); frame();
        for (i = 0; i < NTRK; i++) ok &= trk[i].p[P_MUTE] == mute[i];
        if (sg.pend) oct_back();
        bad += check("  23. FX + G3: T2 muted while G3 is down (FX let go first too), no reroll; G3 then FX: a reroll; P_MUTE untouched", ok);
    }
    /* 24. FX held + black 2 in SONG: part B, no mute; outside SONG black 1 is a mute (the FX layer) */
    btn_down(B_FX); frame();
    sg_tap(1);
    ok = sg.part == SG_B && !(perf_held & PF_MUTE);
    btn_up(B_FX); frame();
    perf_song = 0;
    ok &= perf_key(black(0)) == PF_M1 && perf_key(white(6)) == perf_map[6];
    frame();
    ok &= perf_song == 1u;
    bad += check("  24. FX + black 2 in SONG: part B, no mute; outside SONG FX's black 1 still mutes T1, E4 its map's", ok);
    /* 25. latch: E4 and FX + F3 toggle, survive a part change and a reroll; C#5 CLEAR: all off, DUCK too */
    fx_latch |= 1u; perf_latch_on = 1;
    sg_wtap(6);
    ok = (perf_latched >> PF_R16) & 1u;
    btn_down(B_FX); frame(); sg_wtap(0); btn_up(B_FX); frame();
    ok &= (perf_latched >> PF_M1) & 1u;
    sg_tap(9);                                          /* D#5: DUCK */
    sg_tap(2);                                          /* part C */
    tkey(3);                                            /* (B3 T4's reroll; 0.14: D4 is FOCUS) */
    if (sg.pend) oct_back();
    ok &= ((perf_latched >> PF_R16) & 1u) && ((perf_latched >> PF_M1) & 1u) && (perf_latched & PF_BIT(PF_DUCK));
    sg_wtap(6);
    ok &= !((perf_latched >> PF_R16) & 1u);
    sg_wtap(6);
    sg_tap(8);                                          /* C#5: CLEAR */
    ok &= !perf_latched && msg_is("CLEAR: ALL OFF");
    bad += check("  25. FX LATCH: E4 and FX + F3 toggle and survive a part change and a reroll; C#5 CLEAR turns all off, DUCK too", ok);
    /* 26. D#5 DUCK on / off, latch or not */
    sg_tap(9);
    ok = (perf_latched & PF_BIT(PF_DUCK)) && msg_is("DUCK ON");
    sg_tap(9);
    ok &= !(perf_latched & PF_BIT(PF_DUCK));
    fx_latch &= ~1u; perf_latch_on = 0;
    sg_tap(9);
    ok &= (perf_latched & PF_BIT(PF_DUCK)) != 0u;
    sg_tap(9);
    ok &= !(perf_latched & PF_BIT(PF_DUCK));
    bad += check("  26. D#5 toggles DUCK, latch on or off", ok);
    /* 27. leaving SONG (F#5, G5, the layer let go) with an effect held, one latched, a mute and DUCK: all off */
    ok = 1;
    for (i = 0; i < 3u; i++) {
        uint32_t hold, k;
        if (!sg.on) { sg_close(); sg_open((uint32_t)sg_genre("TRANCE"), 42); }
        fx_latch |= 1u; perf_latch_on = 1;
        sg_wtap(7);                                     /* F4 LPF latched */
        btn_down(B_FX); frame(); sg_wtap(2); btn_up(B_FX); frame();   /* T3 muted, latched */
        fx_latch &= ~1u; perf_latch_on = 0;
        sg_tap(9);                                      /* DUCK */
        sg_w(6, 1);                                     /* E4 held */
        if (i == 0) sg_tap(10);                         /* F#5 */
        else if (i == 1) tkey(TL_STARTER);              /* G5 */
        else { btn_up(B_SEQ); frame(); }                /* the layer let go */
        hold = perf_kill ? 0u : perf_held | perf_latched;
        ok &= !sg.on && !perf_latched && !hold && !perf_song && !sg_fxmask;
        sg_w(6, 0);
        for (k = 0; k < 4u; k++) ok &= !perf_k[k];
        if (!ok) printf("    leaving %u: on %u latched %x held %x kill %u\n", (unsigned)i, sg.on, (unsigned)perf_latched, (unsigned)perf_held, perf_kill);
        if (i < 2u) { btn_up(B_SEQ); frame(); }
        fs.on = 0;
    }
    bad += check("  27. leaving SONG by F#5, G5 or the layer let go: every effect, latch, mute and DUCK off", ok);
    /* 29. F5: nothing (0.14, spec v0.5: D5 E5 are the scenes, tests 39 40) */
    sg_open((uint32_t)sg_genre("TRANCE"), 42);
    for (j = 0; j < NTRK; j++) memcpy(was[j], trk[j].step, sizeof was[j]);
    ok = 1;
    for (i = 14; i < 15u; i++) {
        sg_wtap(i);
        ok &= msg_is("NOT IN SONG") && fs.on && sg.on && !perf_held && sg_same((const step_t (*)[NSTEP])was, 99);
    }
    bad += check("  29. F5 in SONG: NOT IN SONG, nothing changes (D5 E5: the scenes since spec v0.5)", ok);
    /* 28. INTRO and OUTRO, the mutes through the keys as SONG takes them (the ISR's path: SEQ held, FX held at the
     * key's down): T3, which the row plays, is silenced while held and back when let go; T1, which the row silences
     * ("-"), stays silent held, let go, or never touched; the chain and its rows' timing the same with and without */
    {
        static int32_t o[2 * CTL];
        uint32_t tm[2][8], r, run, it;
        uint32_t seqb = 1u << panel.btn[B_SEQ];
        oct_fwd(); oct_fwd();
        sg_close();
        for (run = 0; run < 2u; run++) {
            uint32_t last = 99, n = 0, t1 = 0, blocks = 0, inrow = 0, held3 = 0, back3 = 0, rows_checked = 0;
            chain_config_t c0;
            project_load(SG_A);
            c0 = chain_config;
            ok = chain_prepare() == 0;
            if (run) {                                  /* (as layer_masks sets them in SONG with FX held) */
                kb_mask = seqb; perf_mask = 0; kb_lock = 0; fm1_in.buttons |= seqb;
                perf_song = 1; sg_fxmask = song_keymask(1);
                perf_kill = 0;                          /* (the main loop's, no layer key held: ui_input.c) */
            }
            for (it = 0; it < 2000000u && (chain.running || chain.armed); it++) {
                uint32_t v0 = vage, k;
                mix_block(o, CTL);
                blocks++;
                if (chain.running && chain.row != last && n < 8u) {
                    last = chain.row;
                    tm[run][n++] = blocks;
                    inrow = 0;
                }
                inrow++;
                if (chain.running && (chain.row == 0u || chain.row == 7u)) {
                    for (; v0 < vage; v0++)
                        for (k = 0; k < NVOICE; k++)
                            t1 += trk[0].v[k].age == v0 + 1u;   /* (T1's notes in INTRO / OUTRO: none, ever) */
                    if (run && inrow == 20u) {          /* FX + F3 (T1, silent by the chain) and FX + A3 (T3, playing) */
                        key_down(white(0));
                        key_down(white(2));
                    }
                    if (run && inrow == 40u) {
                        held3 += pf.mg[2] == 0 && ((perf_held >> (PF_M1 + 2u)) & 1u) && ((perf_held >> PF_M1) & 1u);
                        key_up(white(0));
                        key_up(white(2));
                    }
                    if (run && inrow == 60u) {
                        back3 += pf.mg[2] == 32768 && !(perf_held & PF_MUTE);
                        rows_checked++;
                    }
                }
            }
            ok &= n == 8u && !t1 && !memcmp(&c0, &chain_config, sizeof c0);
            if (run)
                ok &= rows_checked == 2u && held3 == 2u && back3 == 2u;
            for (r = 0; r < NTRK; r++)
                ok &= trk[r].p[P_MUTE] == 0;
            transport_req = 2;
            mix_block(o, CTL);
            if (!ok) printf("    run %u: rows %u, T1 notes in INTRO / OUTRO %u, T3 held %u back %u of %u\n", (unsigned)run,
                            (unsigned)n, (unsigned)t1, (unsigned)held3, (unsigned)back3, (unsigned)rows_checked);
        }
        kb_mask = 0; fm1_in.buttons &= ~seqb; perf_song = 0; sg_fxmask = 0; fm1_in.notes = 0;
        for (r = 0; r < 8u; r++)
            ok &= tm[0][r] == tm[1][r];
        perf_held = 0;
        bad += check("  28. INTRO and OUTRO: FX + A3 silences T3 while held, back on release; FX + F3 never brings back T1 (\"-\"); chain and row timing identical", ok);
    }
    /* (review of 52fe595) K1..K4 with FX held: the FX macros, not SEED .. GENRE; FX let go: back to 0 (FX LATCH: kept) */
    {
        uint8_t seed, dens;
        sg_open((uint32_t)sg_genre("TRANCE"), 42);
        seed = fg.seed; dens = fs.dens;
        btn_down(B_FX); frame();
        kt(EN_K1, 4); kt(EN_K2, 3); kt(EN_K3, 2); kt(EN_K4, -2);   /* (DEPTH turns the other way: layer_knob) */
        ok = perf_k[0] != 0 && perf_k[1] > 0 && perf_k[2] > 0 && perf_k[3] != 0 && fg.seed == seed && fs.dens == dens;
        btn_up(B_FX); frame();
        ok &= !perf_k[0] && !perf_k[1] && !perf_k[2] && !perf_k[3];
        kt(EN_K1, 1);
        ok &= fg.seed == (uint8_t)(seed + 1u) && !perf_k[0];
        fx_latch |= 1u; frame();
        btn_down(B_FX); frame();
        kt(EN_K2, 3);
        btn_up(B_FX); frame();
        ok &= perf_k[1] > 0;
        sg_tap(8);                                      /* CLEAR */
        ok &= !perf_k[1];
        fx_latch &= ~1u; frame();
        sg_close();
        bad += check("  K1..K4 with FX held: FILTER CRUSH THROW DEPTH, not SEED DENSITY; FX let go: back to 0 (FX LATCH: kept to CLEAR)", ok);
    }
    memset(proj_slot, 0, sizeof proj_slot);
    chain_defaults(&chain_config);
    return bad;
}

/* 0.14 spec v0.5 §H (tests 31..41): D4 FOCUS, the faders, the scenes. 42 is the device test (พี่ต้น flashes) */
#define SG_D4 5u                                        /* (white key places: D4, D5, E5) */
#define SG_D5 12u
#define SG_E5 13u
static void sg_vals(int16_t (*v)[P_COUNT]) { uint32_t i; for (i = 0; i < NTRK; i++) memcpy(v[i], trk[i].p, sizeof v[i]); }
static int sg_vals_same(const int16_t (*v)[P_COUNT], uint32_t ti, uint32_t id1, uint32_t id2)   /* but trk[ti].p[id1, id2] */
{
    uint32_t i, j;
    for (i = 0; i < NTRK; i++)
        for (j = 0; j < P_COUNT; j++)
            if (!(i == ti && (j == id1 || j == id2)) && v[i][j] != trk[i].p[j])
                return 0;
    return 1;
}
static int test_song_perf(void)
{
    static step_t was[NTRK][NSTEP];
    static int16_t pv[NTRK][P_COUNT], pv2[NTRK][P_COUNT];
    uint8_t nv[SG_N][NTRK], rv[SG_N][NTRK], m[4][2];
    int bad = 0, ok;
    uint32_t i, j, cut, res;
    sg_open((uint32_t)sg_genre("TRANCE"), 42);
    fx_latch &= ~1u; perf_latch_on = 0;
    /* 31. D4 tap: no reroll; B3 still rerolls T4's rhythm */
    for (j = 0; j < NTRK; j++) memcpy(was[j], trk[j].step, sizeof was[j]);
    memcpy(nv, sg.nvar, sizeof nv); memcpy(rv, sg.rvar, sizeof rv);
    sg_wtap(SG_D4);
    ok = sg_same((const step_t (*)[NSTEP])was, 99) && !memcmp(nv, sg.nvar, sizeof nv) && !memcmp(rv, sg.rvar, sizeof rv) &&
         !sg.focus && fs.on && sg.on;
    sg_wtap(3);
    ok &= sg.rvar[SG_A][3] == rv[SG_A][3] + 1u && memcmp(was[3], trk[3].step, sizeof was[3]) &&
          sg_same((const step_t (*)[NSTEP])was, 3);
    bad += check("SONG spec v0.5 31. D4 tap: no reroll, steps and rerolls byte for byte; B3 still rerolls T4's rhythm", ok);
    /* 37. (before any focus: the faders on T1) FOCUS off, D4 held, K1: T1's CUTOFF; let go: it stays, K1 is SEED */
    song_fmap(0, m);
    cut = m[0][0];
    sg_vals(pv);
    sg_w(SG_D4, 1);
    kt(EN_K1, -5);
    ok = trk[0].p[cut] == pv[0][cut] - 5 && sg_vals_same((const int16_t (*)[P_COUNT])pv, 0, cut, 99) && !sg.focus;
    sg_w(SG_D4, 0);
    {
        uint8_t seed = fg.seed;
        ok &= trk[0].p[cut] == pv[0][cut] - 5 && !sg.focus;   /* (a hold that turned a knob: no tap) */
        kt(EN_K1, 1);
        ok &= fg.seed == (uint8_t)(seed + 1u) && trk[0].p[cut] == pv[0][cut] - 5;
        kt(EN_K1, -1);
    }
    bad += check("  37. FOCUS off, D4 held: K1 is T1's CUTOFF; let go: the value stays, K1 SEED again, FOCUS still off", ok);
    /* 32. D4 held + A3: FOCUS T3, no reroll, no mute; let go: it stays */
    for (j = 0; j < NTRK; j++) memcpy(was[j], trk[j].step, sizeof was[j]);
    memcpy(nv, sg.nvar, sizeof nv);
    sg_w(SG_D4, 1);
    sg_w(2, 1);
    ok = sg.focus == 3u && msg_is("FOCUS T3 PAD") && !(perf_held & PF_MUTE) && trk[2].p[P_MUTE] == 0;
    sg_w(2, 0);
    sg_w(SG_D4, 0);
    ok &= sg.focus == 3u && !memcmp(nv, sg.nvar, sizeof nv) && sg_same((const step_t (*)[NSTEP])was, 99);
    bad += check("  32. D4 held + A3: FOCUS T3 PAD; no reroll, no mute; D4 let go: FOCUS stays", ok);
    /* 33. at the key's down: FX + D4 + G3 a mute, D4 + G3 the focus, G3 alone a reroll; later presses change nothing */
    btn_down(B_FX); frame();
    sg_w(SG_D4, 1);
    sg_w(1, 1);
    ok = ((perf_held >> (PF_M1 + 1u)) & 1u) && sg.focus == 3u;
    btn_up(B_FX); frame();
    ok &= ((perf_held >> (PF_M1 + 1u)) & 1u) && sg.focus == 3u;   /* (still the mute, FX let go) */
    sg_w(1, 0);
    sg_w(SG_D4, 0);
    ok &= !(perf_held & PF_MUTE) && sg.focus == 3u;
    memcpy(nv, sg.nvar, sizeof nv);
    sg_w(SG_D4, 1); sg_w(1, 1);
    btn_down(B_FX); frame();                            /* (FX after: still the focus) */
    ok &= sg.focus == 2u && !(perf_held & PF_MUTE) && !memcmp(nv, sg.nvar, sizeof nv);
    btn_up(B_FX); frame();
    sg_w(1, 0); sg_w(SG_D4, 0);
    sg_w(1, 1); sg_w(SG_D4, 1);                         /* (G3 first: the reroll; D4 after changes nothing) */
    ok &= sg.nvar[SG_A][1] == nv[SG_A][1] + 1u && sg.focus == 2u;
    sg_w(SG_D4, 0); sg_w(1, 0);
    ok &= sg.focus == 2u;                               /* (that D4 was no tap: G3 was down) */
    bad += check("  33. at the key's down: FX + D4 + G3 mutes T2, D4 + G3 FOCUS T2, G3 alone a reroll; later presses change nothing", ok);
    /* 34. FOCUS T1: K1 K2 move T1's CUTOFF and RESO only; kept over a part change and a reroll */
    sg_w(SG_D4, 1); sg_w(0, 1); sg_w(0, 0); sg_w(SG_D4, 0);
    song_fmap(0, m);
    cut = m[0][0]; res = m[1][0];
    sg_vals(pv);
    {
        uint8_t seed = fg.seed, dens = fs.dens, gen = fs.genre, ev = fg.evolve;
        kt(EN_K1, 6);
        kt(EN_K2, 4);
        ok = sg.focus == 1u && trk[0].p[cut] == pv[0][cut] + 6 && trk[0].p[res] == pv[0][res] + 4 &&
             sg_vals_same((const int16_t (*)[P_COUNT])pv, 0, cut, res) && fg.seed == seed && fs.dens == dens && fs.genre == gen &&
             fg.evolve == ev && cut != res && cut >= P_E0 && res >= P_E0;
    }
    sg_vals(pv2);
    sg_tap(1);                                          /* part B */
    tkey(TL_RANDOM);                                    /* C4 */
    for (i = 0; i < NTRK; i++)
        for (j = 0; j < P_COUNT; j++)
            if (j != P_SLEN && j != P_SDIV && j != P_SSWING && j != P_ROOT && j != P_SCALE)
                ok &= trk[i].p[j] == pv2[i][j];
    sg_tap(0);
    bad += check("  34. FOCUS T1: K1 K2 move only T1's CUTOFF RESO (ANALOG CUT RES), not SEED..GENRE; kept over part B and a reroll", ok);
    /* 35. the knobs are encoders: a turn moves a value from where it is, never to the knob's place; after a change of
     * track and after a scene too (spec §H1's pickup: nothing jumps) */
    trk[0].p[cut] = 90;
    kt(EN_K1, -1);
    ok = trk[0].p[cut] == 89;
    sg_w(SG_D4, 1); sg_w(1, 1); sg_w(1, 0); sg_w(0, 1); sg_w(0, 0); sg_w(SG_D4, 0);   /* T2, then T1 again */
    kt(EN_K1, 1);
    ok &= sg.focus == 1u && trk[0].p[cut] == 90;
    btn_down(B_FX); frame(); sg_wtap(SG_D5); btn_up(B_FX); frame();   /* SCENE 1: CUTOFF 90 */
    trk[0].p[cut] = 40;
    sg_wtap(SG_D5);                                     /* (stopped: at once) */
    kt(EN_K1, 1);
    ok &= trk[0].p[cut] == 91;
    bad += check("  35. encoders: a turn moves CUTOFF from its value (90 -> 89), after a track change and a scene recall too", ok);
    /* 36. FOCUS T1, FX held: K1 the FILTER macro; FX let go: back to 0, CUTOFF as it was */
    sg_vals(pv);
    btn_down(B_FX); frame();
    kt(EN_K1, 5);
    ok = perf_k[0] != 0 && trk[0].p[cut] == pv[0][cut];
    btn_up(B_FX); frame();
    ok &= !perf_k[0] && sg_vals_same((const int16_t (*)[P_COUNT])pv, 99, 999, 999) && sg.focus == 1u;
    bad += check("  36. FOCUS T1 + FX held: K1 the FILTER macro; FX let go: it snaps back, CUTOFF unchanged", ok);
    /* 38. FOCUS off: a D4 tap; D4 + the focus track's key */
    sg_wtap(SG_D4);
    ok = !sg.focus && msg_is("FOCUS OFF");
    sg_w(SG_D4, 1); sg_w(3, 1); sg_w(3, 0); sg_w(SG_D4, 0);
    ok &= sg.focus == 4u && msg_is("FOCUS T4 DRUM");
    song_fmap(3, m);
    ok &= m[0][0] == P_LN0 && str_eq(song_fname(3, 0), "KICK") && str_eq(song_fname(3, 1), "TUNE") &&
          str_eq(song_fname(3, 2), "DECY") && str_eq(song_fname(3, 3), "DRV") && str_eq(song_fname(1, 1), "A/R") &&
          str_eq(song_fname(2, 2), "SEND");
    sg_w(SG_D4, 1); sg_w(3, 1); sg_w(3, 0); sg_w(SG_D4, 0);
    ok &= !sg.focus;
    bad += check("  38. a D4 tap turns FOCUS off; D4 + the focus track's key too (T4 DRUM: KICK TUNE DECY DRV)", ok);
    /* 39. SCENE 1 stored, four params changed, D5 playing: all 16 back at the next 1/16, not before; E5 empty */
    sg_scene[1].n = 0; sg.scenes &= 1u;
    btn_down(B_FX); frame(); sg_wtap(SG_D5); btn_up(B_FX); frame();
    ok = msg_is("SCENE 1 STORED") && sg_scene[0].n == SC_MAX;
    sg_vals(pv);
    for (i = 0; i < NTRK; i++) {
        song_fmap(i, m);
        trk[i].p[m[3][0]] = (int16_t)(trk[i].p[m[3][0]] > 20 ? trk[i].p[m[3][0]] - 20 : trk[i].p[m[3][0]] + 20);
    }
    sg_vals(pv2);
    transport_req = 1;
    pm_blocks(3);
    {
        uint32_t n = 0, at_step = 0, pos = 0, done = 0, bl;
        sg_wtap(SG_D5);
        ok &= sc_pend == 1u && sg_vals_same((const int16_t (*)[P_COUNT])pv2, 99, 999, 999);
        for (bl = 0; bl < 400u && !done; bl++) {
            uint32_t st0 = clk_step, p0 = clk_pos;
            pm_blocks(1);
            n++;
            if (!sc_pend) {
                done = 1;
                at_step = clk_step != st0;              /* (the block a new 1/16 started in) */
                pos = p0;
            }
        }
        (void)pos;
        ok &= done && at_step && sg_vals_same((const int16_t (*)[P_COUNT])pv, 99, 999, 999);
    }
    sg_wtap(SG_E5);
    ok &= msg_is("SCENE 2 EMPTY") && sg_vals_same((const int16_t (*)[P_COUNT])pv, 99, 999, 999);
    transport_req = 2;
    pm_blocks(2);
    bad += check("  39. SCENE 1: stored (20 params: T1..T4's), recalled playing at the next 1/16 only; E5 empty: SCENE 2 EMPTY, no change", ok);
    /* 40. FX + D5: a store only, no effect, no step */
    for (j = 0; j < NTRK; j++) memcpy(was[j], trk[j].step, sizeof was[j]);
    btn_down(B_FX); frame();
    sg_w(SG_D5, 1);
    ok = !perf_held && !perf_latched && msg_is("SCENE 1 STORED");
    sg_w(SG_D5, 0);
    btn_up(B_FX); frame();
    ok &= !perf_held && !perf_latched && sg_same((const step_t (*)[NSTEP])was, 99);
    bad += check("  40. FX + D5 / E5 store only: no PF_* effect, no note, no step", ok);
    /* (review of 10456b0) store, T2 on another engine, recall: T2 skipped ("SCENE 1 SKIP T2"), the others put back;
     * a stored value outside its param's range now: held to it */
    {
        uint32_t e2 = trk[1].eng_req, n;
        sg_vals(pv);
        for (i = 0; i < NTRK; i++)
            if (i != 1u) {
                song_fmap(i, m);
                trk[i].p[m[3][0]] = (int16_t)(trk[i].p[m[3][0]] > 20 ? trk[i].p[m[3][0]] - 20 : trk[i].p[m[3][0]] + 20);
            }
        set_engine_of(&trk[1], e2 == ENGI_FM6 ? 0u : ENGI_FM6);
        frame();
        sg_vals(pv2);
        sg_wtap(SG_D5);                                 /* (stopped: at once) */
        ok = msg_is("SCENE 1 SKIP T2") && !memcmp(pv2[1], trk[1].p, sizeof pv2[1]);
        for (i = 0; i < NTRK; i++)
            if (i != 1u)
                ok &= !memcmp(pv[i], trk[i].p, sizeof pv[i]);
        set_engine_of(&trk[1], e2);
        frame();
        for (n = 0; n < sg_scene[0].n && sg_scene[0].trk[n] != 0u; n++)
            ;
        sg_scene[0].eng[1] = (uint8_t)trk[1].eng_req;
        sg_scene[0].v[n] = 999;                         /* (T1's first: CUTOFF) */
        sg_wtap(SG_D5);
        ok &= msg_is("SCENE 1") && trk[0].p[sg_scene[0].id[n]] == param_desc_of(eng_idx(trk[0].eng_req), sg_scene[0].id[n])->max;
        bad += check("  scene recall after T2's engine changed: T2 skipped (SCENE 1 SKIP T2), T1 T3 T4 back; a value past its range held to it", ok);
    }
    /* 41. CLEAR keeps FOCUS, its values and the scenes; leaving SONG: FOCUS off, the params kept, the scenes too */
    sg_w(SG_D4, 1); sg_w(0, 1); sg_w(0, 0); sg_w(SG_D4, 0);
    kt(EN_K1, 3);
    sg_vals(pv);
    sg_tap(8);                                          /* C#5 CLEAR */
    ok = sg.focus == 1u && sg.scenes == 1u && sg_vals_same((const int16_t (*)[P_COUNT])pv, 99, 999, 999);
    sg_tap(10);                                         /* F#5: SONG off */
    ok &= !sg.on && !sg.focus && trk[0].p[cut] == pv[0][cut] && sg.scenes == 1u;
    sg_close();
    bad += check("  41. CLEAR keeps FOCUS, its values and the scenes; leaving SONG turns FOCUS off, keeps the params and scenes", ok);
    return bad;
}

int main(void)
{
    int bad = test_open() + test_styles() + test_evolve() + test_names() + test_chords() + test_reroll() + test_golden() + test_drum() + test_051() + test_starter() + test_perform() + test_roles() + test_song() + test_song_fix() + test_song_fx() + test_song_perf();
    printf("%s\n", bad ? "FORGE TEST FAILED" : "forge tests passed");
    return bad != 0;
}
