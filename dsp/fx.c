/*
 * Effects chain:
 *   fuzz -> filter (low-pass / band-pass / high-pass, one resonant state-variable filter)
 *   -> [dry | BBD delay send | FDN reverb send] -> drive -> bus compressor -> master
 *   -> DC block -> look-ahead peak limiter (ceiling -0.5 dBFS) -> soft clip for whatever still gets past it
 *
 * The fuzz is after the Big Muff: a 60 Hz high-pass, two cascaded gain stages each into a soft clipper (a little
 * asymmetric, like a diode pair) with a 5.5 kHz low-pass and a 90 Hz high-pass between them, then the tone control:
 * a blend from a 500 Hz low-pass to a 1.4 kHz high-pass, the scoop in the middle. Sustain is the first stage's
 * gain (24 to 52 dB). It acts on the whole mix, so it makes sum and difference tones between the notes: on a just
 * scale those land back on the harmonic series, which is why it suits this instrument. At the sample rate (v8 build F:
 * the 2x oversampling was cut for the device's budget; what the clippers make above 22 kHz folds down, softly).
 *
 * The reverb runs at half the sample rate (build F, for the budget): its input through a [1 3 3 1] low-pass, every
 * other sample, its output joined up by a straight line. Line lengths and pre-delay in half-rate steps.
 *
 * Gain staging: DRIVE puts the mix into the compressor around -18 dBFS RMS, the compressor glues it (a few dB),
 * the limiter takes only the peaks (a roll about -15 to -17 dBFS RMS; Master pushes it further if wanted).
 * The limiter: peaks over 8-sample segments, the loudest of the 8 segments ahead (64 samples) sets the target gain;
 * the gain falls to it with a 16-sample time constant (all there before the peak arrives) and recovers in ~280 ms (slow enough not to ride a bass note's own cycles).
 * Coefficients are refreshed once per control tick in fx_update().
 */
#define MANIFOLD_DSP_UNIT
#include "synth.h"
#include "mstring.h"

#define DLN 16384
#define DRIVE 2.0f            /* +6 dB into the compressor (it was +8: the rhythmic styles sat in the limiter half the
                                 time, up to 9-12 dB, and its fast gain turned into heat at the top: ear fatigue) */
#define LIM_CEIL .944f        /* -0.5 dBFS */
static const float RV_BASE[8] = {1493, 1931, 2213, 2617, 2999, 3413, 3907, 4373};   /* mutually prime, ~34 to 99 ms */
static const int AP_LEN[4] = {71, 54, 190, 139};   /* (half-rate steps: 142, 107, 379, 277 samples) */
static const float DL_DIV[10] = {.25f, 1.f/3, .375f, .5f, 2.f/3, .75f, 1.f, 4.f/3, 1.5f, 2.f};
static const float DL_TAPS[5] = {1.f, .75f, 2.f/3, .5f, .8f};
/* coefficient block, refreshed per tick */
static struct {
    float fz_g1, fz_mix, fz_tone, fz_hp, fz_lp, fz_hp2, fz_tl, fz_th;
    float m_a1, m_a2, m_a3, m_k, m_wl, m_wb, m_wh;
    float d_send, d_fb, d_tone_a, d_D_tgt, d_rate_tgt, d_tap, d_aa;
    float r_dry, r_send, r_glo[8], r_ghi[8], r_pre, r_mod, r_to, r_out, r_lp;
    int   r_prei;                                   /* the pre-delay in the reverb's steps */
    int   r_freeze;
    float c_att, c_rel, c_thresh, c_ratio, c_mix, c_makeup, c_pump;
    float master;
} C;
static float g_drate = 1.f;
static int g_rint[8];
static float g_rf0[8], g_rf1[8];   /* a moving line's length at the block's start and end (its reads between samples) */
static uint8_t g_rmov;             /* which lines are moving (a bit each) */
static float g_rm0[4], g_rm1[4];                  /* the four modulated lines' lengths, last tick and this */
static const int RV_MODL[4] = {0, 2, 5, 7};

void fx_init(Fx *f) {
    memset(f, 0, sizeof *f);
    f->lim_g = f->lim_tgt = 1.f;
    for (int i = 0; i < 8; i++) f->rlen[i] = f->rlen_tgt[i] = RV_BASE[i] * .7f;
    f->dD = 4000.f; g_drate = 1.f;
    f->cgain = 1.f;
}

