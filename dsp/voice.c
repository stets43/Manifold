/*
 * The voice engine: every note is a bank of up to 16 oscillators at its harmonics 1..16 (exact whole-number
 * multiples: one phase a note, oscillator k reads k times it). Nobody sets 16 levels: one continuous function of a
 * few controls sets every oscillator's level and phase, and it is evaluated again every control tick, so any of them
 * can be moved (by a knob, an LFO, a generator) while notes sound and the timbre follows without a seam.
 *
 *   Center, Band   which part of the series sounds: a bell curve over the partials, its centre (1..12) and width
 *   Tilt           the slope across them, k^(2 Tilt - 1)
 *   Stride         a comb over the partials: 1 every one, 2 the odd ones, 3 every third ... (0.5 + 0.5 cos(2 pi
 *                  (k - 1) / stride))^2, continuous between
 *   Wave           each oscillator from a sine to a richer wave (its own 2nd and 3rd harmonics): the oscillators'
 *                  partials then overlap, and their phases decide whether they add or cancel
 *   Phase          the phase law across the bank, phi_k = pi D k^2: aligned (a sharp pulse) through Schroeder phase
 *                  (smooth, glassy) and on
 *   Spin           turns the phase pattern: oscillator k drifts by (k - 1) x a fraction of a hertz, so overlapping
 *                  partials beat slowly against each other (and against the other notes): a shimmer
 *   Cross          each oscillator's phase is modulated by the sounding oscillator below it: FM between neighbours,
 *                  whole-number ratios, so every sideband is a harmonic of the note
 *   Bloom          a note's loudness opens its upper partials: louder is brighter, per note, sample by sample
 *   Width          odd and even partials lean left and right
 *
 * The function is normalised to unit energy, so the controls change the colour and not the level. Nothing is above
 * 0.45 of the sample rate: an oscillator that would be is not played, and Wave's extra harmonics drop out first.
 * Integer oscillators (32-bit phase, int16 tables of 2048, no interpolation: the error is under -70 dB), summed in
 * int32 a note, then one float gain ramp a note.
 */
#define MANIFOLD_DSP_UNIT
#include "synth.h"
#include "mstring.h"

static float T_SIN[OSC_N] DSP_POOL;      /* sin */
static float T_D1[OSC_N] DSP_POOL;       /* Wave's addition with room for a 3rd harmonic: 0.5 sin 2x + 0.33 sin 3x */
static float T_D2[OSC_N] DSP_POOL;
#define OSC_SH 20                           /* a 32-bit phase to a 4096-entry table */
#define TAIL_N 4096
static float T_T1[TAIL_N] DSP_POOL;      /* a bass note's partials 14..32, falling as 13/k (1/8 scale) */
static float T_T2[TAIL_N] DSP_POOL;      /* ... and 33..64 */       /* ... with room for a 2nd only: 0.5 sin 2x */

static float LOG2K[MAX_HARM + 1];          /* flog2(k): the bank's tilt, k^-p, without a log each time */
/* the slots of a note's partials: harmonics 1..13 (slots 0..12), then its undertones 1/2, 1/4, 1/8 (13..15). PM: each
   one's multiple of the note's phase, which runs at an eighth of its pitch (so the undertones are whole multiples
   too, and every partial keeps its phase to the note's, block after block); LOG2R: its octave, log2 of its ratio */
static const uint32_t PM[MAX_HARM] = {8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 4, 2, 1};
static float LOG2R[MAX_HARM];
static const float PR[MAX_HARM] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, .5f, .25f, .125f};
/* the last inputs each layer's bank was worked out for (its levels, its phases): a tick whose controls did not move
   reuses them */
static float g_lv_key[NUM_LAYERS][6], g_ph_key[NUM_LAYERS], g_ph_t[NUM_LAYERS][MAX_HARM];
/* the oscillator cap's floor: a partial under it (of its note's level) is not played. It rises while a layer has more
   oscillators than the cap and falls back to -44 dB (6e-3, where it always was) when it has fewer: in a dense moment
   the quietest partials go first, faded over a knee (an octave of level above the floor), never whole notes */
#ifndef DR_OUT
#define DR_OUT    1.1f        /* a drum hit's level at the layer's output, at full Level: its own, not the layer's */
#endif
#define OSC_FLOOR 6e-3f
#define OSC_CAP   64          /* oscillators a layer (detuned copies and tails count) */

/* (1.0.7) The oscillators. Each writes straight into the layer's left and right, its level ramped sample by sample
   (the partial's own level from the last block's to this one's, times the voice's level, duck and hits, times its
   place in the stereo field): no per-voice buffers and no mix pass after them, and no partial's level stepping once a
   block (a buzz at 689 Hz while Bloom, Pluck or a hit moved it). The plain ones (no Wave), the detuned copies, the
   undertones and partials 1-3 with Wave are exact: a recursion (x -= e y; y += e x, the "magic circle") started each
   block from the oscillator's own 32-bit phase, so nothing drifts; a 4096-entry table read at the nearest entry left
   a stepped sine about 60 dB down, the low notes' "bit-reduced" buzz. Partials 4-13 with Wave keep the table (their
   2nd and 3rd from one read): they carry about 1 % of that error. The bass tails read their tables between entries. */
#define TURN (1.f / 4294967296.f)
#ifndef WAVE_EXACT
#define WAVE_EXACT 1                                  /* (Wave on partials 1..this: exact; above it, the table) */
#endif
static float PS[NUM_LAYERS][MAX_NODES][19][3] DSP_POOL;   /* each exact oscillator's state and its e (16 partials, 3 copies) */
static float g_phop[NUM_LAYERS][MAX_HARM]; static uint32_t g_blk[NUM_LAYERS];   /* (Phase's offsets last block; blocks) */
static uint32_t PK[NUM_LAYERS][MAX_NODES][19] DSP_POOL;  /* (1.0.7) the step each one's e was worked out for: a block
                                                          at the same pitch takes e (and e/2) as they are */
/* (1.0.7) a note's Tone weights, kept until its pitch or Tone moves: 1 / (1 + r^4) each partial (and the two tails'),
   Wave's share at its 2nd and 3rd, and the register's level. Worked out again only when they change */
typedef struct { float f, ifc2, reg, tn[MAX_HARM], wr[MAX_HARM], tt[2]; uint8_t pk[MAX_HARM]; } ToneC;
/* (1.0.8) how much of the stereo each partial takes, by its own frequency (0..255): the bass stays in the middle. Under
   PAN_LO Hz none (the note's place and Width's spread both left out), over PAN_HI all of it as before, a smooth step
   between (on a log scale). With the note's pitch only, kept until it moves; in the flash (a new pitch only) */
#define PAN_LO 110.f
#define PAN_HI 330.f
static __attribute__((noinline)) void pan_keys(float f, uint8_t *pk) {
    for (int j = 0; j < MAX_HARM; j++) {
        float u = flog2(f * PR[j] * (1.f / PAN_LO)) * (1.f / 1.5849625f);   /* (log2(PAN_HI / PAN_LO)) */
        u = u < 0.f ? 0.f : u > 1.f ? 1.f : u;
        pk[j] = (uint8_t)(u * u * (3.f - 2.f * u) * 255.f + .5f);
    }
}
static ToneC TC[NUM_LAYERS][MAX_NODES] DSP_POOL;
static float PL[NUM_LAYERS][MAX_NODES][21] DSP_POOL;   /* each oscillator's level last block (slots: 16 partials, 3 copies,
                                                          2 tails): this block ramps from it; -1 a voice starting */
