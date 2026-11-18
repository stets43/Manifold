/*
 * Six LFOs (with loose Kuramoto coupling), the modulation matrix and the performance knobs' paths.
 * Everything is evaluated once per control tick; LFO -> LFO routes use
 * the previous tick's values, which keeps feedback loops stable.
 */
#define MANIFOLD_DSP_UNIT
#include "synth.h"

static const float SYNC_CPB[] = {1.f/8, 1.f/6, 1.f/4, 1.f/3, 1.f/2, 2.f/3, 3.f/4, 4.f/5,
                                 1.f, 5.f/4, 4.f/3, 3.f/2, 2.f, 3.f, 4.f};
#define N_SYNC ((int)(sizeof SYNC_CPB / sizeof SYNC_CPB[0]))

#include "ctl_on.h"
DSP_CTL void lfo_tick(Synth *s, float dt) {
    float K = pval(s, P_LFO_COUPLE), tempo = pval(s, P_TEMPO);
    float ph[NUM_LFO];
    for (int j = 0; j < NUM_LFO; j++) ph[j] = s->lfo[j].phase;
    for (int j = 0; j < NUM_LFO; j++) {
        Lfo *l = &s->lfo[j];
        int base = P_LFO1_RATE + 3 * j;
        float rate = pval(s, base);
        if (pint(s, base + 2)) {                      /* tempo sync incl. just ratios */
            float cpb = rate * 60.f / tempo; int bi = 0; float bd = 1e9f;
            for (int k = 0; k < N_SYNC; k++) { float d = fabsf(flog2(cpb / SYNC_CPB[k])); if (d < bd) { bd = d; bi = k; } }
            rate = SYNC_CPB[bi] * tempo / 60.f;
        }
        float dph = rate * dt;
        if (K > 0.f) {                                /* Kuramoto: drift toward the others */
            float c = 0; for (int k = 0; k < NUM_LFO; k++) if (k != j) c += fsin(ph[k] - ph[j]);
            dph += K * 0.05f * c * dt;
        }
        float np = l->phase + dph; int wrap = np >= 1.f;
        np -= floorf(np); l->phase = np;
        if (wrap) {
            l->prev = l->target;
            l->target = frand(&s->rng) * 2.f - 1.f;
            if (l->logistic <= 0.f || l->logistic >= 1.f) l->logistic = 0.37f;
            l->logistic = 3.91f * l->logistic * (1.f - l->logistic);
            l->step++;
        }
        float v;
        switch (pint(s, base + 1)) {
        case 0: v = fsin(np); break;
        case 1: v = np < .5f ? 4.f * np - 1.f : 3.f - 4.f * np; break;
        case 2: v = 2.f * np - 1.f; break;
        case 3: v = np < .5f ? 1.f : -1.f; break;
        case 4: v = l->target; break;
        case 5: { float f = np * np * (3.f - 2.f * np); v = l->prev + (l->target - l->prev) * f; } break;
        case 6: v = 2.f * l->logistic - 1.f; break;
        case 7: { int st = (int)(np * 8.f); v = ((st * 5) % 8) < 5 ? 1.f : -0.3f; } break;
        default: { float e = s->eng.env_follow * 4.f; v = (e > 1.f ? 1.f : e) * 2.f - 1.f; } break;
        }
        l->value = v;
    }
}

/* The performance knobs. Each rides a path through every parameter of its part of the instrument: a parameter's
   offset is 0 with the knob at its centre and follows four rolled waypoints (at -1, -1/2, +1/2, +1), straight lines
   between them, so a full turn passes through several different settings of all of them at once.
     Timbre   both voices (the oscillator banks)        Groove   the rhythm and the drums
     Space    the effects                               Motion   both generators, the path, the LFOs */
