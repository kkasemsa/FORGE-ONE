/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the FORGE ONE contributors */
/* FORGE ONE 0.15: SONG's own screen (Song Mode spec v0.6 §J), read at arm's length. Included by ui_layer.c after
 * ui_song.c; ui_draw.c draws it instead of the layer's map while SONG is open (not at its confirm).
 *
 *   y   0..35   the tempo and the key, large (the key in the accent while a change waits for the bar)
 *      38..59   the parts A B C D (the one playing filled; one asking SAVE X? outlined, blinking)
 *      62..91   BAR 3/8 large, STEP 09/16 (STEP n/LEN past 16)
 *      94..98   the position: in the chain (GLO > SONG playing) or in the part's loop
 *     104..114  16 cells, the playhead in the accent (beats 1 5 9 13 brighter); a page of 16 past LEN 16
 *     120..195  T1..T4: a dot that lights on each note (~120 ms), the name, a level meter, M (muted: the name struck
 *               through), L (locked), FOCUS (the row tinted)
 *     197..213  the two scenes (filled: stored; the one recalled flashes), then the state or the last message
 *     216..239  the four knob cards as the knobs are now: FX held its macros, D4 held or FOCUS the track's, else SEED
 *               DENSITY EVOLVE GENRE
 * A change of tempo, key, preset, scene or favourite opens a box over 46..175 for 1.2 s after the last one (while a
 * change waits for its bar: until then); what it covers is drawn again when it closes.
 *
 * Drawn by regions: each its own signature, redrawn when that changes (never the whole screen while it shows, but on
 * entering it or a palette / style change). Per 1/16: the STEP digits, the two playhead cells; the meters at most
 * ~30 Hz, only the pixels that moved; the dots on a note and when they fade. Nothing in the audio ISR but one OR
 * per note (voice.c trk_act) */

enum { SV_HEAD, SV_PARTS, SV_BAR, SV_STEPN, SV_CHAIN, SV_SCN, SV_CARDS, SV_OV, SV_N };
static struct {
    uint8_t shown, ov_was, meter[NTRK], dot[NTRK], ph, lit;
    uint32_t gen, sig[SV_N], dot_ms[NTRK], meter_ms;
    /* what the screen says (tests/forge_test.c reads it): BAR bar / bars, STEP step / len, the playhead cell
     * (0xFF stopped), the position bar's fill (px of SV_CHW) */
    uint32_t bar, bars, step, len, chain_px;
} sv;
#define SV_X0 4
#define SV_W 232
#define SV_CHW 232
#define SV_DOT_MS 120u
#define SV_MW 66                                        /* a meter's width */

static int sv_full(void) { return !sv.shown; }
static int sv_dirty(uint32_t r, uint32_t sig)           /* region r shows sig now: 1 when it must be drawn */
{
    if (!sv_full() && sv.sig[r] == sig)
        return 0;
    sv.sig[r] = sig;
    return 1;
}
static int sv_large_ok(const char *s)                   /* every glyph in the sparse L face */
{
    for (; *s; s++)
        if (glyph_at(&AF_L, (uint8_t)*s) < 0)
            return 0;
    return 1;
}
static uint32_t sv_muted(uint32_t i)
{
    uint32_t held = perf_kill ? 0u : perf_held | perf_latched;
    return trk[i].p[P_MUTE] || ((held >> (PF_M1 + i)) & 1u);
}
static void sv_num2(char *b, uint32_t v)                /* "09" */
{
    b[0] = (char)('0' + v / 10u % 10u);
    b[1] = (char)('0' + v % 10u);
    b[2] = 0;
}

