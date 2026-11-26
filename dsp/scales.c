/*
 * The scales. Every note has two tunings and a place:
 *   just     a whole-number ratio to the ding (the harmonic series and its simplest inversions)
 *   equal    the same note in 12-tone equal temperament (to play with other instruments)
 *   Temper   moves every note continuously from the one to the other (engine.c)
 *   lattice  its exponents of 3, 5 and 7: where it sits on the Tonnetz the generators move on
 *
 * The bank starts from the harmonic series itself and works outward:
 *   Harmonic     the series as it is (partials 1, 2, 3 ... of the ding, not folded into an octave: wide in the bass,
 *                close in the treble, and every chord fuses into one tone), its odd partials, and octave scales cut
 *                from it (4-8, 6-12, 8-16), the undertone mirror
 *   Tongue drum  the tunings steel tongue drums and handpans are built in: the 1-3-5-9 hexany, Golden Gate, Celtic
 *                minor (Amara), Kurd, Integral, Pygmy, Akebono, Sabye, Aegean, Hijaz
 *   Pentatonic   two to five notes: fifths, suspended, the pentatonics and three Japanese ones
 *   Modes        the seven-note modes, harmonic and melodic minor, the acoustic (overtone) scale, two with an
 *                augmented second
 *   Other        blues, the hexatonics, whole tone, diminished, chromatic
 *
 * A note's tonal weight is its harmonicity to the ding (Tenney: 1.15 - 0.1 log2(n d), from 1 for the ding down to
 * 0.45): the generators lean toward the notes that hold the key. Rotation makes another note the ding.
 */
#define MANIFOLD_DSP_UNIT
#include "synth.h"
#include "mstring.h"

/* the palette: a just ratio and its 12-tone step */
typedef struct { uint8_t num, den, step; } Rat;
static const Rat PAL[] = {
    {1, 1, 0}, {16, 15, 1}, {10, 9, 2}, {9, 8, 2}, {8, 7, 2}, {7, 6, 3}, {6, 5, 3}, {5, 4, 4}, {4, 3, 5}, {11, 8, 6},
    {7, 5, 6}, {45, 32, 6}, {3, 2, 7}, {8, 5, 8}, {13, 8, 8}, {5, 3, 9}, {27, 16, 9}, {7, 4, 10}, {16, 9, 10}, {9, 5, 10},
    {15, 8, 11}, {11, 6, 10}, {16, 13, 4}, {16, 11, 6}};

typedef struct { const char *name; uint8_t series, n, deg[MAX_DEGREES]; } Def;
static const Def SC[] = {
    /* harmonic */
    {"Harmonic series", 1, 0, {0}},
    {"Odd harmonics", 2, 0, {0}},
    {"Harmonics 4-8", 0, 4, {0, 7, 12, 17}},
    {"Harmonics 6-12", 0, 6, {0, 5, 8, 12, 15, 21}},
    {"Harmonics 8-16", 0, 8, {0, 3, 7, 9, 12, 14, 17, 20}},
    {"Harmonic pentatonic", 0, 5, {0, 3, 7, 12, 17}},
    {"Undertones 8-16", 0, 8, {0, 1, 4, 22, 8, 23, 13, 18}},
    /* tongue drum and handpan tunings (the names their makers use) */
    {"Hexany 1-3-5-9", 0, 6, {0, 2, 7, 8, 12, 15}},
    {"Golden Gate", 0, 6, {0, 3, 7, 11, 12, 20}},
    {"Celtic minor (Amara)", 0, 6, {0, 3, 6, 8, 12, 19}},
    {"Kurd", 0, 7, {0, 3, 6, 8, 12, 13, 19}},
    {"Integral", 0, 6, {0, 3, 6, 12, 13, 19}},
    {"Pygmy", 0, 5, {0, 3, 6, 12, 19}},
    {"Akebono", 0, 5, {0, 3, 6, 12, 13}},
    {"Sabye", 0, 6, {0, 3, 7, 12, 15, 20}},
    {"Aegean", 0, 5, {0, 7, 11, 12, 20}},
    {"Hijaz", 0, 7, {0, 1, 7, 8, 12, 13, 19}},
    /* pentatonic and smaller */
    {"Fifths", 0, 2, {0, 12}},
    {"Suspended", 0, 3, {0, 3, 12}},
    {"Major pentatonic", 0, 5, {0, 3, 7, 12, 15}},
    {"Minor pentatonic", 0, 5, {0, 6, 8, 12, 19}},
    {"Egyptian", 0, 5, {0, 3, 8, 12, 18}},
    {"Septimal pentatonic", 0, 5, {0, 5, 8, 12, 17}},
    {"In", 0, 5, {0, 1, 8, 12, 13}},
    {"Kumoi", 0, 5, {0, 3, 6, 12, 15}},
    /* modes */
    {"Major", 0, 7, {0, 3, 7, 8, 12, 15, 20}},
    {"Dorian", 0, 7, {0, 3, 6, 8, 12, 15, 18}},
    {"Phrygian", 0, 7, {0, 1, 6, 8, 12, 13, 19}},
    {"Lydian", 0, 7, {0, 3, 7, 11, 12, 15, 20}},
    {"Mixolydian", 0, 7, {0, 3, 7, 8, 12, 15, 18}},
    {"Minor", 0, 7, {0, 3, 6, 8, 12, 13, 19}},
    {"Harmonic minor", 0, 7, {0, 3, 6, 8, 12, 13, 20}},
    {"Melodic minor", 0, 7, {0, 3, 6, 8, 12, 15, 20}},
    {"Acoustic (overtone)", 0, 7, {0, 3, 7, 9, 12, 15, 17}},
    {"Phrygian dominant", 0, 7, {0, 1, 7, 8, 12, 13, 19}},
    {"Double harmonic", 0, 7, {0, 1, 7, 8, 12, 13, 20}},
    /* other */
    {"Blues", 0, 6, {0, 6, 8, 10, 12, 17}},
    {"Major hexatonic", 0, 6, {0, 3, 7, 8, 12, 15}},
    {"Whole tone", 0, 6, {0, 3, 7, 10, 13, 19}},
    {"Diminished", 0, 8, {0, 3, 6, 8, 10, 13, 15, 20}},
    {"Chromatic", 0, 12, {0, 1, 3, 6, 7, 8, 11, 12, 13, 15, 19, 20}},
};
#define SC_COUNT ((int)(sizeof SC / sizeof SC[0]))
static const int GROUP[6] = {0, 7, 17, 25, 36, SC_COUNT};   /* harmonic, tongue drum, pentatonic, modes, other */

