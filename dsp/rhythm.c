/*
 * The rhythm. There is no meter, no grid and no stored pattern: only functions of time.
 *
 * The stream.   R(x) = sum_k w_k b(k x - p_k),  x in bars,  b(z) = exp(5 (cos 2 pi z - 1))  (period 1, peak 1 at 0)
 *   k:  1 2 4          always (the bar, its half, its beat)
 *       8 16 32        the twos    \  Threes crossfades the two families (both at the middle: 3 against 4);
 *       3 6 12 24 18   the threes  /  Fine opens the faster ones in turn
 *       4, phase p     the off-beat: Swing turns its phase from 1/2 (straight) through 2/3 (triplet swing) to 3/4
 *                      (hard), and as it comes in the straight 8 16 32 give way to 12 and 24 underneath
 *   Turn shifts the threes (and 18) in time against the twos, by up to a third of a bar: any rotation is a rhythm.
 *
 * An event is a peak of R above the threshold: its time (found between control ticks by a parabola through three
 * values, so it lands on the sample) and its strength (how far over, 0..1). Density is the threshold. Motion lets a
 * generator's point move it. The events carry no instrument.
 *
 * Form (long patterns without bars or fills): slower functions, cos 2 pi (x / q - p), move the threshold (q = 4, 16,
 * 64, 3), the sixteenths' weight (8), the three-feel (5), the selector (2, 16) and the tom (7). With all of them the
 * whole only repeats after 6,720 bars, yet neighbouring bars stay alike because the stream under them never changes.
 *
 * Which drum (the second layer):
 *   Select   a selector function, cos 2 pi (r x - p) at r = 1/2 .. 3 a bar, is read at each event. Each drum scores
 *            1 - 1.6 |strength - its own| (Place: the kick wants the strongest events, the hat the weakest), plus the
 *            selector for the kick, minus it for the snare (so they answer each other), plus its Density, minus
 *            Feedback x how lately it played. The best score plays; the hat also plays under the others. The very
 *            strongest events (where every function peaks together) are always the kick's: the anchor.
 *   Count    the hat on every event, the kick every n-th, the snare every 2n-th (between two kicks), the tom on the
 *            strong ones left; the count starts again at each event of full strength, so the kick keeps its anchor.
 * A drum set to follow a layer is outside this: it fires from that layer's point (drums.c).
 *
 * Lock (the other engine): four phase oscillators, one a drum, each a little off a whole-number rate (kick 2, snare 2
 * half a cycle round, hat 8, tom 3 a bar). Pull couples each to its exact rate, d(theta) = r (1 + detune) + Pull r
 * sin 2 pi (r x - theta): above its detune it locks, below it slips and drifts and catches again; every hit also
 * nudges the others toward their own next hit. Hits are loudest in step.
 */
#define MANIFOLD_DSP_UNIT
#include "synth.h"
#include "mstring.h"

static const uint8_t RH_K[RH_NH] = {1, 2, 4, 8, 16, 32, 3, 6, 12, 24, 18, 4};
static const float SELRATE_V[NUM_SELRATES] = {.5f, 1.f, 1.5f, 2.f, 3.f};
static const char *const SELRATE_N[NUM_SELRATES] = {"1 per 2 bars", "1 a bar", "3 per 2 bars", "2 a bar", "3 a bar"};
const char *selrate_name(int r) { return r >= 0 && r < NUM_SELRATES ? SELRATE_N[r] : "?"; }
const char *rh_engine_name(int e) { return e == RH_LOCK ? "Lock" : "Threshold"; }
const char *rh_assign_name(int a) { return a == RH_COUNT ? "Count" : "Select"; }

void drums_trigger(Synth *s, int d, int node, float vel, int32_t t);

#include "ctl_on.h"
DSP_INL float sm01(float x) { x = fclamp(x, 0.f, 1.f); return x * x * (3.f - 2.f * x); }
DSP_INL float bump(float z) { return fexp2(7.2135f * (fsin(z + .25f) - 1.f)); }   /* exp(5 (cos 2 pi z - 1)) */