#ifdef OSC_SHADOW
double g_shL[SUB], g_shR[SUB]; long g_shn[4], g_shv, g_shc[4];   /* (and voice-samples; oscillator-blocks of each kind) */ float g_eb[NUM_LAYERS][2][SUB], g_xb[NUM_LAYERS][2][SUB];   /* (each layer: error, exact) */       /* (the host's check: each oscillator again, exactly, in double) */
static double sh_sin(uint32_t ph) { return __builtin_sin(6.283185307179586 * (double)ph * (1.0 / 4294967296.0)); }
static double sh_tail(uint32_t ph, int tl) { double x = (double)ph / 4294967296.0, a = 0; int k0 = tl ? 33 : 14, k1 = tl ? 64 : 32;
    for (int k = k0; k <= k1; k++) a += 13.0 / k * __builtin_sin(6.283185307179586 * k * x); return a; }
#define SH(t, y, a) do { g_shL[off0 + (t)] += (y) * (a) * gl; g_shR[off0 + (t)] += (y) * (a) * gr; } while (0)
#endif
/* sin and cos of r (radians, 0 .. pi/2): series, so a step coefficient is right to the float (a table's linear error
   would make a partial run a little fast or slow across a block) */
DSP_INL void sc_series(float r, float *sn, float *cs) {
    float r2 = r * r;
    *sn = r * (1.f - r2 * (1.f / 6.f) * (1.f - r2 * (1.f / 20.f) * (1.f - r2 * (1.f / 42.f) * (1.f - r2 * (1.f / 72.f)))));
    *cs = 1.f - r2 * .5f * (1.f - r2 * (1.f / 12.f) * (1.f - r2 * (1.f / 30.f) * (1.f - r2 * (1.f / 56.f) * (1.f - r2 * (1.f / 90.f)))));
}
/* an exact oscillator's coefficient e = 2 sin(w/2) for its step ik (and sin, cos of w/2, for Wave), and, when sync,
   its state from its own phase: x = sin(ph - w), y = -cos(ph - w/2), so the first step lands on ph. Else the state goes
   on from the block before (ps): a recursion keeps its phase to the float, and a resync now and then (a voice every 64
   blocks, staggered), at a start, when the partial comes back or when Phase moves it keeps it on the 32-bit phase */
DSP_RAM static __attribute__((noinline)) void mcf_seed(uint32_t ph, uint32_t ik, float *st, float *ps, uint32_t *pk, int sync) {
    float w = (float)ik * TURN, hs, hc;
    if (*pk == ik && ps[2] > 0.f) { st[2] = ps[2]; st[3] = .5f * ps[2]; st[4] = -1.f; }   /* (the same step: e as it was; cos only for Wave) */
    else { sc_series(3.14159265f * w, &hs, &hc); st[2] = 2.f * hs; st[3] = hs; st[4] = hc; *pk = ik; }
    /* (from its phase also when its pitch moved, its orbit another; or the table had it last: 1e9 there) */
    if (sync || st[2] != ps[2] || !(ps[1] < 100.f && ps[1] > -100.f)) {
        float p = (float)ph * TURN; st[0] = fsin(p - w); st[1] = -fsin(p - .5f * w + .25f);
    }
    else { st[0] = ps[0]; st[1] = ps[1]; }
    ps[2] = st[2];
}
DSP_INL float mcf_cos(const float *st, uint32_t ik) {   /* cos(w/2), for Wave: kept, or from the series */
    if (st[4] >= 0.f) return st[4];
    float hs, hc; sc_series(3.14159265f * (float)ik * TURN, &hs, &hc); return hc;
}
/* the loops: an oscillator's samples into L and R, its level a line from A0 by d1 a sample for the first n1, then
   from Am2 by d2 to n (a hit landing inside the block: its slope changes there); gl and gr its gains. One call a
   block, its arguments in one place (OSeg: fewer to pass, and the second line without a second call) */
typedef struct {
    float *L, *R; int n1, n; float A0, d1, Am2, d2, gl, gr;
#ifdef OSC_SHADOW
    uint32_t ph, ik; float c2, c3; int tl;
#endif
} OSeg;
#ifdef OSC_SHADOW
/* (the exact copy: the same level lines, each oscillator again in double) */
#define SHLOOP(expr) do { float gl = g->gl, gr = g->gr; const int off0 = 0; float aa = g->A0, dd = g->d1; uint32_t ph = g->ph, ik = g->ik; \
    for (int t = 0; t < g->n; t++) { if (t == g->n1) { aa = g->Am2; dd = g->d2; } aa += dd; SH(t, (expr), aa); ph += ik; } } while (0)
#endif
DSP_RAM static __attribute__((noinline)) void o_mcf(const OSeg *g, float *st) {
    float x = st[0], y = st[1], e = st[2], gl = g->gl, gr = g->gr, *L = g->L, *R = g->R;
    float a = g->A0, da = g->d1; int t = 0, te = g->n1, n = g->n;
    for (;;) {
        for (; t < te; t++) { x -= e * y; y += e * x; a += da; float v = x * a; L[t] += v * gl; R[t] += v * gr; }
        if (te >= n) break;
        te = n; a = g->Am2; da = g->d2;                   /* (the second line) */
    }
    st[0] = x; st[1] = y;
#ifdef OSC_SHADOW
    g_shc[0]++; g_shn[0] += n; SHLOOP(sh_sin(ph));
#endif
}
/* exact, with Wave: c2 sin 2t + c3 sin 3t = x (K1 x + K2 y + K0 + K3 x^2), x = sin t and y = -cos(t + w/2) the state */
DSP_RAM static __attribute__((noinline)) void o_mcfw1(float *L, float *R, int n, float *st, const float *K, float a, float da, float gl, float gr) {
    float x = st[0], y = st[1], e = st[2], K0 = K[0], K1 = K[1], K2 = K[2], K3 = K[3];
    for (int t = 0; t < n; t++) { x -= e * y; y += e * x; a += da; float v = x * a * ((K3 * x + K1) * x + K2 * y + K0); L[t] += v * gl; R[t] += v * gr; }
    st[0] = x; st[1] = y;
}
/* (its two lines as two calls: one loop over both kept a register spilled, 27 instructions a sample, not 24) */
DSP_INL void o_mcfw(const OSeg *g, float *st, const float *K) {
    o_mcfw1(g->L, g->R, g->n1, st, K, g->A0, g->d1, g->gl, g->gr);
    if (g->n1 < g->n) o_mcfw1(g->L + g->n1, g->R + g->n1, g->n - g->n1, st, K, g->Am2, g->d2, g->gl, g->gr);
#ifdef OSC_SHADOW
    g_shn[1] += g->n; g_shc[1]++; SHLOOP(sh_sin(ph) + g->c2 * sh_sin(2u * ph) + g->c3 * sh_sin(3u * ph));
#endif
}
/* the table, with Wave (partials 2-13) */
DSP_RAM static __attribute__((noinline)) void o_tabw(const OSeg *g, uint32_t ph, uint32_t ik, const float *T, const float *D, float w) {
    float gl = g->gl, gr = g->gr, *L = g->L, *R = g->R, a = g->A0, da = g->d1; int t = 0, te = g->n1, n = g->n;
#ifdef OSC_SHADOW
    g_shn[2] += n; g_shc[2]++; SHLOOP(sh_sin(ph) + g->c2 * sh_sin(2u * ph) + g->c3 * sh_sin(3u * ph));
#endif
    for (;;) {
        for (; t < te; t++) { uint32_t x = ph >> 20; a += da; float v = (T[x] + D[x] * w) * a; L[t] += v * gl; R[t] += v * gr; ph += ik; }
        if (te >= n) break;
        te = n; a = g->Am2; da = g->d2;
    }
}
/* a bass tail: its table read between entries */
DSP_RAM static __attribute__((noinline)) void o_tail(const OSeg *g, uint32_t ph, uint32_t ik, const float *T) {
    float gl = g->gl, gr = g->gr, *L = g->L, *R = g->R, a = g->A0, da = g->d1; int t = 0, te = g->n1, n = g->n;
#ifdef OSC_SHADOW
    g_shn[3] += n; g_shc[3]++; SHLOOP(sh_tail(ph, g->tl));
#endif
    for (;;) {
        for (; t < te; t++) { uint32_t x = ph >> 20; float fr = (float)(ph & 0xFFFFFu) * (1.f / 1048576.f);
            a += da; float v = (T[x] + fr * (T[(x + 1) & 4095u] - T[x])) * a; L[t] += v * gl; R[t] += v * gr; ph += ik; }
        if (te >= n) break;
        te = n; a = g->Am2; da = g->d2;
    }
}
static float g_thr[NUM_LAYERS] = {OSC_FLOOR, OSC_FLOOR};

