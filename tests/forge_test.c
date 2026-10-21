/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the Felucca-FORGE contributors */
/* Felucca-FORGE (firmware/src/ui_forge.c): SEQ TOOLS F5 FORGE, on the real sequencer and UI sources (with the stubs of
 * tests/ui_test.c):
 *   open       F5 on a melodic track writes steps 1..LEN at once ("FORGE <seed> <style>"), F5 lit; past LEN untouched
 *   the steps  every note in ROOT / SCALE, step 1 a note, a TIE always after a note or a TIE, at most 4 notes a step, VEL 40..124,
 *              notes 24..108, conditions / chances only with EVOLVE, none at EVOLVE 0 (a pure loop: copies the same)
 *   seeded     the same SEED STYLE DENSITY EVOLVE LEN ROOT SCALE: the same steps (again after other values); another
 *              SEED: other steps; DENSITY alone: the melody of the notes kept stays (the pitch stream is its own)
 *   knobs      KNOB 1 SEED (0..99, round), 2 DENSITY (10..100 by 5), 3 EVOLVE (0..4), 4 STYLE (8; its DENSITY)
 *   styles     all 8 at LEN 16, 32 and 64, in MIN, MAJ, PEN and CHR, every ROOT: the rules above hold
 *   close      OCT- cancels (the steps and a lock as they were), OCT+ / F5 again / the layer let go apply (an emptied
 *              step's lock goes); SAVE held after: one undo for all of it
 *   chords     (0.3) by interval: CHR minor 0 3 7 10, seven-note scales their own; TRANCE / SYNTH in CHR on the triad
 *   rerolls    (0.4) C4 new notes, D4 a new rhythm while open; K1 starts them again; VEL by place
 *   golden     (0.3) pinned seeds: tests/forge_golden.txt
 *   DRUM       (0.5) F5 opens: hits only, KICK on 1, DENSITY, C4 LOW / D4 HATS, the rules over the styles
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
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
    kt(EN_K4, 20);
    ok &= fg.style == FS_AMBIENT && fg.dens == FORGE_STYLE[FS_AMBIENT].dens;
    kt(EN_K4, -20);
    ok &= fg.style == FS_BERLIN && fg.dens == 85u;
    bad += check("  KNOB 1 SEED 0..99 round, 2 DENSITY 10..100, 3 EVOLVE 0..4, 4 STYLE BERLIN..AMBIENT", ok);
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

int main(void)
{
    int bad = test_open() + test_styles() + test_evolve() + test_names() + test_chords() + test_reroll() + test_golden() + test_drum();
    printf("%s\n", bad ? "FORGE TEST FAILED" : "forge tests passed");
    return bad != 0;
}
