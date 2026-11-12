/* SPDX-License-Identifier: MPL-2.0
 * The few libc calls the engine makes. The device has no libc headers: felucca-bsp's libc.c gives memset, memcpy and
 * memcmp; the host gets them from its own libc. */
#ifndef MANIFOLD_MSTRING_H
#define MANIFOLD_MSTRING_H
#if defined(PLAT_HOST) || !defined(MANIFOLD_FW)
#include <string.h>
#else
typedef __SIZE_TYPE__ size_t;
void *memset(void *d, int c, unsigned n);
void *memcpy(void *d, const void *s, unsigned n);
int memcmp(const void *a, const void *b, unsigned n);
#endif
#endif
