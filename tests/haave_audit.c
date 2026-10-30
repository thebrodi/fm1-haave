/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Dominik Siemon */
/* HAAVE's audit: every world, each of its sounds alone, into OUT_DIR/aNN.raw (float L/R, 44.1 kHz), AUDIT_MS each, one
 * after another: the layers PAD BASS VOX ARP MELODY SHIMMER (the music playing, TIDE STILL), SYNTH (two keys held),
 * BELLS (three keys struck), the GONG (BELLS' first key), BASS (a key held), then the world's six sprinkles.
 * tests/haave_audit.py reads them: each one's level on headphones and through a small speaker, beside the pad.
 *   cc -O2 -ffp-contract=off -w -Ibuild/gen -Ifirmware/src -Iweb/emu -o build/haave_audit tests/haave_audit.c -lm
 *   build/haave_audit OUT_DIR && python3 tests/haave_audit.py OUT_DIR */
#include "felucca_web.c"

#define AUDIT_MS 5000u
static const uint8_t PLAYK[9] = {12, 14, 16, 18, 19, 21, 23, 24, 26};   /* the instrument's nine (white keys 8..16) */
static const uint8_t BLACK[11] = {1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25};
static FILE *rec;                        /* (while a sound is taken) */

static void run(uint32_t ms)
{
    uint32_t k, i;
    for (k = 0; k < ms; k++) {
        web_render(44);
        if (rec)
            for (i = 0; i < 44u; i++) {
                float x[2] = {out_l[i], out_r[i]};
                fwrite(x, sizeof x, 1, rec);
            }
    }
}
static void tap(uint32_t b) { web_buttons(1u << b); run(40); web_buttons(0); run(40); }
static void quiet(void) { haave_layers = 0; haave_voice_held(); web_keys(0); run(4000); }

int main(int argc, char **argv)
{
    static const uint8_t LAYER[6] = {DL_PAD, DL_BASS, DL_CHOIR, DL_ARP, DL_MELODY, DL_SHIMMER};
    char path[600];
    uint32_t w, i;
    FILE *out;
    if (argc != 2) {
        fprintf(stderr, "usage: haave_audit OUT_DIR\n");
        return 2;
    }
    web_nor_erase();
    web_boot();
    run(2500);
    haave_input();
    tap(B_PLAY);
    for (w = 0; w < HAAVE_NWORLDS; w++) {
        haave_load(w);
        haave_tide = DT_STILL;
        quiet();
        snprintf(path, sizeof path, "%s/a%02u.raw", argv[1], w);
        out = fopen(path, "wb");
        if (!out) {
            perror(path);
            return 1;
        }
        for (i = 0; i < 6u; i++) {                   /* the layers, one at a time */
            haave_layers = (uint8_t)(1u << LAYER[i]);
            haave_voice_held();
            run(2500);
            rec = out;
            run(AUDIT_MS);
            rec = 0;
            quiet();
        }
        tap(B_SAVE);                                 /* SYNTH: two keys held */
        run(2000);
        rec = out;
        web_keys(1u << PLAYK[2] | 1u << PLAYK[4]);
        run(AUDIT_MS);
        web_keys(0);
        rec = 0;
        run(2000);
        tap(B_ARP);                                  /* BELLS: three keys struck */
        run(2000);
        rec = out;
        for (i = 0; i < 3u; i++) {
            web_keys(1u << PLAYK[2u + 2u * i]);
            run(150);
            web_keys(0);
            run(1500);
        }
        run(AUDIT_MS - 3u * 1650u);
        rec = 0;
        run(3000);
        rec = out;                                   /* the GONG */
        web_keys(1u << PLAYK[0]);
        run(150);
        web_keys(0);
        run(AUDIT_MS - 150u);
        rec = 0;
        run(3000);
        tap(B_SEQ);                                  /* BASS: a key held */
        run(2000);
        rec = out;
        web_keys(1u << PLAYK[2]);
        run(AUDIT_MS);
        web_keys(0);
        rec = 0;
        run(1000);
        tap(B_ARP);
        run(2500);
        for (i = 0; i < 6u; i++) {                   /* the sprinkles */
            rec = out;
            web_keys(1u << BLACK[5u + i]);
            run(60);
            web_keys(0);
            run(AUDIT_MS - 60u);
            rec = 0;
            run(1500);
        }
        fclose(out);
    }
    printf("audit: %s\n", argv[1]);
    return 0;
}
