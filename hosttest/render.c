/*
 * Host harness (v8): renders WAVs and measures the engine.
 *   ./render scales                                    the scale bank
 *   ./render one <seed> <style> <seconds> <out.wav>   one rolled patch
 *   ./render lim <seed> <style> <seconds>              its level and what the limiter did
 *   ./render sweep <n_seeds>                           every style, n seeds each: NaN, silence, level
 *   ./render rhythm <bars> [engine assign]             the drums' hits, bar by bar (K S H T at 48 places a bar)
 *   ./render rstat <bars>                              long-form statistics of a rolled rhythm (repeat, likeness, anchors)
 *   ./render gstat <out.csv>                           each motion's note loudnesses, 16 times a bar, 6 seeds
 *   ./render bank <out.wav>                            the oscillator bank: each control swept, then all moving
 *   ./render tour <out.wav>                            the audio tour
 */
#include "synth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

static Synth S;
#define BLOCK 128

static void wr32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void wr16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }
static void write_wav(const char *path, const float *L, const float *R, int n) {
    FILE *f = fopen(path, "wb"); if (!f) { perror(path); return; }
    int sr = (int)SR;
    fwrite("RIFF", 1, 4, f); wr32(f, 36 + n * 4); fwrite("WAVEfmt ", 1, 8, f);
    wr32(f, 16); wr16(f, 1); wr16(f, 2); wr32(f, sr); wr32(f, sr * 4); wr16(f, 4); wr16(f, 16);
    fwrite("data", 1, 4, f); wr32(f, n * 4);
    uint32_t r = 12345;
    for (int i = 0; i < n; i++) {
        float ch[2] = {L[i], R[i]};
        for (int c = 0; c < 2; c++) {
            float d = (frand(&r) - frand(&r)) / 32768.f;
            float v = ch[c] + d; if (v > 1) v = 1; if (v < -1) v = -1;
            int16_t q = (int16_t)lrintf(v * 32767.f); fwrite(&q, 2, 1, f);
        }
    }
    fclose(f);
}
typedef struct { float peak, rms, over; int nan; double ns; uint32_t clips; } Stats;
static void stats(const float *L, const float *R, int n, Stats *st) {
    double s2 = 0; float pk = 0; int nan = 0, ov = 0;
    for (int i = 0; i < n; i++) {
        if (L[i] != L[i] || R[i] != R[i]) { nan++; continue; }
        float a = fabsf(L[i]) > fabsf(R[i]) ? fabsf(L[i]) : fabsf(R[i]); if (a > pk) pk = a; if (a > .9f) ov++;
        s2 += L[i] * L[i] + R[i] * R[i];
    }
    st->peak = pk; st->rms = (float)sqrt(s2 / (2.0 * n)); st->nan = nan; st->over = (float)ov / (float)n;
}
typedef void (*Hook)(int block, int nblocks);
static float *g_L, *g_R;
static void render(float seconds, Hook hook, Stats *st, int skip_blocks) {
    int nb = (int)(seconds * SR / BLOCK), n = nb * BLOCK;
    g_L = realloc(g_L, sizeof(float) * n); g_R = realloc(g_R, sizeof(float) * n);
    uint32_t c0 = S.fx.clips;
    clock_t t0 = clock();
    for (int b = 0; b < nb; b++) {
        if (hook) hook(b, nb);
        if (S.build_req) synth_build_now(&S);
        synth_process(&S, g_L + b * BLOCK, g_R + b * BLOCK, BLOCK);
    }
    st->ns = (double)(clock() - t0) / CLOCKS_PER_SEC * 1e9 / n;
    int sk = skip_blocks * BLOCK; if (sk >= n) sk = 0;
    stats(g_L + sk, g_R + sk, n - sk, st);
    st->clips = S.fx.clips - c0;
}
static void setd(int id, int idx) { synth_set_param(&S, id, ((float)idx + .5f) / g_param_meta[id].hi); }
static void setr(int id, float real) {
    const ParamMeta *m = &g_param_meta[id];
    float v = m->curve == CURVE_LOG ? logf(real / m->lo) / logf(m->hi / m->lo) : (real - m->lo) / (m->hi - m->lo);
    synth_set_param(&S, id, v);
}
static void roll(uint32_t seed, int style) {
    synth_init(&S, seed); synth_set_style(&S, style); synth_randomize(&S, seed, 0);
    engine_build(&S); S.rebuild_pending = 0; S.duck = S.duck_tgt = 1.f;
}