/* where the song is: the part's loop on T1 (the clock), or the chain's section while GLO > SONG plays */
static void sv_pos(void)
{
    const track_t *t = &trk[0];
    uint32_t len = tl_len(t), idx = song.playing ? t->seq_idx % len : 0u, bars = (len + 15u) / 16u;
    sv.len = len;
    sv.step = idx + 1u;
    sv.ph = song.playing ? (uint8_t)(idx % 16u) : 0xFFu;
    if (chain.running && chain.config.count) {
        uint32_t rep = chain.config.row[chain.row].repeat, done = rep > chain.remaining ? rep - chain.remaining : 0u;
        sv.bars = rep * bars;
        sv.bar = done * bars + idx / 16u + 1u;
        sv.chain_px = (chain.row * sv.bars + sv.bar - 1u) * SV_CHW / (chain.config.count * sv.bars);
    } else {
        sv.bars = bars;
        sv.bar = idx / 16u + 1u;
        sv.chain_px = song.playing ? (idx + 1u) * SV_CHW / len : 0u;
    }
    if (sv.bar > sv.bars)
        sv.bar = sv.bars;
}

static void sv_head(void)                               /* 128 BPM .. A MIN */
{
    char b[8], k[16];
    uint32_t pend = sg.kpend;
    fmt_int(b, (uint32_t)song.g[G_BPM]);
    song_key_name(k, sizeof k);
    if (!sv_dirty(SV_HEAD, str_hash(pend + 3u * (uint32_t)song.g[G_CLOCK], b) * 31u + str_hash(1u, k) + ux.gen))
        return;
    cv_begin(240, 36, T_BG);
    GFX_HOOK_ALIGN(0, 0, 0, 36, AL_V, "song view head on its middle");
    {
        int32_t x = cv_text_on(8, CAP_IN(L, 36), &AF_L, b, T_TEXT, T_BG);
        cv_text_on(x + 3, CAP_IN(L, 36) + AF_L_CAP_Y + AF_L_CAP_H - AF_S_CAP_Y - AF_S_CAP_H, &AF_S,
                   song.g[G_CLOCK] ? "EXT" : "BPM", T_MID, T_BG);
    }
    GFX_HOOK_ALIGN(0, 0, 0, 36, AL_V, "song view head on its middle");
    cv_text_r(232, CAP_IN(L, 36), sv_large_ok(k) ? &AF_L : &AF_M, k, pend ? T_ACCENT : T_THEME, T_BG);
    cv_blit(0, 0);
}
static void sv_parts(void)
{
    uint32_t i, blink = sg.pend && ((fm1_ms / 300u) & 1u);
    if (!sv_dirty(SV_PARTS, 7u + sg.part * 13u + sg.pend * 131u + blink * 1031u + ux.gen))
        return;
    cv_begin(240, 22, T_BG);
    for (i = 0; i < SG_N; i++) {
        int32_t x = SV_X0 + 59 * (int32_t)i;
        char b[2] = {(char)('A' + i), 0};
        uint16_t f = i == sg.part ? T_THEME : T_RAISE, ink = i == sg.part ? T_INK : T_MID;
        cv_rrect(x, 0, 55, 22, 4, f, T_BG);
        if (sg.pend == i + 1u && blink)
            cv_frame(x, 0, 55, 22, T_ACCENT);
        GFX_HOOK_ALIGN(x, 0, x + 55, 22, AL_HV, "song view part centred");
        cv_text_in(x, CAP_IN(M, 22), 55, &AF_M, b, ink, f);
    }
    cv_blit(0, 38);
}
static void sv_bar(void)                                /* BAR 3/8 (large) and the STEP label and its total */
{
    char b[16], n[8];
    str_cpy(b, "BAR ", sizeof b);
    fmt_int(n, sv.bar);
    str_cpy(b + 4, n, sizeof b - 4u);
    str_cpy(b + str_len(b), "/", sizeof b - str_len(b));
    fmt_int(n, sv.bars);
    str_cpy(b + str_len(b), n, sizeof b - str_len(b));
    if (sv_dirty(SV_BAR, str_hash(sv.len, b) + ux.gen)) {
        cv_begin(240, 30, T_BG);
        GFX_HOOK_ALIGN(0, 0, 0, 30, AL_V, "song view bar on its middle");
        cv_text_on(8, CAP_IN(L, 30), &AF_L, b, T_TEXT, T_BG);
        GFX_HOOK_ALIGN(0, 0, 0, 30, AL_V, "song view bar on its middle");
        cv_text_on(150, CAP_IN(S, 30), &AF_S, "STEP", T_MID, T_BG);
        b[0] = '/';
        fmt_int(b + 1, sv.len);
        GFX_HOOK_ALIGN(0, 0, 0, 30, AL_V, "song view bar on its middle");
        cv_text_r(232, CAP_IN(M, 30), &AF_M, b, T_MID, T_BG);
        cv_blit(0, 62);
        sv.sig[SV_STEPN] = 0;                           /* (its digits on top again) */
    }
    sv_num2(n, sv.step > 99u ? 99u : sv.step);
    if (sv_dirty(SV_STEPN, str_hash(sv.len, n) + ux.gen + 1u)) {   /* the digits alone: each 1/16 */
        int32_t w;
        b[0] = '/';                                     /* (left of "/LEN": b held BAR n/n when its region did */
        fmt_int(b + 1, sv.len);                         /* not change, so the digits landed on STEP: device 0.15) */
        w = text_w(&AF_M, b);
        cv_begin(22, 30, T_BG);
        GFX_HOOK_ALIGN(0, 0, 0, 30, AL_V, "song view bar on its middle");
        cv_text_r(22, CAP_IN(M, 30), &AF_M, n, T_TEXT, T_BG);
        cv_blit((uint32_t)(232 - w - 22), 62);
    }
}
static void sv_chain(void)
{
    if (!sv_dirty(SV_CHAIN, sv.chain_px + ux.gen * 977u))
        return;
    cv_begin(SV_CHW, 5, T_BG);
    cv_rect(0, 0, SV_CHW, 5, T_RAISE);
    cv_rect(0, 0, (int32_t)sv.chain_px, 5, T_SEL);
    cv_blit(SV_X0, 94);
}
/* the 16 cells: a cell's colour; drawn whole on a change of page or palette, else only the old and new playhead */
static uint16_t sv_cell_c(uint32_t i, uint32_t ph) { return i == ph ? T_ACCENT : i % 4u == 0u ? T_LINE : T_RAISE; }
static void sv_cell(uint32_t i, uint32_t ph)
{
    cv_begin(13, 11, T_BG);
    cv_rect(0, 0, 13, 11, sv_cell_c(i, ph));
    cv_blit(SV_X0 + 2 + 14u * i, 104);
}
static void sv_strip(void)
{
    static uint8_t was = 0xFEu;
    uint32_t i;
    if (sv_full()) {
        cv_begin(240, 11, T_BG);
        for (i = 0; i < 16u; i++)
            cv_rect(SV_X0 + 2 + 14 * (int32_t)i, 0, 13, 11, sv_cell_c(i, sv.ph));
        cv_blit(0, 104);
    } else if (was != sv.ph) {
        if (was < 16u)
            sv_cell(was, sv.ph);
        if (sv.ph < 16u)
            sv_cell(sv.ph, sv.ph);
    }
    was = sv.ph;
}
/* a track's row: its text part on a change (name, M, L, FOCUS); the dot and the meter on their own */
static int32_t sv_row_y(uint32_t i) { return 120 + 19 * (int32_t)i; }
static uint16_t sv_row_bg(uint32_t i) { return sg.focus == i + 1u ? T_TINT : T_SURF; }
static void sv_dot(uint32_t i)
{
    int32_t y = sv_row_y(i);
    cv_begin(10, 10, sv_row_bg(i));
    cv_rrect(0, 0, 10, 10, 5, sv.dot[i] ? T_THEME : T_DIM, sv_row_bg(i));
    cv_blit(SV_X0 + 4, (uint32_t)(y + 4));
}
static void sv_meter(uint32_t i, uint32_t from, uint32_t to)   /* px from .. to of track i's meter (to > from) */
{
    int32_t y = sv_row_y(i);
    uint32_t m = sv.meter[i];
    cv_begin(to - from, 5, T_LINE);
    if (m > from)
        cv_rect(0, 0, (int32_t)((m < to ? m : to) - from), 5, sv_muted(i) ? T_DIM : T_THEME);
    cv_blit(SV_X0 + 96 + from, (uint32_t)(y + 7));
}
static uint32_t sv_meter_px(int32_t a)                  /* |sample| Q15 -> px, 60 dB over the meter (as the MIXER's) */
{
    int32_t lg = 0, v;
    if (a < 64)
        return 0;
    while ((a >> lg) > 1)
        lg++;
    v = lg * 8 + (((a << 3) >> lg) & 7);
    return (uint32_t)clamp((v - 48) * SV_MW / 80, 0, SV_MW);
}
static void sv_rows(void)
{
    uint32_t i, now = fm1_ms, act = trk_act, meters = now - sv.meter_ms >= 33u;
    trk_act = 0;                                        /* (the ISR's bits: taken; one lost to a race only re-lights) */
    if (meters)
        sv.meter_ms = now;
    for (i = 0; i < NTRK; i++) {
        const track_t *t = &trk[i];
        int32_t y = sv_row_y(i);
        uint32_t mute = sv_muted(i), lock = (sg.lock >> i) & 1u, foc = sg.focus == i + 1u, d;
        uint16_t bg = sv_row_bg(i);
        char n[12] = {'T', (char)('1' + i), ' ', 0};
        str_cpy(n + 3, FP_ROLE[i], sizeof n - 3u);
        {                                               /* (each row its own signature) */
            static uint32_t rsig[NTRK];
            uint32_t sig = 5u + mute * 7u + lock * 13u + foc * 29u + ux.gen * 977u + t->eng_req * 3u;
            if (sv_full() || rsig[i] != sig) {
                rsig[i] = sig;
                cv_begin(SV_W, 18, T_BG);
                cv_rrect(0, 0, SV_W, 18, 3, bg, T_BG);
                GFX_HOOK_ALIGN(0, 0, 0, 18, AL_V, "song view row text on its middle");
                {
                    int32_t x1 = cv_text_on(20, CAP_IN(S, 18), &AF_S, n, mute ? T_MID : T_TEXT, bg);
                    if (mute)
                        cv_rect(18, 9, x1 - 16, 1, T_REC);   /* (struck through) */
                }
                if (mute) {
                    GFX_HOOK_ALIGN(0, 0, 0, 18, AL_V, "song view row text on its middle");
                    cv_text_on(168, CAP_IN(S, 18), &AF_S, "M", T_REC, bg);
                }
                if (lock) {
                    GFX_HOOK_ALIGN(0, 0, 0, 18, AL_V, "song view row text on its middle");
                    cv_text_on(182, CAP_IN(S, 18), &AF_S, "L", T_MID, bg);
                }
                if (foc) {
                    GFX_HOOK_ALIGN(0, 0, 0, 18, AL_V, "song view row text on its middle");
                    cv_text_r(SV_W - 4, CAP_IN(S, 18), &AF_S, "FOCUS", T_ACCENT, bg);
                }
                cv_rect(96, 7, SV_MW, 5, T_LINE);
                cv_blit(SV_X0, (uint32_t)y);
                sv.dot[i] = 0xFFu;                      /* (the dot and the meter over it again) */
                sv.meter[i] = 0;
                sv_meter(i, 0, SV_MW);
            }
        }
        /* the dot: lit by a note, dark again after SV_DOT_MS (muted: dark) */
        if ((act >> i) & 1u) {
            sv.dot_ms[i] = now;
            sv.lit |= (uint8_t)(1u << i);
        }
        if (((sv.lit >> i) & 1u) && now - sv.dot_ms[i] >= SV_DOT_MS)
            sv.lit &= (uint8_t)~(1u << i);
        d = !mute && ((sv.lit >> i) & 1u);
        if (sv.dot[i] != d) {
            sv.dot[i] = (uint8_t)d;
            sv_dot(i);
        }
        /* the meter: ~30 Hz, the pixels that moved (2 or more); it falls 3 px a step */
        if (meters) {
            uint32_t m = mute ? 0u : sv_meter_px(t->peak), was = sv.meter[i];
            ((track_t *)t)->peak = 0;
            if (m + 3u < was)
                m = was - 3u;
            else if (m < was)
                m = was;
            if (m > was + 1u || m + 2u < was || (!m && was)) {
                sv.meter[i] = (uint8_t)m;
                sv_meter(i, m < was ? m : was, m < was ? was : m);
            }
        }
    }
}
/* the scenes row's text: D4 held the faders' track ("FADER T1 (D4)"), else the last message, SAVE X?, the state */
static const char *sv_state(char *b)                    /* (b: 16 bytes) */
{
    if (song_d4()) {
        str_cpy(b, "FADER T1 (D4)", 16);
        b[7] = (char)('1' + song_ftrack());
        return b;
    }
    if (ui.msg_t && ui.msg[0])
        return ui.msg;
    if (sg.pend) {
        str_cpy(b, "SAVE X? OCT+", 16);
        b[5] = (char)('A' + sg.pend - 1u);
        return b;
    }
    return sg.stage == SG_WRITTEN ? "SAVED A-D" : "NOT SAVED YET";
}
static void sv_scenes(void)                             /* SC [1] [2], then the state or the last message */
{
    char q[16];
    uint32_t fad = song_d4();
    const char *s = sv_state(q);
    uint32_t fl = sg.ov == SOV_SCENE || sg.ov == SOV_SCENE2 ? sg.ov - SOV_SCENE + 1u : 0u, i;
    if (!sv_dirty(SV_SCN, str_hash(sg.scenes + fl * 4u + ux.gen * 16u + (ui.msg_t && ui.msg[0]) * 64u + fad * 128u, s)))
        return;
    cv_begin(240, 16, T_BG);
    GFX_HOOK_ALIGN(0, 0, 0, 16, AL_V, "song view scenes on their middle");
    cv_text_on(8, CAP_IN(S, 16), &AF_S, "SC", T_MID, T_BG);
    for (i = 0; i < 2u; i++) {
        int32_t x = 28 + 24 * (int32_t)i;
        char b[2] = {(char)('1' + i), 0};
        uint32_t on = (sg.scenes >> i) & 1u;
        uint16_t f = fl == i + 1u ? T_ACCENT : on ? T_THEME : T_BG;
        cv_rrect(x, 0, 20, 16, 3, f, T_BG);
        if (!on && fl != i + 1u)
            cv_frame(x, 0, 20, 16, T_LINE);
        GFX_HOOK_ALIGN(x, 0, x + 20, 16, AL_HV, "song view scene slot centred");
        cv_text_in(x, CAP_IN(S, 16), 20, &AF_S, b, on || fl == i + 1u ? T_INK : T_MID, f);
    }
    GFX_HOOK_ALIGN(0, 0, 0, 16, AL_V, "song view scenes on their middle");
    {
        int32_t w = text_w(&AF_S, s), x = 232 - (w < 150 ? w : 150);
        cv_free_text(x, CAP_IN(S, 16), &AF_S, s, fad || sg.pend ? T_ACCENT : ui.msg_t && ui.msg[0] ? T_TEXT : T_MID, T_BG,
                     150);
    }
    cv_blit(0, 197);
}
/* the four cards: name and value, a bar under them */
static void sv_card_txt(uint32_t k, const char **l, char *v, int32_t *r)
{
    static char gb[8];
    if (song_fx_btn()) {                                /* FX held: the macros */
        static const char *const N[4] = {"FILT", "CRSH", "THRW", "DPTH"};
        int32_t x = perf_k[k];
        *l = N[k];
        fmt_int(v, (uint32_t)(x < 0 ? -x : x));
        *r = k ? x * 10 : (x + 100) * 5;
        if (k == 3u)
            *r = (100 - x) * 10;
        return;
    }
    if (sg.focus || song_d4()) {                        /* D4 held or FOCUS: the track's */
        uint32_t i = song_ftrack();
        uint8_t m[4][2];
        song_fmap(i, m);
        *l = song_fname(i, k);
        if (m[k][0] == SG_NONE) {
            v[0] = 0;
            *r = -1;
        } else {
            const param_desc_t *d = track_desc(&trk[i], m[k][0]);
            const char *u;
            int32_t x = trk[i].p[m[k][0]];
            param_format(d, x, v, &u);
            *r = d->max > d->min ? (x - d->min) * 1000 / (d->max - d->min) : 0;
        }
        return;
    }
    switch (k) {                                        /* SEED DENSITY EVOLVE GENRE */
    case 0: *l = "SEED"; fmt_int(v, fg.seed); *r = fg.seed * 1000 / 99; break;
    case 1: *l = "DEN"; fmt_int(v, fs.dens); *r = (fs.dens - 50) * 10; break;
    case 2: *l = "EVOL"; fmt_int(v, fg.evolve); *r = fg.evolve * 250; break;
    default:
        *l = "";
        str_cpy(gb, FS_GENRE[fs.genre % FS_NGENRE].card, sizeof gb);
        str_cpy(v, gb, 8);
        *r = fs.genre * 1000 / (FS_NGENRE - 1u);
        break;
    }
}
static void sv_cards(void)
{
    uint32_t k, sig = ux.gen * 7u + song_fx_btn() * 3u + song_d4() * 5u + sg.focus * 11u;
    const char *l[4];
    char v[4][8];
    int32_t r[4];
    for (k = 0; k < 4u; k++) {
        sv_card_txt(k, &l[k], v[k], &r[k]);
        sig = str_hash(sig * 31u + (uint32_t)r[k], v[k]) + str_hash(k, l[k]);
    }
    if (!sv_dirty(SV_CARDS, sig))
        return;
    cv_begin(240, 24, T_BG);
    for (k = 0; k < 4u; k++) {
        int32_t x = SV_X0 + 59 * (int32_t)k, w = 55, xv;
        uint16_t f = T_SURF;
        cv_rrect(x, 0, w, 24, 3, f, T_BG);
        xv = x + 3;
        if (l[k][0]) {
            GFX_HOOK_ALIGN(0, 0, 0, 17, AL_V, "song view card text on its middle");
            xv = cv_text_on(x + 3, CAP_IN(S, 17), &AF_S, l[k], T_MID, f) + 3;
        }
        if (v[k][0]) {
            GFX_HOOK_ALIGN(0, 0, 0, 17, AL_V, "song view card text on its middle");
            cv_free_text(xv, CAP_IN(S, 17), &AF_S, v[k], T_TEXT, f, x + w - 2 - xv);
        }
        cv_rect(x + 3, 18, w - 6, 3, T_LINE);
        if (r[k] > 0)
            cv_rect(x + 3, 18, (w - 6) * clamp(r[k], 0, 1000) / 1000, 3, T_THEME);
    }
    cv_blit(0, 216);
}

