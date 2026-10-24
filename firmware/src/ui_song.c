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
    uint8_t kpend, kroot, kscale, ppend, pk, fpend, ftrk, armed, kwhat, flast;
    uint32_t pms;
    uint8_t ov, ov_skip;                    /* .. §J1: the overlay shown (SOV_*, 0 none) and a scene's skipped tracks */
    uint32_t ov_ms;
    /* 0.16 (spec v0.7): a key whose hold does something else (SG_HOLD_MS): hk its key + 1, hkind, hp its place, hms its
     * down, hfired the hold done; hbpm the tempo before F5's tap. qpart: a part waiting for the bar (+ 1). click: the
     * visual metronome. chain: auto-chain on, cbar the bars played of the part, cbars[x] its length (index of SG_CBARS).
     * dirty: bit x, part x differs from its slot (an orange dot). octpend / octms: a single OCT- waiting for a second */
    uint8_t hk, hkind, hp, hfired, qpart, click, chain, cbar, cbars[SG_N], dirty, octpend;
    int16_t hbpm;
    uint32_t hms, octms;
    /* 0.17 (spec v0.8 §2): FILL. fk: C4 held for a live fill (+ 1), fms its down; farm: a bar-end fill armed, fwait the
     * bar lines to its bar; fbars: the fills' count (their type's salt). The sequencer's side: seq.c sgf */
    uint8_t fk, farm, fwait, fbars, barhit;
    /* 0.17 (spec v0.8 §2b): the sound browsed (sref_t: pkind, pe, pk), in category pcat or the drum list (pdrum);
     * pjump: the last turn was FX's (a category / group) */
    uint8_t pkind, pe, pdrum, pcat, pjump;
    uint32_t fms;
    uint8_t nvar[SG_N][NTRK], rvar[SG_N][NTRK];
} sg;

/* 0.15 (spec v0.6 §J1): the overlays; 0.16 (spec v0.7 §A): every action in SONG opens the box, its kind, a small line
 * (the context), a large one (the action or value), a note ("@ NEXT BAR") or a value bar, a red frame when refused.
 * A new action replaces it at once; it closes SG_POPUP_MS after the last (ui_song_view.c draws it) */
enum { SOV_NONE, SOV_TEMPO, SOV_KEY, SOV_PRESET, SOV_SCENE, SOV_SCENE2, SOV_FAV, SOV_POP };
#define SG_POPUP_MS 1000u                       /* spec v0.7 §1: the box's time (display, not a hold) */
#define SG_HOLD_MS 500u                         /* .. every hold: FAV store, F5 CLICK, D5 / E5 store */
#define SG_HOLD_SEQ_MS 600u                     /* .. SEQ held: SONG */
#define SOV_MS SG_POPUP_MS
static struct {
    char sm[20], big[24], note[20];
    int16_t bar;                                /* 0..1000, -1 none */
    uint8_t red, n;                             /* n: bumped on each (the view's signature) */
} sg_pop;
static void song_pop(uint32_t kind, const char *sm, const char *big, const char *note, int32_t bar, uint32_t red)
{
    str_cpy(sg_pop.sm, sm, sizeof sg_pop.sm);
    str_cpy(sg_pop.big, big, sizeof sg_pop.big);
    str_cpy(sg_pop.note, note ? note : "", sizeof sg_pop.note);
    sg_pop.bar = (int16_t)(bar < 0 ? -1 : bar > 1000 ? 1000 : bar);
    sg_pop.red = (uint8_t)(red != 0u);
    sg_pop.n++;
    sg.ov = (uint8_t)kind;
    sg.ov_ms = fm1_ms;
}
static void song_refuse(const char *what, const char *big)   /* a refused action: the red box (and the message) */
{
    song_pop(SOV_POP, what, big, "", -1, 1);
    ui_message(what);
}
static void song_ov(uint32_t kind);
static void song_remember(void);
static void song_sound_name(uint32_t e, uint32_t k, uint32_t user, char *b);
static void song_undo(void);
#define SG_OCT2_MS 350u
static const uint8_t SG_CBARS[5] = {1, 2, 4, 8, 16};    /* 0.16 auto-chain: a part's bars (index; default 8) */
#define SG_CBAR_DEF 3u
/* 0.16 (spec v0.7 §3): one level of UNDO, double-tap OCT-: the last reroll (the part's reroll counts: it renders again),
 * the last sound loaded on a track (a preset at the bar, a favourite: its engine, preset and sound values), the last
 * KEY / SCALE change. ~0.3 KB */
enum { SU_NONE, SU_REROLL, SU_SOUND, SU_KEY };
static struct {
    uint8_t kind, what, trk, eng, pre, usr, root, scale, pack;   /* what: the tracks a reroll took (bit i), 0xF C4 */
    uint8_t nvar[SG_N][NTRK], rvar[SG_N][NTRK];
    int16_t p[P_COUNT];
} sgu;
static void song_undo_take(uint32_t kind, uint32_t what)
{
    sgu.kind = (uint8_t)kind;
    sgu.what = (uint8_t)what;
    if (kind == SU_REROLL) {
        memcpy(sgu.nvar, sg.nvar, sizeof sgu.nvar);
        memcpy(sgu.rvar, sg.rvar, sizeof sgu.rvar);
    } else if (kind == SU_SOUND) {
        const track_t *t = &trk[what % NTRK];
        sgu.trk = (uint8_t)(what % NTRK);
        sgu.eng = t->eng_req;
        sgu.pre = t->preset;
        sgu.usr = t->user;
        sgu.pack = fs_packid[what % NTRK];
        memcpy(sgu.p, t->p, sizeof sgu.p);
    } else if (kind == SU_KEY) {
        sgu.root = fs.root;
        sgu.scale = fs.scale;
    }
}
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
    sg.dirty = 0xFu;                                    /* (0.16: no part saved) */
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
static void song_hold_poll(void);
static void song_fill_down(uint32_t k);
static void song_fill_set(uint32_t salt);
static void song_fill_refuse(const char *r);
static uint32_t song_pack_of(const track_t *t);
static void song_snd_put(void);
static void song_fill_stop(void);
static void song_fill_poll(uint32_t bar);
static void song_fill_bar(void);
static void song_chain_bars(int32_t s);
enum { SH_PART = 1, SH_SCENE, SH_TAP };
static void song_hold_start(uint32_t k, uint32_t kind, uint32_t p);
static void song_part_tap(uint32_t x);
static void song_chain_toggle(void);
static void song_autolock(void);
static const char *song_fx_name(uint32_t e);
static int song_oct_do(uint32_t up);
static void song_fx_poll(uint32_t on, uint32_t fx)
{
    song_d4_poll(on);
    if (sg.on && sg.octpend && fm1_ms - sg.octms >= SG_OCT2_MS) {   /* (0.16: OCT- alone, no second tap came) */
        sg.octpend = 0;
        if (song_oct_do(0))
            on = 0;
    }
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
    song_pop(SOV_POP, "DUCK", perf_latched & PF_BIT(PF_DUCK) ? "ON" : "OFF", "", -1, 0);
}

/* F#5 in STARTER: SONG on (part A in the tracks) or off (STARTER's own patterns back) */
static void song_toggle(void)
{
    uint32_t x;
    if (sg.stage == SG_CONFIRM)
        return;
    if (sg.on) {
        song_fx_clear();                                /* (leaving SONG: nothing keeps playing unseen) */
        song_snd_take();                                /* (0.17: its sounds, for coming back) */
    }
    sg.on = (uint8_t)!sg.on;
    sg.part = SG_A;
    sg.lock = 0;
    sg.focus = 0;                                       /* (0.14: FOCUS off either way, the faders on T1 again; the */
    sg.lastf = 0;                                       /* params and the scenes stay) */
    sg.d4 = 0;
    sg.kpend = sg.ppend = sg.fpend = sg.ov = 0;
    sg.qpart = sg.hk = sg.octpend = 0;
    sg.chain = sg.cbar = 0;
    song_fill_stop();
    for (x = 0; x < SG_N; x++)
        sg.cbars[x] = SG_CBAR_DEF;
    song_reset();
    if (sg.on) {
        char b[SG_NAME_LEN + 1u];
        song_snd_put();                                 /* (0.17: the same song: its sounds as left) */
        song_render(SG_A);
        song_say();
        song_autolock();
        song_name(SG_A, b);
        song_pop(SOV_POP, "SONG", b, "", -1, 0);                                /* (0.16 spec v0.7 §1: SONG keeps the layer open) */
    } else {
        song_remember();
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
    b[2] = 0;
    song_pop(SOV_POP, (sg.lock >> i) & 1u ? "LOCK" : "UNLOCK", b, "", -1, 0);
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
        song_refuse("ALL TRACKS LOCKED", "NOTES");
        return;
    }
    song_undo_take(SU_REROLL, 0xFu);
    song_render(sg.part);
    sg.dirty |= (uint8_t)(1u << sg.part);
    song_pop(SOV_POP, "REROLL", "NOTES", "", -1, 0);
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
        ui_message(b);                          /* ("T3 LOCKED", the red box: REROLL refused) */
        song_pop(SOV_POP, b, "REROLL", "", -1, 1);
        return;
    }
    song_undo_take(SU_REROLL, 1u << i);
    {
        char t[3] = {'T', (char)('1' + i), 0};
        song_pop(SOV_POP, "REROLL", t, "", -1, 0);
    }
    if (i < FP_DRUM)
        sg.nvar[sg.part][i] = (uint8_t)((sg.nvar[sg.part][i] + 1u) % 100u);
    else
        sg.rvar[sg.part][i] = (uint8_t)((sg.rvar[sg.part][i] + 1u) % 100u);
    song_render(sg.part);
    sg.dirty |= (uint8_t)(1u << sg.part);
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
 *   FADERS  D4 held alone: KNOB 1..4 the focus track's (none: the last one, T1 at first); let go: the values stay.
 *           0.15 (after the device test): D4 held is the mixer instead, KNOB 1..4 T1..T4 LEVEL (song_mix_knob)
 *   Knobs, first match: FX held the macros (back to 0 when FX is let go), D4 held the levels, FOCUS the track's,
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
/* D4 held: from its down (song_wkey) to a real key-up, the key as the ISR (kb_layer) or the scan (fm1_in.notes) sees it
 * (0.15 on the device: the pass could see the down before the ISR had put D4 in kb_layer, and dropped the hold) */
