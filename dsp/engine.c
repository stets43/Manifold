/*
 * The surface: the scale laid out like a tongue drum (the ding, then the scale's notes over one to three octaves,
 * then the ding an octave or more above), and where each node sits on a plane, torus or sphere. The generators use
 * the places (neighbours, the path's proximity); the voices use them for pan.
 */
#define MANIFOLD_DSP_UNIT
#include "synth.h"
#include "mstring.h"

DSP_CTL static float wrapd(float d) { d -= floorf(d + 0.5f); return d; }   /* -0.5..0.5 */

DSP_CTL float surf_dist(int surf, float x1, float y1, float x2, float y2) {
    if (surf == SURF_PLANE) { float dx = x1 - x2, dy = y1 - y2; return sqrtf(dx * dx + dy * dy); }
    if (surf == SURF_TORUS) { float dx = wrapd(x1 - x2), dy = wrapd(y1 - y2); return sqrtf(dx * dx + dy * dy); }
    float t1 = (y1 - .5f) * .5f, t2 = (y2 - .5f) * .5f;            /* sphere: chord distance / 2 */
    float c1 = fsin(t1 + .25f), c2 = fsin(t2 + .25f);
    float ax = c1 * fsin(x1 + .25f), ay = c1 * fsin(x1), az = fsin(t1);
    float bx = c2 * fsin(x2 + .25f), by = c2 * fsin(x2), bz = fsin(t2);
    float dx = ax - bx, dy = ay - by, dz = az - bz;
    return 0.5f * sqrtf(dx * dx + dy * dy + dz * dz);
}

/* a generator's projection ([-0.5, 0.5]^2) -> surface coordinates, through the path view (scale, rotation, place) */
DSP_CTL void surf_pt(int surf, float x, float y, SurfPt *p) {
    p->x = x; p->y = y; p->ax = p->ay = p->az = 0.f;
    if (surf == SURF_SPHERE) {
        float t = (y - .5f) * .5f, c = dsp_sin(t + .25f);
        p->ax = .5f * c * dsp_sin(x + .25f); p->ay = .5f * c * dsp_sin(x); p->az = .5f * dsp_sin(t);
    }
}

DSP_CTL void path_view(const Synth *s, float px, float py, float *x, float *y) {
    float sc = pval(s, P_PATH_SCALE), rot = pnorm(s, P_PATH_ROT);
    float c = fsin(rot + .25f), sn = fsin(rot);
    float rx = px * c - py * sn, ry = px * sn + py * c;
    /* the motion's +-0.45 fills the notes' frame at Scale 1; Path X and Y move it by up to a frame either way */
    const Engine *e = &s->eng; float k = e->fext / .45f * sc;
    float ox = (pval(s, P_PATH_X) - .5f) * 2.f * e->fext, oy = (pval(s, P_PATH_Y) - .5f) * 2.f * e->fext;
    float X = e->fcx + ox + rx * k, Y = e->fcy + oy + ry * k;
    /* every surface continuous: the plane reflects at its edges, the torus wraps, and on the sphere a path over a
       pole comes down the other side (the latitude folds, the longitude turns half way) */
    if (s->eng.surface == SURF_PLANE) {
        X -= 2.f * floorf(X * .5f); if (X > 1.f) X = 2.f - X;
        Y -= 2.f * floorf(Y * .5f); if (Y > 1.f) Y = 2.f - Y;
    } else if (s->eng.surface == SURF_TORUS) { X -= floorf(X); Y -= floorf(Y); }
    else {
        Y -= 2.f * floorf(Y * .5f); if (Y > 1.f) { Y = 2.f - Y; X += .5f; }
        X -= floorf(X);
    }
    *x = X; *y = Y;
}

/* a small step on the surface (dX, dY), taken back into the motion's coordinates (the inverse of the view's linear map) */
DSP_CTL void path_unview(const Synth *s, float dX, float dY, float *px, float *py) {
    float sc = pval(s, P_PATH_SCALE), rot = pnorm(s, P_PATH_ROT);
    float c = fsin(rot + .25f), sn = fsin(rot), k = s->eng.fext / .45f * sc;
    float rx = dX / k, ry = dY / k;
    *px = rx * c + ry * sn; *py = -rx * sn + ry * c;
}

