/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the FORGE ONE contributors */
/* FORGE ONE 0.7: PERFORM, the FX layer locked (a double tap of FX, #83's lock). Included by ui_layer.c.
 *
 * Held, FX is as it was (the effects while held, black keys 1..4 mutes while held). Locked, it is PERFORM: both hands
 * free, everything a live set needs on one screen:
 *   white keys   the FX map's effects, as always (held, or toggled with MENU > FX LATCH), incl. 0.7's CUTOFF THROW (F5)
 *                and DIST THROW (G5); the throws flow back over 0.74 s when let go (perform.c THR_REL)
 *   black 1..4   (F#3 G#3 A#3 C#4) T1..T4 MUTE, latched (P_MUTE, as GLO's); with OCT+ held: SOLO, latched (perf_psolo)
 *   black 5..8   (D#4 F#4 G#4 A#4) four scenes (mutes and solos): a tap recalls one, held PM_HOLD_MS stores it
 *   black 9      (C#5) QUANT: mutes, solos and scenes wait for the next bar while playing (seq.c pq_*, in the ISR: the
 *                bar's first step plays with them)
 *   black 10     (D#5) DUCK on / off (perform.c PF_DUCK: the tracks that are not DRUM pump on a DRUM track's kick)
 *   black 11     (F#5) CLEAR: every track sounding, no solo, every effect off (at once)
 *   KNOB 1..4    T1..T4 LEVEL (as GLO's); OCT+ tapped alone: the FX macros (FILTER CRUSH THROW DEPTH) and back
 *   OCT-         every effect and macro off (as FX LATCH's)
 * The scenes and QUANT are kept with the settings (favorites.factory[11][24..28]: NOISE has 4 presets, so
 * its favourites' bytes from 24 (presets 192..) are free; 0 in every older setting = no scene, QUANT off). A tap of FX
 * ends it; the mutes stay (they are the tracks'), so do the solos and DUCK until CLEAR (or GLO's C4 UNMUTE ALL) */

#define PM_HOLD_MS 600u
#define PM_ENG 11u                                       /* NOISE (engines.c ENGINES[11]: 4 presets; tests check) */
#define pm_store (&favorites.factory[PM_ENG][24])   /* [0..3] scenes: bits 0..3 mutes, 4..7 solos; [4] flags */
#define PM_STORED(i) ((pm_store[4] >> (i)) & 1u)
#define PM_QUANT 0x10u
typedef char pm_store_free[PM_ENG < 14u ? 1 : -1];   /* (not 14: MIDI LEARN, not 15: the settings) */
static struct {
    uint8_t knobs;                     /* 1: KNOB 1..4 are the FX macros, 0 the levels */
    uint8_t oct_used;                  /* a black key pressed while OCT+ was held (no knob switch on its release) */
    uint8_t sdown;                     /* scene keys down (bit i: black key 5 + i) */
    uint8_t sdone;                     /* .. their hold already stored the scene */
    uint8_t last;                      /* the scene recalled or stored last (+ 1, 0 none) */
    uint32_t st[4];                    /* .. when each went down (fm1_ms) */
} pm;

static int pm_on(void) { return ui.lock == LAYER_FX && ui.layer == LAYER_FX && layer_allowed(); }
static uint32_t pm_mutes(void)
{
    uint32_t i, m = 0;
    for (i = 0; i < NTRK; i++)
        m |= (trk[i].p[P_MUTE] != 0) << i;
    return m;
}
/* what the tracks will be: the pending change's, else as they are now */
static uint32_t pm_want_mute(void) { return pq_pend ? pq_mute : pm_mutes(); }
static uint32_t pm_want_solo(void) { return pq_pend ? pq_solo : perf_psolo; }
/* mutes m, solos s: at the next bar (QUANT, playing) or now */
static void pm_set(uint32_t m, uint32_t s)
{
    uint32_t i;
    if ((pm_store[4] & PM_QUANT) && song.playing) {
        pq_pend = 0;                                    /* (the ISR never sees half of it) */
        pq_mute = (uint8_t)(m & 15u);
        pq_solo = (uint8_t)(s & 15u);
        pq_pend = 1;
        return;
    }
    pq_pend = 0;
    for (i = 0; i < NTRK; i++)
        trk[i].p[P_MUTE] = (int16_t)((m >> i) & 1u);
    perf_psolo = (uint8_t)(s & 15u);
}
static void pm_clear(void)                              /* (CLEAR: at once, QUANT or not) */
{
    uint32_t i;
    pq_pend = 0;
    for (i = 0; i < NTRK; i++)
        trk[i].p[P_MUTE] = 0;
    perf_psolo = 0;
    perf_latched = 0;
    perf_k[0] = perf_k[1] = perf_k[2] = perf_k[3] = 0;
    ui_message("CLEAR: ALL ON");
}