void voices_init(void) {
    for (int k = 1; k <= MAX_HARM; k++) LOG2K[k] = flog2((float)k);
    for (int j = 0; j < MAX_HARM; j++) LOG2R[j] = flog2(PR[j]);
    for (int l = 0; l < NUM_LAYERS; l++) { g_lv_key[l][0] = -1.f; g_ph_key[l] = -1.f; }
    for (int i = 0; i < OSC_N; i++) {
        float x = (float)i / (float)OSC_N;
        T_SIN[i] = fsin(x);
        T_D1[i] = .5f * fsin(2.f * x - (float)(int)(2.f * x)) + .33f * fsin(3.f * x - (float)(int)(3.f * x));
        T_D2[i] = .5f * fsin(2.f * x - (float)(int)(2.f * x));
    }
    for (int i = 0; i < TAIL_N; i++) {
        float x = (float)i / (float)TAIL_N, a = 0.f, b = 0.f;
        for (int k = 14; k <= 64; k++) { float kx = (float)k * x, v = 13.f / (float)k * fsin(kx - (float)(int)kx); if (k <= 32) a += v; else b += v; }
        T_T1[i] = a; T_T2[i] = b;
    }
}

/* Odd/even on a bank's partials (a): out, blended toward a square's or the even series by how far o is from the middle.
   The target has the bank's own energy above the fundamental, at least 15 % of the note's; the fundamental is kept.
   (Inline: the render runs from RAM and calls nothing) */
DSP_INL void colour(const float *a, float o, float *am) {
    float x = o < .5f ? 1.f - 2.f * o : 2.f * o - 1.f;
    for (int k = 0; k < MAX_HARM; k++) am[k] = a[k];
    if (x <= .02f) return;
    if (x > 1.f) x = 1.f;
    int sq = o < .5f;
    float ea = 0.f, et = 0.f, e1 = am[0] * am[0];
    for (int k = 2; k <= NH; k++) { float t = ((k & 1) == sq) ? 1.f / (float)k : 0.f; ea += am[k - 1] * am[k - 1]; et += t * t; }
    float want = ea > .15f * (e1 + ea) ? ea : .15f * (e1 + ea);
    if (want < 1e-12f) return;                            /* (a silent bank: nothing to colour, and no 0 x inf) */
    float sc = want / et; sc *= frsqrt(sc);
    for (int k = 2; k <= NH; k++) {               /* (the harmonics: the undertones keep their own) */
        float t = ((k & 1) == sq) ? sc / (float)k : 0.f;
        am[k - 1] += x * (t - am[k - 1]);
    }
}
/* (in the flash: they run rarely, at a hit or when the notes change; the RAM window is short) */
void hit_colour(const Synth *s, int l, float o, float *out) { colour(s->layer[l].amp, o, out); }
/* (1.0.6) Fixed drums: each drum's own spectrum on its note while its hit settles, after the 808's (its kick and toms
   round sine-like bodies, its snare two tones and a crack, its hats six square waves high-passed: all sizzle).
     Kick  Body:   the fundamental, its 2nd and 3rd falling away fast, and an undertone (the weight under it)
     Tom   Body:   a little more of the low partials
     Snare Crack:  the middle partials (3rd to 9th) over the fundamental
     Hat   Sizzle: its partials over 4.5 kHz flat (at least the 10th to the 13th), the 7th, 11th and 13th included
                   (the clash is the metal), the rest near gone
   Set as heard: Tone's slope (24 dB an octave over its corner) is undone here partial by partial, to 24 dB at most,
   and the whole is set to the energy the note's own bank has under Tone, so a hit is no louder for its profile.
   Odd/even still colours it. Worked out once a hit, in the flash */
void hit_profile(const Synth *s, int d, int l, int node, float o, float *out) {
    float p[MAX_HARM], f = s->eng.node[node].freq * fexp2((float)(pint(s, LP(l, P_V1_REG)) - 1));
    float fc = 250.f * fexp2(6.f * pval(s, LP(l, P_V1_TONE))), ifc2 = 1.f / (fc * fc), e = 0.f, ea = 0.f;
    const float *am = s->layer[l].amp;
    for (int j = 0; j < MAX_HARM; j++) {
        float K = PR[j], v;
        if (j >= NH) v = d == 0 ? (j == NH ? .35f : j == NH + 1 ? .12f : 0.f) : d == 3 ? (j == NH ? .15f : 0.f) : 0.f;
        else if (d == 0) v = fexp2(-1.6f * (K - 1.f));                         /* kick: 1, .33, .11 .. */
        else if (d == 3) v = fexp2(-1.0f * (K - 1.f));                         /* tom: 1, .5, .25 .. */
        else if (d == 1) v = K < 2.5f ? (K < 1.5f ? .5f : .75f) : K <= 9.5f ? 1.f / (1.f + .08f * (K - 3.f)) : .3f;   /* snare */
        else v = f * K >= 4500.f || K > 9.5f ? 1.f : .02f;                     /* hat: what is over 4.5 kHz (at least
                                                                                 the 10th to the 13th, a low note's) */
        p[j] = v;
    }
    colour(p, o, out);
    for (int j = 0; j < MAX_HARM; j++) {                      /* (as heard: under Tone) */
        float fk = f * PR[j], r2 = fk * fk * ifc2, tn = 1.f / (1.f + r2 * r2);
        e += out[j] * out[j]; ea += am[j] * am[j] * tn * tn;
    }
    float sc = e > 1e-12f ? sqrtf(ea / e) : 0.f;
    for (int j = 0; j < MAX_HARM; j++) {
        float fk = f * PR[j], r2 = fk * fk * ifc2, un = 1.f + r2 * r2;
        out[j] *= sc * (un > 251.2f ? 251.2f : un);           /* (Tone undone, to 24 dB at most) */
    }
}
static void clarity_mask(Synth *s, int l, const float *key);
static float pw(float x, float e) { return x <= 0.f ? 0.f : fexp2(e * flog2(x)); }

