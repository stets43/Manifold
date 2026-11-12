/*
 * Parameter table, value mapping, fast math, randomizer, mutate, macros.
 */
#define MANIFOLD_DSP_UNIT
#include "synth.h"
#include "mstring.h"

/* ---------------- fast math table ---------------- */
float g_sin_tab[1025];

/* ---------------- parameter table ---------------- */
#define M(id,nm,sec,cv,md,st,lo,hi,df,rl,rh,tr) [id] = {nm,sec,cv,md,st,lo,hi,df,rl,rh,tr}
ParamMeta g_param_meta[NUM_PARAMS] = {
 M(P_SCALE,"scale",SEC_SCALE,CURVE_DISC,0,1, 0,1, 0,0,1,0),
 M(P_ROOT,"root",SEC_SCALE,CURVE_LIN,0,1, 21,57, .4f,.2f,.6f,1),
 M(P_OCTAVES,"octaves",SEC_SCALE,CURVE_DISC,0,1, 0,6, .6f,.5f,1,0),
 M(P_SURFACE,"surface",SEC_SCALE,CURVE_DISC,0,1, 0,NUM_SURFACES, 0,0,1,0),
 M(P_ROTATE,"rotation",SEC_SCALE,CURVE_DISC,0,1, 0,12, 0,0,0,0),
 M(P_TEMPER,"temper",SEC_SCALE,CURVE_LIN,0,0, 0,1, 0,0,0,0),

#define LAYER_P(V,G,SV,SG,lvl,fn,ct,rt,ev,er) \
 M(V##_CENTER,"center",SV,CURVE_LIN,1,0, 0,1, .12f,0,.5f,0), \
 M(V##_BAND,"band",SV,CURVE_LIN,1,0, 0,1, .35f,.12f,.75f,1), \
 M(V##_TILT,"tilt",SV,CURVE_LIN,1,0, 0,1, .35f,.2f,.55f,1), \
 M(V##_STRIDE,"stride",SV,CURVE_LIN,1,0, 0,1, 0,0,.5f,0), \
 M(V##_WAVE,"wave",SV,CURVE_LIN,1,0, 0,1, .2f,0,.8f,0), \
 M(V##_PHASE,"phase",SV,CURVE_LIN,1,0, 0,1, .3f,0,.8f,0), \
 M(V##_SPIN,"detune",SV,CURVE_LIN,1,0, 0,1, .35f,.1f,.7f,0), \
 M(V##_TONE,"tone",SV,CURVE_LIN,1,0, 0,1, .6f,.35f,.8f,0), \
 M(V##_BLOOM,"bloom",SV,CURVE_LIN,1,0, 0,1, .6f,.3f,.9f,0), \
 M(V##_WIDTH,"width",SV,CURVE_LIN,1,0, 0,1, .5f,.25f,.9f,1), \
 M(V##_LEVEL,"level",SV,CURVE_LIN,1,0, 0,1, lvl,lvl - .1f,lvl + .1f,0), \
 M(V##_REG,"register",SV,CURVE_DISC,0,0, 0,3, .5f,.34f,.67f,0), \
 M(G##_FUNC,"function",SG,CURVE_DISC,0,0, 0,NUM_GENS, fn,0,1,0), \
 M(G##_A,"a",SG,CURVE_LIN,1,0, 0,1, .5f,0,1,0), \
 M(G##_B,"b",SG,CURVE_LIN,1,0, 0,1, .4f,0,1,1), \
 M(G##_RATE,"rate",SG,CURVE_LOG,1,0, .015f,4, rt,.3f,.8f,1), \
 M(G##_POINTS,"points",SG,CURVE_DISC,0,0, 0,MAX_PTS, .3f,.12f,.75f,0), \
 M(G##_SPREAD,"spread",SG,CURVE_LIN,1,0, 0,1, .5f,.1f,1,0), \
 M(G##_ENV,"envelope",SG,CURVE_DISC,0,0, 0,NUM_ENVS, ev,0,1,0), \
 M(G##_ERATE,"envelope rate",SG,CURVE_DISC,0,0, 0,NUM_ERATES, er,.2f,.9f,0), \
 M(G##_CONTOUR,"contour",SG,CURVE_LIN,1,0, 0,1, ct,0,1,1), \
 M(G##_REACH,"reach",SG,CURVE_LIN,1,0, 0,1, .2f,0,.5f,0), \
 M(G##_ENERGY,"energy",SG,CURVE_LIN,1,0, 0,1, .6f,.35f,.85f,1), \
 M(G##_ROUTE,"route",SG,CURVE_DISC,0,0, 0,NUM_ROUTES, 0,0,0,0)
 LAYER_P(P_V1,P_G1,SEC_VOICE1,SEC_GEN1,.7f,.05f,.75f,.75f,.08f,.72f),
 LAYER_P(P_V2,P_G2,SEC_VOICE2,SEC_GEN2,.5f,.6f,.2f,.5f,.25f,.28f),

 M(P_PATH_SCALE,"path scale",SEC_GEN1,CURVE_LIN,1,0, .15f,1, .8f,.5f,1,1),
 M(P_PATH_ROT,"path rotation",SEC_GEN1,CURVE_LIN,1,0, 0,1, 0,0,1,0),
 M(P_PATH_X,"path x",SEC_GEN1,CURVE_LIN,1,0, 0,1, .5f,.35f,.65f,1),
 M(P_PATH_Y,"path y",SEC_GEN1,CURVE_LIN,1,0, 0,1, .5f,.35f,.65f,1),

 M(P_TEMPO,"tempo",SEC_METER,CURVE_LIN,0,0, 10,180, .5294118f,.4f,.6823529f,1),   /* 10..180 BPM (default 100; rolls 78..126) */
 M(P_RH_ENGINE,"rhythm engine",SEC_METER,CURVE_DISC,0,0, 0,NUM_RH_ENGINES, 0,0,1,0),
 M(P_RH_DENS,"density",SEC_METER,CURVE_LIN,1,0, 0,1, .5f,.35f,.7f,1),
 M(P_RH_THREES,"threes",SEC_METER,CURVE_LIN,1,0, 0,1, 0,0,1,0),
 M(P_RH_FINE,"fine",SEC_METER,CURVE_LIN,1,0, 0,1, .55f,.3f,.85f,0),
 M(P_RH_SWING,"swing",SEC_METER,CURVE_LIN,1,0, 0,1, 0,0,1,0),
 M(P_RH_TURN,"turn",SEC_METER,CURVE_LIN,1,0, 0,1, 0,0,1,0),
 M(P_RH_FORM,"form",SEC_METER,CURVE_LIN,1,0, 0,1, .6f,.3f,1,0),
 M(P_RH_MOTION,"motion",SEC_METER,CURVE_LIN,1,0, 0,1, .2f,0,.6f,0),
 M(P_RH_ASSIGN,"assign",SEC_METER,CURVE_DISC,0,0, 0,NUM_RH_ASSIGN, 0,0,1,0),
 M(P_RH_SELRATE,"selector rate",SEC_METER,CURVE_DISC,0,0, 0,NUM_SELRATES, .3f,0,1,0),
 M(P_RH_FEED,"feedback",SEC_METER,CURVE_LIN,1,0, 0,1, .4f,.2f,.7f,0),
 M(P_RH_COUNT,"count",SEC_METER,CURVE_DISC,0,0, 0,7, .36f,.15f,.7f,0),
 M(P_RH_PULL,"pull",SEC_METER,CURVE_LIN,1,0, 0,1, .6f,.2f,1,0),
 M(P_RH_GRIP,"grip",SEC_METER,CURVE_LIN,1,0, 0,1, .5f,.2f,.9f,0),
 M(P_EVOLVE,"evolve",SEC_METER,CURVE_LIN,1,0, 0,1, .15f,0,.45f,0),
 M(P_DR_ON,"drums",SEC_DRUMS,CURVE_DISC,0,0, 0,2, .25f,0,1,0),
 M(P_DR_LAYER,"drum layer",SEC_DRUMS,CURVE_DISC,1,0, 0,NUM_DLAYERS, .84f,0,1,0),
 M(P_DR_LEVEL,"drum level",SEC_DRUMS,CURVE_LIN,1,0, 0,1, .7f,.55f,.85f,0),
 M(P_DR_ACCENT,"accent",SEC_DRUMS,CURVE_LIN,1,0, 0,1, .6f,.3f,.9f,0),
 M(P_DR_SHAPE,"shape by",SEC_DRUMS,CURVE_DISC,0,0, 0,NUM_DSHAPES, .17f,0,1,0),
 M(P_DR_DEPTH,"depth",SEC_DRUMS,CURVE_LIN,1,0, 0,1, .6f,.3f,1,0),
 M(P_DR_DUCK,"duck",SEC_DRUMS,CURVE_LIN,1,0, 0,1, 0.f,0,.6f,0),
/* a drum's block: its triggers (density, place, follow), the note it rides on the drum layer (voice: 0 the lowest
   sounding .. 1 the highest), its envelope (attack, hold, decay, curve), its level, its character on the voice
   (bright, settle, snap). Every one moddable and rolled */
#define DRUM_P(D,dn,pl,vo,at,ho,dc,cv,lv,br,se,sn) \
 M(D##_DENS,"density",SEC_DRUMS,CURVE_LIN,1,0, 0,1, dn,.2f,.8f,0), \
 M(D##_PLACE,"place",SEC_DRUMS,CURVE_LIN,1,0, 0,1, pl,pl - .08f < 0 ? 0 : pl - .08f,pl + .08f,0), \
 M(D##_SRC,"follow",SEC_DRUMS,CURVE_DISC,0,0, 0,NUM_DSRC, .1f,0,1,0), \
 M(D##_VOICE,"voice",SEC_DRUMS,CURVE_LIN,1,0, 0,1, vo,vo - .15f < 0 ? 0 : vo - .15f,vo + .15f > 1 ? 1 : vo + .15f,0), \
 M(D##_ATTACK,"attack",SEC_DRUMS,CURVE_LOG,1,0, .001f,.02f, at,0,.5f,0), \
 M(D##_HOLD,"hold",SEC_DRUMS,CURVE_LIN,1,0, 0,.05f, ho,0,.3f,0), \
 M(D##_DECAY,"decay",SEC_DRUMS,CURVE_LOG,1,0, .02f,3, dc,dc - .12f,dc + .1f,1), \
 M(D##_CURVE,"curve",SEC_DRUMS,CURVE_LIN,1,0, 0,1, cv,0,.6f,0), \
 M(D##_LEVEL,"level",SEC_DRUMS,CURVE_LIN,1,0, 0,1, lv,lv,lv,0), \
 M(D##_BRIGHT,"bright",SEC_DRUMS,CURVE_LIN,1,0, 0,1, br,.1f,.8f,0), \
 M(D##_SETTLE,"settle",SEC_DRUMS,CURVE_LIN,1,0, 0,1, se,.3f,.9f,0), \
 M(D##_SNAP,"snap",SEC_DRUMS,CURVE_LIN,1,0, 0,1, sn,0,.8f,0)
 DRUM_P(P_D1,0.55f,0,0,0.135f,0.1f,0.621f,0.2f,1,0.3f,0.5f,0.4f),
 DRUM_P(P_D2,0.5f,0.42f,0.6f,0,0.04f,0.46f,0.1f,0.55f,0.6f,0.7f,0.7f),
 DRUM_P(P_D3,0.6f,0.8f,1,0,0,0.25f,0.3f,0.4f,0.7f,0.85f,0.3f),
 DRUM_P(P_D4,0.45f,0.6f,0.3f,0.135f,0.08f,0.571f,0.2f,0.75f,0.4f,0.5f,0.4f),

#define LFO_P(n,R,S,Y) \
 M(R,"lfo" #n " rate",SEC_LFO,CURVE_LOG,1,0, .01f,12, .4f,.1f,.75f,1), \
 M(S,"lfo" #n " shape",SEC_LFO,CURVE_DISC,0,0, 0,9, 0,0,1,0), \
 M(Y,"lfo" #n " sync",SEC_LFO,CURVE_DISC,0,0, 0,2, 0,0,1,0)
 LFO_P(1,P_LFO1_RATE,P_LFO1_SHAPE,P_LFO1_SYNC),
 LFO_P(2,P_LFO2_RATE,P_LFO2_SHAPE,P_LFO2_SYNC),
 LFO_P(3,P_LFO3_RATE,P_LFO3_SHAPE,P_LFO3_SYNC),
 LFO_P(4,P_LFO4_RATE,P_LFO4_SHAPE,P_LFO4_SYNC),
 LFO_P(5,P_LFO5_RATE,P_LFO5_SHAPE,P_LFO5_SYNC),
 LFO_P(6,P_LFO6_RATE,P_LFO6_SHAPE,P_LFO6_SYNC),
 M(P_LFO_COUPLE,"lfo couple",SEC_LFO,CURVE_LIN,1,0, 0,2, 0,0,.6f,0),

 M(P_FZ_SUSTAIN,"fuzz sustain",SEC_FX,CURVE_LIN,1,0, 0,1, .3f,0,.7f,0),
 M(P_FZ_TONE,"fuzz tone",SEC_FX,CURVE_LIN,1,0, 0,1, .45f,.25f,.7f,0),
 M(P_FZ_MIX,"fuzz mix",SEC_FX,CURVE_LIN,1,0, 0,1, 0,0,.7f,0),
 M(P_FLT_CUT,"cutoff",SEC_FX,CURVE_LOG,1,0, 60,18000, .9f,.55f,1,1),
 M(P_FLT_RES,"resonance",SEC_FX,CURVE_LIN,1,0, 0,.95f, .1f,0,.55f,0),
 M(P_FLT_MORPH,"morph",SEC_FX,CURVE_LIN,1,0, 0,1, 0,0,.4f,0),
 M(P_DL_SEND,"delay send",SEC_FX,CURVE_LIN,1,0, 0,1, .25f,0,.55f,0),
 M(P_DL_TIME,"delay time",SEC_FX,CURVE_LIN,1,0, 0,1, .5f,0,1,0),
 M(P_DL_FB,"delay fb",SEC_FX,CURVE_LIN,1,0, 0,1.1f, .4f,.15f,.7f,1),
 M(P_DL_TONE,"delay tone",SEC_FX,CURVE_LIN,1,0, 0,1, .5f,.2f,.8f,1),
 M(P_DL_TAP,"delay tap",SEC_FX,CURVE_DISC,0,0, 0,5, 0,0,1,0),
 M(P_RV_SEND,"reverb mix",SEC_FX,CURVE_LIN,1,0, 0,1, .35f,.15f,.8f,1),
 M(P_RV_SIZE,"reverb size",SEC_FX,CURVE_LIN,1,0, .3f,1, .7f,.3f,1,1),
 M(P_RV_DECAY,"reverb decay",SEC_FX,CURVE_LOG,1,0, .3f,60, .5f,.25f,.85f,1),
 M(P_RV_DAMP,"reverb damp",SEC_FX,CURVE_LIN,1,0, 0,1, .4f,.15f,.75f,1),
 M(P_RV_PRE,"predelay",SEC_FX,CURVE_LIN,1,0, 0,.1f, .2f,0,.6f,0),
 M(P_RV_MOD,"reverb mod",SEC_FX,CURVE_LIN,1,0, 0,1, .4f,.1f,.8f,1),
 M(P_RV_TUNED,"reverb tuned",SEC_FX,CURVE_DISC,0,0, 0,2, 0,0,.6f,0),
 M(P_RV_FREEZE,"freeze",SEC_FX,CURVE_DISC,0,0, 0,2, 0,0,0,0),
 M(P_DL_TO_RV,"delay->reverb",SEC_FX,CURVE_LIN,1,0, 0,1, .2f,0,.6f,0),
 M(P_CMP_THRESH,"comp thresh",SEC_FX,CURVE_LIN,1,0, -36,0, .6f,.4f,.8f,1),
 M(P_CMP_RATIO,"comp ratio",SEC_FX,CURVE_LOG,1,0, 1,20, .35f,.15f,.6f,1),
 M(P_CMP_ATT,"comp attack",SEC_FX,CURVE_LOG,1,0, .5f,80, .4f,.2f,.7f,1),
 M(P_CMP_REL,"comp release",SEC_FX,CURVE_LOG,1,0, 20,800, .5f,.3f,.7f,1),
 M(P_CMP_MIX,"comp mix",SEC_FX,CURVE_LIN,1,0, 0,1, 1,.75f,1,0),
 M(P_CMP_PUMP,"pump",SEC_FX,CURVE_LIN,1,0, 0,1, 0,0,.45f,0),
 M(P_MASTER,"master",SEC_FX,CURVE_LIN,1,0, 0,1.5f, .66f,.66f,.66f,0),
};

void params_init_meta(void) {
    for (int i = 0; i <= 1024; i++) g_sin_tab[i] = sinf(TWO_PI * (float)i / 1024.f);
    g_param_meta[P_SCALE].hi = (float)scale_count();
}

/* ---------------- value mapping (cached: pval / pint read val / ival) ---------------- */
DSP_CTL void param_cache(Synth *s, int id) {
    const ParamMeta *m = &g_param_meta[id];
    float v = s->mod[id]; if (v < 0) v = 0; if (v > 1) v = 1;
    s->last_mod[id] = s->mod[id];
    if (m->curve == CURVE_DISC) {
        int n = (int)m->hi, i = (int)(v * (float)n); if (i >= n) i = n - 1; if (i < 0) i = 0;
        s->ival[id] = i; s->val[id] = (float)i;
    } else {
        s->val[id] = m->curve == CURVE_LOG ? m->lo * fexp2(v * flog2(m->hi / m->lo)) : m->lo + v * (m->hi - m->lo);
        s->ival[id] = (int32_t)s->val[id];
    }
}
void params_cache_all(Synth *s) { for (int i = 0; i < NUM_PARAMS; i++) DSP_FAR(param_cache)(s, i); }

/* ---------------- randomizer ---------------- */
static float roll_one(const ParamMeta *m, uint32_t *r) {
    float u = m->tri ? 0.5f * (frand(r) + frand(r)) : frand(r);
    if (m->curve == CURVE_DISC) {
        int n = (int)m->hi; int lo = (int)(m->rlo * n), hi = (int)(m->rhi * n - 1e-4f);
        if (hi < lo) hi = lo;
        int i = lo + (int)(frand(r) * (float)(hi - lo + 1)); if (i > hi) i = hi;
        return ((float)i + 0.5f) / (float)n;
    }
    return m->rlo + u * (m->rhi - m->rlo);
}
static float disc(int id, int i) { return ((float)i + 0.5f) / g_param_meta[id].hi; }

static int rand_moddable(uint32_t *r) {
    for (;;) {
        int id = (int)(frand(r) * NUM_PARAMS) % NUM_PARAMS;
        if (g_param_meta[id].moddable && id != P_MASTER && id != P_V1_LEVEL && id != P_V2_LEVEL) return id;
    }
}

void params_roll_mod(Synth *s) {
    uint32_t *r = &s->rng;
    if (!s->locked[SEC_LFO]) {
        memset(s->slot, 0, sizeof s->slot);
        int n = 5 + (int)(frand(r) * 8.f);
        for (int i = 0; i < n && i < MOD_SLOTS; i++) {
            float u = frand(r); int src;
            if (u < .70f) src = SRC_LFO1 + (int)(frand(r) * 6) % 6;
            else if (u < .78f) src = SRC_RAND;
            else if (u < .86f) src = SRC_TRIG;
            else if (u < .92f) src = SRC_ENV;
            else if (u < .98f) src = frand(r) < .5f ? SRC_PATHX : SRC_PATHY;
            else src = SRC_VEL;
            s->slot[i].src = (uint8_t)src;
            s->slot[i].dst = (uint8_t)rand_moddable(r);
            float d = 0.05f + 0.3f * 0.5f * (frand(r) + frand(r));
            s->slot[i].depth = frand(r) < .5f ? -d : d;
        }
        /* the generators on the drums: with the drums on, often one or two routes from a layer's motion to a drum's
           envelope or character (Motion 2 -> the hats' decay: they open as layer 2 swells) */
        if (s->target[P_DR_ON] > .5f && frand(r) < .6f) {
            static const uint8_t DD[] = {P_D1_DECAY, P_D1_BRIGHT, P_D1_SNAP, P_D1_CURVE, P_D1_ATTACK, P_D1_HOLD, P_D1_LEVEL, P_D1_SETTLE};
            int k = n < MOD_SLOTS ? n : MOD_SLOTS - 2, m = 1 + (frand(r) < .4f);
            for (int j = 0; j < m && k + j < MOD_SLOTS; j++) {
                ModSlot *ms = &s->slot[k + j];
                ms->src = (uint8_t)(frand(r) < .5f ? SRC_MOT1 : SRC_MOT2);
                ms->dst = (uint8_t)DP((int)(frand(r) * 4.f) & 3, DD[(int)(frand(r) * 8.f) & 7]);
                float d = .1f + .25f * frand(r); ms->depth = frand(r) < .7f ? d : -d;
            }
        }
    }
}

/* the knobs' paths: most parameters of each knob's part ride it (four in five), each with its own way to go: how far
   at the ends (either direction, in proportion to the parameter's own musical range) and somewhere else on the way
   (the half-way point may overshoot the end or lean the other way), so the turn is not one straight morph */
void params_roll_knobs(Synth *s) {
    uint32_t *r = &s->rng;
    for (int i = 0; i < NUM_PARAMS; i++) {
        int8_t *w = s->kpath[i];
        w[0] = w[1] = w[2] = w[3] = 0;
        /* the ones a player reaches for are always on their knob: the filter, the reverb and delay, the bank's centre,
           band and tilt, the rhythm's density and swing, the motions' rates */
        int must = i == P_FLT_CUT || i == P_FLT_RES || i == P_RV_SEND || i == P_RV_DECAY || i == P_RV_SIZE || i == P_DL_SEND || i == P_DL_FB
                || i == P_V1_TONE || i == P_V2_TONE || i == P_V1_CENTER || i == P_V1_BAND || i == P_V1_TILT || i == P_V2_CENTER || i == P_RH_DENS || i == P_RH_SWING || i == P_G1_RATE || i == P_G1_REACH;
        if (knob_of(i) < 0 || (frand(r) > .8f && !must)) continue;
        const ParamMeta *m = &g_param_meta[i];
        float span = m->rhi - m->rlo + .15f; if (span < .45f) span = .45f; if (span > 1.f) span = 1.f;
        float dir = frand(r) < .5f ? -1.f : 1.f;              /* the two ends go opposite ways, mostly: a turn is a journey */
        for (int side = 0; side < 2; side++) {
            float far = (.45f + .55f * frand(r)) * span * (frand(r) < .8f ? (side ? dir : -dir) : (side ? -dir : dir));
            if (i == P_FLT_CUT) far = side ? -(.3f + .3f * frand(r)) : .1f;   /* (the filter closes one way only, never fully) */
            float near = far * (-.3f + 1.5f * frand(r));
            if (i == P_DL_FB) { if (far > .15f) far = .15f; if (near > .15f) near = .15f; }   /* (never into runaway) */
            w[side ? 3 : 0] = (int8_t)(fclamp(far, -1.f, 1.f) * 127.f);
            w[side ? 2 : 1] = (int8_t)(fclamp(near, -1.f, 1.f) * 127.f);
        }
    }
    for (int m = 0; m < NUM_MACRO; m++) s->macro[m] = 0.f;
}

/* ---------------- curated styles ---------------- */
static float to_norm(int id, float real) {
    const ParamMeta *m = &g_param_meta[id];
    if (m->curve == CURVE_LOG) return flog2(real / m->lo) / flog2(m->hi / m->lo);
    return (real - m->lo) / (m->hi - m->lo);
}
static void TR(Synth *s, int id, float lo, float hi) {        /* roll a real-unit range */
    float u = frand(&s->rng), v;
    if (g_param_meta[id].curve == CURVE_LOG) v = lo * fexp2(u * flog2(hi / lo)); else v = lo + u * (hi - lo);
    float nv = to_norm(id, v); s->target[id] = nv < 0 ? 0 : nv > 1 ? 1 : nv;
}
static void TD(Synth *s, int id, int idx) { s->target[id] = ((float)idx + .5f) / g_param_meta[id].hi; }
static int pickw(uint32_t *r, const float *w, int n) {        /* weighted choice */
    float t = 0; for (int i = 0; i < n; i++) t += w[i];
    float u = frand(r) * t; for (int i = 0; i < n; i++) { u -= w[i]; if (u <= 0) return w[i] > 0 ? i : 0; }
    return n - 1;
}
static void TP(Synth *s, int id, const float *w) { TD(s, id, pickw(&s->rng, w, (int)g_param_meta[id].hi)); }

static const uint8_t FRIENDLY[] = {P_V1_TILT, P_V1_CENTER, P_V1_BAND, P_V2_TILT, P_V2_CENTER, P_V1_WIDTH, P_FLT_CUT,
    P_RV_SEND, P_RV_MOD, P_DL_SEND, P_G1_REACH, P_G1_ENERGY, P_G2_B, P_PATH_SCALE, P_V1_WAVE, P_V2_BAND};

/* what every roll does, whatever the style: choices that only make sense together are made together */
static void roll_together(Synth *s) {
    uint32_t *r = &s->rng; float *t = s->target;
    for (int l = 0; l < NUM_LAYERS; l++) if (!s->locked[l ? SEC_VOICE2 : SEC_VOICE1]) {
        /* Stride and Cross are strong colours: off in half the rolls. A high, narrow band has no body: widen it */
        if (frand(r) < .55f) t[LP(l, P_V1_STRIDE)] = 0.f;
        else if (frand(r) < .5f) t[LP(l, P_V1_STRIDE)] = .333f;         /* the odd partials */
        float c = t[LP(l, P_V1_CENTER)], need = .25f + .8f * c;
        if (t[LP(l, P_V1_BAND)] < need) t[LP(l, P_V1_BAND)] = need > 1.f ? 1.f : need;
        if (c > .3f && t[LP(l, P_V1_REG)] > .67f) t[LP(l, P_V1_REG)] = .5f;   /* (bright and an octave up: thin) */
    }
    if (!s->locked[SEC_METER]) {
        float u = frand(r);
        t[P_RH_THREES] = u < .55f ? 0.f : u < .7f ? 1.f : .35f + .3f * frand(r);
        t[P_RH_SWING] = (t[P_RH_THREES] < .2f && frand(r) < .4f) ? .4f + .6f * frand(r) : 0.f;
        t[P_RH_ENGINE] = frand(r) < .18f ? .75f : .25f;
        t[P_RH_ASSIGN] = frand(r) < .15f ? .75f : .25f;
        { static const float w[NUM_SELRATES] = {15, 60, 12, 10, 3};
          float tot = 0, x; for (int i = 0; i < NUM_SELRATES; i++) tot += w[i];
          x = frand(r) * tot; int k = 0; while (k < NUM_SELRATES - 1 && (x -= w[k]) > 0) k++;
          t[P_RH_SELRATE] = ((float)k + .5f) / NUM_SELRATES; }
    }
    if (!s->locked[SEC_DRUMS]) for (int d = 0; d < NUM_DRUMS; d++) {
        if (frand(r) < (d >= 2 ? .12f : .04f)) t[DP(d, P_D1_SRC)] = frand(r) < .6f ? .83f : .5f; else t[DP(d, P_D1_SRC)] = .1f;
    }
    if (!s->locked[SEC_FX]) {
        if (frand(r) < .8f) t[P_FZ_MIX] = 0.f; else t[P_FZ_MIX] = .1f + .4f * frand(r);
    }
}

static void apply_style(Synth *s) {
    uint32_t *r = &s->rng; int st0 = s->style;
    if (st0 == STYLE_WILD || st0 >= NUM_STYLES) return;
    /* Drum circle rolls the synth as Tongue drum does, Machine and Breaks as Groove: they differ in the rhythm */
    int CI = st0 == STYLE_CIRCLE, MA = st0 == STYLE_MACHINE, BR = st0 == STYLE_BREAKS;
    int st = CI ? STYLE_TONGUE : (MA || BR) ? STYLE_GROOVE : st0;
    int T = st == STYLE_TONGUE, G = st == STYLE_GROOVE, A = st == STYLE_AMBIENT, C = st == STYLE_COSMIC;
    if (!s->locked[SEC_SCALE]) {
        /* the groups: harmonic (0..), pentatonic, modes, other. Tongue: the series and the pentatonics; groove: the
           pentatonics and modes; ambient: the series, pentatonics, the gentler modes; cosmic: anything */
        const float wg[4][5] = {{3, 6, 3, 1, 0}, {2, 3, 3, 4, 1.5f}, {5, 4, 3, 2, .3f}, {4, 2, 2, 3, 2}};
        int g = pickw(r, wg[st - 1], 5), lo = scale_group_start(g), hi = scale_group_start(g + 1);
        int pick = lo + (int)(frand(r) * (float)(hi - lo)); if (pick >= hi) pick = hi - 1;
        if (g == 0 && frand(r) < .45f) pick = 0;                 /* the harmonic series itself, often */
        if (g == 4 && pick == hi - 1 && frand(r) < .8f) pick = lo;   /* (chromatic: rarely) */
        s->target[P_SCALE] = ((float)pick + .5f) / g_param_meta[P_SCALE].hi;
        TD(s, P_ROTATE, 0);
        TD(s, P_OCTAVES, 3);                                  /* four octaves, or as many as 32 notes allow */
        { const float w[3] = {5, 2, 3}; TP(s, P_SURFACE, w); }
        TR(s, P_ROOT, 28, T ? 38 : 36);                       /* E1 to D2: the ding is a bass note */
    }
    if (!s->locked[SEC_VOICE1]) {
        /* tongue: a low, clear band (tines, bells: often the odd partials, a little Cross); groove: wider, more Wave
           and Cross, loud notes open up; ambient: narrow, soft, turning slowly; cosmic: anywhere, strange combs */
        if (T) { TR(s, P_V1_CENTER, 0.f, .45f); TR(s, P_V1_BAND, .15f, .5f); TR(s, P_V1_TILT, .3f, .58f); TR(s, P_V1_WAVE, 0.f, .45f);
                 TR(s, P_V1_PHASE, .2f, .7f); TR(s, P_V1_SPIN, .1f, .4f); TR(s, P_V1_TONE, .5f, .75f); TR(s, P_V1_BLOOM, .5f, .9f); }
        if (G) { TR(s, P_V1_CENTER, .05f, .65f); TR(s, P_V1_BAND, .3f, .85f); TR(s, P_V1_TILT, .35f, .68f); TR(s, P_V1_WAVE, .2f, .95f);
                 TR(s, P_V1_PHASE, 0.f, .5f); TR(s, P_V1_SPIN, .15f, .5f); TR(s, P_V1_TONE, .55f, .82f); TR(s, P_V1_BLOOM, .6f, 1.f); }
        if (A) { TR(s, P_V1_CENTER, 0.f, .4f); TR(s, P_V1_BAND, .2f, .7f); TR(s, P_V1_TILT, .15f, .42f); TR(s, P_V1_WAVE, 0.f, .2f);   /* pads: soft, wide, low */
                 TR(s, P_V1_PHASE, .3f, .8f); TR(s, P_V1_SPIN, .35f, .75f); TR(s, P_V1_TONE, .42f, .66f); TR(s, P_V1_BLOOM, .2f, .5f); }
        if (C) { TR(s, P_V1_CENTER, .1f, .85f); TR(s, P_V1_BAND, .1f, .65f); TR(s, P_V1_TILT, .25f, .68f); TR(s, P_V1_WAVE, 0.f, .8f);
                 TR(s, P_V1_PHASE, .3f, 1.f); TR(s, P_V1_SPIN, .2f, .7f); TR(s, P_V1_TONE, .42f, .82f); TR(s, P_V1_BLOOM, .4f, .9f); }
        TR(s, P_V1_STRIDE, 0.f, C ? .7f : .4f);
        TR(s, P_V1_WIDTH, .35f, .85f);
        TD(s, P_V1_REG, 1);
    }
    if (!s->locked[SEC_VOICE2]) {
        /* the second voice: lower in the series and narrower than the first (a bed under it), wide in the stereo */
        TR(s, P_V2_CENTER, 0.f, C ? .55f : G ? .4f : .3f); TR(s, P_V2_BAND, .15f, G ? .65f : .5f); TR(s, P_V2_TILT, .2f, .55f);
        TR(s, P_V2_STRIDE, 0.f, .4f); TR(s, P_V2_WAVE, 0.f, G ? .6f : .4f); TR(s, P_V2_PHASE, .3f, .8f);
        TR(s, P_V2_SPIN, .25f, .7f); TR(s, P_V2_TONE, .42f, G ? .7f : .62f); TR(s, P_V2_BLOOM, .3f, .7f);
        TR(s, P_V2_WIDTH, .6f, 1.f);
        TR(s, P_V2_LEVEL, T ? .3f : G ? .4f : .5f, T ? .45f : G ? .6f : .7f);
        TD(s, P_V2_REG, G && frand(r) < .5f ? 0 : 1);
    }
    /* the motions:            orbit ladder swarm lorenz motif drift billiard gravity stride loops erode chords */
    if (!s->locked[SEC_GEN1]) {
        /* the rhythm layer: loops and runs with a pulse (tongue, groove), the stream itself, or passing through the
           notes (cosmic), or swelling as it drifts (ambient) */
        const float wf[4][NUM_GENS] = {{4, 4, 1, 1, 3, 0, 4, 3, 3, 2, 1, 2}, {3, 4, 1, 1, 4, 0, 4, 2, 5, 0, 1, 3},
                                       {1, 1, 3, 1, 1, 4, 0, 4, 0, 6, 4, 5}, {2, 2, 2, 3, 2, 1, 3, 3, 2, 3, 3, 3}};
        TP(s, P_G1_FUNC, wf[st - 1]);
        const float we[4][NUM_ENVS] = {{6, 0, 1, 3, 0, 0, 3, 0}, {6, 0, 1, 2, 1, 0, 6, 0}, {1, 4, 3, 2, 2, 1, 0, 0}, {3, 1, 2, 4, 3, 0, 3, 0}};
        TP(s, P_G1_ENV, we[st - 1]);
        TR(s, P_G1_A, 0.f, 1.f); TR(s, P_G1_B, 0.f, 1.f);
        TD(s, P_G1_POINTS, T ? 1 + (int)(frand(r) * 3.f) : G ? 2 + (int)(frand(r) * 3.f) : A ? 2 + (int)(frand(r) * 4.f) : 2 + (int)(frand(r) * 5.f));
        TR(s, P_G1_SPREAD, .2f, 1.f);
        if (T) { TR(s, P_G1_RATE, .25f, 1.f); TD(s, P_G1_ERATE, frand(r) < .5f ? 10 : 8); TR(s, P_G1_CONTOUR, .7f, .92f); TR(s, P_G1_ENERGY, .5f, .75f); }
        if (G) { TR(s, P_G1_RATE, .33f, 1.5f); TD(s, P_G1_ERATE, frand(r) < .5f ? 10 : frand(r) < .5f ? 12 : 11); TR(s, P_G1_CONTOUR, .8f, 1.f); TR(s, P_G1_ENERGY, .6f, .85f); }
        if (A) { TR(s, P_G1_RATE, 0.f, .22f); TD(s, P_G1_ERATE, (int)(frand(r) * 5.f)); TR(s, P_G1_CONTOUR, 0.f, .3f); TR(s, P_G1_ENERGY, .4f, .7f); }
        if (C) { TR(s, P_G1_RATE, .16f, 1.f); TD(s, P_G1_ERATE, 3 + (int)(frand(r) * 8.f)); TR(s, P_G1_CONTOUR, .2f, .9f); TR(s, P_G1_ENERGY, .5f, .8f); }
        if (pint(s, P_G1_ENV) == ENV_STREAM || (int)(s->target[P_G1_ENV] * NUM_ENVS) == ENV_STREAM) TR(s, P_G1_CONTOUR, .1f, .6f);
        {   int fn = (int)(s->target[P_G1_FUNC] * NUM_GENS);  /* Loops and Erode shape their own notes; Chords are held or swell */
            if (fn == GEN_LOOPS || fn == GEN_ERODE) { TD(s, P_G1_ENV, ENV_HOLD); if (fn == GEN_LOOPS) TR(s, P_G1_RATE, 0.f, .3f); TD(s, P_G1_POINTS, 3 + (int)(frand(r) * 4.f)); }
            if (fn == GEN_CHORDS) { TD(s, P_G1_ENV, frand(r) < .5f ? ENV_HOLD : G || T ? ENV_PULSE : ENV_SWELL); TR(s, P_G1_RATE, .1f, .4f); TD(s, P_G1_POINTS, 2 + (int)(frand(r) * 2.f)); } }
        TR(s, P_G1_REACH, .05f, A ? .45f : .25f);
        TR(s, P_PATH_SCALE, .7f, 1.f); TR(s, P_PATH_ROT, 0.f, 1.f);
    }
    if (!s->locked[SEC_GEN2]) {
        /* the second layer: slow points that swell (a bed), or for groove a low counter-motion */
        const float wf[4][NUM_GENS] = {{2, 1, 3, 1, 1, 4, 1, 4, 0, 3, 1, 3}, {2, 2, 1, 1, 2, 1, 3, 3, 3, 0, 1, 4},
                                       {1, 1, 4, 2, 1, 4, 0, 4, 0, 6, 3, 6}, {2, 1, 3, 3, 1, 2, 1, 3, 1, 3, 2, 3}};
        TP(s, P_G2_FUNC, wf[st - 1]);
        const float we[4][NUM_ENVS] = {{0, 5, 2, 1, 1, 2, 1, 0}, {4, 1, 1, 2, 1, 0, 3, 0}, {0, 5, 2, 1, 2, 2, 0, 0}, {1, 3, 2, 2, 3, 1, 2, 0}};
        TP(s, P_G2_ENV, we[st - 1]);
        TR(s, P_G2_A, 0.f, 1.f); TR(s, P_G2_B, 0.f, 1.f);
        TD(s, P_G2_POINTS, 1 + (int)(frand(r) * 3.f));
        TR(s, P_G2_SPREAD, .3f, 1.f);
        TR(s, P_G2_RATE, G ? .16f : .03f, G ? .67f : .25f);
        TD(s, P_G2_ERATE, G ? 8 : A ? (int)(frand(r) * 4.f) : 2 + (int)(frand(r) * 5.f));
        TR(s, P_G2_CONTOUR, G ? .6f : 0.f, G ? .9f : .3f);
        TR(s, P_G2_REACH, .1f, .45f); TR(s, P_G2_ENERGY, .4f, .7f);
        {   int fn = (int)(s->target[P_G2_FUNC] * NUM_GENS);
            if (fn == GEN_LOOPS || fn == GEN_ERODE) { TD(s, P_G2_ENV, ENV_HOLD); if (fn == GEN_LOOPS) TR(s, P_G2_RATE, 0.f, .3f); TD(s, P_G2_POINTS, 3 + (int)(frand(r) * 3.f)); }
            if (fn == GEN_CHORDS) { TD(s, P_G2_ENV, frand(r) < .6f ? ENV_HOLD : ENV_SWELL); TR(s, P_G2_RATE, .08f, .35f); TD(s, P_G2_POINTS, 2 + (int)(frand(r) * 2.f)); } }
        /* sometimes layer 2 modulates layer 1 instead of playing: the stream's envelope gating or accenting a
           chaotic one, or one path carried on another */
        TD(s, P_G1_ROUTE, ROUTE_PLAY);
        const float pm[4] = {.12f, .35f, .1f, .3f};
        if (frand(r) < (MA ? .7f : CI ? .2f : pm[st - 1])) {
            const float wr[NUM_ROUTES] = {0, 1, 1.5f, 2.5f};
            int ro = pickw(&s->rng, wr, NUM_ROUTES); TD(s, P_G2_ROUTE, ro);
            if (ro != ROUTE_MOVE && frand(r) < .65f) {
                TD(s, P_G2_ENV, ENV_STREAM); TR(s, P_G2_CONTOUR, .2f, .6f);
                if (!s->locked[SEC_GEN1]) { const float wc[NUM_GENS] = {0, 0, 1, 4, 0, 2, 2, 2, 0, 0, 0, 0}; TP(s, P_G1_FUNC, wc); TD(s, P_G1_ENV, ENV_HOLD); }
            }
            if (ro == ROUTE_MOVE) TR(s, P_G2_ENERGY, .2f, .6f); else TR(s, P_G2_ENERGY, .6f, 1.f);
        } else TD(s, P_G2_ROUTE, ROUTE_PLAY);
    }
    if (!s->locked[SEC_METER]) {
        /* the rhythm: tempo, how many of the stream's peaks pass, the twos or the threes, how fine, the swing, how
           much the long form moves it, and what picks the drum */
        float p3 = T ? .3f : G ? .2f : A ? .3f : C ? .55f : 0.f, psw = G ? .4f : T ? .2f : C ? .25f : .1f, plock = .08f;
        if (T) { TR(s, P_TEMPO, 68, 96); TR(s, P_RH_DENS, .35f, .55f); TR(s, P_RH_FINE, .3f, .6f); TR(s, P_RH_FORM, .4f, .8f); }
        if (G) { TR(s, P_TEMPO, 88, 122); TR(s, P_RH_DENS, .5f, .7f); TR(s, P_RH_FINE, .5f, .85f); TR(s, P_RH_FORM, .5f, 1.f); }
        if (A) { TR(s, P_TEMPO, 60, 78); TR(s, P_RH_DENS, .25f, .45f); TR(s, P_RH_FINE, .2f, .5f); TR(s, P_RH_FORM, .4f, .9f); }
        if (C) { TR(s, P_TEMPO, 84, 118); TR(s, P_RH_DENS, .4f, .65f); TR(s, P_RH_FINE, .4f, .8f); TR(s, P_RH_FORM, .6f, 1.f); }
        if (CI) { p3 = .6f; psw = .15f; TR(s, P_RH_DENS, .5f, .7f); TR(s, P_RH_FINE, .5f, .8f); }
        if (MA) { p3 = .5f; psw = .2f; plock = .45f; TR(s, P_TEMPO, 84, 140); TR(s, P_RH_FINE, .6f, 1.f); TR(s, P_RH_FORM, .7f, 1.f); }   /* IDM */
        if (BR) { p3 = .1f; psw = .15f; plock = 0.f; TR(s, P_TEMPO, 164, 176); TR(s, P_RH_DENS, .55f, .75f); TR(s, P_RH_FINE, .55f, .8f); }   /* drum'n'bass */
        float u = frand(r);
        if (u < p3) { if (frand(r) < (CI ? .7f : .35f)) TR(s, P_RH_THREES, 1.f, 1.f); else TR(s, P_RH_THREES, .38f, .62f); }
        else TR(s, P_RH_THREES, 0.f, 0.f);
        if (u >= p3 && frand(r) < psw) TR(s, P_RH_SWING, .45f, BR ? .7f : .9f); else TR(s, P_RH_SWING, 0.f, 0.f);
        TR(s, P_RH_TURN, 0.f, (C || MA) ? 1.f : .15f);
        TR(s, P_RH_MOTION, MA ? .3f : 0.f, MA ? .8f : C ? .5f : .3f);
        TD(s, P_RH_ENGINE, frand(r) < plock ? RH_LOCK : RH_THRESHOLD);
        TR(s, P_RH_PULL, MA ? .15f : .35f, MA ? .7f : 1.f);
        TD(s, P_RH_ASSIGN, frand(r) < (MA ? .25f : .1f) ? RH_COUNT : RH_SELECT);
        { const float w[4][NUM_SELRATES] = {{2, 8, 1, 1, 0}, {1, 8, 1, 2, 0}, {4, 6, 0, 0, 0}, {2, 5, 3, 2, 1}}, wm[NUM_SELRATES] = {1, 3, 3, 2, 2}, wb[NUM_SELRATES] = {1, 8, 0, 2, 0};
          TP(s, P_RH_SELRATE, MA ? wm : BR ? wb : w[st - 1]); }
        TR(s, P_RH_FEED, (MA || BR) ? .5f : .25f, (MA || BR) ? .9f : .6f);
        TD(s, P_RH_COUNT, 1 + (int)(frand(r) * 4.f));
        TR(s, P_RH_GRIP, A ? 0.f : C ? .15f : .35f, A ? .15f : C ? .7f : .9f);   /* (ambient: the notes do not pulse with the stream) */
        TR(s, P_EVOLVE, .05f, G ? .2f : .35f);
        if (MA) TR(s, P_EVOLVE, .25f, .55f);
    }
    if (!s->locked[SEC_DRUMS]) {
        /* the drums: how likely they play, the layer they ride, a kit (a character for each drum's envelope and
           sound), how readily each is chosen, the strength of event it wants, whether it follows a layer */
        float pon = (CI || MA || BR) ? 1.f : G ? .85f : T ? .45f : C ? .4f : .12f;
        int on = frand(r) < pon;
        TD(s, P_DR_ON, on);
        {   /* the drum layer: Split mostly (the kick on the bass notes of any roll); Ambient and Cosmic sometimes one */
            const float wl[NUM_DLAYERS] = {(A || C) ? 1.5f : .6f, (A || C) ? 1.5f : .6f, 4.f};
            TP(s, P_DR_LAYER, wl); }
        TR(s, P_DR_LEVEL, .6f, .8f); TR(s, P_DR_ACCENT, .4f, .9f);
        /* Shape by a layer's envelope (the synth's generator as the drums' volume): often, most in Machine and Cosmic */
        float psh = MA ? .6f : C ? .5f : A ? .5f : .3f;
        if (frand(r) < psh) { TD(s, P_DR_SHAPE, frand(r) < .5f ? DSHAPE_L1 : DSHAPE_L2); TR(s, P_DR_DEPTH, .4f, .9f); }
        else TD(s, P_DR_SHAPE, DSHAPE_NONE);
        /* the kick's duck: Machine, Breaks and Groove, sometimes Drum circle and Cosmic; never Ambient */
        float pdk = (MA || G || BR) ? .7f : (CI || C) ? .3f : 0.f;
        if (frand(r) < pdk) TR(s, P_DR_DUCK, .25f, .7f); else TR(s, P_DR_DUCK, 0.f, 0.f);
        /* the kits, as characters: decay's scale, curve, bright, settle, snap (each a lo..hi; the last three unused) */
        enum { K_SKIN, K_BOOM, K_808, K_WOOD, K_METAL, K_GLITCH, NUM_K };
        static const float KT[NUM_K][12] = {
            /* dec  curve      bright     settle     snap       (unused) */
            {1.f,  0, .3f,   .3f, .6f,  .4f, .7f,  .3f, .6f,  .2f, .5f, 1.f},     /* skin: tabla, frame drum, taiko */
            {1.6f, 0, .2f,   .1f, .35f, .2f, .5f,  .2f, .5f,  .4f, .8f, 1.4f},    /* boom: big, round, a long drop */
            {2.f,  0, .15f,  0, .2f,    .3f, .6f,  0, .2f,    .5f, .9f, 1.8f},    /* 808: almost the note alone, the long sweep */
            {.45f, .3f, .7f, .5f, .8f,  .7f, .95f, .4f, .8f,  0, .2f, .5f},       /* wood: short and dry, clicky */
            {1.5f, 0, .2f,   .5f, .9f,  0, .25f,   .1f, .4f,  0, .1f, 1.f},       /* metal: bright and ringing */
            {.35f, .5f, 1.f, .6f, 1.f,  .8f, 1.f,  .6f, 1.f,  .2f, .7f, .4f},     /* glitch: short tuned clicks and zaps */
        };
        static const float WK[7][NUM_K] = {{4, 1, 0, 3, 2, 0},    /* tongue */
                                          {1, 3, 5, 1, 0, 0},     /* groove */
                                          {2, 1, 1, 1, 3, 0},     /* ambient */
                                          {2, 1, 2, 1, 2, 1},     /* cosmic */
                                          {5, 1, 0, 4, 1, 0},     /* drum circle */
                                          {0, 1, 3, 1, 1, 4},     /* machine (IDM) */
                                          {2, 3, 3, 1, 0, 1}};    /* breaks */
        int row = CI ? 4 : MA ? 5 : BR ? 6 : st - 1;
        const float *K = KT[pickw(r, WK[row], NUM_K)];
        /*                         kick        snare       hat         tom    (density lo, hi) */
        static const float DN[7][8] = {{.45f, .6f, .3f, .5f, .3f, .55f, .45f, .7f},     /* tongue */
                                       {.5f, .65f, .45f, .6f, .45f, .7f, .3f, .5f},    /* groove */
                                       {.4f, .55f, .2f, .4f, .2f, .45f, .35f, .55f},   /* ambient */
                                       {.4f, .6f, .35f, .55f, .35f, .65f, .35f, .65f}, /* cosmic */
                                       {.5f, .65f, .4f, .6f, .35f, .6f, .55f, .8f},    /* drum circle */
                                       {.45f, .65f, .45f, .65f, .45f, .75f, .35f, .65f},/* machine */
                                       {.5f, .62f, .5f, .62f, .5f, .75f, .3f, .45f}};  /* breaks */
        static const float PL[4][2] = {{0.f, .1f}, {.36f, .48f}, {.74f, .88f}, {.5f, .68f}};
        static const float DC[4][2] = {{.28f, .6f}, {.12f, .3f}, {.04f, .14f}, {.22f, .5f}};   /* seconds to -35 dB, before the kit's own scale */
        static const float VO[4][2] = {{0.f, .1f}, {.45f, .75f}, {.85f, 1.f}, {.15f, .45f}};  /* the note each rides: kick lowest, hat highest */
        static const float LV[4] = {1.f, .55f, .4f, .75f};
        float pfol = MA ? .35f : C ? .25f : A ? .25f : .08f;
        for (int d = 0; d < NUM_DRUMS; d++) {
            TR(s, DP(d, P_D1_DENS), DN[row][2 * d], DN[row][2 * d + 1]);
            TR(s, DP(d, P_D1_PLACE), PL[d][0], PL[d][1]);
            int src = DSRC_STREAM;
            if (d >= 2 && frand(r) < pfol) src = frand(r) < .7f ? DSRC_L2 : DSRC_L1;
            TD(s, DP(d, P_D1_SRC), src);
            TR(s, DP(d, P_D1_VOICE), VO[d][0], VO[d][1]);
            TR(s, DP(d, P_D1_ATTACK), .001f, d == 2 ? .002f : K[3] > .5f ? .003f : .006f);   /* (1 ms up; softer for the darker kits) */
            TR(s, DP(d, P_D1_HOLD), 0.f, d == 2 ? .002f : .012f);
            float dl = DC[d][0] * K[0] * (A ? 1.6f : 1.f), dh = DC[d][1] * K[0] * (A ? 1.25f : 1.f);
            TR(s, DP(d, P_D1_DECAY), dl < .02f ? .02f : dl, dh > 3.f ? 3.f : dh);
            TR(s, DP(d, P_D1_CURVE), K[1], K[2]);
            float lv = LV[d] * fexp2((frand(r) - .5f) * 1.f);   /* within +-3 dB of the mix */
            TR(s, DP(d, P_D1_LEVEL), lv > 1.f ? 1.f : lv, lv > 1.f ? 1.f : lv);
            float hb = d == 2 ? .2f : d == 1 ? .1f : 0.f;            /* (the hat and snare a little brighter) */
            TR(s, DP(d, P_D1_BRIGHT), fclamp(K[3] + hb, 0.f, 1.f), fclamp(K[4] + hb, 0.f, 1.f));
            TR(s, DP(d, P_D1_SETTLE), fclamp(K[5] + hb, 0.f, 1.f), fclamp(K[6] + hb, 0.f, 1.f));
            TR(s, DP(d, P_D1_SNAP), K[7], K[8]);
        }
        /* and the other way: in Machine, sometimes layer 1 swells with the kick (its Drums envelope) */
        if (on && MA && !s->locked[SEC_GEN1] && frand(r) < .3f) { TD(s, P_G1_ENV, ENV_DRUMS); TD(s, P_G1_ERATE, 0); TR(s, P_G1_CONTOUR, .1f, .5f); }
    }
    if (!s->locked[SEC_LFO]) {
        for (int k = 0; k < 6; k++) {
            TR(s, P_LFO1_RATE + 3 * k, A ? .01f : .02f, A ? .08f : .35f);
            const float w[9] = {4, 2, 0, 0, 0, 3, 0, 0, 0}; TP(s, P_LFO1_SHAPE + 3 * k, w);
            TD(s, P_LFO1_SYNC + 3 * k, 0);
        }
        TR(s, P_LFO_COUPLE, 0.f, .3f);
    }
    if (!s->locked[SEC_FX]) {
        /* fuzz: Groove, Machine and Breaks often, Cosmic sometimes, a touch in Tongue, rarely Ambient */
        float pfz = (MA || BR) ? .4f : G ? .3f : C ? .2f : T ? .08f : .04f;
        if (frand(r) < pfz) { TR(s, P_FZ_MIX, .1f, (G || MA || BR) ? .5f : .25f); TR(s, P_FZ_SUSTAIN, .1f, .7f); TR(s, P_FZ_TONE, .25f, .7f); }
        else TR(s, P_FZ_MIX, 0.f, 0.f);
        TR(s, P_FLT_CUT, A ? 8000.f : 10000.f, 18000.f); TR(s, P_FLT_RES, 0.f, .15f); TR(s, P_FLT_MORPH, 0.f, .05f);
        TR(s, P_DL_SEND, C ? .2f : .1f, C ? .4f : .28f); TR(s, P_DL_FB, .25f, (A || C) ? .55f : .42f);
        { const float w[10] = {0, 0, 4, 2, 0, 3, 1, 0, 0, 0}, wa[10] = {0, 0, 0, 0, 0, 3, 3, 2, 0, 0}, wc[10] = {0, 0, 6, 0, 2, 2, 0, 0, 0, 0};
          int i = pickw(r, A ? wa : C ? wc : w, 10); s->target[P_DL_TIME] = ((float)i + .5f) / 10.f; }
        TD(s, P_DL_TAP, frand(r) < .5f ? 1 : 0);
        if (T) { TR(s, P_RV_SEND, .3f, .5f); TR(s, P_RV_SIZE, .75f, 1.f); TR(s, P_RV_DECAY, 3.f, 9.f); }
        if (G) { TR(s, P_RV_SEND, .15f, .35f); TR(s, P_RV_SIZE, .6f, .9f); TR(s, P_RV_DECAY, 1.5f, 4.5f); }
        if (A) { TR(s, P_RV_SEND, .5f, .8f); TR(s, P_RV_SIZE, .9f, 1.f); TR(s, P_RV_DECAY, 12.f, 45.f); }
        if (C) { TR(s, P_RV_SEND, .4f, .7f); TR(s, P_RV_SIZE, .85f, 1.f); TR(s, P_RV_DECAY, 6.f, 25.f); }
        TR(s, P_RV_DAMP, .15f, .4f); TR(s, P_RV_MOD, .45f, .8f); TD(s, P_RV_TUNED, 0); TR(s, P_DL_TO_RV, .1f, .4f);
        TR(s, P_CMP_THRESH, -18.f, -8.f);
        TR(s, P_CMP_RATIO, G ? 2.f : 1.3f, G ? 3.5f : 2.f);
        TR(s, P_CMP_PUMP, 0.f, G ? .3f : 0.f);
        TR(s, P_CMP_ATT, G ? 6.f : 12.f, G ? 15.f : 40.f);   /* (slow enough for a 40 Hz note) */
        TR(s, P_CMP_MIX, .75f, 1.f);
    }
}

/* gentle, slow modulation for the curated styles */
static void tame_mod(Synth *s) {
    if (s->style == STYLE_WILD || s->locked[SEC_LFO]) return;
    uint32_t *r = &s->rng; int keep = 2 + (int)(frand(r) * 3.f);
    for (int k = 0; k < MOD_SLOTS; k++) {
        ModSlot *m = &s->slot[k];
        if (k >= keep) { m->src = 0; continue; }
        m->src = (uint8_t)(SRC_LFO1 + (int)(frand(r) * 6) % 6);
        m->dst = FRIENDLY[(int)(frand(r) * sizeof FRIENDLY) % sizeof FRIENDLY];
        m->depth = (frand(r) < .5f ? -1.f : 1.f) * (.04f + .08f * frand(r));
    }
}

void params_roll(Synth *s, uint32_t seed) {
    s->rng = seed * 2654435761u + 1u;
    uint32_t *r = &s->rng;
    for (int i = 0; i < NUM_PARAMS; i++) {
        const ParamMeta *m = &g_param_meta[i];
        if (s->locked[m->section] || i == P_TEMPER) continue;   /* (locked: left as it is; Temper is the player's: never rolled) */
        s->target[i] = (m->rlo == m->rhi && m->curve != CURVE_DISC) ? m->rlo : roll_one(m, r);
        if (m->rlo == 0 && m->rhi == 0) s->target[i] = m->curve == CURVE_DISC ? disc(i, 0) : 0;
    }
    float *t = s->target;
    if (!s->locked[SEC_FX]) {
        /* filter type and cutoff are rolled together so the voice never vanishes */
        float u = frand(r);
        if (u < .75f) { t[P_FLT_MORPH] = .08f * frand(r); }
        else if (u < .92f) { t[P_FLT_MORPH] = .4f + .15f * frand(r); t[P_FLT_CUT] = .45f + .25f * frand(r); t[P_FLT_RES] *= .6f; }
        else { t[P_FLT_MORPH] = .75f + .1f * frand(r); t[P_FLT_CUT] = .2f + .2f * frand(r); }
        if (frand(r) < .85f) t[P_RV_TUNED] = disc(P_RV_TUNED, 0);
        if (t[P_DL_FB] > .65f && t[P_DL_SEND] > .4f) t[P_DL_SEND] = .25f;
    }
    roll_together(s);
    params_roll_mod(s);
    if (!s->locked[SEC_KNOBS]) params_roll_knobs(s);
    apply_style(s);
    tame_mod(s);
}

void synth_set_style(Synth *s, int style) { s->style = style < 0 ? 0 : style % NUM_STYLES; }

/* ---------------- public helpers ---------------- */
/* the randomizer: every section not locked, or only those in mask. The generators and the meter start again only
   when their sections are rolled, so rolling the effects leaves the rhythm alone */
void synth_roll_sections(Synth *s, uint32_t seed, float morph_seconds, uint32_t mask) {
    uint8_t keep[NUM_SECTIONS];
    memcpy(keep, s->locked, sizeof keep);
    for (int i = 0; i < NUM_SECTIONS; i++) if (!(mask >> i & 1u)) s->locked[i] = 1;
    s->seed = seed;
    params_roll(s, seed);
    int structural = 0;
    for (int i = 0; i < NUM_PARAMS; i++) {
        const ParamMeta *m = &g_param_meta[i];
        if (m->structural) { if (s->base[i] != s->target[i]) structural = 1; }
        else if (m->curve == CURVE_DISC || morph_seconds <= 0.f) s->base[i] = s->target[i];
    }
    float ticks = morph_seconds * SR / (float)SUB;
    s->morph_rate = ticks > 1.f ? 1.f / ticks : 1.f;
    s->morphing = 1;
    if (structural) { s->rebuild_pending = 1; s->duck_tgt = 0.f; }
    if (!s->locked[SEC_METER]) { s->meter.tau = 0.f; s->meter.bar = 0; }
    for (int l = 0; l < NUM_LAYERS; l++) if (!s->locked[l ? SEC_GEN2 : SEC_GEN1]) {   /* the keys held carry over */
        GenSt *g = &s->layer[l].g; uint8_t h[MAX_NODES]; float hv[MAX_NODES];
        memcpy(h, g->held, sizeof h); memcpy(hv, g->held_v, sizeof hv);
        gen_reset(g, seed + 77u * (uint32_t)l);
        memcpy(g->held, h, sizeof h); memcpy(g->held_v, hv, sizeof hv);
    }
    memcpy(s->locked, keep, sizeof keep);
}
void synth_randomize(Synth *s, uint32_t seed, float morph_seconds) {
    synth_roll_sections(s, seed, morph_seconds, SEC_ALL);
}

void synth_mutate(Synth *s, float amount) {
    uint32_t *r = &s->rng;
    for (int i = 0; i < NUM_PARAMS; i++) {
        const ParamMeta *m = &g_param_meta[i];
        if (s->locked[m->section] || m->structural) continue;
        if (m->curve == CURVE_DISC) {
            if (frand(r) < amount * .15f && m->rhi > m->rlo) s->target[i] = roll_one(m, r);
            continue;
        }
        float v = s->base[i] + amount * (frand(r) * 2.f - 1.f) * .25f * (m->rhi - m->rlo + .05f);
        s->target[i] = v < 0 ? 0 : v > 1 ? 1 : v;
    }
    if (!s->locked[SEC_LFO])                     /* (a locked matrix is the player's: the routes keep their depths) */
        for (int k = 0; k < MOD_SLOTS; k++) if (s->slot[k].src) s->slot[k].depth *= 1.f + amount * (frand(r) - .5f);
    s->morph_rate = 1.f / (0.5f * SR / SUB);
    s->morphing = 1;
}

void synth_set_param(Synth *s, int id, float v) {
    if (v < 0) v = 0; if (v > 1) v = 1;
    s->base[id] = s->target[id] = v;
    if (g_param_meta[id].structural) { s->rebuild_pending = 1; s->duck_tgt = 0.f; }
}
void synth_set_macro(Synth *s, int m, float v) {
    if (m >= 0 && m < NUM_MACRO) s->macro[m] = v < -1 ? -1 : v > 1 ? 1 : v;
}

static const char *SURF_NAMES[] = {"plane","torus","sphere"};
static const char *STYLE_NAMES[] = {"wild","tongue drum","groove","ambient","cosmic","drum circle","machine","breaks"};

static int slen(const char *b) { int n = 0; while (b[n]) n++; return n; }
void synth_describe(Synth *s, char *buf, int len) {
    snprintf(buf, len, "[%s] seed %u | %s on %s | 1: %s x%d %s, bank c%d w%d | 2: %s x%d %s, bank c%d w%d | %s %d bpm, dens %d%% threes %d%% swing %d%%",
        STYLE_NAMES[s->style % NUM_STYLES], s->seed, s->scale.name, SURF_NAMES[pint(s, P_SURFACE)],
        gen_name(pint(s, P_G1_FUNC)), pint(s, P_G1_POINTS) + 1, env_name(pint(s, P_G1_ENV)), (int)(pval(s, P_V1_CENTER) * 100.f), (int)(pval(s, P_V1_BAND) * 100.f),
        gen_name(pint(s, P_G2_FUNC)), pint(s, P_G2_POINTS) + 1, env_name(pint(s, P_G2_ENV)), (int)(pval(s, P_V2_CENTER) * 100.f), (int)(pval(s, P_V2_BAND) * 100.f),
        rh_engine_name(pint(s, P_RH_ENGINE)), (int)(pval(s, P_TEMPO) + .5f), (int)(pval(s, P_RH_DENS) * 100.f + .5f),
        (int)(pval(s, P_RH_THREES) * 100.f + .5f), (int)(pval(s, P_RH_SWING) * 100.f + .5f));
    if (pint(s, P_DR_ON)) {
        int n = slen(buf);
        static const char *const LN[NUM_DLAYERS] = {"on layer 1", "on layer 2", "split"};
        snprintf(buf + n, len - n, " | drums %s, %s", LN[pint(s, P_DR_LAYER) % NUM_DLAYERS], rh_assign_name(pint(s, P_RH_ASSIGN)));
    }
    if (pval(s, P_FZ_MIX) > .01f) { int n = slen(buf); snprintf(buf + n, len - n, " | fuzz %d%%", (int)(pval(s, P_FZ_MIX) * 100.f + .5f)); }
    int mi = gen_modulator(s);
    if (mi >= 0) {
        int n = slen(buf);
        snprintf(buf + n, len - n, " | layer %d %s (depth %d%%)", mi + 1, route_name(pint(s, LP(mi, P_G1_ROUTE))),
                 (int)(pval(s, LP(mi, P_G1_ENERGY)) * 100.f + .5f));
    }
}