static void pset(int id, float n) { synth_set_param(&S, id, n); }
static void go(float secs, float **L, float **R, int *n) {     /* append secs of audio to L, R */
    int nb = (int)(secs * SR / BLOCK); *L = realloc(*L, sizeof(float) * (size_t)(*n + nb * BLOCK)); *R = realloc(*R, sizeof(float) * (size_t)(*n + nb * BLOCK));
    for (int b = 0; b < nb; b++) { if (S.build_req) synth_build_now(&S); synth_process(&S, *L + *n, *R + *n, BLOCK); *n += BLOCK; }
}
/* the drums' hits over bars, as (bar, place 0..47, drum) rows: the rhythm tests read them */
typedef struct { int bar, pl, d; } Hit;
static int collect(int bars, Hit *h, int max) {
    static float L[SUB], R[SUB]; int n = 0; uint32_t b0 = S.meter.bar; float last[4] = {0, 0, 0, 0};
    while ((int)(S.meter.bar - b0) < bars) {
        synth_process(&S, L, R, SUB);
        for (int d = 0; d < 4; d++) { float f = S.dr.flash[d]; if (f > last[d] + .5f && n < max) { h[n].bar = (int)(S.meter.bar - b0); h[n].pl = ((int)(S.meter.tau * 48.f + .5f)) % 48; h[n].d = d; n++; } last[d] = f; }
    }
    return n;
}
int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: see the header\n"); return 1; }
    if (!strcmp(argv[1], "scales")) {                       /* the bank: each scale's notes (just ratios; cents from 12-TET) */
        static const char *const GN[4] = {"harmonic", "pentatonic", "modes", "other"}; int g = 0;
        for (int i = 0; i < scale_count(); i++) {
            Scale sc; scale_get(i, &sc);
            while (g < 4 && i == scale_group_start(g)) printf("-- %s\n", GN[g++]);
            printf("%2d %-26s %2d notes%s:", i, sc.name, sc.n, sc.series ? " (partials)" : "");
            for (int k = 0; k < sc.n && k < 12; k++) { if (sc.series) printf(" %g", sc.ratio[k]); else printf(" %d/%d", sc.num[k], sc.den[k]); }
            printf("\n");
        }
        return 0;
    }
    if (!strcmp(argv[1], "pitch") && argc >= 5) {          /* pitch <seed> <style> <out.wav> [clean]: scale 0, effects off; prints the nodes */
        roll((uint32_t)atoi(argv[2]), atoi(argv[3])); setd(P_SCALE, argc > 6 ? atoi(argv[6]) : 0); setd(P_DR_ON, 0); pset(P_RV_SEND, 0); pset(P_DL_SEND, 0); pset(P_FZ_MIX, 0); pset(P_CMP_MIX, 0);
        pset(P_FLT_CUT, 1); pset(P_FLT_RES, 0); pset(P_V2_LEVEL, 0);
        if (argc > 5 && atoi(argv[5])) { pset(P_V1_SPIN, 0); pset(P_V1_TONE, 0); pset(P_V1_WAVE, 0); for (int i = 0; i < MOD_SLOTS; i++) S.slot[i].src = 0; }
        engine_build(&S); Stats st; render(12.f, NULL, &st, 0); write_wav(argv[4], g_L, g_R, (int)(12.f * SR / BLOCK) * BLOCK);
        printf("spin %.2f cross %.2f wave %.2f temper %.2f root %.2f Hz reg %d\n", pval(&S, P_V1_SPIN), pval(&S, P_V1_TONE), pval(&S, P_V1_WAVE), pval(&S, P_TEMPER), 440. * pow(2, (floor(pval(&S, P_ROOT) + .5) - 69) / 12), pint(&S, P_V1_REG));
        for (int i = 0; i < S.eng.n_nodes; i++) printf("node %d %.3f Hz\n", i, S.eng.node[i].freq);
        return 0;
    }
    if (!strcmp(argv[1], "mix") && argc >= 4) {            /* mix <seed> <style> [layer]: the synth alone, with the drums, with the kick alone (levels) */
        for (int w = 0; w < 3; w++) {                          /* (the drums ride the synth's voices: they are heard with it) */
            roll((uint32_t)atoi(argv[2]), atoi(argv[3])); setd(P_DR_ON, w > 0); if (argc > 4) setd(P_DR_LAYER, atoi(argv[4]));
            if (w == 2) { pset(P_D2_LEVEL, 0); pset(P_D3_LEVEL, 0); pset(P_D4_LEVEL, 0); }
            Stats st; render(16.f, NULL, &st, (int)(2.f * SR / BLOCK));
            if (w > 0 && argc > 5) write_wav(argv[5], g_L, g_R, (int)(16.f * SR / BLOCK) * BLOCK);
            printf("%s: rms %.1f dBFS peak %.1f dBFS   ", w == 0 ? "synth" : w == 1 ? "+drums" : "+kick", 20 * log10f(st.rms + 1e-9f), 20 * log10f(st.peak + 1e-9f));
        }
        printf("| %s kick decay %.2f s attack %.1f ms bright %.2f snap %.2f\n", drum_layer_name(pint(&S, P_DR_LAYER)), pval(&S, P_D1_DECAY), 1000. * pval(&S, P_D1_ATTACK), pval(&S, P_D1_BRIGHT), pval(&S, P_D1_SNAP)); return 0;
    }
    if (!strcmp(argv[1], "tone") && argc >= 12) {   /* tone <out> <scale> <rank 0..1> <center> <band> <tilt> <wave> <spin> <cross> <phase> [bloom]: one held key, dry */
        roll(42, STYLE_AMBIENT); setd(P_SCALE, atoi(argv[3])); setd(P_DR_ON, 0); pset(P_RV_SEND, 0); pset(P_DL_SEND, 0); pset(P_FZ_MIX, 0); pset(P_CMP_MIX, 0);
        pset(P_FLT_CUT, 1); pset(P_FLT_RES, 0); pset(P_FLT_MORPH, 0); pset(P_V2_LEVEL, 0); pset(P_G1_CONTOUR, .5f);
        pset(P_V1_CENTER, atof(argv[5])); pset(P_V1_BAND, atof(argv[6])); pset(P_V1_TILT, atof(argv[7])); pset(P_V1_WAVE, atof(argv[8])); pset(P_V1_SPIN, atof(argv[9]));
        pset(P_V1_TONE, atof(argv[10])); pset(P_V1_PHASE, atof(argv[11])); pset(P_V1_STRIDE, 0); pset(P_V1_BLOOM, argc > 12 ? atof(argv[12]) : 0); pset(P_V1_WIDTH, 0);
        for (int i = 0; i < MOD_SLOTS; i++) S.slot[i].src = 0;
        for (int i = 0; i < NUM_PARAMS; i++) S.kpath[i][0] = S.kpath[i][1] = S.kpath[i][2] = S.kpath[i][3] = 0;
        S.transport = 0; engine_build(&S);
        int N = S.eng.n_nodes; static int ord[MAX_NODES];
        for (int i = 0; i < N; i++) { int k = i; while (k > 0 && S.eng.node[ord[k - 1]].lr > S.eng.node[i].lr) { ord[k] = ord[k - 1]; k--; } ord[k] = i; }
        int nd = ord[(int)(atof(argv[4]) * (N - 1) + .5)];
        float *L = 0, *R = 0; int n = 0; go(.2f, &L, &R, &n); synth_note(&S, nd, .9f, 1); go(4.f, &L, &R, &n);
        write_wav(argv[2], L, R, n); printf("%.4f %d\n", S.eng.node[nd].freq * exp2(pint(&S, P_V1_REG) - 1), N); return 0;
    }
    if (!strcmp(argv[1], "blocks") && argc >= 5) {         /* blocks <seed> <style> <seconds>: each 128-frame block's time (the least of 5 identical runs) */
        int nb = (int)(atof(argv[4]) * SR / 128); static float L[128], R[128]; double *tm = calloc((size_t)nb, sizeof(double));
        for (int run = 0; run < 5; run++) {
            roll((uint32_t)atoi(argv[2]), atoi(argv[3]));
            for (int b = 0; b < nb; b++) {
                struct timespec a, c; clock_gettime(CLOCK_MONOTONIC, &a); synth_process(&S, L, R, 128); clock_gettime(CLOCK_MONOTONIC, &c);
                double ns = (c.tv_sec - a.tv_sec) * 1e9 + (c.tv_nsec - a.tv_nsec); if (!run || ns < tm[b]) tm[b] = ns;
            }
        }
        double sum = 0, mx = 0; int at = 0; for (int b = 20; b < nb; b++) { sum += tm[b]; if (tm[b] > mx) { mx = tm[b]; at = b; } }
        double mean = sum / (nb - 20); int o15 = 0, o2 = 0, o3 = 0; for (int b = 20; b < nb; b++) { if (tm[b] > 1.5 * mean) o15++; if (tm[b] > 2 * mean) o2++; if (tm[b] > 3 * mean) o3++; }
        printf("mean %.0f ns a block, worst %.1fx the mean (block %d), blocks over 1.5x: %.2f%%, over 2x: %.2f%%, over 3x: %.3f%%; even/odd block mean ratio ", mean, mx / mean, at, 100. * o15 / nb, 100. * o2 / nb, 100. * o3 / nb);
        double e = 0, o = 0; for (int b = 20; b < nb - 1; b += 2) { e += tm[b]; o += tm[b + 1]; } printf("%.2f\n", e / o);
        return 0;
    }
    if (!strcmp(argv[1], "osc")) {                          /* oscillators rendered a block, over rolled patches of every style */
        for (int st_ = 1; st_ < NUM_STYLES; st_++) { double sum = 0; int mx = 0, cnt = 0;
            for (int sd = 0; sd < 6; sd++) { roll(300u + 17u * (uint32_t)sd + 1000u * (uint32_t)st_, st_); static float L[SUB], R[SUB];
                for (int b = 0; b < 12 * SR / SUB; b++) { synth_process(&S, L, R, SUB); if (b < 200) continue; int o = S.layer[0].n_osc + S.layer[1].n_osc; sum += o; cnt++; if (o > mx) mx = o; } }
            printf("style %d: oscillators sounding, both layers: mean %.0f, most %d\n", st_, sum / cnt, mx); }
        return 0;
    }
    if (!strcmp(argv[1], "one") && argc >= 6) {
        Stats st; roll((uint32_t)atoi(argv[2]), atoi(argv[3]));
        render((float)atof(argv[4]), NULL, &st, 0);
        write_wav(argv[5], g_L, g_R, (int)((float)atof(argv[4]) * SR / BLOCK) * BLOCK);
        char d[400]; synth_describe(&S, d, sizeof d);
        printf("peak %.3f rms %.4f (%.1f dBFS) clips %u nan %d %.0f ns/sample\n  %s\n", st.peak, st.rms, 20 * log10f(st.rms + 1e-9f), st.clips, st.nan, st.ns, d);
        return 0;
    }
    if (!strcmp(argv[1], "lim") && argc >= 5) {
        roll((uint32_t)atoi(argv[2]), atoi(argv[3])); int nb = (int)(atof(argv[4]) * SR / BLOCK); static float L[BLOCK], R[BLOCK];
        double s2 = 0; float gmin = 0; int over3 = 0, over1 = 0, cnt = 0;
        for (int b = 0; b < nb; b++) { synth_process(&S, L, R, BLOCK); if (b < nb / 10) continue; for (int t = 0; t < BLOCK; t++) s2 += L[t] * L[t];
            float g = S.fx.lim_gr; if (g < gmin) gmin = g; if (g < -3) over3++; if (g < -1) over1++; cnt++; }
        printf("rms %.1f dBFS  limiter: max %.1f dB, >1 dB %.0f%% of the time, >3 dB %.0f%%  clips %u\n", 10 * log10(s2 / (cnt * BLOCK)), gmin, 100.0 * over1 / cnt, 100.0 * over3 / cnt, S.fx.clips);
        return 0;
    }
    if (!strcmp(argv[1], "sweep") && argc >= 3) {
        int n = atoi(argv[2]); int bad = 0; double sum_db = 0; int cnt = 0; float worst = 0; uint32_t clips = 0; int quiet = 0; double lo = 0, hi = -99;
        for (int st_ = 0; st_ < NUM_STYLES; st_++) for (int i = 0; i < n; i++) {
            Stats st; uint32_t seed = 1000 + 37u * (uint32_t)i + 1000u * (uint32_t)st_;
            roll(seed, st_); render(12.f, NULL, &st, (int)(2.f * SR / BLOCK));
            float db = 20 * log10f(st.rms + 1e-9f);
            if (st.nan || st.rms < 1e-4f) { bad++; char d[400]; synth_describe(&S, d, sizeof d); printf("BAD seed %u style %d rms %.5f nan %d\n  %s\n", seed, st_, st.rms, st.nan, d); }
            if (db < -30.f) quiet++;
            if (getenv("V")) { char d[400]; synth_describe(&S, d, sizeof d); printf("%5.1f dB peak %.2f clips %5u | %s\n", db, st.peak, st.clips, d); }
            sum_db += db; cnt++; if (st.peak > worst) worst = st.peak; clips += st.clips; if (db < lo) lo = db; if (db > hi) hi = db;
        }
        printf("%d patches: mean level %.1f dBFS RMS (%.1f to %.1f), worst peak %.3f, soft-clipped samples %u, bad %d, under -30 dB %d\n", cnt, sum_db / cnt, lo, hi, worst, clips, bad, quiet);
        return 0;
    }
    if (!strcmp(argv[1], "rhythm") && argc >= 3) {
        int bars = atoi(argv[2]); roll(argc > 5 ? (uint32_t)atoi(argv[5]) : 42u, argc > 6 ? atoi(argv[6]) : STYLE_GROOVE);
        setd(P_DR_ON, 1); if (argc > 3) setd(P_RH_ENGINE, atoi(argv[3])); if (argc > 4) setd(P_RH_ASSIGN, atoi(argv[4]));
        static Hit h[20000]; int n = collect(bars, h, 20000);
        for (int b = 0; b < bars; b++) {
            char row[4][49]; for (int d = 0; d < 4; d++) { memset(row[d], '.', 48); row[d][48] = 0; }
            for (int i = 0; i < n; i++) if (h[i].bar == b) row[h[i].d][h[i].pl] = "KSHT"[h[i].d];
            printf("bar %3d  %s\n         %s\n         %s\n         %s\n", b + 1, row[0], row[1], row[2], row[3]);
        }
        return 0;
    }
    if (!strcmp(argv[1], "rstat") && argc >= 3) {          /* over seeds and styles: does a rolled rhythm hold together? */
        int bars = atoi(argv[2]); static Hit h[60000];
        printf("style seed  hits/bar K S H T   kick on the bar's start  snare at its half  neighbours alike  distinct bars  repeats within\n");
        for (int st_ = 1; st_ < NUM_STYLES; st_++) for (int sd = 0; sd < 3; sd++) {
            roll(500u + 91u * (uint32_t)sd + 13u * (uint32_t)st_, st_); if (!pint(&S, P_DR_ON)) setd(P_DR_ON, 1);
            int n = collect(bars, h, 60000); int c[4] = {0, 0, 0, 0}, k0 = 0, s24 = 0;
            static uint8_t B[512][4][48]; memset(B, 0, sizeof B);
            for (int i = 0; i < n; i++) { c[h[i].d]++; if (h[i].bar < 512) B[h[i].bar][h[i].d][h[i].pl] = 1; }
            double sim = 0; int distinct = 0, per = 0;
            for (int b = 1; b < bars; b++) {
                int in = 0, un = 0; for (int d = 0; d < 4; d++) for (int p = 0; p < 48; p++) { in += B[b][d][p] & B[b - 1][d][p]; un += B[b][d][p] | B[b - 1][d][p]; }
                sim += un ? (double)in / un : 1; k0 += B[b][0][0]; s24 += B[b][1][24];
                int nw = 1; for (int q = 1; q < b && nw; q++) if (!memcmp(B[b], B[q], sizeof B[0])) nw = 0; distinct += nw;
            }
            for (int p = 1; p < bars / 2 && !per; p++) { int ok = 1; for (int b = 1; b + p < bars && ok; b++) if (memcmp(B[b], B[b + p], sizeof B[0])) ok = 0; if (ok) per = p; }
            printf("%5d %4d  %5.1f %4.1f %5.1f %4.1f   %3d%%                     %3d%%               %3.0f%%             %3d            %s\n", st_, sd,
                   (double)c[0] / bars, (double)c[1] / bars, (double)c[2] / bars, (double)c[3] / bars, 100 * k0 / (bars - 1), 100 * s24 / (bars - 1), 100 * sim / (bars - 1), distinct,
                   per ? "yes" : "no");
            printf("             %s/%s sel %s dens %.2f threes %.2f swing %.2f fine %.2f | follow %d%d%d%d | %s\n", rh_engine_name(pint(&S, P_RH_ENGINE)), rh_assign_name(pint(&S, P_RH_ASSIGN)), selrate_name(pint(&S, P_RH_SELRATE)),
                   pval(&S, P_RH_DENS), pval(&S, P_RH_THREES), pval(&S, P_RH_SWING), pval(&S, P_RH_FINE), pint(&S, P_D1_SRC), pint(&S, P_D2_SRC), pint(&S, P_D3_SRC), pint(&S, P_D4_SRC), drum_layer_name(pint(&S, P_DR_LAYER)));
        }
        return 0;
    }
    if (!strcmp(argv[1], "gstat") && argc >= 3) {
        FILE *f = fopen(argv[2], "w");
        for (int g = 0; g < NUM_GENS; g++) for (int sd = 0; sd < 6; sd++) {
            roll(1000 + (uint32_t)sd * 77, STYLE_HIPHOP); setd(P_G1_FUNC, g); setd(P_G1_POINTS, 2); pset(P_V2_LEVEL, 0.f); setd(P_DR_ON, 0);
            setr(P_TEMPO, 100); pset(P_EVOLVE, 0); engine_build(&S);
            static float L[64], R[64]; int lastc = -1; uint32_t start = S.meter.bar;
            while ((int)(S.meter.bar - start) < 33) {
                synth_process(&S, L, R, 64);
                int c = (int)(S.meter.tau * 16.f);
                if (c != lastc && S.meter.bar > start) {
                    lastc = c; GenSt *gs = &S.layer[0].g;
                    fprintf(f, "%d,%d,%u,%d", g, sd, S.meter.bar - start, c);
                    for (int i = 0; i < 16; i++) fprintf(f, ",%.3f", i < S.eng.n_nodes ? gs->a[i] : 0.f);
                    fprintf(f, "\n");
                }
            }
        }
        fclose(f); for (int g = 0; g < NUM_GENS; g++) printf("%d %s\n", g, gen_name(g)); return 0;
    }
    if (!strcmp(argv[1], "bank") && argc >= 3) {           /* each control of the bank swept alone on held notes, then all moving */
        float *L = 0, *R = 0; int n = 0;
        static const int ID[8] = {P_V1_CENTER, P_V1_BAND, P_V1_TILT, P_V1_STRIDE, P_V1_WAVE, P_V1_PHASE, P_V1_SPIN, P_V1_TONE};
        for (int c = 0; c < 9; c++) {
            roll(42, STYLE_AMBIENT); setd(P_DR_ON, 0); pset(P_V2_LEVEL, 0.f); pset(P_RV_SEND, .15f); pset(P_DL_SEND, 0.f); pset(P_FZ_MIX, 0.f);
            for (int k = 0; k < 8; k++) pset(ID[k], g_param_meta[ID[k]].def);
            pset(P_V1_BLOOM, 0.f); for (int i = 0; i < NUM_LFO; i++) pset(P_LFO1_RATE + 3 * i, 0.f);
            for (int i = 0; i < MOD_SLOTS; i++) S.slot[i].src = 0;
            int steps = c < 8 ? 36 : 120;
            for (int k = 0; k < steps; k++) {
                float u = (float)k / (float)(steps - 1), tri = 1.f - fabsf(2.f * u - 1.f);
                if (c < 8) pset(ID[c], tri);
                else for (int q = 0; q < 8; q++) pset(ID[q], .5f + .5f * sinf(6.2832f * (u * (float)(q + 2) * .37f + .13f * (float)q)));
                go(.25f, &L, &R, &n);
            }
        }
        write_wav(argv[2], L, R, n); return 0;
    }
    if (!strcmp(argv[1], "tour") && argc >= 3) {           /* one rolled patch per style, 20 s each, then the knobs turned on one */
        float *L = 0, *R = 0; int n = 0;
        for (int st_ = 1; st_ < NUM_STYLES; st_++) { roll(7000u + 31u * (uint32_t)st_, st_); go(20.f, &L, &R, &n); char d[400]; synth_describe(&S, d, sizeof d); printf("%5.1f s  %s\n", (double)(n) / SR - 20., d); }
        /* the keys on two patches, transport stopped: a soft-contour pad, then a sharp-contour pluck (same eight notes) */
        for (int w = 0; w < 2; w++) {
            roll(9100u + (uint32_t)w, w ? STYLE_HIPHOP : STYLE_AMBIENT); setd(P_DR_ON, 0); pset(P_G1_CONTOUR, w ? .9f : .1f); S.transport = 0; engine_build(&S);
            printf("%5.1f s  keys, %s (Contour %s)\n", (double)n / SR, w ? "a struck patch" : "a pad patch", w ? "90%" : "10%");
            int N = S.eng.n_nodes; static int ord[MAX_NODES];
            for (int i = 0; i < N; i++) { int k = i; while (k > 0 && S.eng.node[ord[k - 1]].lr > S.eng.node[i].lr) { ord[k] = ord[k - 1]; k--; } ord[k] = i; }
            for (int k = 0; k < 8; k++) { int nd = ord[(2 + k * 3) % N]; synth_note(&S, nd, .8f, 1); go(w ? .35f : 1.2f, &L, &R, &n); synth_note(&S, nd, 0.f, 0); go(w ? .15f : .6f, &L, &R, &n); }
            go(3.f, &L, &R, &n); S.transport = 1;
        }
        /* Tone swept on a held chord: closed, open, closed (a pad patch, transport stopped) */
        {   roll(9100u, STYLE_AMBIENT); setd(P_DR_ON, 0); S.transport = 0; engine_build(&S);
            printf("%5.1f s  Tone swept on a held chord\n", (double)n / SR);
            int N = S.eng.n_nodes; static int ord[MAX_NODES];
            for (int i = 0; i < N; i++) { int k = i; while (k > 0 && S.eng.node[ord[k - 1]].lr > S.eng.node[i].lr) { ord[k] = ord[k - 1]; k--; } ord[k] = i; }
            for (int k = 0; k < 4; k++) synth_note(&S, ord[(N / 5 + k * (N / 6 + 1)) % N], .8f, 1);
            for (int i = 0; i <= 56; i++) { float u = (float)i / 56.f; pset(P_V1_TONE, .15f + .85f * (1.f - fabsf(2.f * u - 1.f))); go(.25f, &L, &R, &n); }
            for (int k = 0; k < 4; k++) synth_note(&S, ord[(N / 5 + k * (N / 6 + 1)) % N], 0.f, 0);
            go(3.f, &L, &R, &n); S.transport = 1; }
        /* the drums on a groove, split across the layers */
        roll(5151, STYLE_GROOVE); setd(P_DR_ON, 1); setd(P_DR_LAYER, DLAYER_SPLIT); printf("%5.1f s  the drums, split\n", (double)n / SR); go(16.f, &L, &R, &n);
        roll(4242, STYLE_GROOVE); if (!pint(&S, P_DR_ON)) setd(P_DR_ON, 1);
        for (int k = 0; k < 4; k++) {
            printf("%5.1f s  knob %d (%s) turned down, up and back\n", (double)n / SR, k + 1, knob_name(k));
            for (int i = 0; i <= 48; i++) { float u = (float)i / 48.f; synth_set_macro(&S, k, sinf(6.2832f * u)); go(.25f, &L, &R, &n); }
            synth_set_macro(&S, k, 0.f);
        }
        write_wav(argv[2], L, R, n); return 0;
    }
    fprintf(stderr, "unknown command\n"); return 1;
}
