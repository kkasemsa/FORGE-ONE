/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the FORGE ONE contributors */
/* FORGE ONE 0.6: SEQ TOOLS G5 STARTER (TL_STARTER, ui_tools.c), a song starter: one press writes all four tracks, the
 * sound and the pattern of each, from one SEED. Included by ui_layer.c after ui_forge.c, whose generator it drives.
 *
 * The parts: T1 BASS, T2 LEAD, T3 PAD, T4 DRUM. Each STARTER (the eight FORGE styles) names a factory preset for each
 * part (engine and preset by name, resolved at load: tests/forge_test.c checks they all exist), a row of forge_style_t
 * for each melodic part (fg_ovr) and FORGE_BEAT for the drums, each part's LEN, a SCALE (used when the selected
 * track's SCALE is CHR; else that one) and a tempo (not with CLOCK on an external clock). ROOT: the selected track's.
 *
 * G5 opens it (any track; a song playing: "STOP TO EDIT"): the four tracks are copied (sound, parameters, steps, FM6
 * patch, the automation), the starter's sounds loaded and the patterns written at once, heard while playing. KNOB 1
 * SEED, 2 DENSITY (50..150 % of each part's own), 3 EVOLVE, 4 STARTER (wraps; loads its sounds); C4 / D4 reroll the
 * notes / rhythms of all four. OCT+ (or G5 again, another tool, the layer let go) applies it, OCT- puts all four
 * tracks back as they were. The one SAVE-held undo copy can hold one track only, so after STARTER it holds nothing:
 * OCT- is its undo. The mix (LEVEL PAN MUTE) stays as it was: a sound load never changes it */

enum { FP_BASS, FP_LEAD, FP_PAD, FP_DRUM, FP_N };
typedef struct { const char *eng, *preset; } fs_sound_t;
typedef struct {
    fs_sound_t snd[FP_N];
    forge_style_t part[3];       /* BASS LEAD PAD (name / card unused) */
    uint8_t len[FP_N], scale, bpm;
} forge_starter_t;