#include "ctl_on.h"
DSP_CTL static void svf_coef(float fc, float k, float *a1, float *a2, float *a3) {
    if (fc > 0.45f * SR) fc = 0.45f * SR;
    float g = ftan_pi(fc / SR);
    *a1 = 1.f / (1.f + g * (g + k)); *a2 = g * *a1; *a3 = g * *a2;
}

DSP_CTL void fx_update(Synth *s) {
    Fx *f = &s->fx;
    /* fuzz: the stages' gains and their one-pole filters at the rate they run at (2x, or 1x if the profiler says) */
    {   float k = -TWO_PI / SR * 1.4427f;
        C.fz_g1 = 16.f * fexp2(4.65f * pnorm(s, P_FZ_SUSTAIN));          /* x16 .. x400 on a mix that peaks near 0.2 */
        C.fz_mix = pnorm(s, P_FZ_MIX); C.fz_tone = pnorm(s, P_FZ_TONE);
        C.fz_hp = 1.f - fexp2(k * 60.f); C.fz_lp = 1.f - fexp2(k * 5500.f); C.fz_hp2 = 1.f - fexp2(k * 90.f);
        C.fz_tl = 1.f - fexp2(k * 500.f); C.fz_th = 1.f - fexp2(k * 1400.f);
    }

    /* the filter: low-pass at 0, band-pass at 1/2, high-pass at 1, blended between */
    float res = pval(s, P_FLT_RES), m = pnorm(s, P_FLT_MORPH);
    C.m_k = 2.f - 1.9f * res;
    svf_coef(pval(s, P_FLT_CUT), C.m_k, &C.m_a1, &C.m_a2, &C.m_a3);
    C.m_wl = m < .5f ? 1.f - 2.f * m : 0.f;
    C.m_wb = 1.f - fabsf(2.f * m - 1.f);
    C.m_wh = m > .5f ? 2.f * m - 1.f : 0.f;

    /* BBD delay: fixed buffer, write rate sets the time */
    float beat = 60.f / pval(s, P_TEMPO);
    int di = (int)(pnorm(s, P_DL_TIME) * 9.999f);
    float tsec = beat * DL_DIV[di]; if (tsec > 3.f) tsec = 3.f; if (tsec < .02f) tsec = .02f;
    float Dmax = (float)(DLN - 8);
    float rate = Dmax / (tsec * SR); if (rate > 1.f) rate = 1.f; if (rate < .125f) rate = .125f;
    C.d_rate_tgt = rate;
    C.d_D_tgt = tsec * SR * rate; if (C.d_D_tgt > Dmax) C.d_D_tgt = Dmax;
    C.d_tap = DL_TAPS[pint(s, P_DL_TAP)];
    C.d_send = pnorm(s, P_DL_SEND);
    C.d_fb = pval(s, P_DL_FB);
    float tone_fc = 600.f * fexp2(pnorm(s, P_DL_TONE) * 4.3f);
    C.d_tone_a = 1.f - fexp2(-TWO_PI * tone_fc / SR * 1.4427f);

    /* FDN reverb */
    float size = pval(s, P_RV_SIZE), t60 = pval(s, P_RV_DECAY), damp = pnorm(s, P_RV_DAMP);
    int tuned = pint(s, P_RV_TUNED);
    for (int i = 0; i < 8; i++) {
        float L = RV_BASE[i] * size;
        if (tuned && s->eng.n_nodes) {         /* snap each line to a period of a scale pitch */
            float fq = s->eng.node[(i * 3) % s->eng.n_nodes].freq, per = SR / fq;
            float mlt = floorf(L / per + .5f); if (mlt < 1) mlt = 1;
            L = per * mlt; if (L > 4500.f) L = per * (mlt - 1.f > 1.f ? mlt - 1.f : 1.f);
        }
        if (L > 4500.f) L = 4500.f; if (L < 50.f) L = 50.f;
        f->rlen_tgt[i] = (float)(int)(L * .5f + .5f) * 2.f;   /* (a whole number of the reverb's steps: still, it reads whole ones) */
        float th = t60 * (1.f - .75f * damp);   /* the top's decay: as long as the rest at Damp 0, a quarter of it at full */
        C.r_glo[i] = fexp2(-9.9658f * f->rlen[i] / (t60 * SR));
        C.r_ghi[i] = fexp2(-9.9658f * f->rlen[i] / (th * SR));
    }
    C.r_freeze = pint(s, P_RV_FREEZE);
    if (C.r_freeze) for (int i = 0; i < 8; i++) C.r_glo[i] = C.r_ghi[i] = 1.f;
    /* Reverb and delay are mixes, not additions: as Reverb rises the dry falls (equal power: all wet at full), and
       the delay takes its share of the dry too. Proportions, so the sum stays the same size. */
    {   float m = pnorm(s, P_RV_SEND), q = .25f * m;      /* quarter of a cycle at full */
        C.r_send = fsin(q); C.r_dry = fsin(q + .25f) * (1.f - .35f * pnorm(s, P_DL_SEND)); }
    C.r_out = 1.7f / sqrtf(sqrtf(1.f + t60 / 1.2f));    /* long tails store more energy: keep loudness even (gently) */
    C.r_pre = pval(s, P_RV_PRE) * SR;
    const float rs = .5f;                              /* the reverb's steps are two samples long */
    C.r_prei = (int)(C.r_pre * rs);
    C.r_lp = .325f;                                    /* the lines' damping split: 1.5 kHz (it was 2.7: Damp hardly
                                                          moved the tail, which rang as long at 12 kHz as at 250 Hz) */
    C.r_mod = pnorm(s, P_RV_MOD);
    C.r_to = pnorm(s, P_DL_TO_RV);

    /* compressor */
    C.c_att = 1.f - fexp2(-1.4427f / (pval(s, P_CMP_ATT) * .001f * SR));
    C.c_rel = 1.f - fexp2(-1.4427f / (pval(s, P_CMP_REL) * .001f * SR));
    C.c_thresh = pval(s, P_CMP_THRESH);
    C.c_ratio = pval(s, P_CMP_RATIO);
    C.c_mix = pnorm(s, P_CMP_MIX);
    C.c_makeup = fexp2(-C.c_thresh * (1.f - 1.f / C.c_ratio) * .5f * 0.1661f);   /* half the static reduction back */
    C.c_pump = pnorm(s, P_CMP_PUMP);
    C.master = pval(s, P_MASTER);

    /* BBD: rate and anti-alias filter glide at control rate */
    g_drate += 0.016f * (C.d_rate_tgt - g_drate);
    C.d_aa = 1.f - fexp2(-TWO_PI * .21f * g_drate * 1.4427f);
    /* reverb line lengths glide at control rate. An unmodulated line that is still reads whole steps; while its
       length moves (Size turned or modulated, Tuned following the notes) it reads between steps, its length gliding
       across the block: a smooth bend, where a whole-step read jumped a step at a time (a click each). It lands on a
       whole step (rlen_tgt is one), and from there reads whole steps again: the same samples, no seam */
    g_rmov = 0;
    for (int i = 0; i < 8; i++) {
        float d = f->rlen_tgt[i] - f->rlen[i];
        g_rf0[i] = f->rlen[i] * rs;
        if (d > .02f || d < -.02f) {          /* (the glide's tail at least 0.02 a block: it lands, not creeps for seconds) */
            float st = 0.0016f * d;
            if (st > -.02f && st < .02f) st = d > 0.f ? .02f : -.02f;
            f->rlen[i] += st; g_rmov |= (uint8_t)(1u << i);
        } else f->rlen[i] = f->rlen_tgt[i];
        g_rf1[i] = f->rlen[i] * rs;
        if (g_rf0[i] != g_rf1[i]) g_rmov |= (uint8_t)(1u << i);
        g_rint[i] = (int)(f->rlen[i] * rs + .5f);
    }
    float mr = (0.13f + 0.6f * C.r_mod) * (float)SUB / SR;
    f->rmod_ph += mr; if (f->rmod_ph > 1.f) f->rmod_ph -= 1.f;
    float md = 14.f * C.r_mod;
    static const float MR[4] = {1.f, 1.37f, .83f, 1.61f}, MP[4] = {0.f, .3f, .55f, .8f};
    for (int j = 0; j < 4; j++) {
        g_rm0[j] = g_rm1[j];
        float ph = f->rmod_ph * MR[j] + MP[j];
        g_rm1[j] = (f->rlen[RV_MODL[j]] + md * fsin(ph - (float)(int)ph)) * rs;
        g_rf0[RV_MODL[j]] = g_rm0[j]; g_rf1[RV_MODL[j]] = g_rm1[j];   /* (a modulated line: always read between steps) */
        g_rmov |= (uint8_t)(1u << RV_MODL[j]);
    }
}