/* cos 2 pi (x / q - ph), x the bars since the start: exact however long it has run */
DSP_CTL float rhythm_slow(const Synth *s, int q, float ph) {
    const Rhythm *m = &s->meter;
    float z = ((float)(m->bar % (uint32_t)q) + m->tau) / (float)q - ph;
    return fsin(z + .25f);
}

DSP_CTL float rhythm_R(const Synth *s, float tau) {
    const Rhythm *m = &s->meter; float r = 0.f;
    for (int i = 0; i < RH_NH; i++) if (m->w[i] > 1e-4f) r += m->w[i] * bump((float)RH_K[i] * tau - m->ph[i]);
    return m->top > 1e-6f ? r / m->top : 0.f;
}

DSP_CTL static void weights(Synth *s) {
    Rhythm *m = &s->meter;
    float T = pval(s, P_RH_THREES), F = pval(s, P_RH_FINE), Fm = pval(s, P_RH_FORM), sw = sm01(pval(s, P_RH_SWING) * 3.f);
    float g1 = sm01(F / .3f), g2 = sm01((F - .25f) / .4f), g3 = sm01((F - .6f) / .4f);
    float a = sm01((1.f - T) * 1.6f), b = sm01(T * 1.6f), turn = pval(s, P_RH_TURN);
    float *w = m->w;
    w[0] = .5f; w[1] = .45f; w[2] = .4f;
    w[3] = .45f * a * g1 * (1.f - sw);
    w[4] = .24f * a * g2 * (1.f - sw) * (1.f + Fm * .6f * rhythm_slow(s, 8, .9f));
    w[5] = .12f * a * g3 * (1.f - sw);
    w[6] = .32f * b * ((1.f - Fm) + Fm * (.5f + .5f * rhythm_slow(s, 5, 0.f)));
    w[7] = .40f * b * g1;
    w[8] = .28f * b * g2 + .20f * a * sw * g2;
    w[9] = .13f * b * g3 + .12f * a * sw * g3;
    w[10] = .18f * b * g3 * (.5f + .5f * rhythm_slow(s, 4, .8f));
    w[11] = .34f * a * g1 * sw;
    for (int i = 0; i < RH_NH; i++) m->ph[i] = 0.f;
    for (int i = 6; i <= 10; i++) m->ph[i] = (float)RH_K[i] * turn * (1.f / 3.f);
    m->ph[11] = .5f + .25f * pval(s, P_RH_SWING);
    float top = 0.f; for (int i = 0; i < RH_NH; i++) top += w[i];
    m->top = top;
    /* the threshold: Density, the long form, a generator's motion */
    float d = 1.f - pval(s, P_RH_DENS), th = .03f + .6f * d * d;
    th += Fm * (-.043f * rhythm_slow(s, 4, .9f) - .026f * rhythm_slow(s, 16, .95f) + .03f * rhythm_slow(s, 64, 0.f) + .017f * rhythm_slow(s, 3, 0.f));
    th -= pval(s, P_RH_MOTION) * .07f * fclamp(2.2f * s->layer[0].g.pxs, -1.f, 1.f);
    m->th = fclamp(th, .01f, .95f) * top;
    /* the selector, and the tom's lean */
    float r = SELRATE_V[pint(s, P_RH_SELRATE)];
    float z = ((float)(m->bar % 2u) + m->tau) * r - Fm * .08f * rhythm_slow(s, 16, .25f);
    m->sel = fsin(z + .25f) + Fm * .35f * rhythm_slow(s, 2, 0.f);
    m->tomb = .25f * fsin(m->tau) + Fm * .15f * rhythm_slow(s, 7, 0.f);
}