/* the bank's function itself: a bell over the partials (centre c, width w), a tilt k^te, a comb of stride st; unit
   energy over partials 1..kmax. The voices use it, and the drums (each drum a bank of its own) */
void bank_fn(float c, float w, float te, float st, int kmax, float *a) {
    float e = 0.f, ist = 1.f / st, iw = 1.f / w;
    for (int k = 1; k <= MAX_HARM; k++) {
        float K = (float)k, d = (K - c) * iw, z = (K - 1.f) * ist;
        float cb = .5f + .5f * fsin(z - (float)(int)z + .25f);
        float v = k > kmax ? 0.f : fexp2(-.72135f * d * d) * pw(K, te) * cb * cb;
        a[k - 1] = v; e += v * v;
    }
    if (e < 1e-8f) { a[0] = 1.f; e = 1.f; }               /* never an empty bank */
    e = frsqrt(e);
    for (int k = 0; k < MAX_HARM; k++) a[k] *= e;
}
/* A voice's spectrum. It falls with the partial number, as every warm source's does (k^-1.6 at Tilt 0; k^-1 in the
   middle, a saw's slope; k^-0.4 at Tilt 1), the fundamental always the strongest; Center and Band
   lift a region of it (a formant, up to +9.5 dB once it is above the 5th partial; nothing at the bottom), they do not cut the rest away; Stride thins it to every n-th partial.
   Partials 7, 11 and 13 (no note of any scale but the series') are held back. Under it, Depth: its undertones, an
   octave, two and three below (in tune: octaves of the note), at 1.2 D, 1.2 D^2, 1.2 D^3 of the fundamental (before
   the whole is set to unit energy: the deeper, the more of the note's energy is down there). Unit energy. */
static void bank_levels(const Synth *s, int l, float *a) {
    float c = 2.f + 10.f * pw(pval(s, LP(l, P_V1_CENTER)), 1.6f), iw = 1.f / (.5f + 7.f * pval(s, LP(l, P_V1_BAND)));
    float p = 1.6f - 1.2f * pval(s, LP(l, P_V1_TILT)), ist = 1.f / (1.f + 3.f * pval(s, LP(l, P_V1_STRIDE)));
    int kmax = s->cap_harm < 4 ? 4 : s->cap_harm > MAX_HARM ? MAX_HARM : s->cap_harm;
    float bg = 2.f * fclamp((c - 2.f) * (1.f / 3.f), 0.f, 1.f);   /* (a lift low down would only be a duller sound: it grows as it climbs) */
    float e = 0.f;
    for (int k = 1; k <= NH; k++) {
        float K = (float)k, d = (K - c) * iw, z = (K - 1.f) * ist;
        float cb = .5f + .5f * fsin(z - (float)(int)z + .25f);
        float v = k > kmax ? 0.f : fexp2(-p * LOG2K[k]) * (1.f + bg * fexp2(-.72135f * d * d)) * cb * cb;
        a[k - 1] = v; e += v * v;
    }
    a[6] *= .55f; a[10] *= .4f; a[12] *= .4f;
    {   float D = pval(s, P_V1_DEPTH + l), u = 1.2f * a[0];
        a[13] = u * D; a[14] = u * D * D; a[15] = u * D * D * D; }
    e = 0.f; for (int k = 0; k < MAX_HARM; k++) e += a[k] * a[k];
    e = frsqrt(e > 1e-8f ? e : 1e-8f);
    for (int k = 0; k < MAX_HARM; k++) a[k] *= e;
}
static void bank_phases(const Synth *s, int l, float *p) {
    float D = pval(s, LP(l, P_V1_PHASE)) * .125f;            /* Schroeder phase at Phase = 1/2 */
    for (int k = 0; k < MAX_HARM; k++) {
        float K = (float)(k + 1), x = k < NH ? .5f * D * K * K : 0.f;   /* (the undertones: in phase with the note) */
        p[k] = x - floorf(x);
    }
}

void voices_build(Synth *s) {
    for (int l = 0; l < NUM_LAYERS; l++) {
        g_lv_key[l][0] = -1.f; g_ph_key[l] = -1.f;
        Layer *L = &s->layer[l];
        DSP_FAR(bank_levels)(s, l, L->amp_t);
        memcpy(L->amp, L->amp_t, sizeof L->amp);
        memcpy(L->amc, L->amp, sizeof L->amc);
        {   /* Clarity's mask, now (a rebuild must not leave a block without partials) */
            Engine *e = &s->eng; int N = e->n_nodes < MAX_NODES ? e->n_nodes : MAX_NODES;
            float k[4] = {pval(s, P_V1_CLARITY + l), DSP_FAR(dsp_exp2)((float)(pint(s, LP(l, P_V1_REG)) - 1)), (float)N,
                          N > 0 ? e->node[0].freq + 7.f * e->node[N - 1].freq : 0.f};
            clarity_mask(s, l, k);
        }
        DSP_FAR(bank_phases)(s, l, L->pho);
    }
}

/* Clarity: a hard limit on how many partials, of all the layer's notes, each octave may hold (octaves from 20 Hz:
   20-40, 40-80, ...: 1, 2, 3, 5, 8, 13, ... at full Clarity, 16 times that at none: no limit). Every note's
   fundamental plays; then, from the lowest note up, each note's undertones and harmonics (the nearest first) take what
   room is left. A note's place in it does not depend on whether it sounds (so nothing comes and goes as notes do):
   the low notes have the low octaves, the chord above does not crowd under them. Nothing under 20 Hz is ever played.
   Worked out when the notes, the register or Clarity change; the render reads a bit a partial */
