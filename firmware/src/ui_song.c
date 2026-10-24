/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the FORGE ONE contributors */
/* FORGE ONE 0.13: SONG, a sub-mode of STARTER (SEQ TOOLS G5): one song seed writes four related parts into the four
 * project slots and a default arrangement into GLO > SONG (FORGE-ONE Song Mode spec v0.3). Included by ui_layer.c after
 * ui_starter.c, whose sounds, genre and knobs it shares.
 *
 *   A VERSE    the starter as it is, DENSITY x0.85, the lead sparser (x0.7)
 *   B CHORUS   DENSITY x1.15, the lead an octave up, EVOLVE +1; the beat with open hats on the off-beat 8ths and an
 *              accented open hat on step 1 (the kits have no crash)
 *   C BRIDGE   DENSITY x0.9, a new motif (its own pitch and rhythm streams), BASS and PAD moved up to the scale's 4th
 *              or 6th degree (by the seed), the beat half-time (kick on 1, snare on 9)
 *   D LIFT     (5 genres, starter.c FS_GENRE_D) CHORUS 2: the lead an octave up, the PAD an octave up, DENSITY x1.25,
 *              EVOLVE +2 (its own evolution stream), open 8th hats, the accented open hat on 1 and a snare / tom fill
 *              on the section's last bar (AMBIENT: x1.1, none of the hats, accent or fill)
 *   D BREAK    (8 genres) the breakdown: T1 empty (still in the chain: the clock is unaffected), no kick, snare or
 *              clap, the hats at x0.4 and a snare roll on the last bar rising in VEL, the PAD's filter (its engine's
 *              CUT / FREQ / TONE .., else its LEVEL) automated low from step 1 and opening over the last bar, the
 *              lead a sparse fragment (x0.5; a PLUCK lead: empty)
 * Shared by all four: the sounds (STARTER's, loaded once), ROOT, SCALE, tempo, SWING, the genre and the song seed. The
 * last bar is the section's (the chain's repeats x T1's LEN): D's DRUM (and BREAK's PAD) LEN is that, up to 64 steps,
 * so a fill sits in it unconditionally; longer, the fill gets the b:b condition of its loops (seq.c: a section starts
 * every track at loop 1). Every part is deterministic: the same seed, genre, DENSITY, EVOLVE and rerolls write the same
 * steps. A part's own rerolls (C4 notes; F3..B3 a track's notes, T4's rhythm) count per track, so a locked track (black
 * keys 5..8) keeps its steps exactly. (0.14, spec v0.5: D4's rhythm reroll of every track is gone; B3 rerolls T4's.)
 *
 * Keys (the STARTER layer): F#5 SONG on / off. Then black 1..4 a part A..D (it plays: its steps in the four tracks),
 * black 5..8 the T1..T4 locks, C4 rerolls the part shown's notes, KNOB 1..4 SEED DENSITY EVOLVE GENRE (as STARTER).
 * 0.14 (spec v0.5 §H, the performance layer): D4 is FOCUS (D4 + F3..B3 a track's sound on KNOB 1..4; D4 held alone its
 * faders), D5 / E5 two scenes; see song_focus_knob.
 *   OCT+  GENERATE: the confirm, "OVERWRITE A-D?" with the slots' names; OCT+ again writes the four slots (S42 VERSE,
 *         S42 CHORUS, S42 BRIDGE, S42 LIFT / S42 BREAK) and the chain (8 rows, by D's type); OCT- there cancels it
 *         all (the four tracks as STARTER found them, no slot written)
 *   after it, a reroll asks "SAVE B?" (that part only): OCT+ saves slot B, OCT- keeps it in the tracks, unsaved.
 *   Written, OCT+ or OCT- ends SONG and STARTER with the part shown in the tracks.
 * Saving needs the transport stopped (project.c: a flash erase stalls the audio): playing, GENERATE says STOP TO SAVE. */

enum { SG_A, SG_B, SG_C, SG_D, SG_N };
#ifndef SG_SAVE
#define SG_SAVE project_save_as                 /* (tests/forge_test.c: one that fails on a chosen slot) */
#endif
#define FS_G_AMBIENT 10u                        /* FS_GENRE's AMBIENT (its LIFT is the gentle one; tests check the name) */
#define SG_NAME_LEN 12u                         /* project.c PROJ_NAME_LEN (included after this; tests check they match) */
enum { SG_EDIT, SG_CONFIRM, SG_WRITTEN };
static const char *const SG_ROLE[SG_N + 1] = {"VERSE", "CHORUS", "BRIDGE", "LIFT", "BREAK"};
static struct {
    uint8_t on, stage, part, lock, pend;    /* lock: bit i T(i+1); pend: SAVE X? asked (X + 1), 0 none */
    uint8_t flash;                          /* 0.13 (spec v0.4.1 §C): the track row a per-track reroll flashes (+ 1) */
    uint32_t flash_ms;
    uint8_t focus, lastf;                   /* 0.14 (spec v0.5 §H1): the focus track + 1 (0: off); the last one (0..3) */
    uint8_t d4, d4_used;                    /* .. D4 held (since its down), and it picked a track or a knob turned */
    uint8_t scenes;                         /* .. §H3: bit x, SCENE x + 1 stored */
    /* 0.15 (spec v0.6 §H9 §H10): what waits for the next bar. kpend: KEY / SCALE (kroot, kscale); ppend: a preset browsed
     * on PRESETS (track + 1; pk its preset, pms the last detent); fpend: a favourite to load (slot + 1, on ftrk) */
    uint8_t kpend, kroot, kscale, ppend, pk, fpend, ftrk, armed;
    uint32_t pms;
    uint8_t ov, ov_skip;                    /* .. §J1: the overlay shown (SOV_*, 0 none) and a scene's skipped tracks */
    uint32_t ov_ms;
    uint8_t nvar[SG_N][NTRK], rvar[SG_N][NTRK];
} sg;

/* 0.15 (spec v0.6 §J1): the overlays, shown 1.2 s after the last change (ui_song_view.c draws them) */
enum { SOV_NONE, SOV_TEMPO, SOV_KEY, SOV_PRESET, SOV_SCENE, SOV_SCENE2, SOV_FAV };
#define SOV_MS 1200u
static void song_ov(uint32_t kind) { sg.ov = (uint8_t)kind; sg.ov_ms = fm1_ms; }
static void song_off(void);
static uint32_t song_dtype(void) { return FS_GENRE_D[fs.genre % FS_NGENRE]; }
static const char *song_role(uint32_t x) { return SG_ROLE[x == SG_D && song_dtype() == SD_BREAK ? SG_N : x]; }
static void song_name(uint32_t x, char *b)                /* "S42 CHORUS" (b: SG_NAME_LEN + 1) */
{
    char n[4];
    fmt_int(n, fg.seed);
    str_cpy(b, "S", SG_NAME_LEN + 1u);
    str_cpy(b + str_len(b), n, SG_NAME_LEN + 1u - str_len(b));
    str_cpy(b + str_len(b), " ", SG_NAME_LEN + 1u - str_len(b));
    str_cpy(b + str_len(b), song_role(x), SG_NAME_LEN + 1u - str_len(b));
}
/* the section's length in steps (the chain's repeats of T1, its clock: §3b) and D's LEN for its DRUM / BREAK's PAD */
static uint32_t song_d_steps(void)
{
    return (song_dtype() == SD_BREAK ? 2u : 4u) * (uint32_t)FORGE_STARTER[fs_idx()].len[FP_BASS];
}