/* Select: the drum this event plays */
DSP_CTL static void assign_select(Synth *s, float st, int delay) {
    Rhythm *m = &s->meter; float acc = pval(s, P_DR_ACCENT), fb = pval(s, P_RH_FEED);
    static const float POL[NUM_DRUMS] = {.5f, -.6f, 0.f, 0.f};
    float best = -1e9f; int bd = -1, hat_in = 0;
    for (int d = 0; d < NUM_DRUMS; d++) {
        float dens = pval(s, DP(d, P_D1_DENS));
        if (pint(s, DP(d, P_D1_SRC)) != DSRC_STREAM || dens < .01f) continue;
        if (d == 2) hat_in = dens >= .35f;
        float c = 1.f - pval(s, DP(d, P_D1_PLACE)), df = st - c; if (df < 0.f) df = -df;
        float sc = 1.f - 1.6f * df + POL[d] * m->sel + (d == 3 ? m->tomb : 0.f) + (dens - .5f) * .8f - fb * m->tired[d];
        if (d == 0) sc += 1.2f * sm01((st - .8f) / .15f);        /* the strongest events are the kick's: its anchor */
        if (sc > best) { best = sc; bd = d; }
    }
    if (bd < 0) return;
    drums_trigger(s, bd, -1, (1.f - acc) + acc * st, delay);
    if (bd != 2 && hat_in && st > .25f) drums_trigger(s, 2, -1, ((1.f - acc) + acc * st) * .6f, delay);
    for (int d = 0; d < NUM_DRUMS; d++) m->tired[d] *= .55f;
    if (bd != 2) m->tired[bd] += 1.f;
}

/* Count: by the events' order */
DSP_CTL static void assign_count(Synth *s, float st, int delay) {
    Rhythm *m = &s->meter; float acc = pval(s, P_DR_ACCENT), vel = (1.f - acc) + acc * st;
    int n = 2 + pint(s, P_RH_COUNT);
    if (st > .9f) m->count = 0;                              /* the strongest event: the count starts again */
    uint32_t c = m->count++;
    #define IN(d) (pint(s, DP(d, P_D1_SRC)) == DSRC_STREAM && pval(s, DP(d, P_D1_DENS)) >= .01f)
    if (c % (uint32_t)(2 * n) == (uint32_t)n) { if (IN(1)) drums_trigger(s, 1, -1, vel, delay); }
    else if (c % (uint32_t)n == 0) { if (IN(0)) drums_trigger(s, 0, -1, vel, delay); }
    else if (st > .3f && c % (uint32_t)n == (uint32_t)(n - 1)) { if (IN(3)) drums_trigger(s, 3, -1, vel, delay); }
    int hn = n / 4 < 1 ? 1 : n / 4;
    if (IN(2) && c % (uint32_t)hn == 0) drums_trigger(s, 2, -1, .4f + .6f * vel, delay);
    #undef IN
}

