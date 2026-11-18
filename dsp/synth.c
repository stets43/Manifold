/*
 * Top level: the control tick (morph, modulation, the rhythm's clock and events, both generators, the oscillator
 * banks' functions), then the audio: layer 2 (on the second core when there is one), layer 1, their mix, the drums,
 * then the effects. Every section is timed for the profiler when s->clock is set.
 *
 * Gain staging: every bank is at unit energy, every node plays at its loudness a_n (0..1), a layer's sum is scaled by
 * Level x LAYER_GAIN over the root of how much is sounding. The effects end in drive, the bus compressor and the
 * look-ahead limiter (fx.c), which make it loud.
 */
#define MANIFOLD_DSP_UNIT
#include "synth.h"
#include "mstring.h"

#define LAYER_GAIN 0.17f

static Synth *g_job_synth;                 /* the second core's job reads its arguments from here */

void synth_restart(Synth *s, uint32_t seed) {
    s->meter.tau = 0.f; s->meter.bar = 0;
    for (int l = 0; l < NUM_LAYERS; l++) {                /* the keys held carry over */
        GenSt *g = &s->layer[l].g; uint8_t h[MAX_NODES]; float hv[MAX_NODES];
        memcpy(h, g->held, sizeof h); memcpy(hv, g->held_v, sizeof hv);
        gen_reset(g, seed + 77u * (uint32_t)l);
        memcpy(g->held, h, sizeof h); memcpy(g->held_v, hv, sizeof hv);
    }
}

void synth_init(Synth *s, uint32_t seed) {
    static int meta_done = 0;
    if (!meta_done) { params_init_meta(); voices_init(); meta_done = 1; }
    uint32_t (*clk)(void) = s->clock;      /* keep the platform's hooks across a re-init */
    int (*pr)(void (*)(uint32_t), uint32_t) = s->par_run; int (*pw)(void) = s->par_wait; void (*ps)(void) = s->par_sync;
    memset(s, 0, sizeof *s);
    s->clock = clk; s->par_run = pr; s->par_wait = pw; s->par_sync = ps;
    for (int i = 0; i < NUM_PARAMS; i++) s->base[i] = s->target[i] = s->mod[i] = g_param_meta[i].def;
    params_cache_all(s);
    s->rng = seed * 2654435761u + 7u; s->seed = seed;
    s->duck = s->duck_tgt = 1.f;
    s->dr.dk0 = s->dr.dk1 = 1.f; s->dr.shape = s->dr.shape_tgt = 1.f;   /* (no duck yet; Shape by at rest) */
    s->style = STYLE_AMBIENT;
    s->cap_pad_voices = MAX_VOICES;  s->cap_nodes = MAX_VOICES; s->cap_harm = MAX_HARM;
    s->transport = 1;
    s->lay_t[0] = s->lay_t[1] = s->lay_g[0] = s->lay_g[1] = 1.f;
    fx_init(&s->fx);
    for (int l = 0; l < NUM_LAYERS; l++) s->layer[l].eavg = .25f;
    synth_restart(s, seed);
    s->meter.beats = 4.f; s->meter.bar_sec = 2.f;
    voices_build(s);
    engine_build(s);
    DSP_FAR(fx_update)(s);
}

/* the control tick's sine, 2^x and log2 (ctl_on.h): one copy each, in RAM */
DSP_CTL __attribute__((noinline)) float dsp_sin(float t) { return fsin(t); }
DSP_CTL __attribute__((noinline)) float dsp_exp2(float x) { return fexp2(x); }
DSP_CTL __attribute__((noinline)) float dsp_log2(float x) { return flog2(x); }