/* note n moved up k notes of the track's scale */
static int32_t song_degree_up(const track_t *t, int32_t n, uint32_t k)
{
    while (k-- && n < 108)
        do
            n++;
        while (n < 108 && !tl_in_scale(t, n));
    return n;
}
static void song_transpose(track_t *t, uint32_t len, int32_t semis, uint32_t degrees)
{
    uint32_t i, k;
    for (i = 0; i < len; i++) {
        step_t *s = &t->step[i];
        for (k = 0; s->time == ST_NOTE && k < s->n && k < 4u; k++) {
            int32_t n = degrees ? song_degree_up(t, s->note[k], degrees) : s->note[k] + semis;
            s->note[k] = (uint8_t)clamp(n, 24, 108);
        }
    }
}
static void song_hits(step_t *s, uint32_t add, uint32_t del, uint32_t vel)
{
    uint32_t h = (s->time == ST_NOTE ? s->hit : 0u);
    h = (h | add) & ~del;
    if (h && s->time != ST_NOTE) {
        memset(s, 0, sizeof *s);
        s->time = ST_NOTE;
        s->vel = (uint8_t)vel;
    }
    s->hit = (uint8_t)h;
    s->acc &= (uint8_t)h;
    if (!h) {
        memset(s, 0, sizeof *s);
        s->time = ST_REST;
    }
}
/* the PAD's filter: its engine's first CUTOFF-format edit, else one named FREQ TONE DCW BRIT MLVL; none: its LEVEL */
static uint32_t song_filter_id(const track_t *t)
{
    static const char *const NAMES[] = {"CUT", "FREQ", "TONE", "DCW", "BRIT", "MLVL"};
    const engine_t *en = ENGINES[t->eng_req % NENGINES];
    uint32_t i, k;
    for (i = 0; i < 8u; i++)
        if (en->edit[i].fmt == F_CUTOFF)
            return P_E0 + i;
    for (k = 0; k < NELEM(NAMES); k++)
        for (i = 0; i < 8u; i++)
            if (en->edit[i].label && str_eq(en->edit[i].label, NAMES[k]))
                return P_E0 + i;
    return P_LEVEL;
}
static int32_t song_param_min(const track_t *t, uint32_t id)
{
    return id >= P_E0 ? ENGINES[t->eng_req % NENGINES]->edit[id - P_E0].min : TP[id].min;
}

/* BREAK's PAD: one loop that is the whole section, so its filter opens in the section's last bar only (a loop that
 * repeated would open it every time): at 1/16 up to 64 steps, past that (a T1 of 64: 128 steps) at 1/8 for half the
 * steps. *div: P_SDIV (2 1/16, 1 1/8); returns the LEN */
static uint32_t song_break_pad_len(uint32_t sec, int16_t *div)
{
    *div = sec > NSTEP ? 1 : 2;
    return sec > NSTEP ? sec / 2u : sec;
}
/* the sweep: the filter (id) down to lo from step 1, opening to top over the pattern's last bar (8 steps at 1/8, 16
 * at 1/16; a pattern of a bar or less: its second half; under 4 steps none), never over top */
static void song_sweep(track_t *t, uint32_t len, uint32_t id, int32_t lo, int32_t top)
{
    uint32_t bar = t->p[P_SDIV] == 1 ? 8u : 16u, last = len > bar ? len - bar : len / 2u, st = bar == 8u ? 1u : 2u, j;
    if (len < 4u)
        return;
    motion_set_enabled(t, 1);
    motion_set_event(t, 0, id, (int16_t)lo);
    for (j = last; j < len; j += st) {
        int32_t v = lo + (top - lo) * (int32_t)(j - last + st) / (int32_t)(len - last);
        motion_set_event(t, j ? j : 1u, id, (int16_t)(v < top ? v : top));
    }
}

/* part x of the song into the four tracks (their steps, LEN, DIV, SWING, ROOT, SCALE and automation; the sounds stay) */
static void song_render(uint32_t x)
{
    const forge_starter_t *s = &FORGE_STARTER[fs_idx()];
    uint8_t trk0 = fg.trk, style0 = fg.style, dens0 = fg.dens, ev0 = fg.evolve, n0 = fg.nvar, r0 = fg.rvar;
    uint32_t i, j, d = song_dtype(), amb = fs.genre % FS_NGENRE == FS_G_AMBIENT, sec = song_d_steps();
    static const uint8_t MULT[SG_N][FP_N] = {{85, 70, 85, 85}, {115, 115, 115, 115}, {90, 90, 90, 90}, {125, 125, 125, 125}};
    fg_bovr = fs_beat(s->beat);
    fg_bvd = s->dvel;
    for (i = 0; i < FP_N; i++) {
        track_t *t = &trk[i];
        forge_style_t row;
        uint32_t own = i < FP_DRUM ? s->part[i].dens : fg_bovr->dens, m = MULT[x][i], len = s->len[i], ev = fg.evolve;
        if (x == SG_D && d == SD_LIFT && amb)
            m = 110;
        if (x == SG_D && d == SD_BREAK)
            m = i == FP_DRUM ? 40u : i == FP_LEAD ? 50u : 100u;
        t->p[P_SDIV] = 2;
        if (x == SG_D && i == FP_DRUM)
            len = sec < NSTEP ? sec : NSTEP;            /* (the section's last bar is in the pattern, else b:b) */
        if (x == SG_D && d == SD_BREAK && i == FP_PAD)
            len = song_break_pad_len(sec, &t->p[P_SDIV]);   /* (one loop: the whole section) */
        ev += x == SG_B ? 1u : x == SG_D && d == SD_LIFT ? 2u : 0u;
        t->p[P_SLEN] = (int16_t)len;
        t->p[P_SSWING] = s->swing;
        t->p[P_ROOT] = fs.root;
        t->p[P_SCALE] = fs.scale ? fs.scale : s->scale;
        motion_clear(t);                                /* (each part its own automation: BREAK's PAD sweep) */
        fg.trk = (uint8_t)i;
        fg.style = (uint8_t)fs_idx();
        fg.dens = (uint8_t)clamp((int32_t)(own * fs.dens / 100u * m / 100u), 10, 100);
        fg.evolve = (uint8_t)(ev > 4u ? 4u : ev);
        fg.nvar = sg.nvar[x][i];
        fg.rvar = sg.rvar[x][i];
        fg_role = (uint8_t)(i + 1u);
        fg_xsalt[1] = fg_xsalt[2] = x == SG_C ? 0x5EED0C0Du : 0u;   /* (C: a new motif, M2) */
        fg_xsalt[3] = x == SG_D ? 0xD00D1E5Du : 0u;     /* (D: M1 developed its own way, not as B) */
        if (i < FP_DRUM) {
            row = s->part[i];
            fg_ovr = &row;
        } else {
            fg_ovr = 0;
        }
        forge_render();
        len = tl_len(t);
        for (j = len; j < NSTEP; j++) {                 /* (past LEN: empty, so a part never carries another's tail) */
            memset(&t->step[j], 0, sizeof t->step[j]);
            t->step[j].time = ST_REST;
        }
        /* the part's own shape over what the generator wrote */
        if (i == FP_LEAD && (x == SG_B || (x == SG_D && d == SD_LIFT)))
            song_transpose(t, len, 12, 0);              /* (the chorus, the lift: an octave up) */
        if (i == FP_PAD && x == SG_D && d == SD_LIFT)
            song_transpose(t, len, 12, 0);
        if ((i == FP_BASS || i == FP_PAD) && x == SG_C)
            song_transpose(t, len, 0, fg.seed & 1u ? 5u : 3u);   /* (the bridge's harmony: the 6th or the 4th) */
        if (i == FP_BASS && x == SG_D && d == SD_BREAK)
            for (j = 0; j < NSTEP; j++) {               /* (the bass out: every step a REST, LEN kept) */
                memset(&t->step[j], 0, sizeof t->step[j]);
                t->step[j].time = ST_REST;
            }
        if (i == FP_LEAD && x == SG_D && d == SD_BREAK && track_cat(t) == CAT_PLUCK)
            for (j = 0; j < NSTEP; j++) {
                memset(&t->step[j], 0, sizeof t->step[j]);
                t->step[j].time = ST_REST;
            }
        if (i == FP_DRUM) {
            uint32_t kick = 1u << DV_KICK, sn = 1u << DV_SNARE, cl = 1u << DV_CLAP, ho = 1u << DV_HATO, hc = 1u << DV_HATC;
            uint32_t tom = 1u << DV_TOM, last = len > 16u ? len - 16u : 0u;
            for (j = 0; j < len; j++) {
                step_t *st = &t->step[j];
                if (x == SG_B)
                    song_hits(st, j % 4u == 2u || !j ? ho : 0u, 0u, 104u);
                else if (x == SG_C)
                    song_hits(st, j % 16u == 8u ? sn : j % 16u == 0u ? kick : 0u,
                              (j % 16u ? kick : 0u) | (j % 16u != 8u ? sn | cl : 0u), 100u);
                else if (x == SG_D && d == SD_LIFT && !amb)
                    song_hits(st, (j % 4u == 2u ? ho : 0u) | (j % 2u == 0u ? hc : 0u) | (!j ? ho : 0u), 0u, 100u);
                else if (x == SG_D && d == SD_BREAK)
                    song_hits(st, 0u, kick | sn | cl | tom, 0u);
                if (st->time == ST_NOTE && !j && x != SG_A && x != SG_C && !(x == SG_D && (amb || d == SD_BREAK)))
                    st->acc = st->hit;                  /* (step 1 accented: the chorus's "crash") */
            }
            if (x == SG_D && !(d == SD_LIFT && amb)) {  /* the last bar: LIFT a fill, BREAK a roll building */
                uint32_t loops = sec > len ? sec / len : 1u, c = loops >= 4u ? 9u : loops == 3u ? 5u : loops == 2u ? 2u : 0u;
                for (j = last; j < len; j++) {
                    step_t *st = &t->step[j];
                    uint32_t q = j - last;              /* 0..15 */
                    if (d == SD_BREAK) {
                        if (q % (q < 8u ? 2u : 1u))
                            continue;                   /* (8ths, then 16ths) */
                        song_hits(st, sn, kick | cl | tom, 0u);
                        st->vel = (uint8_t)(48u + q * 5u);
                    } else {
                        if (q < 12u)
                            continue;                   /* (the last beat: snare, tom, snare, tom) */
                        song_hits(st, q & 1u ? tom : sn, hc | ho, 0u);
                        st->vel = (uint8_t)(96u + (q - 12u) * 8u);
                        step_set_ratchet(st, 2u);
                    }
                    if (c)
                        step_set_chance_v(st, 100u + c);   /* (the section's last loop only: b:b) */
                }
            }
        }
        if (i == FP_PAD && x == SG_D && d == SD_BREAK) {   /* the filter down, opening over the section's last bar */
            uint32_t id = song_filter_id(t);
            int32_t top = t->p[id], lo = song_param_min(t, id) + (top - song_param_min(t, id)) * 3 / 10;
            song_sweep(t, len, id, lo, top);
        }
    }
    fg_xsalt[1] = fg_xsalt[2] = fg_xsalt[3] = 0;
    fg_role = 0;
    fg_ovr = 0;
    fg_bovr = 0;
    fg.trk = trk0;
    fg.style = style0;
    fg.dens = dens0;
    fg.evolve = ev0;
    fg.nvar = n0;
    fg.rvar = r0;
}

