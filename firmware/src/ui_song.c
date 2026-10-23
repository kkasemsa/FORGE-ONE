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
 * steps. A part's own rerolls (C4 notes, D4 rhythm) count per track, so a locked track (black keys 5..8) keeps its
 * steps exactly.
 *
 * Keys (the STARTER layer): F#5 SONG on / off. Then black 1..4 a part A..D (it plays: its steps in the four tracks),
 * black 5..8 the T1..T4 locks, C4 / D4 reroll the part shown, KNOB 1..4 SEED DENSITY EVOLVE GENRE (as STARTER).
 *   OCT+  GENERATE: the confirm, "OVERWRITE A-D?" with the slots' names; OCT+ again writes the four slots (S42 VERSE,
 *         S42 CHORUS, S42 BRIDGE, S42 LIFT / S42 BREAK) and the chain (8 rows, by D's type); OCT- there cancels it
 *         all (the four tracks as STARTER found them, no slot written)
 *   after it, a reroll asks "SAVE B?" (that part only): OCT+ saves slot B, OCT- keeps it in the tracks, unsaved.
 *   Written, OCT+ or OCT- ends SONG and STARTER with the part shown in the tracks.
 * Saving needs the transport stopped (project.c: a flash erase stalls the audio): playing, GENERATE says STOP TO SAVE. */

enum { SG_A, SG_B, SG_C, SG_D, SG_N };
#define SG_NAME_LEN 12u                         /* project.c PROJ_NAME_LEN (included after this; tests check they match) */
enum { SG_EDIT, SG_CONFIRM, SG_WRITTEN };
static const char *const SG_ROLE[SG_N + 1] = {"VERSE", "CHORUS", "BRIDGE", "LIFT", "BREAK"};
static struct {
    uint8_t on, stage, part, lock, pend;    /* lock: bit i T(i+1); pend: SAVE X? asked (X + 1), 0 none */
    uint8_t nvar[SG_N][NTRK], rvar[SG_N][NTRK];
} sg;

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

/* part x of the song into the four tracks (their steps, LEN, DIV, SWING, ROOT, SCALE and automation; the sounds stay) */
static void song_render(uint32_t x)
{
    const forge_starter_t *s = &FORGE_STARTER[fs_idx()];
    uint8_t trk0 = fg.trk, style0 = fg.style, dens0 = fg.dens, ev0 = fg.evolve, n0 = fg.nvar, r0 = fg.rvar;
    uint32_t i, j, d = song_dtype(), amb = fs.genre % FS_NGENRE == 10u, sec = song_d_steps();
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
        if (x == SG_D && (i == FP_DRUM || (d == SD_BREAK && i == FP_PAD)))
            len = sec < NSTEP ? sec : NSTEP;            /* (the section's last bar is in the pattern) */
        ev += x == SG_B ? 1u : x == SG_D && d == SD_LIFT ? 2u : 0u;
        t->p[P_SLEN] = (int16_t)len;
        t->p[P_SDIV] = 2;
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
        if (i == FP_PAD && x == SG_D && d == SD_BREAK) {   /* the filter down, opening over the last bar */
            uint32_t id = song_filter_id(t), last = len > 16u ? len - 16u : 0u;
            int32_t top = t->p[id], lo = song_param_min(t, id) + (top - song_param_min(t, id)) * 3 / 10;
            motion_set_enabled(t, 1);
            motion_set_event(t, 0, id, (int16_t)lo);
            for (j = last ? last : 1u; j < len; j += 2u)
                motion_set_event(t, j, id, (int16_t)(lo + (top - lo) * (int32_t)(j - last + 2u) / (int32_t)(len - last)));
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
/* F#5 in STARTER: SONG on (part A in the tracks) or off (STARTER's own patterns back) */
static void song_toggle(void)
{
    if (sg.stage == SG_CONFIRM)
        return;
    sg.on = (uint8_t)!sg.on;
    sg.part = SG_A;
    sg.lock = 0;
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
static void song_reroll(uint32_t rhythm)                /* C4 / D4: the part shown, its tracks not locked */
{
    uint32_t i, any = 0;
    for (i = 0; i < NTRK; i++)
        if (!((sg.lock >> i) & 1u)) {
            uint8_t *v = rhythm ? &sg.rvar[sg.part][i] : &sg.nvar[sg.part][i];
            *v = (uint8_t)((*v + 1u) % 100u);
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
        ui_message(rhythm ? "NEW RHYTHM" : "NEW NOTES");
    }
}
/* the four slots and the chain, after the confirm's OCT+; D first, A last (the slot in use and the tracks: A) */
static int song_write(void)
{
    char b[SG_NAME_LEN + 1u];
    uint32_t x;
    if (transport_busy()) {
        ui_message("STOP TO SAVE");
        return 1;
    }
    song_chain(&chain_config);
    for (x = SG_N; x-- > 0;) {
        song_render(x);
        song_name(x, b);
        if (project_save_as(x, b))
            return 2;
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
    if (sg.pend) {                                      /* SAVE X?: that slot only, or nothing */
        uint32_t x = sg.pend - 1u;
        sg.pend = 0;
        if (!up) {
            ui_message("NOT SAVED");
            return 0;
        }
        song_name(x, b);
        if (!project_save_as(x, b)) {
            char m[12] = {(char)('A' + x), ' ', 'S', 'A', 'V', 'E', 'D', 0};
            ui_message(m);
        }
        return 0;
    }
    if (sg.stage == SG_EDIT) {
        if (!up) {
            sg.on = 0;
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
            sg.stage = SG_EDIT;
            sg.on = 0;
            fs_close(0);
            ui_message("SONG CANCELLED");
            return 1;
        }
        if (song_write())
            sg.stage = SG_EDIT;
        return 0;
    }
    sg.on = 0;                                          /* (written: done, the part shown stays in the tracks) */
    fs_close(1);
    ui_message("SONG DONE");
    return 1;
}
static uint32_t song_sig(void)
{
    return sg.on ? 29u + sg.stage * 7u + sg.part * 131u + sg.lock * 1031u + sg.pend * 8191u : 0u;
}

/* the footer and the help row of each stage */
static const khint_t SONG_FOOT[4][3] = {
    {{KC_OCTUP, "GENERATE"}, {KC_OCTDN, "CANCEL"}, {0, 0}},     /* editing */
    {{KC_OCTUP, "OVERWRITE"}, {KC_OCTDN, "CANCEL"}, {0, 0}},    /* OVERWRITE A-D? */
    {{KC_OCTUP, "DONE"}, {KC_OCTDN, "DONE"}, {0, 0}},          /* written */
    {{KC_OCTUP, "SAVE"}, {KC_OCTDN, "KEEP"}, {0, 0}}};         /* SAVE X? */
static const char H_SONG[] = "[KEYS]BLACK 1-4 PART 5-8 LOCK";
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
        cv_text_on(26, y + CAP_IN(S, 17), &AF_S, FP_ROLE[i], T_TEXT, T_SURF);
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
        cv_text_r(230, 100 + CAP_IN(S, 18), &AF_S, "C4 / D4 REROLL", T_MID, T_SURF);
    }
}
/* the LEDs of the black keys in SONG: the part shown lit (the others breathe), a lock lit (free: breathes), F#5 lit */
static void song_led(uint32_t p, uint32_t *on, uint32_t *can)
{
    *on = (p < SG_N && p == sg.part) || (p >= 4u && p < 8u && ((sg.lock >> (p - 4u)) & 1u)) || p == 10u;
    *can = p < 8u;
}
/* STARTER or SONG closed some other way (the layer let go, another tool): SONG with it, nothing written */
static void song_off(void)
{
    sg.on = 0;
    sg.stage = SG_EDIT;
    sg.pend = 0;
}
/* KNOB 1..4 in SONG: as STARTER's (SEED DENSITY EVOLVE GENRE), then the part shown again; a new song is not written */
static void song_knob(uint32_t k, int32_t s)
{
    uint8_t seed = fg.seed, genre = fs.genre;
    fs_knob(k, s);
    if (fg.seed != seed || fs.genre != genre)
        song_reset();
    sg.stage = SG_EDIT;
    sg.pend = 0;
    song_render(sg.part);
    song_say();
}
