/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the Felucca-FORGE contributors */
/* Felucca-FORGE (firmware/src/ui_forge.c): SEQ TOOLS F5, on the real sequencer and UI sources (with the stubs of
 * tests/ui_test.c). Scaffold: F5 on a melodic track says FORGE and its version and leaves the steps as they were;
 * on a DRUM track it is not there. Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static void tkey(uint32_t w) { key_down(white(w)); frame(); key_up(white(w)); frame(); }

static track_t *melody(void)
{
    track_t *t;
    uint32_t i;
    ui_power_on();
    song.sel = 0;
    t = &trk[0];
    track_defaults_steps(t);
    motion_clear(t);
    t->p[P_SLEN] = 16;
    for (i = 0; i < 16u; i += 2u)
        t->step[i] = (step_t){{(uint8_t)(60u + i)}, 1, ST_NOTE, 0, 0};
    return t;
}

static int test_scaffold(void)
{
    int bad = 0;
    step_t ref[NSTEP];
    track_t *t = melody();
    memcpy(ref, t->step, sizeof ref);
    go_page(GR_ROLL); frame();
    btn_down(B_SEQ); frame();
    tkey(TL_FORGE);                                    /* F5 */
    bad += check("FORGE (F5) on a melodic track: its message, the steps untouched",
                 msg_is("FORGE " FORGE_VERSION) && !memcmp(t->step, ref, sizeof ref));
    btn_up(B_SEQ); frame();
    bad += check("FORGE: F5 (white key 14), there on a melodic track", tl_cell(t, TL_FORGE) && TL_FORGE == 14u);
    return bad;
}

int main(void)
{
    int bad = test_scaffold();
    printf("%s\n", bad ? "FORGE TEST FAILED" : "forge tests passed");
    return bad != 0;
}