/* the default arrangement (§3b), by D's type: INTRO, VERSE, CHORUS, VERSE, BRIDGE, then CHORUS + LIFT or BREAK +
 * CHORUS, OUTRO. T1 BASS T2 LEAD T3 PAD T4 DRUM; "-" silent */
static void song_chain(chain_config_t *c)
{
    static const uint8_t ROWS[2][8][5] = {
        {{4, 4, 0, 0, 2}, {0, 0, 0, 0, 4}, {1, 1, 1, 1, 4}, {0, 0, 0, 0, 4}, {2, 2, 2, 2, 2}, {1, 1, 1, 1, 4},
         {3, 3, 3, 3, 4}, {4, 4, 1, 1, 2}},             /* LIFT: ends on its peak, then the OUTRO */
        {{4, 4, 0, 0, 2}, {0, 0, 0, 0, 4}, {1, 1, 1, 1, 4}, {0, 0, 0, 0, 4}, {2, 2, 2, 2, 2}, {3, 3, 3, 3, 2},
         {1, 1, 1, 1, 4}, {4, 4, 1, 1, 2}}};            /* BREAK: then the last CHORUS hits at full */
    uint32_t r, k, d = song_dtype();
    chain_defaults(c);
    c->count = 8;
    for (r = 0; r < 8u; r++) {
        for (k = 0; k < NTRK; k++)
            c->row[r].slot[k] = ROWS[d][r][k] == 4u ? (uint8_t)CHAIN_SILENT : ROWS[d][r][k];
        c->row[r].repeat = ROWS[d][r][4];
    }
}

static void song_say(void)                              /* "SONG 42 SYNTHWAVE" (too wide: the genre's card) */
{
    char b[28], n[4];
    const fs_genre_t *g = &FS_GENRE[fs.genre % FS_NGENRE];
    if (sg.pend) {                                      /* "SAVE B?" */
        char q[8] = {'S', 'A', 'V', 'E', ' ', (char)('A' + sg.pend - 1u), '?', 0};
        ui_message(q);
        return;
    }
    str_cpy(b, "SONG ", sizeof b);
    fmt_int(n, fg.seed);
    str_cpy(b + str_len(b), n, sizeof b - str_len(b));
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), g->name, sizeof b - str_len(b));
    if (text_w(&AF_M, b) > HEAD_MSG_W - 8) {
        b[str_len(b) - str_len(g->name)] = 0;
        str_cpy(b + str_len(b), g->card, sizeof b - str_len(b));
    }
    ui_message(b);
}
static void song_reset(void)                            /* a new song: its rerolls back to 0, written no more */
{
    memset(sg.nvar, 0, sizeof sg.nvar);
    memset(sg.rvar, 0, sizeof sg.rvar);
    sg.stage = SG_EDIT;
    sg.pend = 0;
}
/* 0.13 (spec v0.4.1 §B2): SONG's punch-in effects, routed through perform.c as FX held routes them: E4..C5 the six
 * of seq.c SG_FX held (no FX button), FX held + F3..B3 the T1..T4 mutes (the choice at the key's down: seq.c kb_fx
 * keeps it to its up), C#5 CLEAR, D#5 DUCK. FX LATCH: they toggle. Never saved, never a step, never P_MUTE */
static uint32_t song_fx_btn(void) { return (fm1_in.buttons >> panel.btn[B_FX]) & 1u; }
static uint32_t song_keymask(uint32_t fx)               /* the keys that take an effect at their down now */
{
    uint32_t k, m = 0, p;
    for (k = 0; k < 27u; k++) {
        if (key_black(k))
            continue;
        p = key_place(k);
        if ((p >= SG_FX_KEY0 && p < SG_FX_KEY0 + 6u) || (fx && p < NTRK))
            m |= 1u << k;
    }
    return m;
}
/* each pass (ui_layer.c layer_masks): FX let go in SONG, its knob macros back to 0 as the FX layer's are when it lets go
 * (FX LATCH: they stay, as there) */
static uint8_t sg_fx_was;
static void song_d4_poll(uint32_t on);
static void song_poll(void);
static void song_fx_poll(uint32_t on, uint32_t fx)
{
    song_d4_poll(on);
    if (on)
        song_poll();
    if (on && sg_fx_was && !fx && !perf_latch_on)
        perf_k[0] = perf_k[1] = perf_k[2] = perf_k[3] = 0;
    sg_fx_was = (uint8_t)(on && fx);
}
static void song_fx_clear(void)                         /* C#5, and leaving SONG: every effect, latch, mute, DUCK off */
{
    perf_kill = 1;                                      /* (held ones: off until their keys are let go) */
    perf_latched = 0;
    perf_k[0] = perf_k[1] = perf_k[2] = perf_k[3] = 0;
}
static void song_duck(void)                             /* D#5: DUCK on / off, latch or not */
{
    perf_latched ^= PF_BIT(PF_DUCK);
    ui_message(perf_latched & PF_BIT(PF_DUCK) ? "DUCK ON" : "DUCK OFF");
}