static void clarity_mask(Synth *s, int l, const float *key) {
    static const uint8_t ORD[MAX_HARM - 1] = {13, 1, 14, 2, 3, 15, 4, 5, 6, 7, 8, 9, 10, 11, 12};   /* (slots by nearness) */
    static const float BASE[11] = {1.f, 2.f, 3.f, 5.f, 8.f, 13.f, 21.f, 34.f, 55.f, 89.f, 144.f};
    Layer *L = &s->layer[l]; Engine *e = &s->eng;
    int N = e->n_nodes < MAX_NODES ? e->n_nodes : MAX_NODES;
    float cl = key[0], oct = key[1];
    for (int i = 0; i < 4; i++) L->mkey[i] = key[i];
    float m = 1.f + 15.f * (1.f - cl) * (1.f - cl);
    int cnt[11] = {0};
    for (int i = 0; i < N; i++) {
        float f = e->node[i].freq * oct; L->pmask[i] = 0;
        if (f >= 20.f && f < .45f * SR) {
            int o = 0; for (float b = 40.f; f >= b && o < 10; b *= 2.f) o++;
            L->pmask[i] = 1; cnt[o]++;
        }
    }
    for (int i = 0; i < N; i++) {
        float f = e->node[i].freq * oct;
        for (int q = 0; q < MAX_HARM - 1; q++) {
            int j = ORD[q]; float fj = f * PR[j];
            if (fj < 20.f || fj >= .45f * SR) continue;
            int o = 0; for (float b = 40.f; fj >= b && o < 10; b *= 2.f) o++;   /* (its octave from 20 Hz) */
            if (cl > .001f && (float)cnt[o] >= BASE[o] * m) continue;
            L->pmask[i] |= (uint16_t)(1u << j); cnt[o]++;
        }
    }
    for (int i = N; i < MAX_NODES; i++) L->pmask[i] = 0;
}
/* (1.0.7: in the flash, with the bank's functions: once a tick a layer, and the oscillators needed the RAM) */
void voices_tick(Synth *s, int l) {
    Layer *L = &s->layer[l];
    float key[6] = {pval(s, LP(l, P_V1_CENTER)), pval(s, LP(l, P_V1_BAND)), pval(s, LP(l, P_V1_TILT)), pval(s, LP(l, P_V1_STRIDE)), (float)s->cap_harm, pval(s, P_V1_DEPTH + l)};
    float *lk = g_lv_key[l];
    if (key[0] != lk[0] || key[1] != lk[1] || key[2] != lk[2] || key[3] != lk[3] || key[4] != lk[4] || key[5] != lk[5]) {
        bank_levels(s, l, L->amp_t);
        for (int i = 0; i < 6; i++) lk[i] = key[i];
    }
    {   /* Clarity's mask: again only when what it depends on moved (a call into the flash, rare) */
        Engine *e = &s->eng; int N = e->n_nodes < MAX_NODES ? e->n_nodes : MAX_NODES;
        float k[4] = {pval(s, P_V1_CLARITY + l), fexp2((float)(pint(s, LP(l, P_V1_REG)) - 1)), (float)N,
                      N > 0 ? e->node[0].freq + 7.f * e->node[N - 1].freq : 0.f};
        if (k[0] != L->mkey[0] || k[1] != L->mkey[1] || k[2] != L->mkey[2] || k[3] != L->mkey[3]) DSP_FAR(clarity_mask)(s, l, k);
    }
    float *tgt = g_ph_t[l];
    if (pval(s, LP(l, P_V1_PHASE)) != g_ph_key[l]) { bank_phases(s, l, tgt); g_ph_key[l] = pval(s, LP(l, P_V1_PHASE)); }
    for (int k = 0; k < MAX_HARM; k++) {
        L->amp[k] += (L->amp_t[k] - L->amp[k]) * .12f;      /* glide: 12 ms */
        float d = tgt[k] - L->pho[k]; d -= floorf(d + .5f);  /* the short way round, a little a tick: no jumps */
        d = fclamp(d, -.012f, .012f);
        L->pho[k] += d; L->pho[k] -= floorf(L->pho[k]);
    }
    colour(L->amp, pval(s, P_V1_ODDEVEN + l), L->amc);      /* the layer's Odd/even, once a tick (the notes take it) */
}