/* --------------------------------------------------------------- */
#include "ctl_off.h"
DSP_INL float svf_run(Svf *z, float v0, float a1, float a2, float a3, float *lp, float *hp, float k) {
    float v3 = v0 - z->ic2;
    float v1 = a1 * z->ic1 + a2 * v3;
    float v2 = z->ic2 + a2 * z->ic1 + a3 * v3;
    z->ic1 = 2.f * v1 - z->ic1; z->ic2 = 2.f * v2 - z->ic2;
    if (lp) *lp = v2;
    if (hp) *hp = v0 - k * v1 - v2;
    return v1;
}

/* one (oversampled) sample through the fuzz */
DSP_INL float fz_clip(float x) { return x > 0.f ? ftanh(x) : .9f * ftanh(x * 1.1111f); }
DSP_INL float shape(Fx *f, float x, int c) {
    f->fz_hp[c] += C.fz_hp * (x - f->fz_hp[c]); x -= f->fz_hp[c];
    float y = fz_clip(x * C.fz_g1);
    f->fz_l1[c] += C.fz_lp * (y - f->fz_l1[c]); y = f->fz_l1[c];
    f->fz_h1[c] += C.fz_hp2 * (y - f->fz_h1[c]); y -= f->fz_h1[c];
    y = fz_clip(y * 6.f);
    f->fz_l2[c] += C.fz_lp * (y - f->fz_l2[c]); y = f->fz_l2[c];
    f->fz_tl[c] += C.fz_tl * (y - f->fz_tl[c]);
    f->fz_thx[c] += C.fz_th * (y - f->fz_thx[c]);
    return ((1.f - C.fz_tone) * f->fz_tl[c] + C.fz_tone * .8f * (y - f->fz_thx[c])) * .13f;
}
DSP_INL float rd16(const int16_t *b, int len, float pos, float sc) {
    if (pos < 0) pos += (float)len;
    if (pos < 0) pos += (float)len;
    int i0 = (int)pos; float fr = pos - (float)i0; if (i0 >= len) i0 -= len;
    int i1 = i0 + 1 == len ? 0 : i0 + 1;
    return ((float)b[i0] + ((float)b[i1] - (float)b[i0]) * fr) * sc;
}
DSP_INL int16_t to16(float v, float sc) {
    v *= sc; if (v > 32767.f) v = 32767.f; if (v < -32767.f) v = -32767.f;
    return (int16_t)(v < 0.f ? v - .5f : v + .5f);   /* (rounded: a cast cuts toward zero, a loss the tails then gated on) */
}