/* F#5 in STARTER: SONG on (part A in the tracks) or off (STARTER's own patterns back) */
static void song_toggle(void)
{
    if (sg.stage == SG_CONFIRM)
        return;
    if (sg.on)
        song_fx_clear();                                /* (leaving SONG: nothing keeps playing unseen) */
    sg.on = (uint8_t)!sg.on;
    sg.part = SG_A;
    sg.lock = 0;
    sg.focus = 0;                                       /* (0.14: FOCUS off either way, the faders on T1 again; the */
    sg.lastf = 0;                                       /* params and the scenes stay) */
    sg.d4 = 0;
    sg.kpend = sg.ppend = sg.fpend = sg.ov = 0;
    song_reset();
    if (sg.on) {
        song_render(SG_A);
        song_say();
    } else {
        fs_render();
        fs_say();
    }
}
static void song_part(uint32_t x)                       /* black 1..4: part x plays (a SAVE X? not answered: no) */
{
    sg.pend = 0;
    sg.part = (uint8_t)(x % SG_N);
    song_render(sg.part);
    {
        char b[20];
        b[0] = (char)('A' + sg.part);
        b[1] = ' ';
        str_cpy(b + 2, song_role(sg.part), sizeof b - 2u);
        ui_message(b);
    }
}
static void song_lock(uint32_t i)                       /* black 5..8: T1..T4 locked for rerolls */
{
    char b[16] = {'T', (char)('1' + i), ' ', 0};
    sg.lock ^= (uint8_t)(1u << i);
    str_cpy(b + 3, (sg.lock >> i) & 1u ? "LOCKED" : "FREE", sizeof b - 3u);
    ui_message(b);
}
static void song_reroll(void)                           /* C4: the part shown's notes, its tracks not locked */
{
    uint32_t i, any = 0;
    for (i = 0; i < NTRK; i++)
        if (!((sg.lock >> i) & 1u)) {
            sg.nvar[sg.part][i] = (uint8_t)((sg.nvar[sg.part][i] + 1u) % 100u);
            any = 1;
        }
    if (!any) {
        ui_message("ALL TRACKS LOCKED");
        return;
    }
    song_render(sg.part);
    if (sg.stage == SG_WRITTEN) {
        sg.pend = (uint8_t)(sg.part + 1u);
        song_say();
    } else {
        ui_message("NEW NOTES");
    }
}
/* F3 G3 A3 B3 (no FX held at the key's down): T1..T4 of the part shown rerolled alone, T1..T3 their notes (the rhythm
 * kept), T4 its rhythm; a locked track: nothing, "T3 LOCKED". After the song is written: SAVE X? */
static void song_track(uint32_t i)
{
    char b[12] = {'T', (char)('1' + i), ' ', 0};
    if ((sg.lock >> i) & 1u) {
        str_cpy(b + 3, "LOCKED", sizeof b - 3u);
        ui_message(b);
        return;
    }
    if (i < FP_DRUM)
        sg.nvar[sg.part][i] = (uint8_t)((sg.nvar[sg.part][i] + 1u) % 100u);
    else
        sg.rvar[sg.part][i] = (uint8_t)((sg.rvar[sg.part][i] + 1u) % 100u);
    song_render(sg.part);
    sg.flash = (uint8_t)(i + 1u);
    sg.flash_ms = fm1_ms;
    if (sg.stage == SG_WRITTEN) {
        sg.pend = (uint8_t)(sg.part + 1u);
        song_say();
    } else {
        str_cpy(b, "NEW T", sizeof b);
        b[5] = (char)('1' + i);
        b[6] = 0;
        ui_message(b);
    }
}
/* 0.14, spec v0.5 §H: the performance layer.
 *   FOCUS   D4 held + F3..B3: T1..T4's sound on KNOB 1..4 (the same key again, or a D4 tap that picked nothing and
 *           turned nothing: off). The keys' choice is made at their down: FX held a mute, else D4 held the focus,
 *           else a reroll
 *   FADERS  D4 held alone: KNOB 1..4 the focus track's (none: the last one, T1 at first); let go: the values stay
 *   Knobs, first match: FX held the macros (back to 0 when FX is let go), D4 held the faders, FOCUS the track's,
 *           else SEED DENSITY EVOLVE GENRE. The knobs are encoders: a turn moves a value from where it is, so none
 *           jumps after a change of mode or a scene (spec §H1's pickup holds by itself)
 *   Per track: T1 BASS CUTOFF RESO ENV-MOD DECAY, T2 LEAD and T3 PAD FILTER, A/R (ATK and REL together), SEND (REV and
 *           DLY together), LEVEL, T4 DRUM KICK level, TUNE, DECY, DRV (the kit's: DRUM has no tune or decay a lane).
 *           A filter its engine names (song_filter_id); RESO RES or Q, else FB FOLD DUTY, else the track's DIST.
 *           They are the tracks' own params: kept over parts, rerolls, CLEAR and leaving SONG (FOCUS turns off);
 *           saved with a part as any sound edit
 *   SCENES  D5 / E5 recall SCENE 1 / 2 at the next 1/16 (stopped: at once); FX held + D5 / E5 stores the focus params
 *           of all four tracks and their engines; a track on another engine at the recall is skipped ("SCENE 1 SKIP
 *           T2"), the values held to their ranges (seq.c sc_apply; no ramp, see there). RAM only (perform.c
 *           sg_scene): kept to power-off, not saved with the song */
