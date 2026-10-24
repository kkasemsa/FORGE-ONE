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
 * 0.16 (spec v0.7): every action opens a box over 64..173 (song_pop: the context, the action large, a value bar or
 * "@ NEXT BAR"; refused: framed red) for SG_POPUP_MS after the last one (while a change waits for its bar: until then),
 * the next replacing it at once; what it covers is drawn again when it closes. D4 held: MIXER, large, over the head;
 * CLICK on: CLK in the head, the BAR row filled on beat 1 and framed on the others (SV_FLASH_MS); a part not as its
 * slot: an orange dot on its block; CHAIN on: CHAIN n/N where the state was (NOT SAVED YET is gone).
 *
 * Drawn by regions: each its own signature, redrawn when that changes (never the whole screen while it shows, but on
 * entering it or a palette / style change). Per 1/16: the STEP digits, the two playhead cells; the meters at most
 * ~30 Hz, only the pixels that moved; the dots on a note and when they fade. Nothing in the audio ISR but one OR
 * per note (voice.c trk_act) */

enum { SV_HEAD, SV_PARTS, SV_BAR, SV_STEPN, SV_CHAIN, SV_SCN, SV_CARDS, SV_OV, SV_N };
static struct {
    uint8_t shown, ov_was, meter[NTRK], dot[NTRK], ph, lit, beat, strong;
    uint32_t gen, sig[SV_N], dot_ms[NTRK], meter_ms, fl_ms;
    /* what the screen says (tests/forge_test.c reads it): BAR bar / bars, STEP step / len, the playhead cell
     * (0xFF stopped), the position bar's fill (px of SV_CHW) */
    uint32_t bar, bars, step, len, chain_px;
} sv;
#define SV_X0 4
#define SV_W 232
#define SV_CHW 232
#define SV_DOT_MS 120u
#define SV_MW 66                                        /* a meter's width */
#define SV_FLASH_MS 80u                                 /* 0.16 CLICK: a beat's flash */
#define SV_BOX_X 20                                     /* 0.16 (spec v0.7 §A): the action box, centred */
#define SV_BOX_Y 64
#define SV_BOX_W 200
#define SV_BOX_H 110

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
    if (song.playing && idx % 4u == 0u && sv.beat != (uint8_t)(idx + 1u)) {   /* (0.16 CLICK: a beat begins) */
        sv.fl_ms = fm1_ms;
        sv.strong = (uint8_t)(idx % 16u == 0u);
    }
    sv.beat = (uint8_t)(song.playing ? idx + 1u : 0u);
}
/* 0.16 CLICK (spec v0.7 §4): 0 none, 1 a beat (the BAR row framed), 2 beat 1 (the BAR row filled), SV_FLASH_MS long */
static uint32_t sv_flash(void)
{
    if (!sg.click || !song.playing || fm1_ms - sv.fl_ms >= SV_FLASH_MS)
        return 0;
    return sv.strong ? 2u : 1u;
}