/* the box over the middle: what changed, large */
static void sv_ov_text(char *t, char *big, char *sub)
{
    t[0] = big[0] = sub[0] = 0;
    switch (sg.ov) {
    case SOV_TEMPO:
        str_cpy(t, song.g[G_CLOCK] ? "TEMPO (EXT)" : "TEMPO", 16);
        fmt_int(big, (uint32_t)song.g[G_BPM]);
        str_cpy(sub, "FX+SELECT +-5  F5 TAP", 28);
        break;
    case SOV_KEY:
        str_cpy(t, sg.kpend ? "KEY > NEXT BAR" : "KEY", 16);
        song_key_name(big, 24);
        str_cpy(sub, "FX+ALGO: SCALE", 28);
        break;
    case SOV_PRESET: {
        uint32_t i = sg.ppend ? sg.ppend - 1u : song_ftrack(), k = sg.ppend ? sg.pk : trk[i].preset, n;
        const track_t *tt = &trk[i];
        const engine_t *e = ENGINES[tt->eng_req % NENGINES];
        char a[6];
        t[0] = 'T';
        t[1] = (char)('1' + i);
        str_cpy(t + 2, " PRESET ", 16 - 2u);
        n = song_pre_rank(tt, k) + 1u;
        fmt_int(a, n);
        str_cpy(t + str_len(t), a, 16 - str_len(t));
        str_cpy(t + str_len(t), "/", 16 - str_len(t));
        fmt_int(a, song_pre_n(tt));
        str_cpy(t + str_len(t), a, 16 - str_len(t));
        str_cpy(big, e->npresets ? e->presets[k % e->npresets].name : "-", 24);
        str_cpy(sub, sg.ppend ? "AT THE BAR  OCT-: CANCEL" : "LOADED", 28);
        break;
    }
    case SOV_SCENE: case SOV_SCENE2: {
        uint32_t i;
        str_cpy(big, sg.ov == SOV_SCENE ? "SCENE 1" : "SCENE 2", 24);
        str_cpy(sub, sg.ov_skip ? "" : "AT THE NEXT 1/16", 28);
        for (i = 0; i < NTRK; i++)
            if ((sg.ov_skip >> i) & 1u) {
                char n[4] = {'T', (char)('1' + i), ' ', 0};
                str_cpy(sub + str_len(sub), n, 28 - str_len(sub));
            }
        if (sg.ov_skip)
            str_cpy(sub + str_len(sub), "SKIPPED (SOUND)", 28 - str_len(sub));
        break;
    }
    case SOV_FAV: {
        uint32_t i = sg.fpend ? sg.ftrk : song_ftrack();
        char n[8] = "FAV 1";
        n[4] = (char)('0' + (sg.fpend ? sg.fpend : 1u));
        str_cpy(big, n, 24);
        str_cpy(sub, "> T1", 28);
        sub[3] = (char)('1' + i);
        str_cpy(sub + 4, sg.fpend ? " AT THE BAR" : "", 24);
        break;
    }
    default:
        break;
    }
}
static void sv_overlay(void)
{
    char t[16], big[24], sub[28];
    sv_ov_text(t, big, sub);
    if (!sv_dirty(SV_OV, str_hash(str_hash(str_hash(sg.ov + ux.gen * 7u, t), big), sub)))
        return;
    cv_begin(216, 130, T_BG);
    cv_rrect(0, 0, 216, 130, 8, T_THEME, T_BG);
    cv_rrect(2, 2, 212, 126, 7, T_SURF, T_THEME);
    if (t[0]) {
        GFX_HOOK_ALIGN(0, 8, 216, 28, AL_HV, "song view box title centred");
        cv_text_in(0, 8 + CAP_IN(M, 20), 216, &AF_M, t, T_MID, T_SURF);
    }
    GFX_HOOK_ALIGN(0, 40, 216, 80, AL_HV, "song view box value centred");
    if (sv_large_ok(big) && text_w(&AF_L, big) <= 204)
        cv_text_in(0, 40 + CAP_IN(L, 40), 216, &AF_L, big, T_TEXT, T_SURF);
    else
        cv_text_in(0, 40 + CAP_IN(M, 40), 216, &AF_M, big, T_TEXT, T_SURF);
    if (sub[0]) {
        GFX_HOOK_ALIGN(0, 96, 216, 116, AL_HV, "song view box note centred");
        cv_text_in(0, 96 + CAP_IN(S, 20), 216, &AF_S, sub, T_THEME, T_SURF);
    }
    cv_blit(12, 46);
}