#define SG_KD4 9u                               /* D4's key */
#define SG_NONE 0xFFu
static uint32_t song_named(const track_t *t, const char *const *names, uint32_t n)
{
    const engine_t *en = ENGINES[t->eng_req % NENGINES];
    uint32_t i, k;
    for (k = 0; k < n; k++)
        for (i = 0; i < 8u; i++)
            if (en->edit[i].label && str_eq(en->edit[i].label, names[k]))
                return P_E0 + i;
    return 0;
}
/* track i's focus params: m[knob][2] (a second for A/R, SEND), SG_NONE where none */
static void song_fmap(uint32_t i, uint8_t m[4][2])
{
    static const char *const RES[] = {"RES", "Q", "FB", "FOLD", "DUTY"};
    static const char *const TUNE[] = {"TUNE"}, *const DECY[] = {"DECY"}, *const DRV[] = {"DRV"};
    const track_t *t = &trk[i % NTRK];
    uint32_t r;
    memset(m, SG_NONE, 8);
    if (i == FP_DRUM && drum_track(t)) {
        m[0][0] = P_LN0;
        r = song_named(t, TUNE, 1);
        m[1][0] = (uint8_t)(r ? r : SG_NONE);
        r = song_named(t, DECY, 1);
        m[2][0] = (uint8_t)(r ? r : SG_NONE);
        r = song_named(t, DRV, 1);
        m[3][0] = (uint8_t)(r ? r : P_DIST);
    } else if (i == FP_BASS) {
        m[0][0] = (uint8_t)song_filter_id(t);
        r = song_named(t, RES, NELEM(RES));
        m[1][0] = (uint8_t)(r ? r : P_DIST);
        m[2][0] = P_ED_FLT;
        m[3][0] = P_DEC;
    } else {
        m[0][0] = (uint8_t)song_filter_id(t);
        m[1][0] = P_ATK;
        m[1][1] = P_REL;
        m[2][0] = P_REV;
        m[2][1] = P_DLY;
        m[3][0] = P_LEVEL;
    }
}
static const char *song_fname(uint32_t i, uint32_t k)   /* KNOB k's name on track i (<= 5 characters) */
{
    uint8_t m[4][2];
    song_fmap(i, m);
    if (m[k & 3u][1] != SG_NONE)
        return (k & 3u) == 1u ? "A/R" : "SEND";
    return m[k & 3u][0] == SG_NONE ? "-" : (k & 3u) == 0u && i == FP_DRUM ? "KICK" : track_desc(&trk[i % NTRK], m[k & 3u][0])->label;
}
static uint32_t song_ftrack(void) { return sg.focus ? sg.focus - 1u : sg.lastf; }
static uint32_t song_d4(void) { return sg.on && sg.d4 && ((kb_layer >> SG_KD4) & 1u); }
/* KNOB k (s detents) on the focus track (D4 held alone: its faders); "T1 CUT 74" */
static void song_focus_knob(uint32_t k, int32_t s)
{
    uint32_t i = song_ftrack(), j;
    track_t *t = &trk[i];
    uint8_t m[4][2];
    const param_desc_t *d;
    char b[24], val[8];
    const char *unit;
    int32_t st;
    song_fmap(i, m);
    if (m[k][0] == SG_NONE)
        return;
    d = track_desc(t, m[k][0]);
    st = accel(EN_K1 + k, s, d->fmt == F_ENUM ? 0 : d->max - d->min);
    for (j = 0; j < 2u; j++)
        if (m[k][j] != SG_NONE) {
            const param_desc_t *e = track_desc(t, m[k][j]);
            if (e->max != e->min)
                t->p[m[k][j]] = (int16_t)param_turn(e, t->p[m[k][j]], st);
        }
    if (sg.d4)
        sg.d4_used = 1;                         /* (a D4 hold that turned a knob is no tap) */
    b[0] = 'T';
    b[1] = (char)('1' + i);
    b[2] = ' ';
    str_cpy(b + 3, song_fname(i, k), sizeof b - 3u);
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    param_format(d, t->p[m[k][0]], val, &unit);
    str_cpy(b + str_len(b), val, sizeof b - str_len(b));
    str_cpy(b + str_len(b), unit, sizeof b - str_len(b));
    ui_message(b);
}
static void song_focus_say(void)                        /* "FOCUS T1 BASS" / "FOCUS OFF" */
{
    char b[20] = "FOCUS T1 ";
    if (!sg.focus) {
        ui_message("FOCUS OFF");
        return;
    }
    b[7] = (char)('0' + sg.focus);
    str_cpy(b + 9, FP_ROLE[sg.focus - 1u], sizeof b - 9u);
    ui_message(b);
}
static void song_focus_pick(uint32_t i)                 /* 0.15 (spec v0.6 §H7): F3..B3 alone, the same key again: off */
{
    if (sg.focus == i + 1u) {
        sg.focus = 0;
    } else {
        sg.focus = (uint8_t)(i + 1u);
        sg.lastf = (uint8_t)i;
    }
    song_focus_say();
}
/* each pass (song_fx_poll): D4 let go. (0.15, spec v0.6: a D4 tap does nothing; v0.5's tap turned FOCUS off) */
static void song_d4_poll(uint32_t on)
{
    if (!sg.d4 || (on && ((kb_layer >> SG_KD4) & 1u)))
        return;
    sg.d4 = 0;
    ui.force = 1;
}
static void song_scene(uint32_t x, uint32_t store)      /* D5 / E5: recall, FX held: store */
{
    char b[28] = "SCENE 1 ";
    sg_scene_t *c = &sg_scene[x & 1u];
    b[6] = (char)('1' + (x & 1u));
    if (store) {
        uint32_t i, k, j;
        sc_pend = 0;                            /* (the ISR never puts in one being written) */
        c->n = 0;
        for (i = 0; i < NTRK; i++) {
            uint8_t m[4][2];
            c->eng[i] = trk[i].eng_req;
            c->pre[i] = trk[i].preset;          /* (0.15, spec v0.6 §H11: the sound too) */
            c->usr[i] = trk[i].user;
            song_fmap(i, m);
            for (k = 0; k < 4u; k++)
                for (j = 0; j < 2u; j++)
                    if (m[k][j] != SG_NONE && c->n < SC_MAX) {
                        c->trk[c->n] = (uint8_t)i;
                        c->id[c->n] = m[k][j];
                        c->v[c->n++] = trk[i].p[m[k][j]];
                    }
        }
        sg.scenes |= (uint8_t)(1u << (x & 1u));
        str_cpy(b + 8, "STORED", sizeof b - 8u);
    } else if (!((sg.scenes >> (x & 1u)) & 1u)) {
        str_cpy(b + 8, "EMPTY", sizeof b - 8u);
    } else {
        uint32_t i;
        b[7] = 0;
        sg.ov_skip = 0;
        for (i = 0; i < NTRK; i++)              /* (a track with another engine or sound now: skipped, said) */
            if (sc_differs(c, i)) {
                char n[3] = {'T', (char)('1' + i), 0};
                str_cpy(b + str_len(b), str_len(b) > 7u ? " " : " SKIP ", sizeof b - str_len(b));
                str_cpy(b + str_len(b), n, sizeof b - str_len(b));
                sg.ov_skip |= (uint8_t)(1u << i);
            }
        sc_pend = (uint8_t)((x & 1u) + 1u);
        if (!song.playing)
            sc_apply();                         /* (stopped: no 1/16 to wait for) */
        song_ov(SOV_SCENE + (x & 1u));
    }
    ui_message(b);
}

/* ------------------------------------------------- 0.15: spec v0.6, usability (§H7..§H11) --- */
/* the scales ALGORITHM + FX steps through, the starter's kind first (N_SCALE numbers: MIN DOR PHRY MPEN HARM MAJ LYD MIX
 * PEN) */
static const uint8_t SG_SCALES[] = {2, 3, 8, 6, 7, 1, 9, 4, 5};
static uint32_t song_scale_now(void) { return fs.scale ? fs.scale : FORGE_STARTER[fs_idx()].scale; }
static uint32_t song_root_shown(void) { return sg.kpend ? sg.kroot : fs.root; }
static uint32_t song_scale_shown(void) { return sg.kpend ? sg.kscale : song_scale_now(); }
static const char *const SG_NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static void song_key_name(char *b, uint32_t n)          /* "C# MIN" */
{
    str_cpy(b, SG_NOTE[song_root_shown() % 12u], n);
    str_cpy(b + str_len(b), " ", n - str_len(b));
    str_cpy(b + str_len(b), N_SCALE[song_scale_shown() % NELEM(N_SCALE)], n - str_len(b));
}
static int song_ext_clock(void) { return song.g[G_CLOCK] != 0; }
/* SELECT in SONG: BPM +-1 a detent (FX held: +-5), at once; BPM LOCK and an external clock refuse */
static void song_bpm(int32_t s)
{
    if (song_ext_clock()) {
        ui_message("EXT CLOCK");
        return;
    }
    if (ui_prefs & PREF_BPM_LOCK) {
        ui_message("BPM LOCKED");
        return;
    }
    song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + s * (song_fx_btn() ? 5 : 1), GP[G_BPM].min, GP[G_BPM].max);
    ui.bpm_t = 40;
    song_ov(SOV_TEMPO);
}
static void glo_tap(void);
static void song_tap(void)                              /* F5: TAP tempo (GLO's: the last 4 taps, 2 s starts over) */
{
    int16_t b = song.g[G_BPM];
    if (song_ext_clock()) {
        ui_message("EXT CLOCK");
        return;
    }
    if (ui_prefs & PREF_BPM_LOCK) {                     /* (review of 081a8ff: as SELECT) */
        ui_message("BPM LOCKED");
        return;
    }
    glo_tap();
    if (song.g[G_BPM] != b)
        song_ov(SOV_TEMPO);
    else
        ui_message("TAP");
}
static void song_key_apply(void)                        /* the KEY / SCALE waiting: all four tracks, the part again */
{
    if (!sg.kpend)
        return;
    sg.kpend = 0;
    fs.root = sg.kroot;
    fs.scale = sg.kscale;
    song_render(sg.part);
    if (sg.stage == SG_WRITTEN)
        sg.pend = (uint8_t)(sg.part + 1u);      /* (the part written differs now: SAVE X?) */
}
/* ALGORITHM in SONG: KEY +-1 semitone (wraps), FX held: SCALE; playing, at the next bar (a second turn replaces it) */
static void song_keyturn(int32_t s)
{
    uint32_t r = song_root_shown(), sc = song_scale_shown(), i;
    if (song_fx_btn()) {
        for (i = 0; i < NELEM(SG_SCALES) && SG_SCALES[i] != sc; i++)
            ;
        i = i >= NELEM(SG_SCALES) ? 0u : (i + NELEM(SG_SCALES) + (s > 0 ? 1u : NELEM(SG_SCALES) - 1u)) % NELEM(SG_SCALES);
        sc = SG_SCALES[i];
    } else {
        r = (r + 12u + (s > 0 ? 1u : 11u)) % 12u;
    }
    sg.kroot = (uint8_t)r;
    sg.kscale = (uint8_t)sc;
    sg.kpend = 1;
    if (!song.playing)
        song_key_apply();                       /* (stopped: at once) */
    song_ov(SOV_KEY);
}
/* PRESETS in SONG: the focus track's engine's presets (FOCUS off: the last focused); loaded at the next bar 600 ms
 * after the last detent (stopped: 600 ms after it), OCT- before then: nothing loaded */