/* (the RAM code has little room: the render's short set-up loops stay loops, the oscillators are what is unrolled) */
#if defined(__clang__)
#define NOUNROLL _Pragma("clang loop unroll(disable) vectorize(disable)")
#else
#define NOUNROLL
#endif
/* Pluck's rate (1/T, per partial step, per second) for an amount: T from 1.5 s down to 25 ms; 0 off */
DSP_INL float pluck_rate(float p) { return p > .01f ? (1.f / 1.5f) * fexp2(6.f * p) : 0.f; }
DSP_RAM void layer_render(Synth *s, int l, float *Lo, float *Ro, int n) {
#ifdef OSC_SHADOW
    for (int t = 0; t < n; t++) g_shL[t] = g_shR[t] = 0.;
    float L0_[SUB], R0_[SUB]; for (int t = 0; t < n; t++) { L0_[t] = Lo[t]; R0_[t] = Ro[t]; }
#endif
    Layer *L = &s->layer[l]; GenSt *g = &L->g; Engine *e = &s->eng;
    int N = e->n_nodes < g->n ? e->n_nodes : g->n;
    float oct = fexp2((float)(pint(s, LP(l, P_V1_REG)) - 1));
    float width = pval(s, LP(l, P_V1_WIDTH)), bloom = pval(s, LP(l, P_V1_BLOOM));
    float wave = pval(s, LP(l, P_V1_WAVE)), tone = pval(s, LP(l, P_V1_TONE)), det = pval(s, LP(l, P_V1_SPIN));
    /* the voice's colour (1.0.5). Odd/even: the partials above the fundamental blended toward a square's (odd, 1/k:
       hollow, a clarinet's) under the middle, toward the even ones' (1/k: octaves, nasal, organ-like) over it; at the
       ends, all the way. The blend's target has the bank's own energy above the fundamental, and at least 15 % of the
       note's (a near-sine still takes the colour); the fundamental is left as it is. Pluck: a partial k fades as
       exp(-(k - 1) t / T) after the note's level last rose, T from 1.5 s down to 25 ms: the top goes first, as a
       plucked or struck string's does (and a drum hit on it strikes it again) */
    float plk = pval(s, P_V1_PLUCK + l);
    /* the drums' hits on this layer (drums.c: a slot each, and one fading out): which voices they ride */
    const DrumHit *hs[2 * NUM_DRUMS]; int nh = 0;
    for (int d = 0; d < NUM_DRUMS; d++) {
        const DrumV *dv = &s->dr.v[d];
        if (dv->cur.layer == l && (dv->cur.h0 > 0.f || dv->cur.h1 > 0.f)) hs[nh++] = &dv->cur;
        if (dv->old.layer == l && (dv->old.h0 > 0.f || dv->old.h1 > 0.f)) hs[nh++] = &dv->old;
    }
    float dk0 = s->dr.dk0, dk1 = s->dr.dk1;                          /* the kick's duck on the voices' own level */
    const float wn0 = frsqrt(1.f + .359f * wave * wave);              /* Wave adds energy: taken back */
    /* the voice cap, as a crossfade: the loudest nodes up to the cap fade in, the rest fade out over 8 ms; a node is
       rendered while its fade is above zero. Nothing is cut */
    int cap = l == 0 ? s->cap_nodes : s->cap_pad_voices;
    if (cap < 3) cap = 3; if (cap > MAX_VOICES) cap = MAX_VOICES;
    uint8_t in[MAX_NODES]; NOUNROLL for (int i = 0; i < MAX_NODES; i++) in[i] = 0;   /* (no memset: this runs from RAM and calls nothing in the flash) */
    int cand = 0;                                                    /* (the usual case: no more are sounding than the cap: all of them) */
    float hl[MAX_NODES]; NOUNROLL for (int i = 0; i < MAX_NODES; i++) hl[i] = 0.f;   /* a voice's hits: it comes first */
    for (int h = 0; h < nh; h++) if (hs[h]->node < N) hl[hs[h]->node] += 4.f * (hs[h]->h0 > hs[h]->h1 ? hs[h]->h0 : hs[h]->h1);
    float lvs[MAX_NODES];
    for (int i = 0; i < N; i++) { float lv = (g->a[i] > L->vs[i].a ? g->a[i] : L->vs[i].a) + hl[i]; lvs[i] = lv; if (lv > 1e-4f) { in[i] = 1; cand++; } }
    if (cand > cap) {   /* (more than the cap: the loudest cap of them, in one pass; a tie to the lower node, as before) */
        int top[MAX_VOICES], nt = 0;
        for (int i = 0; i < N; i++) {
            in[i] = 0; float lv = lvs[i]; if (!(lv > 1e-4f)) continue;
            if (nt == cap && !(lv > lvs[top[nt - 1]])) continue;
            int p = nt < cap ? nt++ : nt - 1;
            while (p > 0 && lv > lvs[top[p - 1]]) { top[p] = top[p - 1]; p--; }
            top[p] = i;
        }
        for (int c = 0; c < nt; c++) in[top[c]] = 1;
    }
    float gstep = (float)n / (SR * .008f), inv_n = 1.f / (float)n, lg = L->level_g;
    /* The oscillators are floating point throughout: 32-bit table reads and FPU multiplies. (They were 16-bit table
       reads beside integer multiplies, the pattern fm1-base's bsp/docs/CPU.md reports as leaving a register a bit off
       on this chip; in an accumulator that is a click.) */
    uint32_t phm = 0; NOUNROLL for (int j = 0; j < MAX_HARM; j++) if (L->pho[j] != g_phop[l][j]) { phm |= 1u << j; g_phop[l][j] = L->pho[j]; }
    uint32_t blk = g_blk[l]++;
    int nv = 0, nosc = 0;
    const int capo = OSC_CAP, hard = capo + capo / 8;   /* (every oscillator counts against it: copies and tails too) */
    float thr = g_thr[l];
    float kw = thr > 2.f * OSC_FLOOR ? 1.f : (thr - OSC_FLOOR) * (1.f / OSC_FLOOR);   /* (the knee deepens as the floor rises) */
    float fc = 250.f * fexp2(6.f * tone), ifc2 = 1.f / (fc * fc);   /* (Tone's corner: the layer's, worked out once) */
    for (int i = 0; i < N; i++) {
        VoiceSt *v = &L->vs[i]; Node *nd = &e->node[i];
        float g0 = v->gv, g1 = in[i] ? fclamp(g0 + gstep, 0.f, 1.f) : fclamp(g0 - gstep, 0.f, 1.f);
        v->gv = g1;
        float a0 = v->a, a1 = g->a[i];
        v->a = a1;
        float f = nd->freq * oct;
        /* the hits on this voice: their level at the tick's start and end (reached from the sample the newest lands
           on), and their character, each in proportion to the hits' share of the voice's level */
        float H0 = 0.f, H1 = 0.f, tilt = 0.f, snap = 0.f, oe = 0.f, hpl = -1.f, om = 0.f; int off = 0;
        if (hl[i] > 0.f) {
            float hw = 0.f;
            for (int h = 0; h < nh; h++) if (hs[h]->node == i) {
                const DrumHit *x = hs[h]; float m = .5f * (x->h0 + x->h1);
                H0 += x->h0; H1 += x->h1; hw += m;
                tilt += m * x->w * x->tilt; snap += m * x->w * x->snap;
                oe += m * x->w * x->oe;
                { float q = m * x->w * (x->oe < 0.f ? -x->oe : x->oe); if (q > om) { om = q; v->hd = (int8_t)x->d; } }   /* (its strongest push) */
                if (x->pl > hpl && x->h1 > 0.f) hpl = x->pl;   /* (a hit still sounding: its Pluck) */
                if (x->off > off) off = x->off;
            }
            float sh = hw > 1e-9f ? 1.f / hw : 0.f;           /* the hits' own character (their mean), not their share of */
            tilt *= sh; snap *= sh; oe *= sh;     /* the note's level. Timbre only: the pitch is never touched */
        }
        if (((g0 <= 0.f && g1 <= 0.f) || (a0 < 1e-6f && a1 < 1e-6f)) && H0 < 1e-6f && H1 < 1e-6f) { v->on = 0; continue; }
        if (f > .45f * SR) { v->on = 0; continue; }
        if (!v->on) { v->ph = 0; v->ph2 = (uint32_t)i * 0x9E3779B9u; v->on = 1; v->age = 0.f; v->pk = 0.f; v->hp = 0.f; v->hq = 0.f; v->pq = 1.f; v->oe = 0.f; v->hd = -1; for (int z = 0; z < 21; z++) PL[l][i][z] = -1.f; }   /* a note starting: its oscillators from the phase law */
        float plv = plk;
        {   /* Pluck's clock: back to 0 while the note's level (and its hits) rises past where it was. A drum's Pluck
               holds on the note after its hit: until the note rises by itself, or, 2 s without a plucking hit, let go
               over a second (the partials come back slowly, never in a step) */
            float lv = a1 * g1 + H1, dt = (float)n * (1.f / SR);
            int rise = lv > v->pk * 1.05f + 1e-4f;
            if (rise) v->age = 0.f; else v->age += dt;
            v->pk = lv > v->pk ? lv : v->pk * (1.f - dt * 2.f);   /* (let down over ~0.5 s) */
            if (hpl > 0.f) { if (rise || hpl > v->hp) v->hp = hpl; v->hq = 0.f; }
            else {
                v->hq += dt;
                if (rise && H1 <= 0.f) v->hp = 0.f;
                else if (v->hq > 2.f && v->hp > 0.f) { v->hp -= dt; if (v->hp < 0.f) v->hp = 0.f; }
            }
            if (v->hp > plv) plv = v->hp;
        }
        /* a hit's Odd/even: this note's own colour, pushed (else the layer's, worked out once) */
        /* (both glide toward where they are going, about 4 ms: a partial's level is set once a block, and a step of
           the whole spectrum at a hit's start would be a click) */
        v->oe += .35f * (oe - v->oe);
        /* (the layer's colour is worked out once a tick, voices_tick; a note a hit pushes, here) */
        float vam[MAX_HARM]; const float *av = L->amc;
        if ((v->oe > .02f || v->oe < -.02f) && v->hd >= 0) {     /* toward its drum's colour (drums.c), by its share */
            const float *ha = s->dr.oa[v->hd]; float of = s->dr.of[v->hd], t = of != 0.f ? fclamp(v->oe / of, 0.f, 1.f) : 0.f;
            NOUNROLL for (int j = 0; j < MAX_HARM; j++) vam[j] = L->amc[j] + t * (ha[j] - L->amc[j]);
            av = vam;
        }
        nv++;
#ifdef OSC_SHADOW
        g_shv += n;
#endif
        uint32_t inc8 = (uint32_t)(f * (4294967296.f / (8.f * SR))), inc = inc8 * 8u, ph8 = v->ph;   /* (an eighth of its pitch) */
        uint16_t pmk = L->pmask[i];
        /* Detune: the note's first three partials are doubled by a copy a few cents away (up to 14, each note its own
           amount and side), so every note beats slowly against itself and across the stereo field: the movement a
           single phase-locked oscillator cannot have. At 0 there is no copy. */
        int dup = det > .02f;
        float cents = 14.f * det * det * (.6f + .8f * (float)((i * 7) % 5) * .25f) * ((i & 1) ? 1.f : -1.f);
        uint32_t inc2 = (uint32_t)(f * (1.f + cents * 5.7779e-4f) * (4294967296.f / SR));
        /* Bloom: quieter is darker, each partial step down by exp(-0.25 Bloom (1 - loudness)) */
        float lb = a0 > a1 ? a0 : a1; if (H1 > lb) lb = H1;
        float br = fclamp(lb * 1.6f, 0.f, 1.f), dm = fexp2(-.36f * bloom * (1.f - br)), bm = 1.f;
        float pl_r = pluck_rate(plv);
        float pq = pl_r > 0.f ? fexp2(-pl_r * v->age * 1.4427f) : 1.f, pm = 1.f;   /* Pluck: each partial step this much lower */
        if (pq < 1e-3f) pq = 1e-3f;
        if (pq > v->pq) pq = v->pq + .35f * (pq - v->pq);         /* (opening: a glide; closing is slow already) */
        v->pq = pq;
        float wq = wave, wnorm = wn0;
        if (snap > 0.f) { wq = fclamp(wave + snap, 0.f, 2.5f); wnorm = frsqrt(1.f + .359f * wq * wq); }   /* and its Snap: a push on Wave, past its top */
        /* Tone: where the note's partials stop, in Hz (250 Hz to 16 kHz), falling 24 dB an octave above it. Not a
           filter: each oscillator's level is set by its own frequency, so nothing rings and nothing is smeared; a high
           note under a low Tone is just its fundamental, then nothing. Higher notes are a little quieter too
           (1.5 dB an octave above 350 Hz): the ear is keener there. */
        ToneC *tc = &TC[l][i];
        if (tc->f != f || tc->ifc2 != ifc2) {         /* (a new pitch or Tone: its weights again) */
            if (tc->f != f) { tc->reg = f > 350.f ? fexp2(-.25f * flog2(f * (1.f / 350.f))) : 1.f;   /* (the register's: by pitch only) */
                              DSP_FAR(pan_keys)(f, tc->pk); }
            tc->f = f; tc->ifc2 = ifc2;
            NOUNROLL for (int j = 0; j < MAX_HARM; j++) {
                float fk = f * PR[j], r2 = fk * fk * ifc2, r4 = r2 * r2;
                tc->tn[j] = 1.f / (1.f + r4); tc->wr[j] = (1.f + r4) / (1.f + 39.06f * r2 * r2);
            }
            for (int z = 0; z < 2; z++) { float mid = f * (z ? 46.f : 23.f), r2 = mid * mid * ifc2; tc->tt[z] = 1.f / (1.f + r2 * r2); }
        }
        float reg = tc->reg;
        /* Nothing low is cut: a note's partials reach down to its undertones (Depth), and only Clarity's mask and the
           20 Hz floor (in the mask) keep them out */
        /* a hit's Bright (a tilt per octave, settling: drums.c): a weight for each partial, then all of them together at
           the note's energy as it was (brighter, not louder: at most +-12 dB) */
        /* Bright up tilts toward the harmonics; Bright down (Dark) toward the undertones, and brings them in (up to the
           fundamental's level an octave down, half two down, a quarter three) even where Depth has none: a hit that
           starts low and settles up to the note, the kick's and the tom's fall without a pitch sweep. Both at the
           note's energy (the partials that play: Clarity's mask), and the fundamental never more than 6 dB down: the
           crack stays, the note does not turn into its top (or its bottom) */
        float wk[MAX_HARM], ab[MAX_HARM]; int wo = tilt != 0.f; const float *ap = av;
        if (wo) {
            float e0 = 0.f, e1 = 0.f, b = 1.f, dk = tilt < 0.f ? -2.f * tilt * av[0] : 0.f;
            NOUNROLL for (int j = 0; j < MAX_HARM; j++) {
                float tn = (j < NH ? b : 1.f) * tc->tn[j];
                ab[j] = av[j] + (j >= NH ? dk * (float)(8 >> (j - NH)) * .125f : 0.f);   /* (1, 1/2, 1/4 of it) */
                float w = fexp2(tilt * LOG2R[j]);
                wk[j] = w;
                if (pmk & (1u << j)) { float a = av[j] * tn, a2 = ab[j] * tn; e0 += a * a; e1 += a2 * a2 * w * w; }
                if (j < NH) b *= dm;
            }
            float nr = e1 > 1e-12f ? e0 / e1 : 1.f; if (nr > 16.f) nr = 16.f; if (nr < 1.f / 16.f) nr = 1.f / 16.f;
            nr *= frsqrt(nr);                             /* (its root: no call into the flash) */
            float w0 = wk[0] * nr, cap = w0 < .5f ? .5f / (1.f - w0) : 1.f;   /* (the fundamental: at most -6 dB) */
            NOUNROLL for (int k = 0; k < MAX_HARM; k++) wk[k] = 1.f + cap * (wk[k] * nr - 1.f);
            ap = ab;
        }
        /* the voice's place, level and hits, worked out before its oscillators now (they take them in their own level) */
        float pn = fclamp(.5f + (nd->x - .5f) * width * .9f, 0.f, 1.f);   /* place on the surface, by Width */
        float gl = fsin(.25f + pn * .25f) * lg * wnorm, gr = fsin(pn * .25f) * lg * wnorm;
        const float gc = .70710678f * lg * wnorm, dgl = gl - gc, dgr = gr - gc;   /* (the middle; a low partial's place nearer it) */
        /* the voice's own level, under the kick's duck; the hits' on top (not ducked), from the sample they land. Its
           value before the first sample (K0), at the sample before the newest hit lands (Km) and at the last (K1): a
           straight line between them, so an oscillator's level is too (a hit inside the block: two lines) */
        float amp0 = a0 * g0 * dk0, amp1 = a1 * g1 * dk1, da = (amp1 - amp0) * inv_n;
        float hsc = lg > 1e-6f ? DR_OUT / lg : 0.f;                  /* (the hits' own gain: not the layer's level staging) */
        float hv0 = hl[i] > 0.f ? H0 * hsc : 0.f, hv1 = hl[i] > 0.f && off < n ? H1 * hsc : hv0;
        int n1 = hl[i] > 0.f && off > 0 && off < n ? off : n;     /* (the first line's length) */
        float K0 = amp0 + hv0, K1 = amp1 + hv1;
        float Km = n1 < n ? amp0 + da * (float)n1 + hv0 : K1;   /* (one line: it ends at K1, a hit's rise from the block's start included) */
        float i1 = 1.f / (float)n1, i2 = n1 < n ? 1.f / (float)(n - n1) : 0.f;
        /* the stereo: side = 0.6 Width (odd - even) + 0.8 Width (+-1) on the detuned copies; partials 2 and 3 their own
           (on a detuned note their side is not taken down with their share of the level, so it is 1/0.8125 of their
           mid's). Each oscillator's left gain is gl (1 + side), its right gr (1 - side) */
        float swo = .6f * width, w23 = dup ? swo * (1.f / .8125f) : swo, wd = !dup ? 0.f : (i & 1) ? .8f * width : -.8f * width;
        float *pl = PL[l][i];
        /* (while a hit sounds on the note, or its own level rises fast, the partials take this block's levels at once,
           as they always did: the drums' bite is partly that step, block to block (ramped, a snare lost about 7 dB above
           1.6 kHz), and an attack's crack comes with it. Elsewhere they ramp: no steps in a note's Bloom, Pluck or
           colour as it moves) */
        int jump = hl[i] > 0.f || K1 > 1.25f * K0 + 1e-6f;
        int vsync = ((blk + (uint32_t)i * 7u) & 63u) == 0u;   /* (this voice's resync, a block in 64) */
        /* an oscillator's level line: from its last level pa (times K0) to its new one ai (times K1), through Km at n1 */
        /* (a hit landing inside the block: the old levels up to the sample it lands on, its own from there, so a
           hit's colour comes with it and not before; else the levels move across the block) */
        OSeg sg; sg.L = Lo; sg.R = Ro; sg.n1 = n1; sg.n = n;
        #define LEVELS(pa_, ai_) do { float A0 = (pa_) * K0, Am = n1 < n ? (pa_) * Km : (ai_) * K1, A1 = (ai_) * K1; \
                                      sg.A0 = A0; sg.d1 = (Am - A0) * i1; sg.d2 = (A1 - (ai_) * Km) * i2; sg.Am2 = (ai_) * Km; } while (0)
        /* each partial: a harmonic (slots 0..12: Bloom and Pluck step down along them) or an undertone (13..15: they
           keep their level, and are the last to go; no Wave on them, its 3rd would be out of tune) */
        for (int j = 0; j < MAX_HARM; j++) {
            int hm = j < NH;
            float sb = hm ? bm * pm : 1.f;
            if (hm) { bm *= dm; pm *= pq; }
            if (!(pmk & (1u << j))) { pl[j] = 0.f; if (j < 3) pl[16 + j] = 0.f; continue; }   /* (Clarity's mask, and the 20 Hz floor) */
            float fk = f * PR[j], ak = ap[j] * sb * reg * tc->tn[j];
            if (wo) ak *= wk[j];
            if (ak < thr || (nosc >= hard && j > 0)) { pl[j] = 0.f; if (j < 3) pl[16 + j] = 0.f; continue; }   /* (under -42 dB of the note, or the cap's floor; or a burst past the cap) */
            nosc++;
            float ai = ak;
            if (kw > 0.f && ak < 2.f * thr) ai *= 1.f - kw * (1.f - (ak - thr) / thr);   /* the cap's knee */
            int k = j + 1;
            float sd = !hm || k == 1 ? 0.f : k == 2 ? -w23 : k == 3 ? w23 : (k & 1) ? swo : -swo;   /* (undertones: the centre) */
            float kp = (float)tc->pk[j] * (1.f / 255.f);  /* (its share of the stereo, by its frequency) */
            float glp = gc + dgl * kp, grp = gc + dgr * kp; sd *= kp;
            float glk = glp * (1.f + sd), grk = grp * (1.f - sd);
            uint32_t ph = ph8 * PM[j] + (uint32_t)(L->pho[j] * 4294967296.f), ik = inc8 * PM[j];
            const float *D = !hm ? 0 : 3.f * fk < .45f * SR ? T_D1 : 2.f * fk < .45f * SR ? T_D2 : 0;
            /* Wave's 2nd and 3rd of this partial sit at 2 and 3 times its frequency: under Tone there, not here (taken
               at 2.5 times: their middle). Before, they kept this partial's level and climbed over Tone's slope by an
               octave and a half: the top end hot whatever Tone said */
            float w = D ? wq * tc->wr[j] : 0.f;
            int cp = dup && k <= 3 && nosc < hard;        /* (its detuned copy: under the cap) */
            if (cp) ai *= .8125f;                      /* (shared with its detuned copy: the pair at the same power) */
            float pa_ = pl[j], pa = pa_ >= 0.f && !jump ? pa_ : ai; pl[j] = ai;
            LEVELS(pa, ai); sg.gl = glk; sg.gr = grk;
#ifdef OSC_SHADOW
            sg.ph = ph; sg.ik = ik; sg.c2 = .5f * w; sg.c3 = D == T_D1 ? .33f * w : 0.f;
#endif
            if (w == 0.f || k <= WAVE_EXACT) {                /* exact: plain, or Wave on the lowest partials */
                float st[5]; mcf_seed(ph, ik, st, PS[l][i][j], &PK[l][i][j], vsync || pa_ <= 0.f || ((phm >> j) & 1u));
                if (w == 0.f) o_mcf(&sg, st);
                else {
                    float c2 = .5f * w, c3 = D == T_D1 ? .33f * w : 0.f, icw = 1.f / mcf_cos(st, ik);
                    float Kw[4] = {1.f + 3.f * c3, 2.f * c2 * st[3] * icw, -2.f * c2 * icw, -4.f * c3};
                    o_mcfw(&sg, st, Kw);
                }
                PS[l][i][j][0] = st[0]; PS[l][i][j][1] = st[1];
            } else {                                          /* the table, with Wave's 2nd and 3rd (partials 2-13) */
                PS[l][i][j][1] = 1e9f;                        /* (back on the recursion: from its phase) */
                o_tabw(&sg, ph, ik, T_SIN, D, w);
            }
            if (cp) {                                         /* the detuned copy of the first three partials (exact) */
                nosc++;
                float ad = ai * .625f, pd_ = pl[16 + j], pd = pd_ >= 0.f && !jump ? pd_ : ad; pl[16 + j] = ad;
                uint32_t q = v->ph2 * (uint32_t)k, iq = inc2 * (uint32_t)k;
                float st[5]; mcf_seed(q, iq, st, PS[l][i][16 + j], &PK[l][i][16 + j], vsync || pd_ <= 0.f);
                LEVELS(pd, ad); sg.gl = glp * (1.f + wd * kp); sg.gr = grp * (1.f - wd * kp);
#ifdef OSC_SHADOW
                sg.ph = q; sg.ik = iq;
#endif
                o_mcf(&sg, st);
                PS[l][i][16 + j][0] = st[0]; PS[l][i][16 + j][1] = st[1];
            } else if (j < 3) pl[16 + j] = 0.f;
        }
        /* a low note has more than thirteen harmonics in the audible range: the rest (14..32, and 33..64) come from one
           table each, continuing at the 13th's level and a saw's slope, under Tone like the others (at their middle) */
        for (int tl = 0; tl < 2; tl++) {
            float top = f * (tl ? 64.f : 32.f);
            float ak = av[NH - 1] * pm * reg * tc->tt[tl];
            if (top > .45f * SR || ak < 4e-3f || !(pmk & (1u << (NH - 1))) || nosc >= hard) { for (int z = tl; z < 2; z++) pl[19 + z] = 0.f; break; }
            nosc++;
            float ai = ak, pa = pl[19 + tl] >= 0.f && !jump ? pl[19 + tl] : ai; pl[19 + tl] = ai;
            LEVELS(pa, ai); sg.gl = gl; sg.gr = gr;
#ifdef OSC_SHADOW
            sg.ph = ph8 * 8u; sg.ik = inc; sg.tl = tl;
#endif
            o_tail(&sg, ph8 * 8u, inc, tl ? T_T2 : T_T1);
        }
        #undef LEVELS
        v->ph2 += inc2 * (uint32_t)n;
        v->ph = ph8 + inc8 * (uint32_t)n;
    }
