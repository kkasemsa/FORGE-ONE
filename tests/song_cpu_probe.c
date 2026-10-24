/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the FORGE ONE contributors */
/* FORGE ONE 0.13 SONG: a host CPU proxy for spec v0.4.1 test 30 (the device's CPU / PEAK / LATE stay a device test).
 * A BREAK song (TRANCE 42) is generated, then played through its whole 8-row chain on the host, three ways: no
 * effect, REPEAT 1/16 held, REPEAT 1/16 + ECHO THROW held + DUCK on. Per way: the CPU time per 44.1 kHz sample (the
 * best of 5 runs, CLOCK_PROCESS_CPUTIME_ID, cc -O2 on x86-64: a relative figure, not the FM-1's), each track's peak
 * and the master samples at full scale. Not part of tests/run_tests.sh (timing). Build and run:
 *   cc -O2 -w -Ibuild/gen -Ifirmware/src -Itests -o /tmp/song_cpu tests/song_cpu_probe.c -lm -Wl,--defsym=main=cpu
 *   /tmp/song_cpu */
#include "forge_test.c"
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
/* a BREAK song (TRANCE 42) played from its chain on stock code; per configuration: CPU time per 44.1 kHz sample over
 * the whole song, the worst block, the output's peak */
static double run1(uint32_t fx, uint32_t duck, int32_t *pk, uint32_t *full)
{
    int32_t o[2 * CTL];
    uint32_t it, blocks = 0, k;
    double t0;
    project_load(SG_A);
    perf_held = fx; perf_latched = duck ? PF_BIT(PF_DUCK) : 0u; perf_kill = 0;
    chain_prepare();
    t0 = now();
    for (it = 0; it < 3000000u && (chain.running || chain.armed); it++) {
        mix_block(o, CTL);
        for (k = 0; k < NTRK; k++) { if (trk[k].peak > pk[k]) pk[k] = trk[k].peak; trk[k].peak = 0; }
        for (k = 0; k < 2u * CTL; k++) *full += o[k] >= 32700 || o[k] <= -32700;
        blocks++;
    }
    t0 = now() - t0;
    transport_req = 2; mix_block(o, CTL);
    perf_held = 0; perf_latched = 0;
    return t0 * 1e9 / ((double)blocks * CTL);
}
static void run(const char *label, uint32_t fx, uint32_t duck)
{
    int32_t pk[NTRK] = {0, 0, 0, 0};
    uint32_t full = 0, r;
    double best = 1e9;
    for (r = 0; r < 5u; r++) { double v = run1(fx, duck, pk, &full); if (v < best) best = v; }
    printf("%-38s %6.1f ns/sample (best of 5)  track peaks %5d %5d %5d %5d  full-scale samples %u\n", label, best,
           (int)pk[0], (int)pk[1], (int)pk[2], (int)pk[3], (unsigned)(full / 5u));
}
int cpu(void)
{
    sg_open((uint32_t)sg_genre("TRANCE"), 42);
    oct_fwd(); oct_fwd();
    sg_close();
    run("BREAK song, no FX", 0, 0);
    run("BREAK song + REPEAT 1/16", PF_BIT(PF_R16), 0);
    run("BREAK song + R16 + ECHO THROW + DUCK", PF_BIT(PF_R16) | PF_BIT(PF_ETHR), 1);
    return 0;
}