/* SCALE numbers (seq.c SCALE_MASK): 2 MIN, 3 DOR, 9 LYD */
static const forge_starter_t FORGE_STARTER[FS_N] = {
    /*  rhythm  accent motif dens base contour  span tie chord ch% slide oct orn rat vel */
    {{{"ANALOG", "SQR BASS"}, {"TRIO", "ARP LEAD"}, {"GRAIN", "CLOUD PAD"}, {"DRUM", "DEEP KIT"}},    /* BERLIN */
     {{"", "", 0xFFFF, 0x0101,  8, 90, 36, FC_PEDAL, 3,   0, 0,   0,  0,  25, 10,  0, 18},
      {"", "", 0xFFFF, 0x0101, 16, 55, 60, FC_ARC,   5,  30, 0,   0,  0,  10, 15,  0, 16},
      {"", "", 0x0101, 0x0000, 16, 100, 48, FC_WALK, 2,  95, 3, 100,  0,   0,  0,  0,  6}},
     {16, 16, 32, 16}, 2, 120},
    {{{"FM6", "FM BASS"}, {"ANALOG", "PWM STR"}, {"FM6", "TINE EP"}, {"DRUM", "TIGHT KIT"}},          /* DETROIT */
     {{"", "", 0x4444, 0x0000, 16, 85, 36, FC_ROOT5, 3,  20, 0,   0,  0,  10, 10,  0, 14},
      {"", "", 0x4949, 0x0001, 16, 70, 60, FC_WALK,  3,  20, 4,  70,  0,   5, 15,  0, 16},
      {"", "", 0x0101, 0x0000, 16, 100, 48, FC_WALK, 2,  60, 4, 100,  0,   0, 10,  0,  8}},
     {16, 16, 32, 16}, 3, 126},
    {{{"ANALOG", "SUB BASS"}, {"PHASE", "STRING"}, {"GRAIN", "SHIMMER"}, {"DRUM", "DEEP KIT"}},       /* DUB */
     {{"", "", 0x4545, 0x0001, 16, 60, 36, FC_PEDAL, 2,  50, 0,   0,  0,   0, 10,  0,  8},
      {"", "", 0x4444, 0x0000, 16, 55, 48, FC_WALK,  2,  60, 3, 100,  0,   0, 20,  0, 10},
      {"", "", 0x0001, 0x0000, 16, 100, 48, FC_WALK, 2,  98, 3, 100,  0,   0,  0,  0,  6}},
     {16, 16, 32, 16}, 3, 118},
    {{{"TRIO", "FAT BASS"}, {"ANALOG", "SAW LEAD"}, {"FM6", "PAD"}, {"DRUM", "DRUM KIT"}},            /* MELODIC */
     {{"", "", 0xEEEE, 0x2222, 16, 85, 36, FC_ROOT5, 3,   0, 0,   0,  0,   5, 10,  0, 16},
      {"", "", 0x5555, 0x0101, 16, 75, 60, FC_ARC,   7,  25, 0,   0,  0,   5, 15,  0, 18},
      {"", "", 0x0101, 0x0000, 16, 100, 48, FC_WALK, 3,  95, 3, 100,  0,   0,  0,  0,  6}},
     {16, 32, 32, 16}, 2, 122},
    {{{"ANALOG", "ACID"}, {"LOFI", "ARP 8BIT"}, {"ANALOG", "SOFT PAD"}, {"DRUM", "TIGHT KIT"}},       /* ACID */
     {{"", "", 0xFFFF, 0x2224, 16, 75, 36, FC_PEDAL, 4,   0, 0,   0, 25,  30, 10, 15, 24},
      {"", "", 0x5555, 0x0101,  8, 60, 60, FC_CHORD, 7,   0, 0,   0,  0,  10, 10,  0, 14},
      {"", "", 0x0001, 0x0000, 16, 100, 48, FC_WALK, 2,  98, 3, 100,  0,   0,  0,  0,  6}},
     {16, 16, 32, 16}, 2, 128},
    {{{"ANALOG", "SUB BASS"}, {"TRIO", "ARP LEAD"}, {"ANALOG", "STRINGS"}, {"DRUM", "DRUM KIT"}},     /* TRANCE */
     {{"", "", 0x4444, 0x0000, 16, 100, 36, FC_PEDAL, 2,  0, 0,   0,  0,   0,  0,  0, 10},
      {"", "", 0xFFFF, 0x1111,  8, 90, 60, FC_CHORD, 7,   0, 0,   0,  0,  10, 10, 10, 20},
      {"", "", 0x0101, 0x0000, 16, 100, 48, FC_WALK, 3,  95, 3, 100,  0,   0,  0,  0,  6}},
     {16, 16, 32, 16}, 2, 138},
    {{{"TRIO", "FAT BASS"}, {"ANALOG", "BRASS"}, {"FM6", "PAD"}, {"DRUM", "DRUM KIT"}},               /* SYNTH */
     {{"", "", 0x5555, 0x0101, 16, 90, 36, FC_ROOT5, 3,   0, 0,   0,  0,  20,  5,  0, 14},
      {"", "", 0x5555, 0x0101,  8, 90, 48, FC_ROOT5, 7,  10, 0,   0,  0,   0,  5,  0, 16},
      {"", "", 0x0101, 0x0000, 16, 100, 48, FC_WALK, 3,  95, 3, 100,  0,   0,  0,  0,  6}},
     {16, 16, 32, 16}, 2, 110},
    {{{"ANALOG", "SUB BASS"}, {"PHYS", "KALIMBA"}, {"GRAIN", "CLOUD PAD"}, {"DRUM", "PERC KIT"}},     /* AMBIENT */
     {{"", "", 0x0001, 0x0000, 16, 100, 36, FC_PEDAL, 2, 100, 0,   0,  0,   0,  0,  0,  6},
      {"", "", 0x0441, 0x0000, 16, 50, 60, FC_WIDE,  9,  80, 3,  30,  0,   0, 40,  0, 12},
      {"", "", 0x0001, 0x0000, 16, 100, 48, FC_WIDE, 3, 100, 3, 100,  0,   0,  0,  0,  6}},
     {32, 32, 64, 16}, 9, 80},
};
static const char *const FP_ROLE[FP_N] = {"BASS", "LEAD", "PAD", "DRUM"};
static const char H_STARTER[] = "[K1]SEED[K3]EVOLVE[K4]START";   /* (the footer holds three) */