/* the exponents of 3, 5, 7 in num/den (the powers of 2 are the octave), and where the note sits on the lattice:
   fifths along x, major thirds at 60 degrees, sevenths at -60 degrees; 11, 13 and the higher primes of the harmonic
   series each have a direction of their own, further out */
static void lattice_of(int num, int den, int8_t *e, float *lx, float *ly) {
    static const int PR[3] = {3, 5, 7};
    static const uint8_t HP[7] = {11, 13, 17, 19, 23, 29, 31};
    static const float HX[7] = {.9f, -1.3f, 1.9f, -1.9f, 1.6f, -.6f, .4f}, HY[7] = {-1.7f, 1.6f, 1.2f, -1.1f, -1.9f, 2.3f, 2.5f};
    float x = 0.f, y = 0.f;
    for (int k = 0; k < 3; k++) {
        int a = 0;
        while (num % PR[k] == 0) { num /= PR[k]; a++; }
        while (den % PR[k] == 0) { den /= PR[k]; a--; }
        e[k] = (int8_t)a;
    }
    x = (float)e[0] + .5f * (float)e[1] + .45f * (float)e[2];
    y = .866f * (float)e[1] - .78f * (float)e[2];
    for (int k = 0; k < 7; k++) {
        while (num % HP[k] == 0) { num /= HP[k]; x += HX[k]; y += HY[k]; }
        while (den % HP[k] == 0) { den /= HP[k]; x -= HX[k]; y -= HY[k]; }
    }
    *lx = x; *ly = y;
}