/* a black key of PERFORM (place p: 0 = F#3) */
static void pm_key(uint32_t p)
{
    if (p < NTRK) {
        uint32_t solo = (fm1_in.buttons >> panel.btn[B_OCTUP]) & 1u;
        if (solo) {
            pm.oct_used = 1;
            pm_set(pm_want_mute(), pm_want_solo() ^ (1u << p));
        } else {
            pm_set(pm_want_mute() ^ (1u << p), pm_want_solo());
        }
    } else if (p < NTRK + 4u) {                         /* a scene: on its release (a tap) or its hold (pm_poll) */
        pm.sdown |= (uint8_t)(1u << (p - NTRK));
        pm.sdone &= (uint8_t)~(1u << (p - NTRK));
        pm.st[p - NTRK] = fm1_ms;
    } else if (p == 8u) {
        pm_store[4] ^= PM_QUANT;
        lys.rp_dirty = 1;
        ui_message(pm_store[4] & PM_QUANT ? "QUANT: NEXT BAR" : "QUANT OFF: AT ONCE");
    } else if (p == 9u) {
        perf_latched ^= PF_BIT(PF_DUCK);
        ui_message(perf_latched & PF_BIT(PF_DUCK) ? "DUCK ON" : "DUCK OFF");
    } else if (p == 10u) {
        pm_clear();
    }
}
/* each pass: the scene keys' holds and releases */
static void pm_poll(void)
{
    uint32_t i;
    char b[20], n[2] = {0, 0};
    for (i = 0; i < 4u; i++) {
        uint32_t bit = 1u << i, k, down = 0;
        if (!(pm.sdown & bit))
            continue;
        for (k = 0; k < 27u; k++)                       /* (the scene key still held?) */
            if (key_black(k) && key_place(k) == NTRK + i && ((kb_layer >> k) & 1u))
                down = 1;
        n[0] = (char)('1' + i);
        if (down && !(pm.sdone & bit) && fm1_ms - pm.st[i] >= PM_HOLD_MS) {
            pm_store[i] = (uint8_t)(pm_want_mute() | pm_want_solo() << 4);
            pm_store[4] |= (uint8_t)bit;
            pm.sdone |= (uint8_t)bit;
            pm.last = (uint8_t)(i + 1u);
            lys.rp_dirty = 1;
            str_cpy(b, "SCENE ", sizeof b);
            str_cpy(b + 6, n, sizeof b - 6);
            str_cpy(b + str_len(b), " STORED", sizeof b - str_len(b));
            ui_message(b);
        } else if (!down) {
            pm.sdown &= (uint8_t)~bit;
            if (!(pm.sdone & bit)) {                    /* a tap: recall */
                str_cpy(b, "SCENE ", sizeof b);
                str_cpy(b + 6, n, sizeof b - 6);
                if (PM_STORED(i)) {
                    pm_set(pm_store[i] & 15u, pm_store[i] >> 4);
                    pm.last = (uint8_t)(i + 1u);
                    str_cpy(b + str_len(b), pq_pend ? " AT THE BAR" : "", sizeof b - str_len(b));
                } else {
                    str_cpy(b + str_len(b), " EMPTY", sizeof b - str_len(b));
                }
                ui_message(b);
                if (!PM_STORED(i))
                    str_cpy(ui.msg2, "HOLD IT TO STORE", sizeof ui.msg2);
            }
        }
    }
}
/* OCT+ let go in PERFORM: alone, the knobs switch (levels / macros); after a solo, nothing */
static void pm_oct_up(void)
{
    if (pm.oct_used) {
        pm.oct_used = 0;
        return;
    }
    pm.knobs ^= 1u;
    ui_message(pm.knobs ? "KNOBS: FX MACROS" : "KNOBS: LEVELS");
}
static uint32_t pm_sig(void)
{
    return pm_on() ? 3u + pm.knobs * 7u + pm_store[4] * 131u + perf_psolo * 977u + pq_pend * 31u + pq_mute * 7919u +
                     pq_solo * 104729u + pm.last * 13u + pm.sdown * 17u : 0u;
}
/* the LEDs of the black keys in PERFORM: a track sounding lit (pending: breathing), a stored scene lit, QUANT and DUCK
 * on lit, CLEAR breathing */
static void pm_led(uint32_t p, uint32_t *on, uint32_t *can)
{
    uint32_t so = pm_want_solo();
    if (p < NTRK) {
        uint32_t snd = !((pm_mutes() >> p) & 1u) && (!perf_psolo || ((perf_psolo >> p) & 1u));
        uint32_t will = !((pm_want_mute() >> p) & 1u) && (!so || ((so >> p) & 1u));
        *on = snd && will;
        *can = snd != will;
    } else if (p < NTRK + 4u) {
        *on = PM_STORED(p - NTRK);
        *can = 1;
    } else {
        *on = (p == 8u && (pm_store[4] & PM_QUANT)) || (p == 9u && (perf_latched & PF_BIT(PF_DUCK)));
        *can = p <= 10u;
    }
}