DSP_CTL static void control_tick(Synth *s) {
    const float dt = (float)SUB / SR;
    if (s->morphing) {
        float k = s->morph_rate * 4.f; if (k > 1.f) k = 1.f;
        float mx = 0;
        for (int i = 0; i < NUM_PARAMS; i++) {
            if (g_param_meta[i].structural) continue;
            float d = s->target[i] - s->base[i];
            s->base[i] += d * k; if (fabsf(d) > mx) mx = fabsf(d);
        }
        if (mx < 1e-4f) s->morphing = 0;
    }
    if (s->rebuild_pending && s->duck < 0.002f) {
        s->rebuild_pending = 0;
        if (s->build_defer) { s->build_req = 1; return; }   /* the main loop builds; the render waits, silent */
        DSP_FAR(engine_build)(s);
        s->duck_tgt = 1.f;
    }
    lfo_tick(s, dt);
    mod_apply(s);
    uint32_t c0 = s->clock ? s->clock() : 0u;
    rhythm_tick(s, dt);
    {   /* a modulating layer runs first, so the other reads it this tick */
        int mi = gen_modulator_r(s);
        if (mi >= 0) { gen_tick(s, mi, dt); gen_tick(s, 1 - mi, dt); }
        else for (int l = 0; l < NUM_LAYERS; l++) gen_tick(s, l, dt);
    }
    gen_harmony(s, dt);
    drums_tick(s, dt);
    if (s->clock) s->prof[PROF_SEQ] += s->clock() - c0;
    for (int l = 0; l < NUM_LAYERS; l++) {
        voices_tick(s, l);
        /* gain staging: Level, over the square root of how much is sounding (its energy, averaged over ~1.5 s: the
           rhythm's own dynamics pass, a dense function and a sparse one sit at the same level) */
        Layer *L = &s->layer[l]; float e = 0.f;
        for (int i = 0; i < s->eng.n_nodes; i++) e += L->g.a[i] * L->g.a[i];
        /* a 1 s average of e squared, read as its root: the same as the mean energy for a steady sound, larger
           for a peaky one (narrow pulses together), so a pulsing layer gets the headroom its peaks need. Still an
           average: a level, not an envelope; it does not follow the pulses */
        L->eavg += (e * e - L->eavg) * (dt / 1.f);
        float er = sqrtf(L->eavg);
        float want = pval(s, LP(l, P_V1_LEVEL)) * LAYER_GAIN * frsqrt(er > .08f ? er : .08f);
        if (pint(s, P_DR_ON)) want *= .45f;   /* -6.9 dB: the drums' share of the mix (their hits keep their own gain): room for their attacks under the limiter */
        L->level_g += (want - L->level_g) * .05f;
    }
    engine_update(s);
    fx_update(s);
    float on = s->layer[0].g.onset * 2.f; if (on > 1.f) on = 1.f;
    s->trig_env = on > s->trig_env * .97f ? on : s->trig_env * .97f;
    s->tick++;
}

DSP_CTL static int finite_block(const float *a, int n) {
    for (int i = 0; i < n; i++) if (!(a[i] == a[i]) || fabsf(a[i]) > 1e4f) return 0;
    return 1;
}

/* layer 2's sub-block, as a job for the second core (or called in line) */
DSP_CTL static void l2_job(uint32_t m) {
    Synth *s = g_job_synth;
    uint32_t c0 = s->clock ? s->clock() : 0u;
    memset(s->l2_l, 0, sizeof(float) * m); memset(s->l2_r, 0, sizeof(float) * m);
    DSP_FAR(layer_render)(s, 1, s->l2_l, s->l2_r, (int)m);
    if (s->clock) s->prof[PROF_PADWORK] += s->clock() - c0;
}

void synth_silence(Synth *s) {
    for (int l = 0; l < NUM_LAYERS; l++) {
        GenSt *g = &s->layer[l].g;
        for (int i = 0; i < MAX_NODES; i++) { g->a[i] = g->u[i] = g->kin[i] = g->hlev[i] = 0.f; g->held[i] = 0; s->layer[l].vs[i].a = 0.f;
            for (int q = 0; q < 4; q++) g->kvel[i][q] = 0.f; }
    }
    memset(s->held, 0, sizeof s->held);
}

void synth_build_now(Synth *s) {
    if (!s->build_req) return;
    engine_build(s);
    s->duck = 0.f; s->duck_tgt = 1.f;
    __asm__ volatile("" ::: "memory");
    s->build_req = 0;
}

#define CLK() (s->clock ? s->clock() : 0u)