static void layout(Synth *s) {
    Engine *e = &s->eng; int N = e->n_nodes;
    e->spacing = 0.f;
    /* the Tonnetz: fifths along x, major thirds at 60 degrees, sevenths at -60 degrees; each octave of the drum a
       small step out from its pitch class (golden angle), so the register spreads without breaking the lattice */
    float px[MAX_NODES], py[MAX_NODES], mnx = 1e9f, mxx = -1e9f, mny = 1e9f, mxy = -1e9f;
    for (int i = 0; i < N; i++) {
        Node *nd = &e->node[i];
        float x = nd->lx, y = nd->ly;
        float a = (float)nd->oct * .381966f, r = (float)nd->oct * .22f;
        px[i] = x + r * fsin(a + .25f); py[i] = y + r * fsin(a);
        if (px[i] < mnx) mnx = px[i]; if (px[i] > mxx) mxx = px[i];
        if (py[i] < mny) mny = py[i]; if (py[i] > mxy) mxy = py[i];
    }
    float span = (mxx - mnx) > (mxy - mny) ? (mxx - mnx) : (mxy - mny); if (span < 1e-3f) span = 1.f;
    for (int i = 0; i < N; i++) {
        Node *nd = &e->node[i];
        switch (e->surface) {
        case SURF_PLANE:                 /* the lattice, centred, in [0.1, 0.9] */
            nd->x = .5f + (px[i] - .5f * (mnx + mxx)) / span * .8f;
            nd->y = .5f + (py[i] - .5f * (mny + mxy)) / span * .8f;
            break;
        case SURF_TORUS:                 /* the lattice wrapped: fifths around one way, thirds the other */
            nd->x = (px[i] - mnx) / (span * 1.15f) + .05f; nd->x -= floorf(nd->x);
            nd->y = (py[i] - mny) / (span * 1.15f) + .05f; nd->y -= floorf(nd->y);
            break;
        default: {                       /* Fibonacci sphere, low pitches south */
            float z = -1.f + (2.f * (float)i + 1.f) / (float)N;
            nd->x = (float)i * 0.381966f; nd->x -= floorf(nd->x);
            nd->y = asinf(z) / PI_F + .5f;
        }
        }
    }
    for (int i = 0; i < N; i++) {       /* each node's place in 3-D (halved), for surf_d2 on the sphere */
        Node *nd = &e->node[i]; float t = (nd->y - .5f) * .5f, c = fsin(t + .25f);
        nd->sx = .5f * c * fsin(nd->x + .25f); nd->sy = .5f * c * fsin(nd->x); nd->sz = .5f * fsin(t);
    }
    for (int i = 0; i < N; i++) {       /* the four nearest neighbours */
        float bd[4] = {9, 9, 9, 9}; int bi[4] = {i, i, i, i};
        for (int j = 0; j < N; j++) {
            if (j == i) continue;
            float d = DSP_FAR(surf_dist)(e->surface, e->node[i].x, e->node[i].y, e->node[j].x, e->node[j].y);
            for (int k = 0; k < 4; k++) if (d < bd[k]) {
                for (int m = 3; m > k; m--) { bd[m] = bd[m - 1]; bi[m] = bi[m - 1]; }
                bd[k] = d; bi[k] = j; break;
            }
        }
        for (int k = 0; k < 4; k++) e->node[i].nbr[k] = (uint8_t)bi[k];
        /* the spacing counts the nearest note of another pitch class (an octave twin sits close by on purpose) */
        float dn = 9.f;
        for (int j = 0; j < N; j++) {
            if (e->node[i].pc == e->node[j].pc) continue;
            float d = DSP_FAR(surf_dist)(e->surface, e->node[i].x, e->node[i].y, e->node[j].x, e->node[j].y); if (d < dn) dn = d;
        }
        e->spacing += dn < 9.f ? dn : .1f;
    }
    e->spacing /= (float)(N > 0 ? N : 1);
    if (e->spacing < .02f) e->spacing = .02f;
    /* the motions' frame: centred on the notes, as wide as they are (so a path at full scale crosses all of them,
       wherever the layout put them). On the sphere the notes cover it, so the frame is the whole surface */
    if (e->surface == SURF_SPHERE || N < 2) { e->fcx = .5f; e->fcy = .5f; e->fext = .5f; }
    else {
        float cx = 0.f, cy = 0.f, ux[MAX_NODES], uy[MAX_NODES];
        for (int i = 0; i < N; i++) {          /* unwrapped about node 0 (on the torus) */
            float dx = e->node[i].x - e->node[0].x, dy = e->node[i].y - e->node[0].y;
            if (e->surface == SURF_TORUS) { dx -= floorf(dx + .5f); dy -= floorf(dy + .5f); }
            ux[i] = e->node[0].x + dx; uy[i] = e->node[0].y + dy; cx += ux[i]; cy += uy[i];
        }
        cx /= (float)N; cy /= (float)N;
        float mx = 0.f;
        for (int i = 0; i < N; i++) { float d = sqrtf((ux[i] - cx) * (ux[i] - cx) + (uy[i] - cy) * (uy[i] - cy)); if (d > mx) mx = d; }
        e->fcx = cx - floorf(cx); e->fcy = e->surface == SURF_TORUS ? cy - floorf(cy) : cy;
        e->fext = fclamp(mx + .5f * e->spacing, 2.f * e->spacing, .5f);
    }
}

/* the nodes' pitches: Temper moves each from its just ratio (0) to its equal-tempered step (1) */
static void tune(Synth *s) {
    Engine *e = &s->eng; float t = pval(s, P_TEMPER);
    float root = 440.f * fm_exp2f((floorf(pval(s, P_ROOT) + .5f) - 69.f) / 12.f);
    for (int i = 0; i < e->n_nodes; i++) {
        Node *nd = &e->node[i];
        nd->lr = nd->lrj + t * (nd->lre - nd->lrj);
        nd->ratio = fm_exp2f(nd->lr);                  /* (the exact one: a tuning is not a place for 0.5 cent) */
        nd->freq = root * nd->ratio;
    }
    e->temper = t;
}

