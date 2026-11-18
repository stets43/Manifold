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
static void clarity_mask(Synth *s, int l, const float *key);
#include "ctl_on.h"
DSP_CTL static float pw(float x, float e) { return x <= 0.f ? 0.f : fexp2(e * flog2(x)); }

/* the bank's function itself: a bell over the partials (centre c, width w), a tilt k^te, a comb of stride st; unit
   energy over partials 1..kmax. The voices use it, and the drums (each drum a bank of its own) */
DSP_CTL void bank_fn(float c, float w, float te, float st, int kmax, float *a) {
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
DSP_CTL static void bank_levels(const Synth *s, int l, float *a) {
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
DSP_CTL static void bank_phases(const Synth *s, int l, float *p) {
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
        L->mkey[0] = -1.f;                                    /* (the mask: at the next tick) */
        for (int i = 0; i < MAX_NODES; i++) L->pmask[i] = 0;
        DSP_FAR(bank_phases)(s, l, L->pho);
    }
}

/* Clarity: a hard limit on how many partials, of all the layer's notes, each octave may hold (octaves from 20 Hz:
   20-40, 40-80, ...: 1, 2, 3, 5, 8, 13, ... at full Clarity, 16 times that at none: no limit). Every note's
   fundamental plays; then, from the lowest note up, each note's undertones and harmonics (the nearest first) take what
   room is left. A note's place in it does not depend on whether it sounds (so nothing comes and goes as notes do):
   the low notes have the low octaves, the chord above does not crowd under them. Nothing under 20 Hz is ever played.
   Worked out when the notes, the register or Clarity change; the render reads a bit a partial */
#include "ctl_off.h"
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
            int o = (int)flog2(f * (1.f / 20.f)); if (o > 10) o = 10;
            L->pmask[i] = 1; cnt[o]++;
        }
    }
    for (int i = 0; i < N; i++) {
        float f = e->node[i].freq * oct;
        for (int q = 0; q < MAX_HARM - 1; q++) {
            int j = ORD[q]; float fj = f * PR[j];
            if (fj < 20.f || fj >= .45f * SR) continue;
            int o = (int)flog2(fj * (1.f / 20.f)); if (o > 10) o = 10;
            if (cl > .001f && (float)cnt[o] >= BASE[o] * m) continue;
            L->pmask[i] |= (uint16_t)(1u << j); cnt[o]++;
        }
    }
    for (int i = N; i < MAX_NODES; i++) L->pmask[i] = 0;
}
#include "ctl_on.h"
DSP_CTL void voices_tick(Synth *s, int l) {
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

#include "ctl_off.h"
/* (the RAM code has little room: the render's short set-up loops stay loops, the oscillators are what is unrolled) */
#if defined(__clang__)
#define NOUNROLL _Pragma("clang loop unroll(disable) vectorize(disable)")
#else
#define NOUNROLL
#endif
/* Pluck's rate (1/T, per partial step, per second) for an amount: T from 1.5 s down to 25 ms; 0 off */
DSP_INL float pluck_rate(float p) { return p > .01f ? (1.f / 1.5f) * fexp2(6.f * p) : 0.f; }
DSP_RAM void layer_render(Synth *s, int l, float *Lo, float *Ro, int n) {
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
    for (int i = 0; i < N; i++) { float lv = (g->a[i] > L->vs[i].a ? g->a[i] : L->vs[i].a) + hl[i]; if (lv > 1e-4f) { in[i] = 1; cand++; } }
    if (cand > cap) for (int i = 0; i < N; i++) in[i] = 0;
    for (int c = 0; c < cap && cand > cap; c++) {
        int bi = -1; float bl = 1e-4f;
        for (int i = 0; i < N; i++) if (!in[i]) { float lv = (g->a[i] > L->vs[i].a ? g->a[i] : L->vs[i].a) + hl[i]; if (lv > bl) { bl = lv; bi = i; } }
        if (bi < 0) break;
        in[bi] = 1;
    }
    float gstep = (float)n / (SR * .008f), inv_n = 1.f / (float)n, lg = L->level_g;
    /* The oscillators are floating point throughout: 32-bit table reads and FPU multiplies. (They were 16-bit table
       reads beside integer multiplies, the pattern fm1-base's bsp/docs/CPU.md reports as leaving a register a bit off
       on this chip; in an accumulator that is a click.) */
    /* Each oscillator adds into one buffer of four (one accumulate a sample, not two): the fundamental and the tails
       (centre), the odd partials, the even ones, and the detuned copies. Mid and side are formed once a voice from
       them: side = 0.6 Width (odd - even) + 0.8 Width (+-1) detuned. Partials 2 and 3 have buffers of their own: on a
       detuned note their side is not taken down with their share of the level (it never was), so it is 1/0.8125 of
       their mid's. (One way through for every note: rolls nearly always have Spin and Width, and the RAM is short.) */
    static float accB[NUM_LAYERS][6][SUB] DSP_POOL;                  /* (a layer each: layer 2 may run on the second core) */
    float *aC = accB[l][0], *aO = accB[l][1], *aE = accB[l][2], *aD = accB[l][3], *a2 = accB[l][4], *a3 = accB[l][5];
    int nv = 0, nosc = 0;
    const int capo = OSC_CAP, hard = capo + capo / 4;
    float thr = g_thr[l];
    float kw = thr > 2.f * OSC_FLOOR ? 1.f : (thr - OSC_FLOOR) * (1.f / OSC_FLOOR);   /* (the knee deepens as the floor rises) */
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
        if (!v->on) { v->ph = 0; v->ph2 = (uint32_t)i * 0x9E3779B9u; v->on = 1; v->age = 0.f; v->pk = 0.f; v->hp = 0.f; v->hq = 0.f; v->pq = 1.f; v->oe = 0.f; v->hd = -1; }   /* a note starting: its oscillators from the phase law */
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
        float fc = 250.f * fexp2(6.f * tone), ifc2 = 1.f / (fc * fc);
        float reg = f > 350.f ? fexp2(-.25f * flog2(f * (1.f / 350.f))) : 1.f;
        /* Nothing low is cut: a note's partials reach down to its undertones (Depth), and only Clarity's mask and the
           20 Hz floor (in the mask) keep them out */
        /* a hit's Bright (a tilt per octave, settling: drums.c): a weight for each partial, then all of them together at
           the note's energy as it was (brighter, not louder: at most +-12 dB) */
        float wk[MAX_HARM]; int wo = tilt != 0.f;
        if (wo) {
            float e0 = 0.f, e1 = 0.f, b = 1.f;
            NOUNROLL for (int j = 0; j < MAX_HARM; j++) {
                float fk = f * PR[j], r2 = fk * fk * ifc2, a = av[j] * (j < NH ? b : 1.f) / (1.f + r2 * r2);
                float w = fexp2(tilt * LOG2R[j]);
                wk[j] = w; e0 += a * a; e1 += a * a * w * w;
                if (j < NH) b *= dm;
            }
            float nr = e1 > 1e-12f ? e0 / e1 : 1.f; if (nr > 16.f) nr = 16.f; if (nr < 1.f / 16.f) nr = 1.f / 16.f;
            nr *= frsqrt(nr);                             /* (its root: no call into the flash) */
            NOUNROLL for (int k = 0; k < MAX_HARM; k++) wk[k] *= nr;
        }
        for (int t = 0; t < n; t++) { aC[t] = 0.f; aO[t] = 0.f; aE[t] = 0.f; aD[t] = 0.f; a2[t] = 0.f; a3[t] = 0.f; }
        /* each partial: a harmonic (slots 0..12: Bloom and Pluck step down along them) or an undertone (13..15: they
           keep their level, and are the last to go; no Wave on them, its 3rd would be out of tune) */
        for (int j = 0; j < MAX_HARM; j++) {
            int hm = j < NH;
            float sb = hm ? bm * pm : 1.f;
            if (hm) { bm *= dm; pm *= pq; }
            if (!(pmk & (1u << j))) continue;                 /* (Clarity's mask, and the 20 Hz floor) */
            float fk = f * PR[j], r2 = fk * fk * ifc2, ak = av[j] * sb * reg / (1.f + r2 * r2);
            if (wo) ak *= wk[j];
            if (ak < thr) continue;                       /* (under -42 dB of the note, or the cap's floor: not played) */
            if (nosc >= hard && j > 0) continue;              /* (a burst past the cap, before the floor has risen) */
            nosc++;
            float ai = ak;
            if (kw > 0.f && ak < 2.f * thr) ai *= 1.f - kw * (1.f - (ak - thr) / thr);   /* the cap's knee */
            int k = j + 1;
            float *A = !hm || k == 1 ? aC : k == 2 ? a2 : k == 3 ? a3 : (k & 1) ? aO : aE;   /* (undertones: the centre) */
            uint32_t ph = ph8 * PM[j] + (uint32_t)(L->pho[j] * 4294967296.f), ik = inc8 * PM[j];
            const float *D = !hm ? 0 : 3.f * fk < .45f * SR ? T_D1 : 2.f * fk < .45f * SR ? T_D2 : 0;
            float w = D ? wq : 0.f;
            if (dup && k <= 3) ai *= .8125f;           /* (shared with its detuned copy: the pair at the same power) */
            if (w == 0.f) {                                         /* the plain oscillator */
                for (int t = 0; t < n; t++) { A[t] += T_SIN[ph >> OSC_SH] * ai; ph += ik; }
            } else {                                          /* with Wave's addition (its 2nd and 3rd harmonic) */
                float aw = ai * w;
                for (int t = 0; t < n; t++) { uint32_t x = ph >> OSC_SH; A[t] += T_SIN[x] * ai + D[x] * aw; ph += ik; }
            }
            if (dup && k <= 3) {                              /* the detuned copy of the first three partials */
                nosc++;
                float ad = ai * .625f;
                uint32_t q = v->ph2 * (uint32_t)k, iq = inc2 * (uint32_t)k;
                for (int t = 0; t < n; t++) { aD[t] += T_SIN[q >> OSC_SH] * ad; q += iq; }
            }
        }
        /* a low note has more than thirteen harmonics in the audible range: the rest (14..32, and 33..64) come from one
           table each, continuing at the 13th's level and a saw's slope, under Tone like the others (at their middle) */
        for (int tl = 0; tl < 2; tl++) {
            float top = f * (tl ? 64.f : 32.f), mid = f * (tl ? 46.f : 23.f), r2 = mid * mid * ifc2;
            float ak = av[NH - 1] * pm * reg / (1.f + r2 * r2);
            if (top > .45f * SR || ak < 4e-3f || !(pmk & (1u << (NH - 1)))) break;
            nosc++;
            float ai = ak;
            const float *T = tl ? T_T2 : T_T1; uint32_t ph = ph8 * 8u;
            for (int t = 0; t < n; t++) { aC[t] += T[ph >> 20] * ai; ph += inc; }
        }
        v->ph2 += inc2 * (uint32_t)n;
        v->ph = ph8 + inc8 * (uint32_t)n;
        float pn = fclamp(.5f + (nd->x - .5f) * width * .9f, 0.f, 1.f);   /* place on the surface, by Width */
        float gl = fsin(.25f + pn * .25f) * lg * wnorm, gr = fsin(pn * .25f) * lg * wnorm;
        /* the voice's own level, under the kick's duck; the hits' on top (not ducked), from the sample they land */
        float amp = a0 * g0 * dk0, da = (a1 * g1 * dk1 - amp) * inv_n;
        float hsc = lg > 1e-6f ? DR_OUT / lg : 0.f;                  /* (the hits' own gain: not the layer's level staging) */
        float hv = H0 * hsc, dh = off < n ? (H1 - H0) * hsc / (float)(n - off) : 0.f;
        {   float wo = .6f * width, w23 = dup ? wo * (1.f / .8125f) : wo, wd = !dup ? 0.f : (i & 1) ? .8f * width : -.8f * width;
            if (hl[i] == 0.f) for (int t = 0; t < n; t++) {           /* (no hit on it: the plain mix) */
                amp += da;
                float o = aO[t], ev = aE[t], dd = aD[t], p2 = a2[t], p3 = a3[t];
                float m = (aC[t] + o + ev + dd + p2 + p3) * amp, sd = (wo * (o - ev) + w23 * (p3 - p2) + wd * dd) * amp;
                Lo[t] += (m + sd) * gl; Ro[t] += (m - sd) * gr;
            } else for (int t = 0; t < n; t++) {
                amp += da; if (t >= off) hv += dh;
                float lev = amp + hv;
                float o = aO[t], ev = aE[t], dd = aD[t], p2 = a2[t], p3 = a3[t];
                float m = (aC[t] + o + ev + dd + p2 + p3) * lev, sd = (wo * (o - ev) + w23 * (p3 - p2) + wd * dd) * lev;
                Lo[t] += (m + sd) * gl; Ro[t] += (m - sd) * gr;
            }
        }
    }
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