/* one step of the FDN reverb: the pre-delay, the input diffusion, the eight lines, their mix. The lines' lengths
   (g_rint, g_rm0 / g_rm1) and the damping (C.r_lp) are already in steps of the rate it runs at (fx_update) */
DSP_INL void rv_step(Fx *f, float rin, float fr, float gd, const int *apl, float *rlo, float *rro) {
    f->pre[f->pre_w] = to16(rin, 8192.f);
    int pi2 = f->pre_w - C.r_prei; if (pi2 < 0) pi2 += RV_PRE;
    float pd = (float)f->pre[pi2] * (1.f / 8192.f) + gd;
    if (++f->pre_w >= RV_PRE) f->pre_w = 0;
    for (int a = 0; a < 4; a++) {                             /* input diffusion */
        float *b = f->ap_buf[a]; int w = f->ap_w[a];
        float y = -0.6f * pd + b[w];
        b[w] = pd + 0.6f * y; if (++w >= apl[a]) w = 0; f->ap_w[a] = w; pd = y;
    }
    float v[8], y8[8], lp = C.r_lp;
    /* every line has the same buffer and they move together: one write index (rw[0]) for all eight. Lines 0, 2,
       5 and 7 are modulated (a moving, lush tail: read between samples), the others read whole samples */
    int rw = f->rw[0];
    #define RV_LINE(i, y) do { y += gd; y8[i] = y; f->rlp[i] += lp * (y - f->rlp[i]); \
                               v[i] = C.r_ghi[i] * y + (C.r_glo[i] - C.r_ghi[i]) * f->rlp[i]; } while (0)
    /* every line reads between steps, its length gliding across the block (g_rf0 .. g_rf1): a modulated line, or a
       still one while its length moves (Size turned or modulated, Tuned following the notes). A still line's length
       is a whole number of steps, so it reads whole samples, as before */
    /* (a lean read: a line is shorter than its buffer, so one wrap; whole samples come out exactly) */
    #define RV_RD(i) do { float pos = (float)rw - (g_rf0[i] + (g_rf1[i] - g_rf0[i]) * fr); if (pos < 0.f) pos += (float)RV_BUF; \
                          int i0 = (int)pos; float fq = pos - (float)i0; const int16_t *bf = f->rbuf[i]; \
                          float y0 = (float)bf[i0], y = (y0 + ((float)bf[i0 + 1 < RV_BUF ? i0 + 1 : 0] - y0) * fq) * (1.f / 8192.f); \
                          RV_LINE(i, y); } while (0)
    RV_RD(0); RV_RD(1); RV_RD(2); RV_RD(3); RV_RD(4); RV_RD(5); RV_RD(6); RV_RD(7);
    #undef RV_RD
    #undef RV_LINE
    {   /* fast Hadamard, unrolled: 24 add/sub */
        float a0 = v[0] + v[1], a1 = v[0] - v[1], a2 = v[2] + v[3], a3 = v[2] - v[3];
        float a4 = v[4] + v[5], a5 = v[4] - v[5], a6 = v[6] + v[7], a7 = v[6] - v[7];
        float b0 = a0 + a2, b2 = a0 - a2, b1 = a1 + a3, b3 = a1 - a3;
        float b4 = a4 + a6, b6 = a4 - a6, b5 = a5 + a7, b7 = a5 - a7;
        v[0] = b0 + b4; v[4] = b0 - b4; v[1] = b1 + b5; v[5] = b1 - b5;
        v[2] = b2 + b6; v[6] = b2 - b6; v[3] = b3 + b7; v[7] = b3 - b7;
    }
    {   float pa = .35f * pd;
        f->rbuf[0][rw] = to16(v[0] * 0.35355339f + pa, 8192.f); f->rbuf[1][rw] = to16(v[1] * 0.35355339f - pa, 8192.f);
        f->rbuf[2][rw] = to16(v[2] * 0.35355339f + pa, 8192.f); f->rbuf[3][rw] = to16(v[3] * 0.35355339f - pa, 8192.f);
        f->rbuf[4][rw] = to16(v[4] * 0.35355339f + pa, 8192.f); f->rbuf[5][rw] = to16(v[5] * 0.35355339f - pa, 8192.f);
        f->rbuf[6][rw] = to16(v[6] * 0.35355339f + pa, 8192.f); f->rbuf[7][rw] = to16(v[7] * 0.35355339f - pa, 8192.f);
        if (++rw >= RV_BUF) rw = 0;
        f->rw[0] = rw;
    }
    *rlo = (y8[0] - y8[1] + y8[2] - y8[3] + y8[4] - y8[5] + y8[6] - y8[7]) * .5f * C.r_out;
    *rro = (y8[0] + y8[1] - y8[2] - y8[3] + y8[4] + y8[5] - y8[6] - y8[7]) * .5f * C.r_out;
}