void engine_build(Synth *s) {
    Engine *e = &s->eng;
    for (int i = 0; i < NUM_PARAMS; i++)                    /* commit structural params */
        if (g_param_meta[i].structural) { s->base[i] = s->mod[i] = s->target[i]; DSP_FAR(param_cache)(s, i); }
    scale_get(pint(s, P_SCALE), &s->scale);
    scale_get_rot(pint(s, P_SCALE), pint(s, P_ROTATE) % (s->scale.n > 0 ? s->scale.n : 1), &s->scale);
    e->surface = pint(s, P_SURFACE);
    int octs = pint(s, P_OCTAVES) + 1, deg, N;
    float root = 440.f * fm_exp2f((floorf(pval(s, P_ROOT) + .5f) - 69.f) / 12.f);
    if (s->scale.series) {                                  /* the series: 4 partials an "octave" of the control, 16 at most
                                                               (past the 16th they are a cluster, not notes) */
        N = 4 * octs; if (N > 16) N = 16;
        while (N > 2 && root * (s->scale.series == 2 ? 2.f * (float)N - 1.f : (float)N) > .2f * SR) N--;
        scale_series_n(&s->scale, N); N = s->scale.n; deg = N;
    } else {
        deg = s->scale.n;
        while (octs > 1 && deg * octs + 1 > MAX_NODES) octs--;
        N = deg * octs + 1; if (N > MAX_NODES) N = MAX_NODES;
    }
    e->n_nodes = N;
    memset(e->node, 0, sizeof e->node);
    int npc = 0; int pcs[MAX_NODES];
    for (int i = 0; i < N; i++) {
        Node *nd = &e->node[i];
        int d = i % deg, o = s->scale.series ? 0 : i / deg;
        nd->lrj = flog2(s->scale.ratio[d]) + (float)o;
        nd->lre = flog2(s->scale.et[d]) + (float)o;
        if (s->scale.series) {                              /* exact: log2 of a whole number, and its nearest semitone */
            int p = s->scale.num[d], od = p, oc = 0;
            while (!(od & 1)) { od >>= 1; }
            for (int q = p; q > 1; q >>= 1) oc++;
            nd->lrj = fm_log2f((float)p); nd->lre = floorf(12.f * nd->lrj + .5f) / 12.f;
            nd->oct = (uint8_t)oc;
            int k = 0; while (k < npc && pcs[k] != od) k++;
            if (k == npc) pcs[npc++] = od;
            nd->pc = (uint8_t)k;
            nd->wt = s->scale.wt[d];
        } else {
            nd->oct = (uint8_t)o; nd->pc = (uint8_t)d;
            /* exact logs here too (the fast log is good to a few cents: not good enough for a tuning) */
            nd->lrj = fm_log2f(s->scale.ratio[d]) + (float)o;
            nd->lre = floorf(12.f * fm_log2f(s->scale.et[d]) + .5f) / 12.f + (float)o;
            /* the note's tonal weight (its place in the scale). (Up to 1.0.5 a bass voicing turned the low notes over
               the ding down to an eighth, the arranger's low interval limits: Clarity keeps the low octaves clear now,
               and that rule had left the bass octaves at 40 % of their share) */
            float w = s->scale.wt[d];
            nd->wt = .7f + .3f * w;
        }
        lattice_of(s->scale.num[d], s->scale.den[d], nd->lat, &nd->lx, &nd->ly);
    }
    e->root = 0;
    tune(s);
    layout(s);
    voices_build(s);
    DSP_FAR(engine_update)(s);
}

DSP_CTL void engine_update(Synth *s) {
    Engine *e = &s->eng;
    { float t = pval(s, P_TEMPER) - e->temper; if (t > .002f || t < -.002f) DSP_FAR(tune)(s); }   /* Temper moved: retune (no rebuild) */
    /* the path point on the surface (layer 1's projection), and each node's level for the screen */
    path_view(s, s->layer[0].g.pxs, s->layer[0].g.pys, &s->px, &s->py);
    int act = 0;
    for (int i = 0; i < e->n_nodes; i++) {
        float a = s->layer[0].g.a[i] > s->layer[1].g.a[i] ? s->layer[0].g.a[i] : s->layer[1].g.a[i];
        e->node[i].level = a; e->node[i].active = a > 2e-3f;
        act += e->node[i].active;
    }
    e->n_active = act;
}

/* the node nearest a pitch (a MIDI note): one log, then a compare per node */
int node_near(const Synth *s, float ratio) {
    float lr = flog2(ratio), bd = 1e9f; int bi = 0;
    for (int i = 0; i < s->eng.n_nodes; i++) {
        float d = s->eng.node[i].lr - lr; if (d < 0) d = -d;
        if (d < bd) { bd = d; bi = i; }
    }
    return bi;
}