DSP_CTL void synth_process(Synth *s, float *outL, float *outR, int n) {
    g_job_synth = s;
    uint32_t call0 = CLK();
    while (n > 0) {
        int m = n < SUB ? n : SUB;
        if (s->build_req) {                            /* the main loop is rebuilding: silence, touch nothing */
            memset(outL, 0, sizeof(float) * (size_t)n); memset(outR, 0, sizeof(float) * (size_t)n);
            return;
        }
        uint32_t c0 = CLK();
        control_tick(s);
        if (s->build_req) continue;
        uint32_t c2 = CLK();

        /* layer 2: on the second core while this one renders layer 1, or in line */
        int par = s->par_on && s->par_run && s->par_run(l2_job, (uint32_t)m);
        if (!par) l2_job((uint32_t)m);
        uint32_t c3 = CLK();

        float *L = s->sub_l, *R = s->sub_r;
        memset(L, 0, sizeof(float) * (size_t)m); memset(R, 0, sizeof(float) * (size_t)m);
        DSP_FAR(layer_render)(s, 0, L, R, m);
        uint32_t c4 = CLK();
        if (par) {
            if (s->par_wait() != 0) l2_job((uint32_t)m);   /* the second core gave up: do it here */
            if (s->par_sync) s->par_sync();
        }
        uint32_t c5 = CLK();
        {   /* the layers, each with its performance mute, faded over ~9 ms (no click) */
            float g1 = s->lay_g[0], g2 = s->lay_g[1], t1 = s->lay_t[0], t2 = s->lay_t[1];
            float n1 = t1 > g1 ? (t1 - g1 > .16f ? g1 + .16f : t1) : (g1 - t1 > .16f ? g1 - .16f : t1);
            float n2 = t2 > g2 ? (t2 - g2 > .16f ? g2 + .16f : t2) : (g2 - t2 > .16f ? g2 - .16f : t2);
            float d1 = (n1 - g1) / (float)m, d2 = (n2 - g2) / (float)m;
            for (int t = 0; t < m; t++) {
                g1 += d1; g2 += d2;
                L[t] = L[t] * g1 + s->l2_l[t] * g2; R[t] = R[t] * g1 + s->l2_r[t] * g2;
            }
            s->lay_g[0] = n1; s->lay_g[1] = n2;
        }
        /* (the drums are in the layers: their hits and the kick's duck ride the voices, voice.c) */
        float e2 = 0;
        for (int t = 0; t < m; t++) {
            s->duck += 0.004f * (s->duck_tgt - s->duck);
            L[t] *= s->duck; R[t] *= s->duck;
            e2 += L[t] * L[t] + R[t] * R[t];
        }
        s->eng.env_follow += 0.05f * (e2 / (2.f * (float)m) * frsqrt(e2 / (2.f * (float)m) + 1e-20f) - s->eng.env_follow);
        if (!finite_block(L, m) || !finite_block(R, m)) {  /* never let a blow-up through */
            DSP_FAR(synth_silence)(s);
            memset(L, 0, sizeof(float) * (size_t)m); memset(R, 0, sizeof(float) * (size_t)m);
        }
        DSP_FAR(fx_process)(s, L, R, m, s->trig_env);
        uint32_t c6 = CLK();
        memcpy(outL, L, sizeof(float) * (size_t)m); memcpy(outR, R, sizeof(float) * (size_t)m);
        outL += m; outR += m; n -= m;
        if (s->clock) {                                 /* the profiler's sums */
            s->prof[PROF_CTRL] += c2 - c0;
            s->prof[PROF_PAD] += par ? (c5 - c4) : (c3 - c2);
            s->prof[PROF_BANK] += c4 - c3;
            s->prof[PROF_FX] += c6 - c5;
            s->prof[PROF_TOTAL] += c6 - c0;
        }
    }
    s->prof_blocks++;
    if (s->clock) { uint32_t d = CLK() - call0; if (d > s->prof_peak) s->prof_peak = d; }
}

/* keys and MIDI: a push into the layer(s) the keys play */
static void push(Synth *s, int node, float vel, int on) {
    if (s->eng.n_nodes == 0) return;
    node %= s->eng.n_nodes;
    vel = fclamp(vel, 0.f, 1.f);
    if (s->keys_to == 3) {                     /* drum keys: key k plays drum k mod 4, tuned to its note */
        if (on) { drums_hit(s, node % NUM_DRUMS, node, vel < .05f ? .7f : vel); s->last_vel = vel; }
        return;
    }
    for (int l = 0; l < NUM_LAYERS; l++)
        if (s->keys_to == 2 || s->keys_to == l) gen_key(s, l, node, vel, on);
    if (on) s->last_vel = vel;
}
void synth_strike(Synth *s, int node, float velocity) { push(s, node, velocity, 1); push(s, node, 0.f, 0); }
void synth_note(Synth *s, int node, float velocity, int on) {
    if (s->eng.n_nodes == 0) return;
    node %= s->eng.n_nodes;
    s->held[node] = (uint8_t)(on != 0);
    push(s, node, velocity, on);
}