DSP_RAM void fx_process(Synth *s, float *L, float *R, int n, float trig_env) {
    Fx *f = &s->fx;
    uint32_t dn = 0x1234567u + (uint32_t)f->ccount;
    static float gds[SUB];                                     /* static: off the interrupt stack */
    /* 0. denormal guard: ~-300 dBFS noise keeps every recursive state out of the subnormal range */
    for (int t = 0; t < n; t++) { float g = (float)(int32_t)xs32(&dn) * 4.6e-25f; gds[t] = g; L[t] += g; R[t] -= g; }

    /* Stages 1-3 run over the whole block, one stage (and one channel, one band) at a time, so each filter's
       state stays in registers through its loop. Each is causal and independent: the output is the same as
       running them sample by sample. */

    /* 1. fuzz, at the sample rate (the 2x oversampling is gone: it cost more than it was heard) */
    if (C.fz_mix > 0.002f) {
        float mix = C.fz_mix;
        for (int c = 0; c < 2; c++) {
            float *X = c ? R : L, dx = f->dc_x[c], dy = f->dc_y[c];
            for (int t = 0; t < n; t++) {
                float w = shape(f, X[t], c), dc = w - dx + .995f * dy; dx = w; dy = dc;
                X[t] += mix * (dc - X[t]);
            }
            f->dc_x[c] = dx; f->dc_y[c] = dy;
        }
    }

    /* 2. the filter (LP / BP / HP blend), per channel over the block */
    for (int c = 0; c < 2; c++) {
        float *X = c ? R : L, i1 = f->morph[c].ic1, i2 = f->morph[c].ic2;
        float a1 = C.m_a1, a2 = C.m_a2, a3 = C.m_a3, k = C.m_k, wl = C.m_wl, wb = C.m_wb * C.m_k, wh = C.m_wh;
        for (int t = 0; t < n; t++) {
            float v0 = X[t], v3 = v0 - i2;
            float v1 = a1 * i1 + a2 * v3, v2 = i2 + a2 * i1 + a3 * v3;
            i1 = 2.f * v1 - i1; i2 = 2.f * v2 - i2;
            X[t] = wl * v2 + wb * v1 + wh * (v0 - k * v1 - v2);
        }
        f->morph[c].ic1 = i1; f->morph[c].ic2 = i2;
    }

    /* 4-6: delay, reverb, compressor and output, sample by sample (feedback paths) */
    for (int t = 0; t < n; t++) {
        float gd = gds[t];
        float x[2] = {L[t], R[t]};

        /* 4a. BBD delay */
        f->dD += 0.0004f * (C.d_D_tgt - f->dD);
        float aa = C.d_aa;
        float wc = (float)f->dw + f->dacc;
        float yl = rd16(f->dbuf, DLN, wc - 1.f - f->dD, 1.f / 16384.f);
        float yr = rd16(f->dbuf, DLN, wc - 1.f - f->dD * C.d_tap, 1.f / 16384.f);
        f->dout_lp[0] += aa * (yl - f->dout_lp[0]); f->dout_lp[1] += aa * (yr - f->dout_lp[1]);
        yl = f->dout_lp[0] + gd; yr = f->dout_lp[1] - gd;   /* int16 reads are exact zeros: guard */
        float fbs = 0.5f * (yl + yr);
        { float c = fbs * .6667f; c = c > 1.f ? 1.f : c < -1.f ? -1.f : c; fbs = c * (1.5f - .5f * c * c); }  /* unity-slope soft clip */
        f->dlp1 += C.d_tone_a * (fbs - f->dlp1);                 /* tone */
        f->dfb_hp += 0.011f * (f->dlp1 - f->dfb_hp);             /* ~80 Hz high-pass */
        float din = 0.5f * (x[0] + x[1]) * C.d_send + C.d_fb * (f->dlp1 - f->dfb_hp);
        f->dlp2 += aa * (din - f->dlp2);                         /* anti-alias before write */
        f->dacc += g_drate;
        if (f->dacc >= 1.f) { f->dacc -= 1.f; f->dbuf[f->dw] = to16(f->dlp2, 16384.f); f->dw = (f->dw + 1) & (DLN - 1); }

        /* 4b. FDN reverb, every other sample: the input through a 4-pole low-pass at 9 kHz (Butterworth: -14 dB at
           12 kHz, -28 at 15: what is above 12 kHz would fold back down into the tail at half the rate; the [1 3 3 1]
           it had let a 15 kHz tone back in at 9 kHz only 7 dB down), the output a straight line between steps */
        float rin = 0.5f * (x[0] + x[1]) * C.r_send + 0.5f * (yl + yr) * C.r_to;
        if (C.r_freeze) rin = 0.f;
        float rl, rr, u;
        {   float *h = f->rv_h;                                   /* (two biquads, transposed direct form II) */
            float y1 = .03797304f * rin + h[0];
            h[0] = .07594608f * rin + .4129187f * y1 + h[1]; h[1] = .03797304f * rin - .07900857f * y1;
            u = y1 + h[2];
            h[2] = 2.f * y1 + .56545007f * u + h[3]; h[3] = y1 - .47759225f * u;
        }
        if ((f->rv_ph ^= 1) != 0) {
            float nl, nr; rv_step(f, u, (float)(t + 1) * (1.f / SUB), gd, AP_LEN, &nl, &nr);
            rl = .5f * (f->rv_o[0] + nl); rr = .5f * (f->rv_o[1] + nr); f->rv_o[0] = nl; f->rv_o[1] = nr;
        } else { rl = f->rv_o[0]; rr = f->rv_o[1]; }

        float oL = (x[0] * C.r_dry + yl + rl) * DRIVE, oR = (x[1] * C.r_dry + yr + rr) * DRIVE;

        /* 5. compressor: detector every sample, gain computer every 8 */
        float pk = fabs_(oL) > fabs_(oR) ? fabs_(oL) : fabs_(oR);
        f->cenv += (pk > f->cenv ? C.c_att : C.c_rel) * (pk - f->cenv);
        if ((f->ccount++ & 7) == 0) {
            float lvl = 6.0206f * flog2(f->cenv + 1e-6f), over = lvl - C.c_thresh, gr;
            float sl = 1.f / C.c_ratio - 1.f;
            if (over <= -3.f) gr = 0.f; else if (over < 3.f) gr = sl * (over + 3.f) * (over + 3.f) / 12.f; else gr = sl * over;
            gr -= C.c_pump * trig_env * 10.f;
            f->cgain_db = fexp2(gr * 0.1661f) * C.c_makeup;
        }
        f->cgain += 0.125f * (f->cgain_db - f->cgain);
        float g = (1.f + C.c_mix * (f->cgain - 1.f)) * C.master;
        oL *= g; oR *= g;

        /* 6. DC block, then the look-ahead limiter, then a soft clip for the rare over */
        float o[2] = {oL, oR};
        for (int c = 0; c < 2; c++) {
            float dc = o[c] - f->odc_x[c] + .9995f * f->odc_y[c]; f->odc_x[c] = o[c]; f->odc_y[c] = dc;
            o[c] = dc;
        }
        {
            float a0 = fabs_(o[0]), a1 = fabs_(o[1]), a = a0 > a1 ? a0 : a1;
            if (a > f->lim_pk) f->lim_pk = a;
            if (++f->lim_n == 8) {                           /* a segment done: the window's peak, the target */
                f->lim_seg[f->lim_w >> 3 & (LIM_LA / 8 - 1)] = f->lim_pk; f->lim_pk = 0.f; f->lim_n = 0;
                float m = 0.f; for (int k = 0; k < LIM_LA / 8; k++) if (f->lim_seg[k] > m) m = f->lim_seg[k];
                f->lim_tgt = m > LIM_CEIL * .998f ? LIM_CEIL * .998f / m : 1.f;   /* a hair under: rounding never lands over */
            }
            float d0 = f->lim_d[0][f->lim_w], d1 = f->lim_d[1][f->lim_w];
            f->lim_d[0][f->lim_w] = o[0]; f->lim_d[1][f->lim_w] = o[1];
            f->lim_w = (f->lim_w + 1) & (LIM_LA - 1);
            f->lim_g += (f->lim_tgt < f->lim_g ? .06f : .00008f) * (f->lim_tgt - f->lim_g);   /* (down over ~16 samples: still
                                                                                       there before the peak, 64 ahead, and smoother) */
            o[0] = d0 * f->lim_g; o[1] = d1 * f->lim_g;
        }
        for (int c = 0; c < 2; c++) {
            float a = fabs_(o[c]);
            if (a > LIM_CEIL) { o[c] = (o[c] > 0 ? 1.f : -1.f) * (LIM_CEIL + (1.f - LIM_CEIL) * ftanh((a - LIM_CEIL) / (1.f - LIM_CEIL))); f->clips++; }
        }
        L[t] = o[0]; R[t] = o[1];
    }
    f->lim_gr = 6.0206f * flog2(f->lim_g > 1e-4f ? f->lim_g : 1e-4f);
}