/* Lock: four coupled oscillators, one a drum */
DSP_CTL static void lock_tick(Synth *s, float dx, int run) {
    Rhythm *m = &s->meter;
    static const float DET[NUM_DRUMS] = {0.f, .035f, -.02f, .05f}, PH0[NUM_DRUMS] = {0.f, .5f, 0.f, .1f};
    float dens = pval(s, P_RH_DENS), fine = pval(s, P_RH_FINE), three = pval(s, P_RH_THREES), Fm = pval(s, P_RH_FORM);
    float hat = (three > .5f ? 6.f : 8.f) * (fine > .66f ? 2.f : fine < .33f ? .5f : 1.f) * (dens < .25f ? .5f : 1.f);
    float rate[NUM_DRUMS] = {2.f, 2.f, hat, dens < .33f ? 1.5f : 3.f};
    float K = pval(s, P_RH_PULL) * .4f * (1.f + Fm * .5f * rhythm_slow(s, 8, 0.f));
    K += pval(s, P_RH_MOTION) * .1f * fclamp(2.2f * s->layer[0].g.pxs, -1.f, 1.f);
    if (K < 0.f) K = 0.f;
    float acc = pval(s, P_DR_ACCENT), loose = .5f + pval(s, P_EVOLVE);
    for (int d = 0; d < NUM_DRUMS; d++) {
        float r = rate[d], z = r * (((float)(m->bar % 2u)) + m->tau) - PH0[d];
        float err = z - m->lk[d]; err -= floorf(err + .5f);
        float q = fsin(err + .25f);
        m->lkq[d] += .05f * (q - m->lkq[d]);
        float old = m->lk[d], nw = old + (r * (1.f + DET[d] * loose) + K * r * fsin(err)) * dx;
        if (nw < old) nw = old;
        if (nw >= 1.f) {
            nw -= 1.f;
            if (run && pint(s, DP(d, P_D1_SRC)) == DSRC_STREAM && pval(s, DP(d, P_D1_DENS)) >= .01f) {
                float fr = (1.f - old) / (nw + 1.f - old + 1e-9f);
                float st = fclamp(.55f + .4f * q, .25f, 1.f);
                drums_trigger(s, d, -1, (1.f - acc) + acc * st, (int32_t)(fr * (float)SUB));
                m->nev++; m->ev_s = st; m->ev_flash = 1.f;
                for (int j = 0; j < NUM_DRUMS; j++) if (j != d) m->lk[j] -= .06f * K * fsin(m->lk[j]) * (1.f / PI_F);
            }
        }
        m->lk[d] = nw;
    }
    for (int d = 0; d < NUM_DRUMS; d++) { if (m->lk[d] < 0.f) m->lk[d] += 1.f; if (m->lk[d] >= 1.f) m->lk[d] -= 1.f; }
}

DSP_CTL void rhythm_tick(Synth *s, float dt) {
    Rhythm *m = &s->meter;
    m->beats = 4.f;
    m->bar_sec = m->beats * 60.f / pval(s, P_TEMPO);
    float dx = 0.f;
    if (s->transport) {
        dx = dt / m->bar_sec;
        m->tau += dx;
        if (m->tau >= 1.f) { m->tau -= 1.f; m->bar++; s->rand_a = s->rand_b; s->rand_b = frand(&s->rng) * 2.f - 1.f; }
    }
    {   /* SRC_RAND: a new random value each bar, reached smoothly across the bar (no steps) */
        float f = m->tau * m->tau * (3.f - 2.f * m->tau);
        s->rand_hold = s->rand_a + (s->rand_b - s->rand_a) * f;
    }
    weights(s);
    float th1 = m->th;
    m->R2 = m->R1; m->R1 = m->R;
    m->R = rhythm_R(s, m->tau) * m->top;
    m->Rn = m->top > 1e-6f ? m->R / m->top : 0.f;
    m->ev_flash *= .95f;
    int on = pint(s, P_DR_ON) && s->transport;
    if (pint(s, P_RH_ENGINE) == RH_LOCK) { lock_tick(s, dx, on); return; }
    if (dx <= 0.f) return;
    /* an event: R peaked at the tick before (R1 above both neighbours) and over the threshold. A parabola through
       the three values puts the peak within the tick; the hit lands a fixed 1.5 ticks (96 samples) after it */
    if (m->R1 > m->R && m->R1 >= m->R2 && m->R1 > th1) {
        float den = m->R2 - 2.f * m->R1 + m->R, off = den < -1e-9f ? .5f * (m->R2 - m->R) / den : 0.f;
        float pk = m->R1 - .25f * (m->R2 - m->R) * off;
        float st = fclamp((pk - th1) / (m->top - th1 + 1e-6f), 0.f, 1.f);
        int delay = (int)((float)(SUB / 2) + (float)SUB * fclamp(off, -.5f, .5f));
        m->nev++; m->ev_s = st; m->ev_flash = 1.f;
        if (on) { if (pint(s, P_RH_ASSIGN) == RH_COUNT) assign_count(s, st, delay); else assign_select(s, st, delay); }
    }
}