static void sv_head(void)                               /* 128 BPM .. A MIN */
{
    char b[8], k[16];
    uint32_t pend = sg.kpend;
    fmt_int(b, (uint32_t)song.g[G_BPM]);
    song_key_name(k, sizeof k);
    if (!sv_dirty(SV_HEAD, str_hash(pend + 3u * (uint32_t)song.g[G_CLOCK] + 7u * sg.click + 17u * song_d4(), b) * 31u +
                               str_hash(1u, k) + ux.gen))
        return;
    if (song_d4()) {                                    /* (0.16 spec v0.7 §2.1: D4 held, the mixer: large) */
        cv_begin(240, 36, T_TINT);
        GFX_HOOK_ALIGN(0, 0, 240, 36, AL_HV, "song view mixer head centred");
        cv_text_in(0, CAP_IN(L, 36), 240, &AF_L, "MIXER", T_ACCENT, T_TINT);
        cv_blit(0, 0);
        return;
    }
    cv_begin(240, 36, T_BG);
    GFX_HOOK_ALIGN(0, 0, 0, 36, AL_V, "song view head on its middle");
    {
        int32_t x = cv_text_on(8, CAP_IN(L, 36), &AF_L, b, T_TEXT, T_BG);
        x = cv_text_on(x + 3, CAP_IN(L, 36) + AF_L_CAP_Y + AF_L_CAP_H - AF_S_CAP_Y - AF_S_CAP_H, &AF_S,
                       song.g[G_CLOCK] ? "EXT" : "BPM", T_MID, T_BG);
        if (sg.click)                                   /* (0.16: the visual CLICK on) */
            cv_text_on(x + 6, CAP_IN(L, 36) + AF_L_CAP_Y + AF_L_CAP_H - AF_S_CAP_Y - AF_S_CAP_H, &AF_S, "CLK",
                       T_THEME, T_BG);
    }
    GFX_HOOK_ALIGN(0, 0, 0, 36, AL_V, "song view head on its middle");
    cv_text_r(232, CAP_IN(L, 36), sv_large_ok(k) ? &AF_L : &AF_M, k, pend ? T_ACCENT : T_THEME, T_BG);
    cv_blit(0, 0);
}
static void sv_parts(void)
{
    uint32_t i, blink = sg.pend && ((fm1_ms / 300u) & 1u);
    if (!sv_dirty(SV_PARTS, 7u + sg.part * 13u + sg.pend * 131u + blink * 1031u + sg.dirty * 4099u + ux.gen))
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
        if ((sg.dirty >> i) & 1u)                       /* (0.16 spec v0.7 §5: not as its slot) */
            cv_rrect(x + 55 - 11, 3, 8, 8, 4, T_ACCENT, f);
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
    if (sv_dirty(SV_BAR, str_hash(sv.len + 7919u * sv_flash(), b) + ux.gen)) {
        uint32_t fl = sv_flash();
        uint16_t bg = fl == 2u ? T_ACCENT : T_BG, tx = fl == 2u ? T_INK : T_TEXT, md = fl == 2u ? T_INK : T_MID;
        cv_begin(240, 30, bg);
        if (fl == 1u)
            cv_frame(0, 0, 240, 30, T_ACCENT);
        GFX_HOOK_ALIGN(0, 0, 0, 30, AL_V, "song view bar on its middle");
        cv_text_on(8, CAP_IN(L, 30), &AF_L, b, tx, bg);
        GFX_HOOK_ALIGN(0, 0, 0, 30, AL_V, "song view bar on its middle");
        cv_text_on(150, CAP_IN(S, 30), &AF_S, "STEP", md, bg);
        b[0] = '/';
        fmt_int(b + 1, sv.len);
        GFX_HOOK_ALIGN(0, 0, 0, 30, AL_V, "song view bar on its middle");
        cv_text_r(232, CAP_IN(M, 30), &AF_M, b, md, bg);
        cv_blit(0, 62);
        sv.sig[SV_STEPN] = 0;                           /* (its digits on top again) */
    }
    sv_num2(n, sv.step > 99u ? 99u : sv.step);
    if (sv_dirty(SV_STEPN, str_hash(sv.len, n) + ux.gen + 1u)) {   /* the digits alone: each 1/16 */
        int32_t w;
        b[0] = '/';                                     /* (left of "/LEN": b held BAR n/n when its region did */
        fmt_int(b + 1, sv.len);                         /* not change, so the digits landed on STEP: device 0.15) */
        w = text_w(&AF_M, b);
        uint16_t bg = sv_flash() == 2u ? T_ACCENT : T_BG;
        cv_begin(22, 28, bg);
        GFX_HOOK_ALIGN(0, -1, 0, 29, AL_V, "song view bar on its middle");
        cv_text_r(22, CAP_IN(M, 30) - 1, &AF_M, n, sv_flash() == 2u ? T_INK : T_TEXT, bg);
        cv_blit((uint32_t)(232 - w - 22), 63);
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
static void sv_rows(uint32_t from)                     /* the rows from track from (the box over the others) */
{
    uint32_t i, now = fm1_ms, act = trk_act, meters = now - sv.meter_ms >= 33u;
    trk_act = 0;                                        /* (the ISR's bits: taken; one lost to a race only re-lights) */
    if (meters)
        sv.meter_ms = now;
    for (i = from; i < NTRK; i++) {
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
static const char *sv_state(char *b)                    /* (b: 20 bytes) */
{
    if (song_d4()) {
        str_cpy(b, "K1-K4: T1-T4 LEVEL", 20);
        return b;
    }
    if (ui.msg_t && ui.msg[0])
        return ui.msg;
    if (sg.pend) {
        str_cpy(b, "SAVE X? OCT+", 16);
        b[5] = (char)('A' + sg.pend - 1u);
        return b;
    }
    if (sg.chain) {                                     /* (0.16: CHAIN 4/8, the bar in the part / its bars) */
        char n[4];
        str_cpy(b, "CHAIN ", 20);
        fmt_int(n, (uint32_t)sg.cbar % 16u + 1u);
        str_cpy(b + str_len(b), n, 20 - str_len(b));
        str_cpy(b + str_len(b), "/", 20 - str_len(b));
        fmt_int(n, SG_CBARS[sg.cbars[sg.part] % NELEM(SG_CBARS)]);
        str_cpy(b + str_len(b), n, 20 - str_len(b));
        return b;
    }
    return "";                                          /* (0.16 spec v0.7 §5: the parts' dots say it) */
}
static void sv_scenes(void)                             /* SC [1] [2], then the state or the last message */
{
    char q[20];
    uint32_t fad = song_d4();
    const char *s = sv_state(q);
    uint32_t fl = sg.ov == SOV_SCENE || sg.ov == SOV_SCENE2 ? sg.ov - SOV_SCENE + 1u : 0u, i;
    if (!sv_dirty(SV_SCN, str_hash(sg.scenes + fl * 4u + ux.gen * 16u + (ui.msg_t && ui.msg[0]) * 64u + fad * 128u +
                                       sg.chain * 256u, s)))
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
        if (s[0])
            cv_free_text(x, CAP_IN(S, 16), &AF_S, s, fad || sg.pend ? T_ACCENT : ui.msg_t && ui.msg[0] ? T_TEXT : T_MID,
                         T_BG, 150);
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
    if (song_d4()) {                                    /* D4 held: the mixer, T1..T4 LEVEL */
        const char *u;
        static const char *const T[4] = {"T1", "T2", "T3", "T4"};
        *l = T[k];
        param_format(&TP[P_LEVEL], trk[k].p[P_LEVEL], v, &u);
        *r = trk[k].p[P_LEVEL] * 1000 / (TP[P_LEVEL].max ? TP[P_LEVEL].max : 1);
        return;
    }
    if (sg.focus) {                                     /* FOCUS: the track's */
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

/* 0.16 (spec v0.7 §A): the action box over the middle, as song_pop left it: the context small, the action large, a
 * value bar or "@ NEXT BAR" under it; refused: a red frame. Drawn when it changes (each new action: at once) */
static void sv_overlay(void)
{
    uint16_t fr = sg_pop.red ? T_REC : T_ACCENT;
    const aafont_t *f;
    if (!sv_dirty(SV_OV, str_hash(str_hash(str_hash(sg_pop.n * 7u + (uint32_t)sg_pop.bar * 131u + sg_pop.red + ux.gen * 977u,
                                                    sg_pop.sm), sg_pop.big), sg_pop.note)))
        return;
    cv_begin(SV_BOX_W, SV_BOX_H, T_BG);
    cv_rrect(0, 0, SV_BOX_W, SV_BOX_H, 8, fr, T_BG);
    cv_rrect(2, 2, SV_BOX_W - 4, SV_BOX_H - 4, 7, T_SURF, fr);
    if (sg_pop.sm[0]) {
        GFX_HOOK_ALIGN(0, 8, SV_BOX_W, 30, AL_HV, "song view box title centred");
        cv_text_in(0, 8 + CAP_IN(M, 22), SV_BOX_W, &AF_M, sg_pop.sm, sg_pop.red ? T_REC : T_MID, T_SURF);
    }
    f = sv_large_ok(sg_pop.big) && text_w(&AF_L, sg_pop.big) <= SV_BOX_W - 12 ? &AF_L : &AF_M;
    GFX_HOOK_ALIGN(0, 34, SV_BOX_W, 74, AL_HV, "song view box value centred");
    if (f == &AF_L)
        cv_text_in(0, 34 + CAP_IN(L, 40), SV_BOX_W, &AF_L, sg_pop.big, T_TEXT, T_SURF);
    else if (text_w(&AF_M, sg_pop.big) <= SV_BOX_W - 12)
        cv_text_in(0, 34 + CAP_IN(M, 40), SV_BOX_W, &AF_M, sg_pop.big, T_TEXT, T_SURF);
    else
        cv_free_text(6, 34 + CAP_IN(M, 40), &AF_M, sg_pop.big, T_TEXT, T_SURF, SV_BOX_W - 12);
    if (sg_pop.note[0]) {
        GFX_HOOK_ALIGN(0, 80, SV_BOX_W, 100, AL_HV, "song view box note centred");
        cv_text_in(0, 80 + CAP_IN(S, 20), SV_BOX_W, &AF_S, sg_pop.note, T_ACCENT, T_SURF);
    } else if (sg_pop.bar >= 0) {
        cv_rect(16, 88, SV_BOX_W - 32, 5, T_LINE);
        cv_rect(16, 88, (SV_BOX_W - 32) * sg_pop.bar / 1000, 5, T_THEME);
    }
    cv_blit(SV_BOX_X, SV_BOX_Y);
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
    sv_parts();
    if (ov) {
        sv_overlay();
        sv_rows(3);                                     /* (T4's row: under the box) */
    } else {
        if (sv.ov_was) {                                /* (the box closed: what it covered, again) */
            uint8_t s = sv.shown;
            sv.shown = 0;
            sv_bar();
            sv_chain();
            sv_strip();
            sv_rows(0);
            sv.shown = s;
        }
        sv_bar();
        sv_chain();
        sv_strip();
        sv_rows(0);
    }
    sv.ov_was = (uint8_t)ov;
    if (!ov)
        sv.sig[SV_OV] = 0;                              /* (the next box: drawn) */
    sv_scenes();
    sv_cards();
    sv.shown = 1;
    return 1;
}
