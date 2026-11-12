/* SPDX-License-Identifier: MPL-2.0
 * A tiny snprintf (%s %d %u %%): the engine's scale, path and patch names, without a libc formatter. */
#include <stdarg.h>
#include <stdint.h>

int mf_snprintf(char *buf, unsigned len, const char *fmt, ...)
{
    va_list ap;
    unsigned n = 0;
    if (!len) return 0;
    va_start(ap, fmt);
#define PUT(c) do { if (n + 1 < len) buf[n] = (c); n++; } while (0)
    for (; *fmt; fmt++) {
        if (*fmt != '%') { PUT(*fmt); continue; }
        fmt++;
        if (*fmt == 's') {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "";
            while (*s) PUT(*s++);
        } else if (*fmt == 'd' || *fmt == 'u') {
            char t[12];
            int k = 0;
            uint32_t u;
            if (*fmt == 'd') {
                int32_t v = va_arg(ap, int);
                if (v < 0) { PUT('-'); u = (uint32_t)0 - (uint32_t)v; } else u = (uint32_t)v;
            } else u = va_arg(ap, unsigned);
            do t[k++] = (char)('0' + u % 10u); while ((u /= 10u) && k < 11);
            while (k) PUT(t[--k]);
        } else if (*fmt == '%') PUT('%');
        else if (!*fmt) break;
    }
#undef PUT
    va_end(ap);
    buf[n < len ? n : len - 1] = 0;
    return (int)n;
}