#ifdef OSC_SHADOW
    for (int t = 0; t < n; t++) { g_eb[l][0][t] = (float)((Lo[t] - L0_[t]) - g_shL[t]); g_eb[l][1][t] = (float)((Ro[t] - R0_[t]) - g_shR[t]);
                                   g_xb[l][0][t] = (float)g_shL[t]; g_xb[l][1][t] = (float)g_shR[t]; }
#endif
    if (nosc > capo) { thr *= 1.3f; if (thr > .1f) thr = .1f; }
    else if (thr > OSC_FLOOR && nosc < capo - capo / 8) { thr *= .92f; if (thr < OSC_FLOOR) thr = OSC_FLOOR; }
    g_thr[l] = thr;
    L->n_voices = nv; L->n_osc = nosc;
}

/* layer l's waveform (the bank's function as it stands, sines) at phase ph, for a pitch f: the drums' Synth kit */
DSP_RAM float layer_wave(const Synth *s, int l, float ph, float f) {
    const float *a = s->layer[l].amp;
    float c2 = 2.f * fsin(ph + .25f), s0 = 0.f, s1 = fsin(ph), y = a[0] * s1;
    NOUNROLL for (int k = 2; k <= NH && (float)k * f < .45f * SR; k++) { float s2 = c2 * s1 - s0; s0 = s1; s1 = s2; y += a[k - 1] * s2; }
    return y;
}
