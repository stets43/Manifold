/* SPDX-License-Identifier: MPL-2.0
 * The DSP's whole maths library: the FM-1 links no libm (fm1-base include/fastmath.h), so every libm name the
 * engine uses is mapped here onto fastmath's float-only, branch-light versions. The desktop harness and the host
 * simulator build the same way, so what they render is what the device renders. */
#ifndef MANIFOLD_FMATH_H
#define MANIFOLD_FMATH_H
/* in the engine's device units every maths helper is inlined where it is used: the tick and the render run from RAM,
   and a helper left as a function of its own would sit in the flash, out of a direct call's reach */
#if defined(MANIFOLD_FW) && !defined(PLAT_HOST) && !defined(MANIFOLD_FLASH_ONLY)
#define inline inline __attribute__((always_inline))
#include "fastmath.h"
#undef inline
#define MF_INL static inline __attribute__((always_inline))
#else
#include "fastmath.h"
#define MF_INL static inline
#endif

MF_INL float mf_fmodf(float a, float b)        /* C fmodf: truncates toward zero; |a/b| < 2^31 */
{
    float q = a / b;
    return a - b * (float)(int32_t)q;
}
MF_INL float mf_asinf(float x)                  /* Abramowitz & Stegun 4.4.45, abs err 7e-5 (layout only) */
{
    float a = fm_fabsf(x), r;
    if (a > 1.0f) a = 1.0f;
    r = 1.5707963f - fm_sqrtf(1.0f - a) * (1.5707288f + a * (-0.2121144f + a * (0.0742610f - 0.0187293f * a)));
    return x < 0.0f ? -r : r;
}

/* the names remapped only inside the engine's own units (each defines MANIFOLD_DSP_UNIT before synth.h): the
 * app, the platform and the harness keep their libc */
#ifdef MANIFOLD_DSP_UNIT
#define sqrtf  fm_sqrtf
#define sinf   fm_sinf
#define cosf   fm_cosf
#define powf   fm_powf
#define logf   fm_logf
#define fabsf  fm_fabsf
#define floorf fm_floorf
#define fmodf  mf_fmodf
#define asinf  mf_asinf

/* no snprintf on the device either: %s %d %u %% only, which is all the engine's names use */
int mf_snprintf(char *buf, unsigned len, const char *fmt, ...);
#define snprintf(b, l, ...) mf_snprintf((b), (unsigned)(l), __VA_ARGS__)
#endif
#endif
