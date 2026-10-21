/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 the Felucca-FORGE contributors */
/* Felucca-FORGE: the SEQ TOOLS layer's F5 (TL_FORGE, ui_tools.c), a seeded motif generator for the selected melodic
 * track that writes only the step fields a project already holds (no format change). Included by ui_layer.c after
 * ui_tools.c. Scaffold: F5 says the version and changes nothing; the generator, its knobs (SEED DENSITY EVOLVE
 * STYLE) and its open / apply / cancel (as EUCLID's) come next. */
#define FORGE_VERSION "0.1"

static void forge_key(track_t *t)
{
    (void)t;
    ui_message("FORGE " FORGE_VERSION);
}