/* the four tracks as STARTER opened (OCT-): in the pool, as FORGE's eu_from */
static struct {
    int16_t p[NTRK][P_COUNT];
    step_t step[NTRK][NSTEP];
    uint8_t fm6[NTRK][FP_SIZE + 1u];
    uint8_t eng[NTRK], preset[NTRK], user[NTRK], fm6_slot[NTRK];
    motion_store_t mo;
    int16_t bpm;
} fs_from __attribute__((section(".pool")));
static struct { uint8_t on, set, style, dens, root, scale; } fs;   /* dens: % of each part's own */

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
        if (!fs_find(&FORGE_STARTER[fs.style % FS_N].snd[i], &e, &k)) {
            track_t *t = &trk[i];
            if (t->eng_req % NENGINES != e)
                set_engine_of(t, e);
            apply_preset_to(t, k);
        }
}
/* the patterns: each part's LEN and DIV 1/16, the key, then FORGE's generator with the part's row and stream */
static void fs_render(void)
{
    const forge_starter_t *s = &FORGE_STARTER[fs.style % FS_N];
    uint8_t trk0 = fg.trk, style0 = fg.style, dens0 = fg.dens;
    uint32_t i;
    for (i = 0; i < FP_N; i++) {
        track_t *t = &trk[i];
        uint32_t own = i < FP_DRUM ? s->part[i].dens : FORGE_BEAT[fs.style % FS_N].dens;
        t->p[P_SLEN] = s->len[i];
        t->p[P_SDIV] = 2;
        t->p[P_ROOT] = fs.root;
        t->p[P_SCALE] = fs.scale ? fs.scale : s->scale;
        fg.trk = (uint8_t)i;
        fg.style = fs.style;
        fg.dens = (uint8_t)clamp((int32_t)(own * fs.dens / 100u), 10, 100);
        fg_role = (uint8_t)(i + 1u);
        fg_ovr = i < FP_DRUM ? &s->part[i] : 0;
        forge_render();
    }
    fg_role = 0;
    fg_ovr = 0;
    fg.trk = trk0;
    fg.style = style0;
    fg.dens = dens0;
}
static void fs_tempo(void)
{
    if (!song.g[G_CLOCK])
        song.g[G_BPM] = (int16_t)clamp(FORGE_STARTER[fs.style % FS_N].bpm, 40, 240);
}
static void fs_say(void)                                /* "START 42 BERLIN" */
{
    char b[24], n[4];
    str_cpy(b, "START ", sizeof b);
    fmt_int(n, fg.seed);
    str_cpy(b + str_len(b), n, sizeof b - str_len(b));
    str_cpy(b + str_len(b), " ", sizeof b - str_len(b));
    str_cpy(b + str_len(b), FORGE_STYLE[fs.style % FS_N].name, sizeof b - str_len(b));
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
        fs.style = fg.set ? fg.style : FS_BERLIN;
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
        fs.style = (uint8_t)(((int32_t)fs.style + s % FS_N + FS_N) % FS_N);
        fg.nvar = fg.rvar = 0;
        fs_sounds();                                    /* (a new starter: its sounds and tempo) */
        fs_tempo();
        undo.trk = 0;
    }
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
    return fs.on ? 17u + fg.seed * 13u + fs.dens * 1031u + fg.evolve * 65537u + fs.style * 524287u + fg.nvar * 97u +
                   fg.rvar * 7919u : 0u;
}

/* the open STARTER: a row a track: T1..T4, the preset, the first 16 steps (a note or hit filled, a TIE half) */
static void layer_starter(void)
{
    uint32_t i, j, e, k;
    for (i = 0; i < FP_N; i++) {
        const track_t *t = &trk[i];
        int32_t y = 4 + 24 * (int32_t)i;
        char n[3] = {'T', (char)('1' + i), 0};
        const fs_sound_t *sd = &FORGE_STARTER[fs.style % FS_N].snd[i];
        GFX_HOOK_ALIGN(0, y, 0, y + 20, AL_V, "starter row text on its middle");
        cv_text_on(8, y + CAP_IN(S, 20), &AF_S, n, T_MID, T_SURF);
        GFX_HOOK_ALIGN(0, y, 0, y + 20, AL_V, "starter row text on its middle");
        cv_text_on(26, y + CAP_IN(S, 20), &AF_S, fs_find(sd, &e, &k) ? FP_ROLE[i] : sd->preset, T_TEXT, T_SURF);
        for (j = 0; j < 16u; j++) {
            const step_t *s = &t->step[j];
            int on = drum_track(t) ? step_lanes(s) != 0u : s->time == ST_NOTE && s->n;
            int32_t x = 122 + 7 * (int32_t)j;
            uint16_t c = j >= tl_len(t) ? T_SURF : on ? T_THEME : s->time == ST_TIE ? T_MID : T_RAISE;
            cv_rect(x, y + 2, 6, 16, c);
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
    draw_column(3, "STARTER", FORGE_STYLE[fs.style % FS_N].card, "", VAL(3u), fs.style * 1000 / (FS_N - 1), ICON_X_SONG);
}
