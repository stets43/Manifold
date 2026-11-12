/* SPDX-License-Identifier: MPL-2.0
 * From here to ctl_off.h, sine, 2^x and log2 are calls to one copy each in RAM (synth.c) instead of being inlined at
 * every use: the control tick's code has to fit the RAM window, and its speed is not in these. Only for functions
 * that are in RAM themselves (DSP_CTL): flash code cannot call them directly. */
#pragma push_macro("fsin")
#pragma push_macro("fexp2")
#pragma push_macro("flog2")
#undef fsin
#undef fexp2
#undef flog2
#define fsin dsp_sin
#define fexp2 dsp_exp2
#define flog2 dsp_log2