/* SONG's screen (ui_draw.c, instead of the layer's map): 1 when it is SONG's to draw */
static int song_view(void)
{
    uint32_t ov;
    if (!(ui.layer == LAYER_SEQ && fs.on && sg.on && sg.stage != SG_CONFIRM && !ui.menu && !ui.confirm)) {
        sv.shown = 0;
        return 0;
    }
    if (sv.gen != ux.gen || (ui.force && !song.playing)) {   /* (a palette or style, or stopped a redraw asked: */
        sv.gen = ux.gen;                                /* everything again; playing: by regions only) */
        sv.shown = 0;
    }
    if (sv_full())
        lcd_fill(0, 0, 240, 240, T_BG);
    sv_pos();
    sv_head();
    ov = sg.ov != SOV_NONE;
    if (ov) {
        sv_overlay();
    } else {
        if (sv.ov_was) {                                /* (the box closed: what it covered, again) */
            uint8_t s = sv.shown;
            sv.shown = 0;
            sv_parts();
            sv_bar();
            sv_chain();
            sv_strip();
            sv_rows();
            sv.shown = s;
        }
        sv_parts();
        sv_bar();
        sv_chain();
        sv_strip();
        sv_rows();
    }
    sv.ov_was = (uint8_t)ov;
    if (!ov)
        sv.sig[SV_OV] = 0;                              /* (the next box: drawn) */
    sv_scenes();
    sv_cards();
    sv.shown = 1;
    return 1;
}