static uint32_t song_pre_n(const track_t *t) { return preset_shown(t->eng_req % NENGINES); }
static uint32_t song_pre_rank(const track_t *t, uint32_t k)
{
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    return e->npresets ? preset_rank(e, preset_orig(e, k % e->npresets)) : 0u;
}
static uint32_t song_pre_at(const track_t *t, uint32_t n)   /* the n-th shown preset */
{
    const engine_t *e = ENGINES[t->eng_req % NENGINES];
    uint32_t k;
    for (k = 0; k < e->npresets && (preset_orig(e, k) != k || n--); k++)
        ;
    return k < e->npresets ? k : 0u;
}
static void song_preset(int32_t s)
{
    uint32_t i = song_ftrack(), n;
    const track_t *t = &trk[i];
    if (song_fx_btn() || !(n = song_pre_n(t)))
        return;
    if (sg.ppend != i + 1u) {
        sg.ppend = (uint8_t)(i + 1u);
        sg.pk = t->preset;
    }
    sg.pk = (uint8_t)song_pre_at(t, (song_pre_rank(t, sg.pk) + n * 4u + (uint32_t)(s > 0 ? 1 : -1)) % n);
    sg.pms = fm1_ms;
    if (sg.pk == t->preset && !t->user)
        sg.ppend = 0;                           /* (back where it was: nothing waits) */
    song_ov(SOV_PRESET);
}
static void song_load(uint32_t i, uint32_t e, uint32_t k)   /* track i: engine e's preset k */
{
    track_t *t = &trk[i % NTRK];
    if (t->eng_req != e)
        set_engine_of(t, e);
    apply_preset_to(t, k);
    ui.force = 1;
}
/* 0.15 §H10: four favourites, D4 + black 1..4 (FX: store), the focus track's sound, in the settings: engine + 1 and
 * preset (0 empty), or SG_FAV_USER and a user preset's slot (the sound the track plays is a user preset's), in
 * favorites.factory[0][24..31], ANALOG's favourite bits of presets 192..255, which no engine has (tests/forge_test.c
 * checks). (The spec's [15][13..20] is taken: [15][14..23] hold the FX layer's key map) */
#define sg_fav (&favorites.factory[0][24])
#define SG_FAV_USER 0x80u
static void song_fav_load(uint32_t i, uint32_t x)       /* favourite x into track i */
{
    uint32_t e = sg_fav[2u * (x & 3u)], k = sg_fav[2u * (x & 3u) + 1u];
    if (e == SG_FAV_USER) {                             /* (up_load loads into the selected track: i, for the call) */
        uint8_t sel = song.sel;
        song.sel = (uint8_t)(i % NTRK);
        (void)up_load(k);
        song.sel = sel;
        ui.force = 1;
    } else {
        song_load(i, e - 1u, k);
    }
}
static void song_fav(uint32_t x, uint32_t store)
{
    uint32_t i = song_ftrack(), e = sg_fav[2u * (x & 3u)], dr, u;
    char b[28] = "FAV 1 ";
    b[4] = (char)('1' + (x & 3u));
    if (store) {
        u = user_of(&trk[i]);
        sg_fav[2u * (x & 3u)] = (uint8_t)(u < UP_SLOTS ? SG_FAV_USER : trk[i].eng_req + 1u);
        sg_fav[2u * (x & 3u) + 1u] = (uint8_t)(u < UP_SLOTS ? u : trk[i].preset);
        settings_later();
        str_cpy(b + 6, u < UP_SLOTS ? "STORED (USER)" : "STORED", sizeof b - 6u);
        ui_message(b);
        return;
    }
    if (e == SG_FAV_USER && !up_used(sg_fav[2u * (x & 3u) + 1u])) {
        str_cpy(b + 5, ": USER SOUND GONE", sizeof b - 5u);   /* (its slot erased or overwritten empty since) */
        ui_message(b);
        return;
    }
    if (!e || (e > NENGINES && e != SG_FAV_USER)) {
        str_cpy(b + 6, "EMPTY", sizeof b - 6u);
        ui_message(b);
        return;
    }
    dr = ENGINES[e == SG_FAV_USER ? up_engine(sg_fav[2u * (x & 3u) + 1u]) : e - 1u] == &ENG_DRUM;
    if (dr != (i == FP_DRUM && drum_track(&trk[i]))) {   /* (a kit on T4 only, and only a kit there) */
        b[5] = ':';
        str_cpy(b + 6, dr ? " DRUM ONLY T4" : " NOT ON DRUM", sizeof b - 6u);
        ui_message(b);
        return;
    }
    sg.fpend = (uint8_t)((x & 3u) + 1u);
    sg.ftrk = (uint8_t)i;
    if (!song.playing) {
        song_fav_load(i, x);
        sg.fpend = 0;
    }
    song_ov(SOV_FAV);
}
/* each pass: what waits for the bar, put in on the bar's last 1/16 (its notes played: the change sounds from the next
 * bar's first), late into it; missed, on the first pass after. Stopped: KEY and a favourite at once, a preset 600 ms
 * after its last detent */
static void song_poll(void)
{
    uint32_t bar = 0, late = clk_pos != CLK_START && clk_pos >= div_samples(2) / 2u;
    if (!sg.on)
        return;
    if (song.playing && clk_pos != CLK_START) {
        if (clk_step == 15u && late)
            bar = 1;
        else if (clk_step == 15u)
            sg.armed = 1;
        else if (sg.armed)
            bar = 1;                            /* (the late part of the 1/16 missed) */
        if (bar)
            sg.armed = 0;
    }
    if (sg.kpend && (bar || !song.playing))
        song_key_apply();
    if (sg.fpend && (bar || !song.playing)) {
        uint32_t x = sg.fpend - 1u;
        sg.fpend = 0;
        song_fav_load(sg.ftrk, x);
    }
    if (sg.ppend && fm1_ms - sg.pms >= 600u && (bar || !song.playing)) {
        uint32_t i = sg.ppend - 1u;
        sg.ppend = 0;
        song_load(i, trk[i].eng_req, sg.pk);
    }
    if (sg.ov && fm1_ms - sg.ov_ms >= SOV_MS && !sg.ppend && !sg.kpend && !sg.fpend)
        sg.ov = 0;                              /* (a change waiting keeps its box up) */
}
/* SELECT ALGORITHM PRESETS in SONG (ui_input.c, the layer's pass); the confirm: SELECT only */
static void song_enc(int32_t sel, int32_t algo, int32_t pre)
{
    if (sel)
        song_bpm(sel);
    if (sg.stage == SG_CONFIRM)
        return;
    for (; algo; algo -= algo > 0 ? 1 : -1)             /* (a detent a step) */
        song_keyturn(algo);
    for (; pre; pre -= pre > 0 ? 1 : -1)
        song_preset(pre);
}
static int song_enc_on(void) { return ui.ly == LAYER_SEQ && fs.on && sg.on; }
static int song_cancel_pending(void)                    /* OCT-: a preset or favourite waiting, dropped */
{
    if (!sg.ppend && !sg.fpend)
        return 0;
    sg.ppend = sg.fpend = 0;
    sg.ov = 0;
    ui_message("PRESET CANCELLED");
    return 1;
}