int scale_count(void) { return SC_COUNT; }
int scale_find(const char *name) {   /* (1.0.8) a scale by its name, or -1: the genres' rolls name theirs */
    for (int i = 0; i < SC_COUNT; i++) { const char *a = SC[i].name, *b = name; while (*a && *a == *b) a++, b++; if (!*a && !*b) return i; }
    return -1;
}
int scale_group_start(int g) { return g < 0 ? 0 : g > 5 ? SC_COUNT : GROUP[g]; }

static int gcd_i(int a, int b) { while (b) { int t = a % b; a = b; b = t; } return a; }
static float weight_of(int num, int den) {
    float w = 1.15f - .1f * flog2((float)num * (float)den);
    return num == den ? 1.f : w < .45f ? .45f : w > .95f ? .95f : w;
}

/* a series scale's notes: the first n partials (all, or the odd ones) */
void scale_series_n(Scale *s, int n) {
    if (n > MAX_NODES) n = MAX_NODES;
    if (n < 2) n = 2;
    s->n = n;
    for (int i = 0; i < n; i++) {
        int p = s->series == 2 ? 2 * i + 1 : i + 1, o = p;
        while (!(o & 1)) o >>= 1;                        /* its odd part: the pitch class */
        float lr = flog2((float)p), st = floorf(12.f * lr + .5f);
        s->num[i] = p; s->den[i] = 1; s->ratio[i] = (float)p; s->et[i] = fexp2(st / 12.f);
        float w = 1.05f - .12f * flog2((float)o);
        s->wt[i] = w < .45f ? .45f : w > 1.f ? 1.f : w;
    }
}

void scale_get_rot(int index, int rot, Scale *s) {
    memset(s, 0, sizeof *s);
    if (index < 0) index = 0;
    if (index >= SC_COUNT) index = SC_COUNT - 1;
    const Def *d = &SC[index];
    s->period = 2.f; s->family = index; s->series = d->series;
    if (d->series) { s->rot = 0; scale_series_n(s, 16); snprintf(s->name, sizeof s->name, "%s", d->name); return; }
    int n = d->n;
    rot %= n; if (rot < 0) rot += n;
    /* rotate: every note over the new ding, octave-reduced; the stored order is ascending, so the rotation is too */
    const Rat *r0 = &PAL[d->deg[rot]];
    for (int i = 0; i < n; i++) {
        const Rat *r = &PAL[d->deg[(i + rot) % n]];
        int a = r->num * r0->den, b = r->den * r0->num;
        while (a >= 2 * b) b *= 2;
        while (a < b) a *= 2;
        int g = gcd_i(a, b);
        s->num[i] = a / g; s->den[i] = b / g;
        s->ratio[i] = (float)s->num[i] / (float)s->den[i];
        int st = (r->step - r0->step + 12) % 12;
        s->et[i] = fexp2((float)st / 12.f);
        s->wt[i] = weight_of(s->num[i], s->den[i]);
    }
    s->n = n; s->rot = rot;
    if (rot) snprintf(s->name, sizeof s->name, "%s /%d", d->name, rot + 1);
    else snprintf(s->name, sizeof s->name, "%s", d->name);
}
void scale_get(int index, Scale *s) { scale_get_rot(index, 0, s); }