static const char *const KNOB_N[NUM_MACRO] = {"Timbre", "Groove", "Space", "Motion"};
const char *knob_name(int k) { return k >= 0 && k < NUM_MACRO ? KNOB_N[k] : "?"; }
DSP_CTL int knob_of_r(int id) {
    if (id < 0 || id >= NUM_PARAMS || !g_param_meta[id].moddable) return -1;
    if (id == P_MASTER || id == P_V1_LEVEL || id == P_V2_LEVEL || id == P_DR_LEVEL || id == P_TEMPER) return -1;
    if ((id >= P_D1_ROT && id <= P_D4_ROT) || id == P_RH_FILL) return 3;   /* the patterns' rotations, the fills: Motion */
    if (id >= P_D1_LEN && id <= P_D4_LEN) return -1;                       /* (a loop's length: by hand) */
    if (id >= P_V1_ODDEVEN && id <= P_V2_PLUCK) return 0;                  /* the voices' colour: Timbre */
    if (id >= P_D1_OE && id <= P_D4_HPLUCK) return 1;                      /* the hits' colour and pluck: Groove */
    if (id == P_V1_DEPTH || id == P_V2_DEPTH) return 0;                    /* the voices' depth: Timbre */
    if (id == P_V1_CLARITY || id == P_V2_CLARITY) return -1;               /* (Clarity: by hand; it switches partials) */
    switch (g_param_meta[id].section) {
    case SEC_VOICE1: case SEC_VOICE2: return 0;
    case SEC_METER: case SEC_DRUMS: return (id >= P_D1_DENS && (id - P_D1_DENS) % DR_STRIDE == P_D1_LEVEL - P_D1_DENS) ? -1 : 1;
    case SEC_FX: return 2;
    case SEC_GEN1: case SEC_GEN2: case SEC_LFO: return 3;
    default: return -1;
    }
}
int knob_of(int id) { return DSP_FAR(knob_of_r)(id); }      /* (the interface's and the roll's call, from the flash) */
DSP_CTL float knob_offset(const Synth *s, int id) {
    int k = knob_of_r(id); if (k < 0) return 0.f;
    float m = s->kmod[k]; if (m == 0.f) return 0.f;
    const int8_t *w = s->kpath[id];
    float a = m < 0.f ? -m : m, near = (float)(m < 0.f ? w[1] : w[2]), far = (float)(m < 0.f ? w[0] : w[3]);
    float v = a < .5f ? near * (a * 2.f) : near + (far - near) * (a * 2.f - 1.f);
    return v * (1.f / 127.f);
}

DSP_CTL void mod_apply(Synth *s) {
    float *src = s->src;
    for (int j = 0; j < NUM_LFO; j++) src[SRC_LFO1 + j] = s->lfo[j].value;
    for (int m = 0; m < NUM_MACRO; m++) src[SRC_MACRO1 + m] = s->macro[m];
    src[SRC_ENV] = s->eng.env_follow * 4.f > 1.f ? 1.f : s->eng.env_follow * 4.f;
    src[SRC_TRIG] = s->trig_env;
    src[SRC_RAND] = s->rand_hold;
    src[SRC_PATHX] = s->px * 2.f - 1.f; src[SRC_PATHY] = s->py * 2.f - 1.f;
    src[SRC_VEL] = s->last_vel;
    for (int l = 0; l < NUM_LAYERS; l++) {                    /* each motion: its loudest point's envelope */
        const GenSt *g = &s->layer[l].g; float e = 0.f;
        for (int j = 0; j < g->npts; j++) if (g->e[j] > e) e = g->e[j];
        src[SRC_MOT1 + l] = e > 1.f ? 1.f : e;
    }

    float km[NUM_MACRO];
    for (int m = 0; m < NUM_MACRO; m++) km[m] = s->macro[m];
    for (int i = 0; i < NUM_PARAMS; i++) s->mod[i] = s->base[i];
    for (int k = 0; k < MOD_SLOTS; k++) {
        const ModSlot *ms = &s->slot[k];
        if (!ms->src || ms->src >= NUM_SRC) continue;
        float v = ms->depth * src[ms->src];
        if (ms->dst >= MOD_DST_KNOB) { if (ms->dst < MOD_NUM_DST) km[ms->dst - MOD_DST_KNOB] += v; continue; }
        if (!g_param_meta[ms->dst].moddable) continue;
        s->mod[ms->dst] += v;
    }
    int any = 0;
    for (int m = 0; m < NUM_MACRO; m++) {
        float v = km[m]; v = v < -1.f ? -1.f : v > 1.f ? 1.f : v;
        if (!(v == v)) v = 0.f;                                  /* (never a NaN into the paths) */
        s->kmod[m] = v; if (v != 0.f) any = 1;
    }
    for (int i = 0; i < NUM_PARAMS; i++) {
        float v = g_param_meta[i].moddable ? s->mod[i] + (any ? knob_offset(s, i) : 0.f) : s->base[i];
        v = v < 0.f ? 0.f : v > 1.f ? 1.f : v;
        s->mod[i] = v;
        if (v != s->last_mod[i]) param_cache(s, i);   /* only what moved pays for its mapping */
    }
}