static uint32_t song_d4_down(void) { return ((kb_layer | fm1_in.notes) >> SG_KD4) & 1u; }
static uint32_t song_d4(void) { return sg.on && sg.d4; }
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
    {   /* the box: "T1 BASS", "CUT 74" and its bar */
        char sm[12] = {'T', (char)('1' + i), ' ', 0};
        str_cpy(sm + 3, FP_ROLE[i], sizeof sm - 3u);
        song_pop(SOV_POP, sm, b + 3, "", d->max > d->min ? (t->p[m[k][0]] - d->min) * 1000 / (d->max - d->min) : -1, 0);
    }
}
/* 0.15 (Ong, after the device test): D4 held, KNOB 1..4 are T1..T4 LEVEL, a mixer (FOCUS on or off); the values stay
 * when D4 is let go. ("T3 LEVEL -6.0 dB") */
static void song_mix_knob(uint32_t k, int32_t s)
{
    track_t *t = &trk[k & 3u];
    const param_desc_t *d = &TP[P_LEVEL];
    char b[24] = "T1 LEVEL ", val[8];
    const char *unit;
    t->p[P_LEVEL] = (int16_t)param_turn(d, t->p[P_LEVEL], accel(EN_K1 + k, s, d->max - d->min));
    sg.d4_used = 1;
    b[1] = (char)('1' + (k & 3u));
    param_format(d, t->p[P_LEVEL], val, &unit);
    str_cpy(b + str_len(b), val, sizeof b - str_len(b));
    str_cpy(b + str_len(b), unit, sizeof b - str_len(b));
    ui_message(b);
    {   /* the box: "MIXER", "T1 -4.0" and its bar */
        char big[16] = {'T', (char)('1' + (k & 3u)), ' ', 0};
        str_cpy(big + 3, val, sizeof big - 3u);
        song_pop(SOV_POP, "MIXER", big, "", (t->p[P_LEVEL] - d->min) * 1000 / (d->max - d->min ? d->max - d->min : 1), 0);
    }
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
    char t[3] = {'T', (char)('1' + i), 0};
    if (sg.focus == i + 1u) {
        sg.focus = 0;
    } else {
        sg.focus = (uint8_t)(i + 1u);
        sg.lastf = (uint8_t)i;
    }
    song_focus_say();
    song_pop(SOV_POP, sg.focus ? "FOCUS" : "FOCUS", sg.focus ? t : "OFF", "", -1, 0);
}
/* each pass (song_fx_poll): D4 let go. (0.15, spec v0.6: a D4 tap does nothing; v0.5's tap turned FOCUS off) */
static uint32_t song_d4_down(void);
static void song_d4_poll(uint32_t on)
{
    if (!sg.d4 || (on && song_d4_down()))
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
        str_cpy(b + 8, "SAVED", sizeof b - 8u);
        song_pop(SOV_POP, b, "T1-T4", "", -1, 0);       /* (0.16: its box) */
        str_cpy(b + 8, "STORED", sizeof b - 8u);
    } else if (!((sg.scenes >> (x & 1u)) & 1u)) {
        str_cpy(b + 8, "EMPTY", sizeof b - 8u);
        b[7] = 0;
        song_pop(SOV_POP, "EMPTY", b, "", -1, 1);
        b[7] = ' ';
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
static void song_bpm_text(char *b)                      /* "126 BPM" (b: 12) */
{
    fmt_int(b, (uint32_t)song.g[G_BPM]);
    str_cpy(b + str_len(b), " BPM", 12 - str_len(b));
}
/* SELECT in SONG: BPM +-1 a detent (FX held: +-5), at once; BPM LOCK and an external clock refuse */
static void song_bpm(int32_t s)
{
    char b[12];
    song_bpm_text(b);
    if (song_ext_clock()) {
        song_refuse("EXT CLOCK", b);
        return;
    }
    if (ui_prefs & PREF_BPM_LOCK) {
        song_refuse("BPM LOCKED", b);
        return;
    }
    song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + s * (song_fx_btn() ? 5 : 1), GP[G_BPM].min, GP[G_BPM].max);
    ui.bpm_t = 40;
    song_ov(SOV_TEMPO);
}
static void glo_tap(void);
static void song_tap(void)                              /* F5: TAP tempo (GLO's: the last 4 taps, 2 s starts over) */
{
    char t[12];
    song_bpm_text(t);
    if (song_ext_clock()) {
        song_refuse("EXT CLOCK", t);
        return;
    }
    if (ui_prefs & PREF_BPM_LOCK) {                     /* (review of 081a8ff: as SELECT) */
        song_refuse("BPM LOCKED", t);
        return;
    }
    glo_tap();
    song_bpm_text(t);
    song_pop(SOV_TEMPO, "TAP", t, "", -1, 0);
    ui_message("TAP");
}
static void song_key_apply(void)                        /* the KEY / SCALE waiting: all four tracks, the part again */
{
    if (!sg.kpend)
        return;
    sg.kpend = 0;
    song_undo_take(SU_KEY, 0);
    sg.dirty = 0xFu;                                    /* (every part is in the new key now) */
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
    sg.kwhat = (uint8_t)song_fx_btn();              /* (the box: KEY or SCALE) */
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
/* 0.17 (spec v0.8 §2b): PRESETS in SONG browses every sound of the focus track's category: the factory presets in the
 * PRESETS list's order, the FORGE PACK (a pack sound's category: its base preset's; HOUSE ORGAN's base, WHEEL's
 * JAZZ PERC, is KEYS), the user presets; from the sound playing, round. A track on an OTHER sound: its role's (T1 BASS,
 * T2 LEAD, T3 PAD, T4 DRUM). T4 on a drum sound: the drum list, the DRUM KIT models (K1 only: the kit's other values
 * kept), the pack's kits, the other drum sounds, the user's. FX + PRESETS: the next / previous category (BASS LEAD PAD
 * PLUCK KEYS FX DRUM, the empty skipped; the drum list: its groups), on its first sound. Built as it is read: no table */
enum { SR_FAC, SR_PACK, SR_USER, SR_KIT };
typedef struct { uint8_t kind, e, k; } sref_t;
static const uint8_t SG_KITS[12] = {0, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};   /* (eng_drum.c DK_*: STD 80 10 66 55 77
                                                                                 * DEEP TIGHT PERC 90 MP SD) */
static const uint8_t SG_CATS[7] = {CAT_BASS, CAT_LEAD, CAT_PAD, CAT_PLUCK, CAT_KEYS, CAT_FX, CAT_DRUM};
static const char *const SG_DGRP[4] = {"KITS", "PACK KITS", "DRUMS", "USER"};
static uint32_t snd_nfac(void)
{
    uint32_t r, n = 0;
    for (r = 0; r < NENG_SHOWN; r++)
        n += preset_shown(eng_vis(r));
    return n;
}
static void snd_at(uint32_t n, uint32_t nf, sref_t *r)  /* entry n of all sounds: factory, pack, user */
{
    uint32_t k;
    if (n < nf) {
        r->kind = SR_FAC;
        r->e = (uint8_t)preset_all_at(n, &k);
        r->k = (uint8_t)k;
    } else if (n - nf < FS_NPACK) {
        r->kind = SR_PACK;
        r->e = 0;
        r->k = (uint8_t)(n - nf);
    } else {
        r->kind = SR_USER;
        r->e = 0;
        r->k = (uint8_t)up_nth(n - nf - FS_NPACK);
    }
}
static int snd_base(uint32_t j, uint32_t *e, uint32_t *k) { return !fs_find_base(FORGE_PACK[j].eng, FORGE_PACK[j].base, e, k); }
static uint32_t snd_cat(const sref_t *r)
{
    static uint8_t pc[FS_NPACK];                        /* (the pack's categories, found once: + 1) */
    uint32_t e, k;
    if (r->kind == SR_FAC)
        return preset_cat(r->e, r->k);
    if (r->kind == SR_PACK) {
        if (!pc[r->k % FS_NPACK])
            pc[r->k % FS_NPACK] = (uint8_t)(1u + (snd_base(r->k % FS_NPACK, &e, &k) ? preset_cat(e, k) : CAT_OTHER));
        return pc[r->k % FS_NPACK] - 1u;
    }
    return r->kind == SR_USER ? up_cat(r->k) : CAT_DRUM;
}
static int snd_eq(const sref_t *a, const sref_t *b) { return a->kind == b->kind && a->e == b->e && a->k == b->k; }
/* the list (drum, else category c): its length; entry n into *r and its group (the drum list's) into *g; w given:
 * its place into *wp (the length: not in it) */
static uint32_t sg_walk(uint32_t drum, uint32_t c, uint32_t n, sref_t *r, uint32_t *g, const sref_t *w, uint32_t *wp)
{
    uint32_t nf = snd_nfac(), tot = nf + FS_NPACK + up_count(), cnt = 0, pass, j, at = 0xFFFFu;
    sref_t x;
    if (drum)
        for (j = 0; j < NELEM(SG_KITS); j++, cnt++) {
            x = (sref_t){SR_KIT, 0, SG_KITS[j]};
            if (cnt == n) {
                *r = x;
                *g = 0;
            }
            if (w && at == 0xFFFFu && snd_eq(&x, w))
                at = cnt;
        }
    for (pass = drum ? 1u : 0u; pass < (drum ? 4u : 1u); pass++)
        for (j = 0; j < tot; j++) {
            snd_at(j, nf, &x);
            if (snd_cat(&x) != (drum ? CAT_DRUM : c) ||
                (pass == 1u && x.kind != SR_PACK) || (pass == 2u && (x.kind != SR_FAC || x.e == ENGI_DRUM)) ||
                (pass == 3u && x.kind != SR_USER))
                continue;
            if (cnt == n) {
                *r = x;
                *g = pass;
            }
            if (w && at == 0xFFFFu && snd_eq(&x, w))
                at = cnt;
            cnt++;
        }
    if (w)
        *wp = at < cnt ? at : cnt;
    return cnt;
}
static uint32_t sg_list(uint32_t drum, uint32_t c, uint32_t n, sref_t *r, uint32_t *g) { return sg_walk(drum, c, n, r, g, 0, 0); }
static uint32_t sg_list_pos(uint32_t drum, uint32_t c, const sref_t *w, uint32_t *n)   /* w's place (n: none) */
{
    uint32_t g, p;
    sref_t x;
    *n = sg_walk(drum, c, 0xFFFFu, &x, &g, w, &p);
    return p;
}
static void sg_cur(uint32_t i, uint32_t drum, sref_t *r)   /* the sound track i plays, as a list entry */
{
    const track_t *t = &trk[i % NTRK];
    uint32_t u = user_of(t), pk = song_pack_of(t);
    r->e = 0;
    if (u < UP_SLOTS) {
        r->kind = SR_USER;
        r->k = (uint8_t)u;
    } else if (pk) {
        r->kind = SR_PACK;
        r->k = (uint8_t)(pk - 1u);
    } else if (drum && drum_track(t)) {
        r->kind = SR_KIT;
        r->k = (uint8_t)t->p[P_E0];
    } else {
        r->kind = SR_FAC;
        r->e = t->eng_req % NENGINES;
        r->k = t->preset;
    }
}
static uint32_t song_tcat(uint32_t i)                   /* the category browsed: the sound's, or the role's */
{
    static const uint8_t ROLE[NTRK] = {CAT_BASS, CAT_LEAD, CAT_PAD, CAT_DRUM};
    uint32_t c = track_cat(&trk[i % NTRK]);
    return c && c != CAT_OTHER ? c : ROLE[i % NTRK];
}
static uint32_t song_tdrum(uint32_t i) { return i == NTRK - 1u && (drum_track(&trk[i]) || song_tcat(i) == CAT_DRUM); }
static void song_preset(int32_t s)
{
    uint32_t i = song_ftrack(), n, pos, g = 0, c;
    sref_t cur;
    if (sg.ppend != i + 1u) {                           /* (the first detent: from the sound playing) */
        sg.pdrum = (uint8_t)song_tdrum(i);
        sg.pcat = (uint8_t)song_tcat(i);
        sg_cur(i, sg.pdrum, &cur);
        sg.pkind = cur.kind;
        sg.pe = cur.e;
        sg.pk = cur.k;
    }
    cur = (sref_t){sg.pkind, sg.pe, sg.pk};
    pos = sg_list_pos(sg.pdrum, sg.pcat, &cur, &n);
    sg.pjump = 0;
    if (song_fx_btn()) {                                /* FX: the next / previous category (the drum list: group) */
        uint32_t x, len = 0, first = 0, gg;
        sref_t r;
        sg.pjump = 1;
        if (sg.pdrum) {
            uint32_t gc = 0;
            if (pos < n)
                sg_list(1, 0, pos, &r, &gc);
            for (x = 1; x <= 4u && !len; x++) {         /* (the next group with something in it) */
                gg = (gc + (s > 0 ? x : 4u - x)) % 4u;
                for (first = 0; first < n; first++) {
                    sg_list(1, 0, first, &r, &g);
                    if (g == gg)
                        break;
                }
                for (len = 0; first + len < n; len++) {
                    sg_list(1, 0, first + len, &r, &g);
                    if (g != gg)
                        break;
                }
            }
            sg_list(1, 0, first, &r, &g);
        } else {
            for (c = 0; c < NELEM(SG_CATS) && SG_CATS[c] != sg.pcat; c++)
                ;
            for (x = 1; x <= NELEM(SG_CATS) && !len; x++) {
                gg = SG_CATS[(c + (s > 0 ? x : NELEM(SG_CATS) - x)) % NELEM(SG_CATS)];
                len = sg_list(0, gg, 0, &r, &g);
                if (len)
                    sg.pcat = (uint8_t)gg;
            }
            if (sg.pcat == CAT_DRUM && i == NTRK - 1u)
                sg.pdrum = 1;                           /* (T4 to DRUM: the drum list) */
            sg_list(sg.pdrum, sg.pcat, 0, &r, &g);
        }
        sg.pkind = r.kind;
        sg.pe = r.e;
        sg.pk = r.k;
    } else if (n) {
        sref_t r;
        pos = pos < n ? (pos + n + (s > 0 ? 1u : n - 1u)) % n : s > 0 ? 0u : n - 1u;
        sg_list(sg.pdrum, sg.pcat, pos, &r, &g);
        sg.pkind = r.kind;
        sg.pe = r.e;
        sg.pk = r.k;
    }
    sg.ppend = (uint8_t)(i + 1u);
    sg.pms = fm1_ms;
    sg_cur(i, sg.pdrum, &cur);
    if (!sg.pjump && sg.pkind == cur.kind && sg.pe == cur.e && sg.pk == cur.k)
        sg.ppend = 0;                                   /* (back where it was: nothing waits) */
    song_ov(SOV_PRESET);
}
/* the popup's lines for the sound browsed (or the track's) */
static void song_pre_text(char *sm, char *big)
{
    uint32_t i = sg.ppend ? sg.ppend - 1u : song_ftrack(), n, pos, g = 0;
    sref_t r;
    char a[6];
    if (!sg.ppend) {
        sg.pdrum = (uint8_t)song_tdrum(i);
        sg.pcat = (uint8_t)song_tcat(i);
        sg_cur(i, sg.pdrum, &r);
        sg.pkind = r.kind;
        sg.pe = r.e;
        sg.pk = r.k;
    }
    r = (sref_t){sg.pkind, sg.pe, sg.pk};
    pos = sg_list_pos(sg.pdrum, sg.pcat, &r, &n);
    str_cpy(sm, "T1 ", 20);
    sm[1] = (char)('1' + i);
    if (sg.pdrum) {
        uint32_t first = pos, len = 0;
        if (pos < n)
            sg_list(1, 0, pos, &(sref_t){0, 0, 0}, &g);
        if (sg.pjump) {                                 /* "T4 DRUM" "PACK KITS 1/8" */
            uint32_t gg;
            sref_t x;
            for (first = 0; first < n; first++) {
                sg_list(1, 0, first, &x, &gg);
                if (gg == g)
                    break;
            }
            for (len = 0; first + len < n; len++) {
                sg_list(1, 0, first + len, &x, &gg);
                if (gg != g)
                    break;
            }
            str_cpy(sm + 3, "DRUM", 17);
            str_cpy(big, SG_DGRP[g & 3u], 24);
            str_cpy(big + str_len(big), " ", 24 - str_len(big));
            fmt_int(a, pos - first + 1u);
            str_cpy(big + str_len(big), a, 24 - str_len(big));
            str_cpy(big + str_len(big), "/", 24 - str_len(big));
            fmt_int(a, len);
            str_cpy(big + str_len(big), a, 24 - str_len(big));
            return;
        }
        str_cpy(sm + 3, r.kind == SR_KIT ? "KIT" : r.kind == SR_USER ? "USER" : "PRESET", 17);
    } else {
        str_cpy(sm + 3, CAT_NAME[sg.pcat % CAT_N], 17);
        if (sg.pjump)
            str_cpy(sm + 3, "CATEGORY", 17);
        str_cpy(a, " ", sizeof a);
        if (sg.pjump) {                                 /* "T2 CATEGORY" "PAD 1/20" */
            str_cpy(big, CAT_NAME[sg.pcat % CAT_N], 24);
            str_cpy(big + str_len(big), " ", 24 - str_len(big));
        } else
            big[0] = 0;
        {
            char *d = sg.pjump ? big : sm;
            uint32_t m = sg.pjump ? 24u : 20u;
            if (!sg.pjump)
                str_cpy(d + str_len(d), " ", m - str_len(d));
            fmt_int(a, pos < n ? pos + 1u : 0u);
            str_cpy(d + str_len(d), a, m - str_len(d));
            str_cpy(d + str_len(d), "/", m - str_len(d));
            fmt_int(a, n);
            str_cpy(d + str_len(d), a, m - str_len(d));
        }
        if (sg.pjump)
            return;
    }
    big[0] = 0;
    if (r.kind == SR_KIT)
        str_cpy(big, N_DRUM_KIT[r.k % DK_COUNT], 24);
    else if (r.kind == SR_PACK) {
        if (!sg.pdrum)
            str_cpy(big, "PACK ", 24);
        str_cpy(big + str_len(big), FORGE_PACK[r.k % FS_NPACK].name, 24 - str_len(big));
    } else if (r.kind == SR_USER) {
        char nm[13];
        if (!sg.pdrum)
            str_cpy(big, "USER ", 24);
        up_name(r.k, nm);
        str_cpy(big + str_len(big), nm, 24 - str_len(big));
    } else {
        if (!sg.pdrum) {
            str_cpy(big, ENGINES[r.e % NENGINES]->name, 24);
            str_cpy(big + str_len(big), " ", 24 - str_len(big));
        }
        {
            char nm[24];
            song_sound_name(r.e, r.k, 0, nm);
            str_cpy(big + str_len(big), nm, 24 - str_len(big));
        }
    }
}
static void song_load(uint32_t i, uint32_t e, uint32_t k)   /* track i: engine e's preset k */
{
    track_t *t = &trk[i % NTRK];
    song_undo_take(SU_SOUND, i % NTRK);
    sg.dirty = 0xFu;                                    /* (a part's slot keeps its sounds) */
    if (t->eng_req != e)
        set_engine_of(t, e);
    apply_preset_to(t, k);
    fs_packid[i % NTRK] = 0;
    ui.force = 1;
}
static void song_load_user(uint32_t i, uint32_t u)      /* a user preset (up_load: into the selected track) */
{
    uint8_t sel = song.sel;
    song_undo_take(SU_SOUND, i % NTRK);
    sg.dirty = 0xFu;
    song.sel = (uint8_t)(i % NTRK);
    (void)up_load(u);
    song.sel = sel;
    fs_packid[i % NTRK] = 0;
    ui.force = 1;
}
static void song_load_ref(uint32_t i, uint32_t kind, uint32_t e, uint32_t k)   /* 0.17: a browsed sound into track i */
{
    track_t *t = &trk[i % NTRK];
    uint32_t be, bk;
    if (kind == SR_USER) {
        song_load_user(i, k);
    } else if (kind == SR_PACK) {
        if (snd_base(k % FS_NPACK, &be, &bk)) {
            song_load(i, be, bk);
            fs_pack_apply(t, &FORGE_PACK[k % FS_NPACK]);
            fs_packid[i % NTRK] = (uint8_t)(k % FS_NPACK + 1u);
        }
    } else if (kind == SR_KIT) {                        /* (a DRUM KIT model: K1 only; another sound: DRUM KIT first) */
        if (drum_track(t) && !t->user && !song_pack_of(t)) {
            song_undo_take(SU_SOUND, i % NTRK);
            sg.dirty = 0xFu;
            ui.force = 1;
        } else
            song_load(i, ENGI_DRUM, 0);
        t->p[P_E0] = (int16_t)k;
    } else {
        song_load(i, e, k);
    }
}
/* 0.15 §H10: four favourites, D4 + black 1..4 (FX: store), the focus track's sound, in the settings: engine + 1 and
 * preset (0 empty), or SG_FAV_USER and a user preset's slot (the sound the track plays is a user preset's), in
 * favorites.factory[0][24..31], ANALOG's favourite bits of presets 192..255, which no engine has (tests/forge_test.c
 * checks). (The spec's [15][13..20] is taken: [15][14..23] hold the FX layer's key map) */
#define sg_fav (&favorites.factory[0][24])
#define SG_FAV_USER 0x80u
/* 0.17: a FORGE PACK sound (SG_FAV_PACK, k its index); a DRUM one keeps its KIT model (K1) too: k = preset | (K1 + 1)
 * << 2 (DRUM has 4 presets; 0.16's favourites, k < 4, load as before) */
#define SG_FAV_PACK 0x40u
static void song_fav_load(uint32_t i, uint32_t x)       /* favourite x into track i */
{
    uint32_t e = sg_fav[2u * (x & 3u)], k = sg_fav[2u * (x & 3u) + 1u];
    if (e == SG_FAV_USER) {                             /* (up_load loads into the selected track: i, for the call) */
        song_load_user(i, k);
    } else if (e == SG_FAV_PACK) {
        song_load_ref(i, SR_PACK, 0, k);
    } else if (e - 1u == ENGI_DRUM && k >= 4u) {
        song_load(i, ENGI_DRUM, k & 3u);
        trk[i % NTRK].p[P_E0] = (int16_t)((k >> 2) - 1u);
    } else {
        song_load(i, e - 1u, k);
    }
}
static void song_fav_name(uint32_t e, uint32_t k, char *b)   /* (b: 24) */
{
    if (e == SG_FAV_PACK)
        str_cpy(b, FORGE_PACK[k % FS_NPACK].name, 24);
    else if (e - 1u == ENGI_DRUM && k >= 4u) {
        str_cpy(b, "KIT ", 24);
        str_cpy(b + 4, N_DRUM_KIT[((k >> 2) - 1u) % DK_COUNT], 20);
    } else
        song_sound_name(e == SG_FAV_USER ? 0u : e - 1u, k, e == SG_FAV_USER, b);
}
static void song_fav(uint32_t x, uint32_t store)
{
    uint32_t i = song_ftrack(), e = sg_fav[2u * (x & 3u)], dr, u;
    char b[28] = "FAV 1 ";
    b[4] = (char)('1' + (x & 3u));
    if (store) {
        uint32_t pk = song_pack_of(&trk[i]);
        u = user_of(&trk[i]);
        sg_fav[2u * (x & 3u)] = (uint8_t)(u < UP_SLOTS ? SG_FAV_USER : pk ? SG_FAV_PACK : trk[i].eng_req + 1u);
        sg_fav[2u * (x & 3u) + 1u] = (uint8_t)(u < UP_SLOTS ? u : pk ? pk - 1u : drum_track(&trk[i]) ?
                                               (trk[i].preset & 3u) | (uint32_t)(trk[i].p[P_E0] + 1) << 2 : trk[i].preset);
        settings_later();
        str_cpy(b + 6, u < UP_SLOTS ? "STORED (USER)" : "STORED", sizeof b - 6u);
        ui_message(b);
        {
            char sm[16] = "FAV 1 SAVED", nm[24];
            sm[4] = (char)('1' + (x & 3u));
            song_fav_name(sg_fav[2u * (x & 3u)], sg_fav[2u * (x & 3u) + 1u], nm);
            song_pop(SOV_POP, sm, nm, "", -1, 0);
        }
        return;
    }
    if (e == SG_FAV_USER && !up_used(sg_fav[2u * (x & 3u) + 1u])) {
        str_cpy(b + 5, ": USER SOUND GONE", sizeof b - 5u);   /* (its slot erased or overwritten empty since) */
        b[5] = 0;
        song_refuse(b, "SOUND GONE");
        return;
    }
    if (!e || (e > NENGINES && e != SG_FAV_USER && e != SG_FAV_PACK) || (e == SG_FAV_PACK && sg_fav[2u * (x & 3u) + 1u] >= FS_NPACK)) {
        str_cpy(b + 6, "EMPTY", sizeof b - 6u);
        song_refuse(b, "EMPTY");
        return;
    }
    dr = e == SG_FAV_PACK ? str_eq(FORGE_PACK[sg_fav[2u * (x & 3u) + 1u]].eng, "DRUM") :
         ENGINES[e == SG_FAV_USER ? up_engine(sg_fav[2u * (x & 3u) + 1u]) : e - 1u] == &ENG_DRUM;
    if (dr != (i == FP_DRUM && drum_track(&trk[i]))) {   /* (a kit on T4 only, and only a kit there) */
        b[5] = ':';
        str_cpy(b + 6, dr ? " DRUM ONLY T4" : " NOT ON DRUM", sizeof b - 6u);
        ui_message(b);
        b[5] = 0;
        song_pop(SOV_POP, b, dr ? "DRUM ONLY T4" : "NOT ON DRUM", "", -1, 1);
        return;
    }
    sg.fpend = (uint8_t)((x & 3u) + 1u);
    sg.flast = (uint8_t)(x & 3u);
    sg.ftrk = (uint8_t)i;
    if (!song.playing) {
        song_fav_load(i, x);
        sg.fpend = 0;
    }
    song_ov(SOV_FAV);
}
/* the box of an overlay kind as it stands (spec v0.7 §A's table) */
static void song_sound_name(uint32_t e, uint32_t k, uint32_t user, char *b)   /* a sound's name (b: 24) */
{
    if (user) {
        up_name(k, b);
        return;
    }
    str_cpy(b, ENGINES[e % NENGINES]->npresets ? ENGINES[e % NENGINES]->presets[k % ENGINES[e % NENGINES]->npresets].name :
               "-", 24);
}
static void song_ov(uint32_t kind)
{
    char sm[20], big[24], note[20] = "";
    sm[0] = big[0] = 0;
    switch (kind) {
    case SOV_TEMPO:
        str_cpy(sm, "TEMPO", sizeof sm);
        song_bpm_text(big);
        break;
    case SOV_KEY:
        str_cpy(sm, sg.kwhat ? "SCALE" : "KEY", sizeof sm);
        str_cpy(big, sg.kwhat ? N_SCALE[song_scale_shown() % NELEM(N_SCALE)] : SG_NOTE[song_root_shown() % 12u],
                sizeof big);
        if (sg.kpend)
            str_cpy(note, "@ NEXT BAR", sizeof note);
        break;
    case SOV_PRESET:
        song_pre_text(sm, big);
        if (sg.ppend && song.playing)
            str_cpy(note, "@ NEXT BAR", sizeof note);
        break;
    case SOV_SCENE: case SOV_SCENE2: {
        uint32_t i;
        str_cpy(sm, kind == SOV_SCENE ? "SCENE 1" : "SCENE 2", sizeof sm);
        str_cpy(big, sg.ov_skip ? "" : "RECALL", sizeof big);
        for (i = 0; i < NTRK; i++)
            if ((sg.ov_skip >> i) & 1u) {
                char n[4] = {'T', (char)('1' + i), ' ', 0};
                str_cpy(big + str_len(big), n, sizeof big - str_len(big));
            }
        if (sg.ov_skip)
            str_cpy(big + str_len(big), "SKIP", sizeof big - str_len(big));
        if (song.playing && !sg.ov_skip)
            str_cpy(note, "@ NEXT 1/16", sizeof note);
        break;
    }
    case SOV_FAV: {
        uint32_t x = sg.fpend ? sg.fpend - 1u : sg.flast & 3u, e = sg_fav[2u * x];
        str_cpy(sm, "FAV 1", sizeof sm);
        sm[4] = (char)('1' + x);
        song_fav_name(e, sg_fav[2u * x + 1u], big);
        if (sg.fpend && song.playing)
            str_cpy(note, "@ NEXT BAR", sizeof note);
        break;
    }
    default:
        return;
    }
    song_pop(kind, sm, big, note, -1, 0);
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
        if (bar && sg.barhit)
            bar = 0;                            /* (0.17: once a bar line; 0.16 counted each pass of the late half) */
        else if (bar)
            sg.barhit = 1;
        if (clk_step != 15u && !bar)
            sg.barhit = 0;
    }
    song_hold_poll();
    song_fill_poll(bar);
    if (bar && sg.chain && !sg.qpart &&
        ++sg.cbar >= SG_CBARS[sg.cbars[sg.part] % NELEM(SG_CBARS)])   /* (0.16 auto-chain: the next part at the bar) */
        sg.qpart = (uint8_t)((sg.part + 1u) % SG_N + 1u);
    if (bar)
        song_fill_bar();                                /* (0.17: a bar-end fill's bar begins) */
    if (sg.qpart && (bar || !song.playing) && !sgf.bar) {   /* (0.16: a part at the bar; 0.17: after a fill's bar) */
        uint32_t x = sg.qpart - 1u;
        sg.qpart = 0;
        sg.cbar = 0;
        song_part(x);
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
        song_load_ref(i, sg.pkind, sg.pe, sg.pk);
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
        if (song_d4()) {
            sg.d4_used = 1;
            song_chain_bars(algo);                      /* (0.16: D4 + ALGORITHM, the part's bars in the chain) */
        } else
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
 * D4 ALT (held alone: KNOB 1..4 T1..T4 LEVEL); D5 E5 the scenes; F5 TAP (0.15, spec v0.6) */
static int song_wkey(uint32_t k)
{
    uint32_t p = key_place(k);
    if (p >= SG_FX_KEY0 && p < SG_FX_KEY0 + 6u) {
        song_pop(SOV_POP, "FX", song_fx_name(SG_FX[p - SG_FX_KEY0]), "", -1, 0);   /* (0.16: the ISR's, while held) */
        return 1;
    }
    if (p < NTRK) {
        if (sg.d4)
            sg.d4_used = 1;
        if (kb_fx[k] || song_fx_btn()) {        /* (the mute: the ISR's, while held) */
            char t[3] = {'T', (char)('1' + p), 0};
            if (p == NTRK - 1u && (sg.farm || sgf.live || sgf.bar)) {
                song_fill_stop();                       /* (0.17: a FILL armed or playing: dropped) */
                song_fill_refuse("T4 MUTED");
            } else if (!(sg.ov == SOV_POP && sg_pop.red && fm1_ms - sg.ov_ms < 100u))
                song_pop(SOV_POP, "MUTE", t, "", -1, 0);   /* (a red box this pass stays: T4 MUTED) */
        } else if (song_d4())
            song_track(p);                      /* (0.15, spec v0.6 §H7: D4 held, the reroll) */
        else
            song_focus_pick(p);                 /* (.. alone: FOCUS, the same key again: off) */
        return 1;
    }
    if (p == TL_RANDOM && song_d4()) {          /* 0.17 (spec v0.8 §2): D4 + C4 FILL (no reroll) */
        song_fill_down(k);
        return 1;
    }
    if (p == TL_COOK) {                         /* D4: ALT (held alone: the levels; a tap: nothing, 0.15) */
        sg.d4 = 1;
        sg.d4_used = (kb_layer & 0x55u) != 0u;
        return 1;
    }
    if (p == TL_FORGE) {                        /* 0.15 F5: TAP tempo; 0.16: held, the visual CLICK */
        sg.hbpm = song.g[G_BPM];
        song_tap();
        song_hold_start(k, SH_TAP, 0);
        return 1;
    }
    if (p == SG_FX_KEY0 + 6u || p == SG_FX_KEY0 + 7u) {   /* D5 / E5: the scenes (0.16: a tap recalls, held stores; */
        if (song_fx_btn())                                 /* FX held: stored at once) */
            song_scene(p - SG_FX_KEY0 - 6u, 1);
        else
            song_hold_start(k, SH_SCENE, p - SG_FX_KEY0 - 6u);
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
    sg.dirty = 0;
    song_remember();
    ui_message("SONG SAVED: A-D");
    song_pop(SOV_POP, "SAVE", "A-D SAVED", "", -1, 0);
    str_cpy(ui.msg2, "GLO > SONG PLAYS IT", sizeof ui.msg2);
    return 0;
}
/* OCT+ (up 1) / OCT- (0) in SONG; returns 1 when SONG (and STARTER) closed */
static int song_oct_do(uint32_t up)
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
        sg.dirty &= (uint8_t)~(1u << x);
        song_remember();
        {
            char m[12] = {(char)('A' + x), ' ', 'S', 'A', 'V', 'E', 'D', 0};
            ui_message(m);
            song_pop(SOV_POP, "SAVE", m, "", -1, 0);
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
/* OCT+ at once; OCT- (0.16, spec v0.7 §3): a second tap within SG_OCT2_MS is UNDO, else the single tap's action (cancel
 * a change waiting, KEEP, cancel, done) SG_OCT2_MS later (song_poll); returns 1 when SONG (and STARTER) closed now */
static int song_oct(uint32_t up)
{
    if (up || sg.stage == SG_CONFIRM) {                 /* (the confirm: no UNDO, answered at once) */
        sg.octpend = 0;
        return song_oct_do(up);
    }
    if (sg.octpend && fm1_ms - sg.octms < SG_OCT2_MS) {
        sg.octpend = 0;
        song_undo();
        return 0;
    }
    sg.octpend = 1;
    sg.octms = fm1_ms;
    return 0;
}

/* 0.17 (spec v0.8 §2c): SONG's sounds stay. A song's default sounds come back only when the song changes (SEED,
 * genre, variation); leaving SONG and coming back (SEQ held, F#5, BOOT), a part, a reroll keep what was chosen. The
 * four tracks' sounds (engine, preset, user preset, FM6 patch and slot, every value) are kept here when SONG is left,
 * with the song they belong to (in the pool, as STARTER's fs_from: not .bss), and put back when the same song opens
 * again. Across power-off: the ids (engine, preset, user slot + 1, FORGE PACK sound + 1) in the settings beside the
 * last song (favorites.factory[5][8..23]: engines 5..7's favourite bits of presets 64..191, which no engine has); the
 * values then are the preset's (a DRUM sound: its KIT model too, as a favourite's; an unsaved song's other knob edits
 * are lost at power-off: its orange dot says so) */
static struct {
    uint8_t valid, seed, genre, var;
    uint8_t eng[NTRK], preset[NTRK], user[NTRK], fm6_slot[NTRK], pack[NTRK];
    int16_t p[NTRK][P_COUNT];
    uint8_t fm6[NTRK][FP_SIZE + 1u];
} sg_snd __attribute__((section(".pool")));
#define sg_ids (&favorites.factory[5][8])                /* track i: [4i] engine, preset, user + 1, pack + 1 */
static uint32_t song_pack_of(const track_t *t)          /* the FORGE PACK sound t plays (+ 1; 0 none): fs_sounds' */
{
    uint32_t i, e, k;
    if (t->user || fs_packid[trk_index(t)] == 0u || fs_packid[trk_index(t)] > FS_NPACK)
        return 0;
    i = fs_packid[trk_index(t)] - 1u;
    return !fs_find_base(FORGE_PACK[i].eng, FORGE_PACK[i].base, &e, &k) && e == t->eng_req % NENGINES && k == t->preset
           ? i + 1u : 0u;
}
static void song_snd_take(void)                         /* SONG left: its sounds and their song */
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        const track_t *t = &trk[i];
        sg_snd.eng[i] = t->eng_req;
        sg_snd.preset[i] = t->preset;
        sg_snd.user[i] = t->user;
        sg_snd.fm6_slot[i] = fm6_slot[i];
        memcpy(sg_snd.p[i], t->p, sizeof sg_snd.p[i]);
        memcpy(sg_snd.fm6[i], fm6_patch[i], sizeof sg_snd.fm6[i]);
        sg_ids[4u * i] = t->eng_req;
        sg_ids[4u * i + 1u] = (uint8_t)(drum_track(t) ? (t->preset & 3u) | (uint32_t)(t->p[P_E0] + 1) << 2 : t->preset);
        sg_ids[4u * i + 2u] = t->user;
        sg_ids[4u * i + 3u] = sg_snd.pack[i] = (uint8_t)song_pack_of(t);
    }
    sg_snd.seed = fg.seed;
    sg_snd.genre = fs.genre;
    sg_snd.var = fs.var;
    sg_snd.valid = 1;
}
static int song_snd_same(void) { return sg_snd.valid && sg_snd.seed == fg.seed && sg_snd.genre == fs.genre && sg_snd.var == fs.var; }
/* the same song opened again: its sounds back (the values too: as fs_close puts STARTER's back, the ISR off) */
static void song_snd_put(void)
{
    uint32_t i;
    if (!song_snd_same())
        return;
    fm1_irq_off();
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        int16_t keep[4] = {t->p[P_SLEN], t->p[P_SDIV], t->p[P_ROOT], t->p[P_SCALE]};   /* (the part's, as rendered) */
        panic_req |= (uint8_t)(1u << i);
        t->eng_req = sg_snd.eng[i];
        t->preset = sg_snd.preset[i];
        t->user = sg_snd.user[i];
        fs_packid[i] = sg_snd.pack[i];
        memcpy(t->p, sg_snd.p[i], sizeof t->p);
        t->p[P_SLEN] = keep[0];
        t->p[P_SDIV] = keep[1];
        t->p[P_ROOT] = keep[2];
        t->p[P_SCALE] = keep[3];
    }
    fm1_irq_on();
    for (i = 0; i < NTRK; i++) {
        if (memcmp(fm6_patch[i], sg_snd.fm6[i], FP_SIZE)) {
            fm6_set_patch(i, sg_snd.fm6[i]);
            fm6_own_ok &= (uint8_t)~(1u << i);
        }
        fm6_slot[i] = sg_snd.fm6_slot[i];
    }
    sync_reload = 1;
}
/* after a power-off: the ids kept with the last song (song_restore, when it was the same song) */
static void song_snd_ids(void)
{
    uint32_t i;
    for (i = 0; i < NTRK; i++) {
        track_t *t = &trk[i];
        uint32_t e = sg_ids[4u * i] % NENGINES, k = sg_ids[4u * i + 1u], u = sg_ids[4u * i + 2u], pk = sg_ids[4u * i + 3u];
        if (u && u <= UP_SLOTS && up_used(u - 1u)) {
            uint8_t sel = song.sel;
            song.sel = (uint8_t)i;
            (void)up_load(u - 1u);
            song.sel = sel;
            continue;
        }
        if (!eng_ok(e) || (e == ENGI_DRUM ? k & 3u : k) >= ENGINES[e]->npresets)
            continue;
        if (t->eng_req % NENGINES != e)
            set_engine_of(t, e);
        apply_preset_to(t, e == ENGI_DRUM ? k & 3u : k);
        if (e == ENGI_DRUM && k >= 4u)
            t->p[P_E0] = (int16_t)((k >> 2) - 1u);      /* (its KIT model, as the favourites keep it) */
        if (pk && pk <= FS_NPACK)
            fs_pack_apply(t, &FORGE_PACK[pk - 1u]);
        fs_packid[i] = (uint8_t)pk;
    }
}
/* 0.16 (spec v0.7 §1): the last song, kept with the settings (written when SONG writes its slots and when it is left):
 * 16 bytes in favorites.factory[2][8..23] and the rerolls in [3][8..23] (nvar) and [4][8..23] (rvar): engines 2..4's
 * favourite bits of presets 64..191, which no engine has (tests/forge_test.c checks) */
#define SG_LAST_MAGIC 0xA5u
#define sg_last (&favorites.factory[2][8])
#define sg_last_nvar (&favorites.factory[3][8])
#define sg_last_rvar (&favorites.factory[4][8])
enum { SL_MAGIC, SL_SEED, SL_GENRE, SL_VAR, SL_ROOT, SL_SCALE, SL_BPM0, SL_BPM1, SL_LOCKPART, SL_DENS, SL_EVOL, SL_CB0,
       SL_CB1, SL_FLAGS, SL_DIRTY, SL_OUTKEY, SL_N };   /* (SL_OUTKEY 0.17: the selected track's ROOT | SCALE << 4 as
                                                          * SONG was left; SL_FLAGS bit 3: it is set) */
typedef char sg_last_fits[SL_N <= 16 ? 1 : -1];
static uint8_t song_on(void) { return sg.on; }
static void song_remember(void)
{
    uint8_t *h = sg_last;
    uint32_t x, i;
    if (sg.on && fs.on)
        song_snd_take();                                /* (0.17: the sounds with it) */
    h[SL_MAGIC] = SG_LAST_MAGIC;
    h[SL_SEED] = fg.seed;
    h[SL_GENRE] = fs.genre;
    h[SL_VAR] = fs.var;
    h[SL_ROOT] = fs.root;
    h[SL_SCALE] = fs.scale;
    h[SL_BPM0] = (uint8_t)song.g[G_BPM];
    h[SL_BPM1] = (uint8_t)((uint16_t)song.g[G_BPM] >> 8);
    h[SL_LOCKPART] = (uint8_t)((sg.lock & 0xFu) | (sg.part & 3u) << 4);
    h[SL_DENS] = fs.dens;
    h[SL_EVOL] = fg.evolve;
    h[SL_CB0] = (uint8_t)(sg.cbars[0] | sg.cbars[1] << 4);
    h[SL_CB1] = (uint8_t)(sg.cbars[2] | sg.cbars[3] << 4);
    h[SL_FLAGS] = (uint8_t)((sg.stage == SG_WRITTEN ? 1u : 0u) | (sg.chain ? 2u : 0u) |
                            (song_snd_same() ? 4u : 0u) | 8u);   /* (0.17: sg_ids are this song's; SL_OUTKEY set) */
    h[SL_OUTKEY] = (uint8_t)((TSEL->p[P_ROOT] % 12) | (TSEL->p[P_SCALE] & 15) << 4);
    h[SL_DIRTY] = sg.dirty;
    for (x = 0; x < SG_N; x++)
        for (i = 0; i < NTRK; i++) {
            sg_last_nvar[x * NTRK + i] = sg.nvar[x][i];
            sg_last_rvar[x * NTRK + i] = sg.rvar[x][i];
        }
    settings_later();
}
/* the last song back into STARTER (open) and SONG (on), as it was left; none kept: the song STARTER has. 1 = restored */
static int song_restore(void)
{
    const uint8_t *h = sg_last;
    uint32_t x, i, ok = h[SL_MAGIC] == SG_LAST_MAGIC;
    if (ok) {
        fg.set = fs.set = 1;
        fg.seed = (uint8_t)(h[SL_SEED] % 100u);
        fs.genre = (uint8_t)(h[SL_GENRE] % FS_NGENRE);
        fs.var = (uint8_t)(h[SL_VAR] < FS_GENRE[fs.genre].n ? h[SL_VAR] : 0u);
        fs.dens = (uint8_t)clamp(h[SL_DENS], 50, 150);
        fg.evolve = (uint8_t)(h[SL_EVOL] > 4u ? 4u : h[SL_EVOL]);
    }
    fs_open();                                          /* (fs.root, fs.scale: the selected track's key now) */
    if (ok && (!(h[SL_FLAGS] & 8u) ||                   /* (0.17: the key set outside SONG since it was left wins, */
               h[SL_OUTKEY] == (uint8_t)((TSEL->p[P_ROOT] % 12) | (TSEL->p[P_SCALE] & 15) << 4))) {   /* SCL or */
        fs.root = (uint8_t)(h[SL_ROOT] % 12u);          /* STARTER ROOT; unchanged: the song's own) */
        fs.scale = (uint8_t)(h[SL_SCALE] % NELEM(N_SCALE));
    }
    if (ok) {
        if (!song_ext_clock())
            song.g[G_BPM] = (int16_t)clamp((int32_t)(h[SL_BPM0] | h[SL_BPM1] << 8), GP[G_BPM].min, GP[G_BPM].max);
    }
    song_toggle();                                      /* (SONG on: part A, the rerolls 0) */
    if (ok) {
        for (x = 0; x < SG_N; x++)
            for (i = 0; i < NTRK; i++) {
                sg.nvar[x][i] = (uint8_t)(sg_last_nvar[x * NTRK + i] % 100u);
                sg.rvar[x][i] = (uint8_t)(sg_last_rvar[x * NTRK + i] % 100u);
            }
        sg.lock = (uint8_t)(h[SL_LOCKPART] & 0xFu);
        sg.part = (uint8_t)((h[SL_LOCKPART] >> 4) & 3u);
        sg.cbars[0] = (uint8_t)(h[SL_CB0] & 7u);
        sg.cbars[1] = (uint8_t)((h[SL_CB0] >> 4) & 7u);
        sg.cbars[2] = (uint8_t)(h[SL_CB1] & 7u);
        sg.cbars[3] = (uint8_t)((h[SL_CB1] >> 4) & 7u);
        for (x = 0; x < SG_N; x++)
            if (sg.cbars[x] >= NELEM(SG_CBARS))
                sg.cbars[x] = SG_CBAR_DEF;
        sg.chain = (uint8_t)((h[SL_FLAGS] >> 1) & 1u);
        sg.stage = (h[SL_FLAGS] & 1u) ? SG_WRITTEN : SG_EDIT;
        sg.dirty = (uint8_t)(h[SL_DIRTY] & 0xFu);
        if (song_snd_same())
            song_snd_put();                             /* (0.17: its sounds as left) */
        else if (h[SL_FLAGS] & 4u)
            song_snd_ids();                             /* (.. after a power-off: the ids) */
        song_render(sg.part);
    }
    {
        char b[SG_NAME_LEN + 1u];
        song_name(sg.part, b);
        char k[16] = "KEY ";
        song_key_name(k + 4, sizeof k - 4u);
        song_pop(SOV_POP, ok ? "LAST SONG" : "SONG", b, k, -1, 0);   /* (0.17: the key it opens in, ALGORITHM moves it) */
    }
    return (int)ok;
}

/* a part asked for (OP1..4 let go before SG_HOLD_MS): playing, at the bar; stopped, at once */
static void song_part_tap(uint32_t x)
{
    char b[2] = {(char)('A' + (x & 3u)), 0};
    sg.cbar = 0;                                        /* (the chain goes on from it) */
    if (song.playing) {
        sg.qpart = (uint8_t)((x & 3u) + 1u);
        if (sg.farm && sg.fwait == 1u) {                /* (0.17 review: a fill armed for the next bar: this bar's */
            sg.farm = 0;                                /* rest instead, the part on time) */
            song_fill_set(++sg.fbars);
            sgf.bar = 1;
        }
        song_pop(SOV_POP, "PART", b, "@ NEXT BAR", -1, 0);
        return;
    }
    sg.qpart = 0;
    song_part(x);
    song_pop(SOV_POP, "PART", b, "", -1, 0);
}
static void song_chain_toggle(void)                     /* D4 + F#5: auto-chain on / off */
{
    char b[16] = "ON ";
    sg.chain = (uint8_t)!sg.chain;
    sg.cbar = 0;
    fmt_int(b + 3, SG_CBARS[sg.cbars[sg.part] % NELEM(SG_CBARS)]);
    str_cpy(b + str_len(b), " BARS", sizeof b - str_len(b));
    song_pop(SOV_POP, "CHAIN", sg.chain ? b : "OFF", "", -1, 0);
    ui_message(sg.chain ? "CHAIN ON" : "CHAIN OFF");
}
static void song_chain_bars(int32_t s)                  /* D4 + ALGORITHM: the part shown's bars 1 2 4 8 16 */
{
    char b[16] = "A: ";
    uint32_t i = sg.cbars[sg.part] % NELEM(SG_CBARS);
    i = (uint32_t)clamp((int32_t)i + (s > 0 ? 1 : -1), 0, (int32_t)NELEM(SG_CBARS) - 1);
    sg.cbars[sg.part] = (uint8_t)i;
    b[0] = (char)('A' + sg.part);
    fmt_int(b + 3, SG_CBARS[i]);
    str_cpy(b + str_len(b), SG_CBARS[i] == 1u ? " BAR" : " BARS", sizeof b - str_len(b));
    song_pop(SOV_POP, "CHAIN", b, "", -1, 0);
}
static void song_click_toggle(void)                     /* F5 held: the visual metronome on / off */
{
    sg.click = (uint8_t)!sg.click;
    song_pop(SOV_POP, "CLICK", sg.click ? "ON" : "OFF", "", -1, 0);
}
/* a key with a hold: its down (song_wkey, layer_key); song_hold_poll decides tap or hold */
static void song_hold_end(uint32_t held);
static void song_hold_start(uint32_t k, uint32_t kind, uint32_t p)
{
    if (sg.hk)
        song_hold_end(0);                               /* (another key meanwhile: the first one was a tap) */
    sg.hk = (uint8_t)(k + 1u);
    sg.hkind = (uint8_t)kind;
    sg.hp = (uint8_t)p;
    sg.hms = fm1_ms;
    sg.hfired = 0;
}
static void glo_tap_drop(void);
static void song_hold_end(uint32_t held)                /* held: SG_HOLD_MS passed, the key down; else let go */
{
    uint32_t kind = sg.hkind, p = sg.hp;
    if (held) {
        sg.hfired = 1;
        if (kind == SH_PART)
            song_fav(p, 1);                             /* (the part not changed) */
        else if (kind == SH_SCENE)
            song_scene(p, 1);
        else if (kind == SH_TAP) {
            song.g[G_BPM] = sg.hbpm;                    /* (the hold's tap undone: no tempo from it) */
            glo_tap_drop();
            song_click_toggle();
        }
        return;
    }
    if (!sg.hfired) {
        if (kind == SH_PART)
            song_part_tap(p);
        else if (kind == SH_SCENE)
            song_scene(p, 0);
    }
    sg.hk = 0;
}
static void song_hold_poll(void)
{
    uint32_t k;
    if (!sg.hk)
        return;
    k = sg.hk - 1u;
    if (!(((kb_layer | fm1_in.notes) >> k) & 1u))
        song_hold_end(0);
    else if (!sg.hfired && fm1_ms - sg.hms >= SG_HOLD_MS)
        song_hold_end(1);
}
/* 0.17 (spec v0.8 §2): FILL on T4 with D4 held. C4 down: a live fill from the next 1/16 (seq.c sgf.live); let go
 * before SG_HOLD_MS: it stops at the next 1/16 and a full-bar fill is armed for the bar end (the last bar of the part
 * when the chain or a part waits); held longer: a crash on the next downbeat. C4 again while armed: off. Refused (red):
 * stopped, no DRUM on T4, T4 muted by FX, T4 silent in the chain's row. Never written, saved or undone */
static const char *const FILL_N[4] = {"SNARE ROLL", "TOM RUN", "KICK BUILD", "CRASH"};
static uint32_t song_t4_muted(void)
{
    uint32_t held = perf_kill ? 0u : perf_held | perf_latched;
    return (held >> (PF_M1 + NTRK - 1u)) & 1u;
}
static const char *song_fill_no(void)
{
    if (!song.playing)
        return "PLAY FIRST";
    if (!drum_track(&trk[NTRK - 1u]))
        return "NO DRUM";
    if (song_t4_muted())
        return "T4 MUTED";
    if (chain.running && chain.lane[NTRK - 1u] >= 4u)
        return "T4 OFF";
    return 0;
}
static void song_fill_refuse(const char *r)
{
    song_pop(SOV_POP, "FILL", r, "", -1, 1);
    ui_message(r);
}
static uint32_t song_fill_type(uint32_t salt)          /* the seed, the part, the fill: one of four */
{
    uint32_t h = fg.seed * 2654435761u ^ sg.part * 40503u ^ salt * 2246822519u;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    return h & 3u;
}
static void song_fill_set(uint32_t salt)
{
    sgf.type = (uint8_t)song_fill_type(salt);
    sgf.dens = (uint8_t)clamp(((int32_t)fs.dens - 50) / 26, 0, 3);   /* (DENSITY 50..150: 0..3) */
}
static void song_fill_stop(void)                        /* leaving SONG, stopped: nothing waits, nothing plays */
{
    sgf.live = sgf.bar = sgf.crash = 0;
    sg.fk = sg.farm = 0;
}
static void song_fill_arm(void)
{
    uint32_t n = SG_CBARS[sg.cbars[sg.part] % NELEM(SG_CBARS)];
    char b[12] = "@ PART A";
    const char *r = song_fill_no();
    if (r) {
        song_fill_refuse(r);
        return;
    }
    b[7] = (char)('A' + (sg.qpart ? sg.qpart - 1u : 0u));
    song_pop(SOV_POP, "FILL", sg.qpart ? b : "@ BAR END", "", -1, 0);
    if (sg.qpart) {                                     /* (0.17 review: a part waits for this bar line: the fill is */
        song_fill_set(++sg.fbars);                      /* this bar's rest, the part on time) */
        sgf.bar = 1;
        return;
    }
    sg.farm = 1;                                        /* (the chain: the part's last bar; in it already: the next */
    sg.fwait = (uint8_t)(!sg.chain ? 1u : sg.cbar + 1u < n ? n - 1u - sg.cbar :   /* part's last, its own bars) */
                         SG_CBARS[sg.cbars[(sg.part + 1u) % SG_N] % NELEM(SG_CBARS)]);
}
static void song_fill_down(uint32_t k)                  /* D4 held, C4 down */
{
    const char *r;
    sg.d4_used = 1;
    if (sg.farm) {
        sg.farm = 0;
        song_pop(SOV_POP, "FILL", "OFF", "", -1, 0);
        return;
    }
    if ((r = song_fill_no()) != 0) {
        song_fill_refuse(r);
        return;
    }
    song_fill_set(fm1_ms);
    sgf.live = 1;                                       /* (the sequencer: from its next 1/16) */
    sg.fk = (uint8_t)(k + 1u);
    sg.fms = fm1_ms;
    song_pop(SOV_POP, "FILL", FILL_N[sgf.type & 3u], "", -1, 0);
}
static void song_fill_poll(uint32_t bar)
{
    if (!song.playing) {
        song_fill_stop();
        return;
    }
    if ((sg.farm || sgf.live || sgf.bar) && song_t4_muted()) {   /* (muted since: dropped) */
        song_fill_stop();
        song_fill_refuse("T4 MUTED");
        return;
    }
    if (sg.fk) {
        uint32_t k = sg.fk - 1u;
        if (!(((kb_layer | fm1_in.notes) >> k) & 1u) || !song_d4_down()) {
            sgf.live = 0;                               /* (the beat back from the next 1/16) */
            sg.fk = 0;
            if (fm1_ms - sg.fms < SG_HOLD_MS)
                song_fill_arm();                        /* (a tap: the full bar at the bar end) */
            else
                sgf.crash = 1;                          /* (held: a crash on the next downbeat) */
        } else if (sg.ov == SOV_POP && str_eq(sg_pop.sm, "FILL"))
            sg.ov_ms = fm1_ms;                          /* (its box stays while held) */
    }
    if (bar && sgf.bar) {                               /* (the fill's bar done) */
        sgf.bar = 0;
        if (sgf.type == FILL_CRASH)
            sgf.crash = 1;
    }
}
static void song_fill_bar(void)                         /* at a bar line, after the chain's count */
{
    if (sg.farm && !--sg.fwait) {
        sg.farm = 0;
        song_fill_set(++sg.fbars);
        sgf.bar = 1;
    }
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
static const char H_SONG[] = "[KEYS]D4+C4 FILL G5 OUT";   /* (0.17) */
static const khint_t *song_foot(void) { return SONG_FOOT[sg.pend ? 3u : sg.stage]; }

/* SONG's map, only at the confirm now (0.16; else ui_song_view.c draws): the four parts, then OVERWRITE A-D? and the
 * four slots' names as they are now ("--" an empty one) */
static void layer_song(void)
{
    uint32_t i;
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
    }                                                   /* (0.16: else SONG's own screen, ui_song_view.c; the 0.14
                                                         * rows and FOCUS / FADER state removed) */
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
    if (sg.on) {
        song_fx_clear();
        song_remember();                                /* (0.16: the last song, for SEQ held and BOOT = SONG) */
    }
    sg.qpart = sg.hk = sg.octpend = 0;
    song_fill_stop();                                   /* (0.17: an armed or playing FILL; spec v0.4.1: leaving SONG
                                                         * turns its effects off) */
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
    if (fg.seed != seed)
        fs_sounds();                                    /* (0.17 spec v0.8 §2c: another song, its sounds; GENRE: fs_knob) */
    if (fg.seed != seed || fs.genre != genre)
        song_reset();                                   /* (another song: EDIT, not written; DENSITY / EVOLVE: the */
    sg.dirty = 0xFu;
    song_render(sg.part);                               /* same song, still written, a reroll still asks SAVE X?) */
    song_say();
    {   /* the box: "SONG", "SEED 42" .. "GENRE TRANCE" and its bar (0.16) */
        static const char *const N[4] = {"SEED ", "DENS ", "EVOL ", ""};
        char b[24];
        int32_t r = k == 0u ? fg.seed * 1000 / 99 : k == 1u ? (fs.dens - 50) * 10 : k == 2u ? fg.evolve * 250 :
                    (int32_t)(fs.genre * 1000u / (FS_NGENRE - 1u));
        str_cpy(b, N[k & 3u], sizeof b);
        if (k == 3u)
            str_cpy(b, FS_GENRE[fs.genre % FS_NGENRE].name, sizeof b);
        else
            fmt_int(b + str_len(b), k == 0u ? fg.seed : k == 1u ? fs.dens : fg.evolve);
        song_pop(SOV_POP, "SONG", b, "", r, 0);
    }
}
/* FX held, KNOB k: the macro's box (ui_layer.c layer_knob) */
static void song_macro_pop(uint32_t k)
{
    static const char *const N[4] = {"FILTER ", "CRUSH ", "THROW ", "DEPTH "};
    char b[20];
    int32_t x = perf_k[k & 3u];
    str_cpy(b, N[k & 3u], sizeof b);
    if (x < 0)
        str_cpy(b + str_len(b), "-", sizeof b - str_len(b));
    fmt_int(b + str_len(b), (uint32_t)(x < 0 ? -x : x));
    song_pop(SOV_POP, "FX", b, "", k ? x * 10 : (x + 100) * 5, 0);
}
/* double-tap OCT-: the last reroll, sound or key back (one level) */
static void song_undo(void)
{
    char b[20] = "";
    if (sgu.kind == SU_REROLL) {
        memcpy(sg.nvar, sgu.nvar, sizeof sg.nvar);
        memcpy(sg.rvar, sgu.rvar, sizeof sg.rvar);
        song_render(sg.part);
        if (sgu.what == 0xFu)
            str_cpy(b, "NOTES", sizeof b);
        else {
            b[0] = 'T';
            b[1] = (char)('1' + (__builtin_ctz(sgu.what) & 3));
            str_cpy(b + 2, " REROLL", sizeof b - 2u);
        }
    } else if (sgu.kind == SU_SOUND) {
        track_t *t = &trk[sgu.trk % NTRK];
        uint32_t i;
        if (t->eng_req != sgu.eng)
            set_engine_of(t, sgu.eng);
        load_begin(t, UNDO_SOUND);
        panic_req |= (uint8_t)(1u << (sgu.trk % NTRK));
        fm1_irq_off();
        for (i = 0; i < P_COUNT; i++)
            if (!param_kept(i) || i == P_LEVEL)
                t->p[i] = sgu.p[i];
        t->preset = sgu.pre;
        t->user = sgu.usr;
        fs_packid[sgu.trk % NTRK] = sgu.pack;
        fm1_irq_on();
        load_end(t);
        b[0] = 'T';
        b[1] = (char)('1' + (sgu.trk % NTRK));
        str_cpy(b + 2, " SOUND", sizeof b - 2u);
    } else if (sgu.kind == SU_KEY) {
        fs.root = sgu.root;
        fs.scale = sgu.scale;
        sg.kpend = 0;
        song_render(sg.part);
        str_cpy(b, "KEY", sizeof b);
    } else {
        song_refuse("NOTHING TO UNDO", "UNDO");
        return;
    }
    sgu.kind = SU_NONE;
    if (sg.stage == SG_WRITTEN)
        sg.pend = (uint8_t)(sg.part + 1u);
    ui_message("UNDO");
    song_pop(SOV_POP, "UNDO", b, "", -1, 0);
}