/* the knob cards with FOCUS on or D4 held: the focus track's four, its values (starter_cards); 1 when drawn */
static int song_cards(void)
{
    uint32_t i = song_ftrack(), k;
    if (!sg.on || sg.stage == SG_CONFIRM || !(sg.focus || song_d4()))
        return 0;
    for (k = 0; k < 4u; k++) {
        uint8_t m[4][2];
        const param_desc_t *d;
        char val[8];
        const char *unit;
        int32_t v;
        song_fmap(i, m);
        if (m[k][0] == SG_NONE) {
            draw_column(k, "", "", "", T_DIM, 0, ICON_LEVEL);
            continue;
        }
        d = track_desc(&trk[i], m[k][0]);
        v = trk[i].p[m[k][0]];
        param_format(d, v, val, &unit);
        draw_column(k, song_fname(i, k), val, unit, VAL(k), d->max > d->min ? (v - d->min) * 1000 / (d->max - d->min) : 0,
                    ICON_LEVEL);
    }
    return 1;
}

/* a white key in SONG (layer_key, at its down): 1 when SONG answered it. E4..C5 are the ISR's effects; F3..B3 the
 * focus (the same key again: off), the mutes when the ISR took them (kb_fx) or FX is held, the rerolls with D4 held;
 * D4 ALT (held alone: the faders); D5 E5 the scenes; F5 TAP (0.15, spec v0.6) */
static int song_wkey(uint32_t k)
{
    uint32_t p = key_place(k);
    if (p >= SG_FX_KEY0 && p < SG_FX_KEY0 + 6u)
        return 1;
    if (p < NTRK) {
        if (sg.d4)
            sg.d4_used = 1;
        if (kb_fx[k] || song_fx_btn())
            ;                                   /* (the mute: the ISR's) */
        else if (song_d4())
            song_track(p);                      /* (0.15, spec v0.6 §H7: D4 held, the reroll) */
        else
            song_focus_pick(p);                 /* (.. alone: FOCUS, the same key again: off) */
        return 1;
    }
    if (p == TL_COOK) {                         /* D4: ALT (held alone: the faders; a tap: nothing, 0.15) */
        sg.d4 = 1;
        sg.d4_used = (kb_layer & 0x55u) != 0u;
        return 1;
    }
    if (p == TL_FORGE) {                        /* 0.15 F5: TAP tempo */
        song_tap();
        return 1;
    }
    if (p == SG_FX_KEY0 + 6u || p == SG_FX_KEY0 + 7u) {   /* D5 / E5: the scenes */
        song_scene(p - SG_FX_KEY0 - 6u, song_fx_btn());
        return 1;
    }
    if (p >= SG_FX_KEY0 + 8u && p < TL_STARTER) {
        ui_message("NOT IN SONG");
        return 1;
    }
    return 0;
}

/* the four slots and the chain, after the confirm's OCT+; D first, A last (the slot in use and the tracks: A) */
static int song_write(void)
{
    char b[SG_NAME_LEN + 1u];
    chain_config_t was = chain_config;
    uint32_t x;
    if (transport_busy()) {                             /* (checked before anything is written: project_save_as's own */
        ui_message("STOP TO SAVE");                     /* refusal, said up front) */
        return 1;
    }
    song_chain(&chain_config);
    for (x = SG_N; x-- > 0;) {
        song_render(x);
        song_name(x, b);
        if (SG_SAVE(x, b)) {                    /* (a slot refused: the chain as it was; the slots after it */
            char m[16] = "SAVE FAILED X";              /* in D..A written already, said by the letter) */
            m[12] = (char)('A' + x);
            chain_config = was;
            ui_message(m);
            return 2;
        }
    }
    sg.part = SG_A;
    sg.stage = SG_WRITTEN;
    ui_message("SONG SAVED: A-D");
    str_cpy(ui.msg2, "GLO > SONG PLAYS IT", sizeof ui.msg2);
    return 0;
}
/* OCT+ (up 1) / OCT- (0) in SONG; returns 1 when SONG (and STARTER) closed */
static int song_oct(uint32_t up)
{
    char b[SG_NAME_LEN + 1u];
    if (!up && song_cancel_pending())                   /* (0.15: a preset waiting for the bar: only that) */
        return 0;
    if (sg.pend) {                                      /* SAVE X?: that slot only, or nothing */
        uint32_t x = sg.pend - 1u;
        if (!up) {
            sg.pend = 0;
            ui_message("NOT SAVED");
            return 0;
        }
        if (transport_busy()) {                         /* (playing: the prompt stays up, nothing written) */
            ui_message("STOP TO SAVE");
            return 0;
        }
        song_name(x, b);
        if (SG_SAVE(x, b)) {
            char m[16] = "SAVE FAILED X";
            m[12] = (char)('A' + x);
            ui_message(m);                              /* (the prompt stays up) */
            return 0;
        }
        sg.pend = 0;
        {
            char m[12] = {(char)('A' + x), ' ', 'S', 'A', 'V', 'E', 'D', 0};
            ui_message(m);
        }
        return 0;
    }
    if (sg.stage == SG_EDIT) {
        if (!up) {
            song_off();
            fs_close(0);
            ui_message("STARTER CANCELLED");
            return 1;
        }
        if (transport_busy()) {
            ui_message("STOP TO SAVE");
            return 0;
        }
        sg.stage = SG_CONFIRM;
        ui_message("OVERWRITE A-D?");
        return 0;
    }
    if (sg.stage == SG_CONFIRM) {
        if (!up) {                                      /* (the tracks and every slot as they were) */
            song_off();
            fs_close(0);
            ui_message("SONG CANCELLED");
            return 1;
        }
        if (song_write())
            sg.stage = SG_EDIT;
        return 0;
    }
    song_off();                                         /* (written: done, the part shown stays in the tracks) */
    fs_close(1);
    ui_message("SONG DONE");
    return 1;
}
static uint32_t song_sig(void)
{
    uint32_t fl = sg.flash && fm1_ms - sg.flash_ms < 300u ? sg.flash : 0u;
    return sg.on ? 29u + sg.stage * 7u + sg.part * 131u + sg.lock * 1031u + sg.pend * 8191u + fl * 65521u +
                   (perf_latched & PF_BIT(PF_DUCK) ? 3u : 0u) + sg.focus * 97u + song_d4() * 389u + sg.scenes * 1543u : 0u;
}

/* the footer and the help row of each stage */
static const khint_t SONG_FOOT[4][3] = {
    {{KC_OCTUP, "GENERATE"}, {KC_OCTDN, "CANCEL"}, {0, 0}},     /* editing */
    {{KC_OCTUP, "OVERWRITE"}, {KC_OCTDN, "CANCEL"}, {0, 0}},    /* OVERWRITE A-D? */
    {{KC_OCTUP, "DONE"}, {KC_OCTDN, "DONE"}, {0, 0}},          /* written */
    {{KC_OCTUP, "SAVE"}, {KC_OCTDN, "KEEP"}, {0, 0}}};         /* SAVE X? */
static const char H_SONG[] = "[KEYS]F3-B3 FOCUS D4+ REROLL";
static const khint_t *song_foot(void) { return SONG_FOOT[sg.pend ? 3u : sg.stage]; }

/* the SONG screen: the four parts (black keys 1..4, the one shown selected), then a row a track: T1..T4, its role
 * (LOCK at the right when locked), the part's first 16 steps; under them what is written. The confirm: the four
 * slots' names as they are now ("--" an empty one) */
static void layer_song(void)
{
    uint32_t i, j;
    for (i = 0; i < SG_N; i++) {
        int32_t x = 1 + 60 * (int32_t)i;               /* (58 wide: "B CHORUS" is) */
        char b[12] = {(char)('A' + i), ' ', 0};
        uint16_t f = i == sg.part ? T_THEME : T_RAISE, ink = i == sg.part ? T_SURF : T_TEXT;
        str_cpy(b + 2, song_role(i), sizeof b - 2u);
        cv_rrect(x, 4, 58, 16, 3, f, T_SURF);
        GFX_HOOK_ALIGN(x, 4, x + 58, 20, AL_HV, "song part cell centred");
        cv_text_in(x, 4 + CAP_IN(S, 16), 58, &AF_S, b, ink, f);
    }
    if (sg.stage == SG_CONFIRM) {
        GFX_HOOK_ALIGN(0, 26, 240, 44, AL_HV, "song confirm title centred");
        cv_text_in(0, 26 + CAP_IN(M, 18), 240, &AF_M, "OVERWRITE A-D?", T_TEXT, T_SURF);
        for (i = 0; i < SG_N; i++) {
            char nm[SG_NAME_LEN + 1u], l[3] = {(char)('A' + i), 0, 0};
            int32_t x = i < 2u ? 14 : 126, y = 52 + 22 * (int32_t)(i % 2u);
            if (!project_name(i, nm) || !nm[0])
                str_cpy(nm, "--", sizeof nm);
            GFX_HOOK_ALIGN(0, y, 0, y + 18, AL_V, "song confirm slot line on its middle");
            cv_text_on(x, y + CAP_IN(S, 18), &AF_S, l, T_THEME, T_SURF);
            cv_text_on(x + 14, y + CAP_IN(S, 18), &AF_S, nm, T_TEXT, T_SURF);
        }
        GFX_HOOK_ALIGN(0, 100, 240, 118, AL_HV, "song confirm note centred");
        cv_text_in(0, 100 + CAP_IN(S, 18), 240, &AF_S, "THE CHAIN TOO (GLO > SONG)", T_MID, T_SURF);
        return;
    }
    for (i = 0; i < FP_N; i++) {
        const track_t *t = &trk[i];
        int32_t y = 24 + 19 * (int32_t)i;
        char n[3] = {'T', (char)('1' + i), 0};
        GFX_HOOK_ALIGN(0, y, 0, y + 17, AL_V, "song row text on its middle");
        cv_text_on(8, y + CAP_IN(S, 17), &AF_S, n, T_MID, T_SURF);
        GFX_HOOK_ALIGN(0, y, 0, y + 17, AL_V, "song row text on its middle");
        cv_text_on(26, y + CAP_IN(S, 17), &AF_S, FP_ROLE[i],
                   (sg.flash == i + 1u && fm1_ms - sg.flash_ms < 300u) || ((sg.focus || song_d4()) && song_ftrack() == i) ? T_THEME : T_TEXT,
                   T_SURF);                             /* (a reroll; 0.14: the focus track) */
        if ((sg.lock >> i) & 1u) {
            GFX_HOOK_ALIGN(0, y, 0, y + 17, AL_V, "song row text on its middle");
            cv_text_r(116, y + CAP_IN(S, 17), &AF_S, "LOCK", T_THEME, T_SURF);
        }
        for (j = 0; j < 16u; j++) {
            const step_t *s = &t->step[j];
            int on = drum_track(t) ? step_lanes(s) != 0u : s->time == ST_NOTE && s->n;
            int32_t x = 122 + 7 * (int32_t)j;
            uint16_t c = j >= tl_len(t) ? T_SURF : on ? T_THEME : s->time == ST_TIE ? T_MID : T_RAISE;
            cv_rect(x, y + 2, 6, 13, c);
        }
    }
    {
        char q[16] = "SAVE X? OCT+";
        q[5] = (char)('A' + (sg.pend ? sg.pend - 1u : 0u));
        GFX_HOOK_ALIGN(0, 100, 0, 118, AL_V, "song state on its middle");
        cv_text_on(10, 100 + CAP_IN(S, 18), &AF_S, sg.pend ? q : sg.stage == SG_WRITTEN ? "SAVED A-D" : "NOT SAVED YET",
                   sg.pend ? T_THEME : sg.stage == SG_WRITTEN ? T_TEXT : T_MID, T_SURF);
        GFX_HOOK_ALIGN(0, 100, 0, 118, AL_V, "song state on its middle");
        if (sg.focus || song_d4()) {            /* 0.14: "FOCUS T2" / "FADER T1" (D4 held); the cards show its four */
            char f[12] = "FOCUS T1";
            if (song_d4())
                memcpy(f, "FADER", 5);
            f[7] = (char)('1' + song_ftrack());
            GFX_HOOK_ALIGN(0, 100, 0, 118, AL_V, "song state on its middle");
            cv_text_r(230, 100 + CAP_IN(S, 18), &AF_S, f, song_d4() ? T_TEXT : T_THEME, T_SURF);
        } else {
            GFX_HOOK_ALIGN(0, 100, 0, 118, AL_V, "song state on its middle");
            cv_text_r(230, 100 + CAP_IN(S, 18), &AF_S, "C4 REROLL", T_MID, T_SURF);
        }
    }
}
/* the LEDs of the black keys in SONG: the part shown lit (the others breathe), a lock lit (free: breathes), F#5 lit */
static void song_led(uint32_t p, uint32_t *on, uint32_t *can)
{
    *on = (p < SG_N && p == sg.part) || (p >= 4u && p < 8u && ((sg.lock >> (p - 4u)) & 1u)) || p == 10u ||
          (p == 9u && (perf_latched & PF_BIT(PF_DUCK)));
    *can = p < 10u;
}
/* the white keys' LEDs in SONG: an effect on (held or latched) lit, the others breathe; F3..B3 breathe (locked:
 * dark), the focus track lit; C4 breathes; D4 lit with FOCUS on or held; D5 E5 lit with a scene stored (else breathe);
 * G5 lit; F5 dark */
static void song_led_w(uint32_t p, uint32_t *on, uint32_t *can)
{
    uint32_t held = perf_kill ? 0u : perf_held | perf_latched;
    *on = p == TL_STARTER || (p >= SG_FX_KEY0 && p < SG_FX_KEY0 + 6u && ((held >> SG_FX[p - SG_FX_KEY0]) & 1u)) ||
          (p < NTRK && (((held >> (PF_M1 + p)) & 1u) || sg.focus == p + 1u)) || (p == TL_COOK && (sg.focus || song_d4())) ||
          ((p == SG_FX_KEY0 + 6u || p == SG_FX_KEY0 + 7u) && ((sg.scenes >> (p - SG_FX_KEY0 - 6u)) & 1u));
    *can = (p < NTRK && !((sg.lock >> p) & 1u)) || p == TL_RANDOM || p == TL_COOK || (p >= SG_FX_KEY0 && p < SG_FX_KEY0 + 8u);
}

/* STARTER or SONG closed some other way (the layer let go, another tool): SONG with it, nothing written */
static void song_off(void)
{
    if (sg.on)
        song_fx_clear();                                /* (spec v0.4.1: leaving SONG turns its effects off) */
    perf_song = 0;
    sg_fxmask = 0;
    sg.on = 0;
    sg.focus = 0;                                       /* (0.14: the params stay, FOCUS off; the scenes kept) */
    sg.d4 = 0;
    sg.kpend = sg.ppend = sg.fpend = sg.ov = 0;         /* (0.15: a change waiting for the bar is lost) */
    sg.stage = SG_EDIT;
    sg.pend = 0;
}
/* KNOB 1..4 in SONG: as STARTER's (SEED DENSITY EVOLVE GENRE), then the part shown again; a new song is not written */
static void song_knob(uint32_t k, int32_t s)
{
    uint8_t seed = fg.seed, genre = fs.genre;
    fs_knob(k, s);
    if (fg.seed != seed || fs.genre != genre)
        song_reset();                                   /* (another song: EDIT, not written; DENSITY / EVOLVE: the */
    song_render(sg.part);                               /* same song, still written, a reroll still asks SAVE X?) */
    song_say();
}
