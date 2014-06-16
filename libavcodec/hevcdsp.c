/*
 * HEVC video decoder
 *
 * Copyright (C) 2012 - 2013 Guillaume Martres
 * Copyright (C) 2013 - 2014 Pierre-Edouard Lepere
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "hevc.h"
#include "hevcdsp.h"

static const int8_t transform[32][32] = {
    { 64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,
      64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64,  64 },
    { 90,  90,  88,  85,  82,  78,  73,  67,  61,  54,  46,  38,  31,  22,  13,   4,
      -4, -13, -22, -31, -38, -46, -54, -61, -67, -73, -78, -82, -85, -88, -90, -90 },
    { 90,  87,  80,  70,  57,  43,  25,   9,  -9, -25, -43, -57, -70, -80, -87, -90,
     -90, -87, -80, -70, -57, -43, -25,  -9,   9,  25,  43,  57,  70,  80,  87,  90 },
    { 90,  82,  67,  46,  22,  -4, -31, -54, -73, -85, -90, -88, -78, -61, -38, -13,
      13,  38,  61,  78,  88,  90,  85,  73,  54,  31,   4, -22, -46, -67, -82, -90 },
    { 89,  75,  50,  18, -18, -50, -75, -89, -89, -75, -50, -18,  18,  50,  75,  89,
      89,  75,  50,  18, -18, -50, -75, -89, -89, -75, -50, -18,  18,  50,  75,  89 },
    { 88,  67,  31, -13, -54, -82, -90, -78, -46, -4,   38,  73,  90,  85,  61,  22,
     -22, -61, -85, -90, -73, -38,   4,  46,  78,  90,  82,  54,  13, -31, -67, -88 },
    { 87,  57,   9, -43, -80, -90, -70, -25,  25,  70,  90,  80,  43,  -9, -57, -87,
     -87, -57,  -9,  43,  80,  90,  70,  25, -25, -70, -90, -80, -43,   9,  57,  87 },
    { 85,  46, -13, -67, -90, -73, -22,  38,  82,  88,  54,  -4, -61, -90, -78, -31,
      31,  78,  90,  61,   4, -54, -88, -82, -38,  22,  73,  90,  67,  13, -46, -85 },
    { 83,  36, -36, -83, -83, -36,  36,  83,  83,  36, -36, -83, -83, -36,  36,  83,
      83,  36, -36, -83, -83, -36,  36,  83,  83,  36, -36, -83, -83, -36,  36,  83 },
    { 82,  22, -54, -90, -61,  13,  78,  85,  31, -46, -90, -67,   4,  73,  88,  38,
     -38, -88, -73,  -4,  67,  90,  46, -31, -85, -78, -13,  61,  90,  54, -22, -82 },
    { 80,   9, -70, -87, -25,  57,  90,  43, -43, -90, -57,  25,  87,  70,  -9, -80,
     -80,  -9,  70,  87,  25, -57, -90, -43,  43,  90,  57, -25, -87, -70,   9,  80 },
    { 78,  -4, -82, -73,  13,  85,  67, -22, -88, -61,  31,  90,  54, -38, -90, -46,
      46,  90,  38, -54, -90, -31,  61,  88,  22, -67, -85, -13,  73,  82,   4, -78 },
    { 75, -18, -89, -50,  50,  89,  18, -75, -75,  18,  89,  50, -50, -89, -18,  75,
      75, -18, -89, -50,  50,  89,  18, -75, -75,  18,  89,  50, -50, -89, -18,  75 },
    { 73, -31, -90, -22,  78,  67, -38, -90, -13,  82,  61, -46, -88,  -4,  85,  54,
     -54, -85,   4,  88,  46, -61, -82,  13,  90,  38, -67, -78,  22,  90,  31, -73 },
    { 70, -43, -87,   9,  90,  25, -80, -57,  57,  80, -25, -90,  -9,  87,  43, -70,
     -70,  43,  87,  -9, -90, -25,  80,  57, -57, -80,  25,  90,   9, -87, -43,  70 },
    { 67, -54, -78,  38,  85, -22, -90,   4,  90,  13, -88, -31,  82,  46, -73, -61,
      61,  73, -46, -82,  31,  88, -13, -90,  -4,  90,  22, -85, -38,  78,  54, -67 },
    { 64, -64, -64,  64,  64, -64, -64,  64,  64, -64, -64,  64,  64, -64, -64,  64,
      64, -64, -64,  64,  64, -64, -64,  64,  64, -64, -64,  64,  64, -64, -64,  64 },
    { 61, -73, -46,  82,  31, -88, -13,  90,  -4, -90,  22,  85, -38, -78,  54,  67,
     -67, -54,  78,  38, -85, -22,  90,   4, -90,  13,  88, -31, -82,  46,  73, -61 },
    { 57, -80, -25,  90,  -9, -87,  43,  70, -70, -43,  87,   9, -90,  25,  80, -57,
     -57,  80,  25, -90,   9,  87, -43, -70,  70,  43, -87,  -9,  90, -25, -80,  57 },
    { 54, -85,  -4,  88, -46, -61,  82,  13, -90,  38,  67, -78, -22,  90, -31, -73,
      73,  31, -90,  22,  78, -67, -38,  90, -13, -82,  61,  46, -88,   4,  85, -54 },
    { 50, -89,  18,  75, -75, -18,  89, -50, -50,  89, -18, -75,  75,  18, -89,  50,
      50, -89,  18,  75, -75, -18,  89, -50, -50,  89, -18, -75,  75,  18, -89,  50 },
    { 46, -90,  38,  54, -90,  31,  61, -88,  22,  67, -85,  13,  73, -82,   4,  78,
     -78,  -4,  82, -73, -13,  85, -67, -22,  88, -61, -31,  90, -54, -38,  90, -46 },
    { 43, -90,  57,  25, -87,  70,   9, -80,  80,  -9, -70,  87, -25, -57,  90, -43,
     -43,  90, -57, -25,  87, -70,  -9,  80, -80,   9,  70, -87,  25,  57, -90,  43 },
    { 38, -88,  73,  -4, -67,  90, -46, -31,  85, -78,  13,  61, -90,  54,  22, -82,
      82, -22, -54,  90, -61, -13,  78, -85,  31,  46, -90,  67,   4, -73,  88, -38 },
    { 36, -83,  83, -36, -36,  83, -83,  36,  36, -83,  83, -36, -36,  83, -83,  36,
      36, -83,  83, -36, -36,  83, -83,  36,  36, -83,  83, -36, -36,  83, -83,  36 },
    { 31, -78,  90, -61,   4,  54, -88,  82, -38, -22,  73, -90,  67, -13, -46,  85,
     -85,  46,  13, -67,  90, -73,  22,  38, -82,  88, -54,  -4,  61, -90,  78, -31 },
    { 25, -70,  90, -80,  43,   9, -57,  87, -87,  57,  -9, -43,  80, -90,  70, -25,
     -25,  70, -90,  80, -43,  -9,  57, -87,  87, -57,   9,  43, -80,  90, -70,  25 },
    { 22, -61,  85, -90,  73, -38,  -4,  46, -78,  90, -82,  54, -13, -31,  67, -88,
      88, -67,  31,  13, -54,  82, -90,  78, -46,   4,  38, -73,  90, -85,  61, -22 },
    { 18, -50,  75, -89,  89, -75,  50, -18, -18,  50, -75,  89, -89,  75, -50,  18,
      18, -50,  75, -89,  89, -75,  50, -18, -18,  50, -75,  89, -89,  75, -50,  18 },
    { 13, -38,  61, -78,  88, -90,  85, -73,  54, -31,   4,  22, -46,  67, -82,  90,
     -90,  82, -67,  46, -22,  -4,  31, -54,  73, -85,  90, -88,  78, -61,  38, -13 },
    {  9, -25,  43, -57,  70, -80,  87, -90,  90, -87,  80, -70,  57, -43,  25, -9,
      -9,  25, -43,  57, -70,  80, -87,  90, -90,  87, -80,  70, -57,  43, -25,   9 },
    {  4, -13,  22, -31,  38, -46,  54, -61,  67, -73,  78, -82,  85, -88,  90, -90,
      90, -90,  88, -85,  82, -78,  73, -67,  61, -54,  46, -38,  31, -22,  13,  -4 },
};

#ifdef SVC_EXTENSION
/*      Upsampling filters      */
DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_chroma[16][4])=
{
    {  0,  64,   0,  0},
    { -2,  62,   4,  0},
    { -2,  58,  10, -2},
    { -4,  56,  14, -2},
    { -4,  54,  16, -2},
    { -6,  52,  20, -2},
    { -6,  46,  28, -4},
    { -4,  42,  30, -4},
    { -4,  36,  36, -4},
    { -4,  30,  42, -4},
    { -4,  28,  46, -6},
    { -2,  20,  52, -6},
    { -2,  16,  54, -4},
    { -2,  14,  56, -4},
    { -2,  10,  58, -2},
    {  0,   4,  62, -2}
};

DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_luma[16][8] )=
{
    {  0,  0,   0,  64,   0,   0,  0,  0},
    {  0,  1,  -3,  63,   4,  -2,  1,  0},
    { -1,  2,  -5,  62,   8,  -3,  1,  0},
    { -1,  3,  -8,  60,  13,  -4,  1,  0},
    { -1,  4, -10,  58,  17,  -5,  1,  0},
    { -1,  4, -11,  52,  26,  -8,  3, -1},
    { -1,  3,  -9,  47,  31, -10,  4, -1},
    { -1,  4, -11,  45,  34, -10,  4, -1},
    { -1,  4, -11,  40,  40, -11,  4, -1},
    { -1,  4, -10,  34,  45, -11,  4, -1},
    { -1,  4, -10,  31,  47,  -9,  3, -1},
    { -1,  3,  -8,  26,  52, -11,  4, -1},
    {  0,  1,  -5,  17,  58, -10,  4, -1},
    {  0,  1,  -4,  13,  60,  -8,  3, -1},
    {  0,  1,  -3,   8,  62,  -5,  2, -1},
    {  0,  1,  -2,   4,  63,  -3,  1,  0}
};

DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_luma_x2[2][8] )= /*0 , 8 */
{
    {  0,  0,   0,  64,   0,   0,  0,  0},
    { -1,  4, -11,  40,  40, -11,  4, -1}
};

DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_luma_x1_5[3][8] )= /* 0, 11, 5 */
{
    {  0,  0,   0,  64,   0,   0,  0,  0},
    { -1,  3,  -8,  26,  52, -11,  4, -1},
    { -1,  4, -11,  52,  26,  -8,  3, -1}
};

DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_chroma_x1_5[3][4])= /* 0, 11, 5 */
{
    {  0,  64,   0,  0},
    { -2,  20,  52, -6},
    { -6,  52,  20, -2}
};
DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_x1_5chroma[3][4])=
{
    {  0,   4,  62, -2},
    { -4,  30,  42, -4},
    { -4,  54,  16, -2}
};

DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_chroma_x2[2][4])=
{
    {  0,  64,   0,  0},
    { -4,  36,  36, -4}
};

DECLARE_ALIGNED(16, static const int8_t, up_sample_filter_chroma_x2_v[2][4])=
{
    { -2,  10,  58, -2},
    { -6,  46,  28, -4},
};

#endif

DECLARE_ALIGNED(16, const int8_t, ff_hevc_epel_filters[7][4]) = {
    { -2, 58, 10, -2},
    { -4, 54, 16, -2},
    { -6, 46, 28, -4},
    { -4, 36, 36, -4},
    { -4, 28, 46, -6},
    { -2, 16, 54, -4},
    { -2, 10, 58, -2},
};

DECLARE_ALIGNED(16, const int8_t, ff_hevc_qpel_filters[3][16]) = {
    { -1,  4,-10, 58, 17, -5,  1,  0, -1,  4,-10, 58, 17, -5,  1,  0},
    { -1,  4,-11, 40, 40,-11,  4, -1, -1,  4,-11, 40, 40,-11,  4, -1},
    {  0,  1, -5, 17, 58,-10,  4, -1,  0,  1, -5, 17, 58,-10,  4, -1}
};

#define BIT_DEPTH 8
#include "hevcdsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 9
#include "hevcdsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 10
#include "hevcdsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 12
#include "hevcdsp_template.c"
#undef BIT_DEPTH

#define BIT_DEPTH 14
#include "hevcdsp_template.c"
#undef BIT_DEPTH

void ff_hevc_dsp_init(HEVCDSPContext *hevcdsp, int bit_depth)
{
#undef FUNC
#define FUNC(a, depth) a ## _ ## depth

#undef PEL_FUNC
#define PEL_FUNC(dst1, idx1, idx2, a, depth)                                   \
    for(i = 0 ; i < 10 ; i++)                                                  \
{                                                                              \
    hevcdsp->dst1[i][idx1][idx2] = a ## _ ## depth;                            \
}

#undef EPEL_FUNCS
#define EPEL_FUNCS(depth)                                                     \
    PEL_FUNC(put_hevc_epel, 0, 0, put_hevc_pel_pixels, depth);                \
    PEL_FUNC(put_hevc_epel, 0, 1, put_hevc_epel_h, depth);                    \
    PEL_FUNC(put_hevc_epel, 1, 0, put_hevc_epel_v, depth);                    \
    PEL_FUNC(put_hevc_epel, 1, 1, put_hevc_epel_hv, depth)

#undef EPEL_UNI_FUNCS
#define EPEL_UNI_FUNCS(depth)                                                 \
    PEL_FUNC(put_hevc_epel_uni, 0, 0, put_hevc_pel_uni_pixels, depth);        \
    PEL_FUNC(put_hevc_epel_uni, 0, 1, put_hevc_epel_uni_h, depth);            \
    PEL_FUNC(put_hevc_epel_uni, 1, 0, put_hevc_epel_uni_v, depth);            \
    PEL_FUNC(put_hevc_epel_uni, 1, 1, put_hevc_epel_uni_hv, depth);           \
    PEL_FUNC(put_hevc_epel_uni_w, 0, 0, put_hevc_pel_uni_w_pixels, depth);    \
    PEL_FUNC(put_hevc_epel_uni_w, 0, 1, put_hevc_epel_uni_w_h, depth);        \
    PEL_FUNC(put_hevc_epel_uni_w, 1, 0, put_hevc_epel_uni_w_v, depth);        \
    PEL_FUNC(put_hevc_epel_uni_w, 1, 1, put_hevc_epel_uni_w_hv, depth)

#undef EPEL_BI_FUNCS
#define EPEL_BI_FUNCS(depth)                                                \
    PEL_FUNC(put_hevc_epel_bi, 0, 0, put_hevc_pel_bi_pixels, depth);        \
    PEL_FUNC(put_hevc_epel_bi, 0, 1, put_hevc_epel_bi_h, depth);            \
    PEL_FUNC(put_hevc_epel_bi, 1, 0, put_hevc_epel_bi_v, depth);            \
    PEL_FUNC(put_hevc_epel_bi, 1, 1, put_hevc_epel_bi_hv, depth);           \
    PEL_FUNC(put_hevc_epel_bi_w, 0, 0, put_hevc_pel_bi_w_pixels, depth);    \
    PEL_FUNC(put_hevc_epel_bi_w, 0, 1, put_hevc_epel_bi_w_h, depth);        \
    PEL_FUNC(put_hevc_epel_bi_w, 1, 0, put_hevc_epel_bi_w_v, depth);        \
    PEL_FUNC(put_hevc_epel_bi_w, 1, 1, put_hevc_epel_bi_w_hv, depth)

#undef QPEL_FUNCS
#define QPEL_FUNCS(depth)                                                     \
    PEL_FUNC(put_hevc_qpel, 0, 0, put_hevc_pel_pixels, depth);                \
    PEL_FUNC(put_hevc_qpel, 0, 1, put_hevc_qpel_h, depth);                    \
    PEL_FUNC(put_hevc_qpel, 1, 0, put_hevc_qpel_v, depth);                    \
    PEL_FUNC(put_hevc_qpel, 1, 1, put_hevc_qpel_hv, depth)

#undef QPEL_UNI_FUNCS
#define QPEL_UNI_FUNCS(depth)                                                 \
    PEL_FUNC(put_hevc_qpel_uni, 0, 0, put_hevc_pel_uni_pixels, depth);        \
    PEL_FUNC(put_hevc_qpel_uni, 0, 1, put_hevc_qpel_uni_h, depth);            \
    PEL_FUNC(put_hevc_qpel_uni, 1, 0, put_hevc_qpel_uni_v, depth);            \
    PEL_FUNC(put_hevc_qpel_uni, 1, 1, put_hevc_qpel_uni_hv, depth);           \
    PEL_FUNC(put_hevc_qpel_uni_w, 0, 0, put_hevc_pel_uni_w_pixels, depth);    \
    PEL_FUNC(put_hevc_qpel_uni_w, 0, 1, put_hevc_qpel_uni_w_h, depth);        \
    PEL_FUNC(put_hevc_qpel_uni_w, 1, 0, put_hevc_qpel_uni_w_v, depth);        \
    PEL_FUNC(put_hevc_qpel_uni_w, 1, 1, put_hevc_qpel_uni_w_hv, depth)

#undef QPEL_BI_FUNCS
#define QPEL_BI_FUNCS(depth)                                                  \
    PEL_FUNC(put_hevc_qpel_bi, 0, 0, put_hevc_pel_bi_pixels, depth);          \
    PEL_FUNC(put_hevc_qpel_bi, 0, 1, put_hevc_qpel_bi_h, depth);              \
    PEL_FUNC(put_hevc_qpel_bi, 1, 0, put_hevc_qpel_bi_v, depth);              \
    PEL_FUNC(put_hevc_qpel_bi, 1, 1, put_hevc_qpel_bi_hv, depth);             \
    PEL_FUNC(put_hevc_qpel_bi_w, 0, 0, put_hevc_pel_bi_w_pixels, depth);      \
    PEL_FUNC(put_hevc_qpel_bi_w, 0, 1, put_hevc_qpel_bi_w_h, depth);          \
    PEL_FUNC(put_hevc_qpel_bi_w, 1, 0, put_hevc_qpel_bi_w_v, depth);          \
    PEL_FUNC(put_hevc_qpel_bi_w, 1, 1, put_hevc_qpel_bi_w_hv, depth)

#define HEVC_DSP(depth)                                                        \
    hevcdsp->put_pcm                = FUNC(put_pcm, depth);                    \
    hevcdsp->transquant_bypass[0]   = FUNC(transquant_bypass4x4, depth);       \
    hevcdsp->transquant_bypass[1]   = FUNC(transquant_bypass8x8, depth);       \
    hevcdsp->transquant_bypass[2]   = FUNC(transquant_bypass16x16, depth);     \
    hevcdsp->transquant_bypass[3]   = FUNC(transquant_bypass32x32, depth);     \
    hevcdsp->transform_skip[0]      = FUNC(transform_skip, depth);             \
    hevcdsp->transform_skip[1]      = FUNC(transform_skip_rot, depth);         \
    hevcdsp->transform_4x4_luma_add = FUNC(transform_4x4_luma_add, depth);     \
    hevcdsp->transform_add[0]       = FUNC(transform_4x4_add, depth);          \
    hevcdsp->transform_add[1]       = FUNC(transform_8x8_add, depth);          \
    hevcdsp->transform_add[2]       = FUNC(transform_16x16_add, depth);        \
    hevcdsp->transform_add[3]       = FUNC(transform_32x32_add, depth);        \
                                                                               \
    hevcdsp->transform_dc_add[0]    = FUNC(transform_4x4_dc_add, depth);       \
    hevcdsp->transform_dc_add[1]    = FUNC(transform_8x8_dc_add, depth);       \
    hevcdsp->transform_dc_add[2]    = FUNC(transform_16x16_dc_add, depth);     \
    hevcdsp->transform_dc_add[3]    = FUNC(transform_32x32_dc_add, depth);     \
    hevcdsp->sao_band_filter    = FUNC(sao_band_filter_0, depth);              \
    hevcdsp->sao_edge_filter[0] = FUNC(sao_edge_filter_0, depth);              \
    hevcdsp->sao_edge_filter[1] = FUNC(sao_edge_filter_1, depth);              \
                                                                               \
    QPEL_FUNCS(depth);                                                         \
    QPEL_UNI_FUNCS(depth);                                                     \
    QPEL_BI_FUNCS(depth);                                                      \
    EPEL_FUNCS(depth);                                                         \
    EPEL_UNI_FUNCS(depth);                                                     \
    EPEL_BI_FUNCS(depth);                                                      \
                                                                               \
    hevcdsp->hevc_h_loop_filter_luma     = FUNC(hevc_h_loop_filter_luma, depth);   \
    hevcdsp->hevc_v_loop_filter_luma     = FUNC(hevc_v_loop_filter_luma, depth);   \
    hevcdsp->hevc_h_loop_filter_chroma   = FUNC(hevc_h_loop_filter_chroma, depth); \
    hevcdsp->hevc_v_loop_filter_chroma   = FUNC(hevc_v_loop_filter_chroma, depth); \
    hevcdsp->hevc_h_loop_filter_luma_c   = FUNC(hevc_h_loop_filter_luma, depth);   \
    hevcdsp->hevc_v_loop_filter_luma_c   = FUNC(hevc_v_loop_filter_luma, depth);   \
    hevcdsp->hevc_h_loop_filter_chroma_c = FUNC(hevc_h_loop_filter_chroma, depth); \
    hevcdsp->hevc_v_loop_filter_chroma_c = FUNC(hevc_v_loop_filter_chroma, depth)
int i = 0;

    switch (bit_depth) {
    case 9:
        HEVC_DSP(9);
        break;
    case 10:
        HEVC_DSP(10);
        break;
    case 12:
        HEVC_DSP(12);
        break;
    case 14:
        HEVC_DSP(14);
        break;
    default:
        HEVC_DSP(8);
        break;
    }

#ifdef SVC_EXTENSION
#define HEVC_DSP_UP(depth)                                                 \
    hevcdsp->upsample_base_layer_frame       = FUNC(upsample_base_layer_frame, depth); \
    hevcdsp->upsample_filter_block_luma_h[0] = FUNC(upsample_filter_block_luma_h_all, depth); \
    hevcdsp->upsample_filter_block_luma_h[1] = FUNC(upsample_filter_block_luma_h_x2, depth); \
    hevcdsp->upsample_filter_block_luma_h[2] = FUNC(upsample_filter_block_luma_h_x1_5, depth); \
    hevcdsp->upsample_filter_block_luma_v[0] = FUNC(upsample_filter_block_luma_v_all, depth); \
    hevcdsp->upsample_filter_block_luma_v[1] = FUNC(upsample_filter_block_luma_v_x2, depth); \
    hevcdsp->upsample_filter_block_luma_v[2] = FUNC(upsample_filter_block_luma_v_x1_5, depth); \
    hevcdsp->upsample_filter_block_cr_h[0]   = FUNC(upsample_filter_block_cr_h_all, depth); \
    hevcdsp->upsample_filter_block_cr_h[1]   = FUNC(upsample_filter_block_cr_h_x2, depth); \
    hevcdsp->upsample_filter_block_cr_h[2]   = FUNC(upsample_filter_block_cr_h_x1_5, depth); \
    hevcdsp->upsample_filter_block_cr_v[0]   = FUNC(upsample_filter_block_cr_v_all, depth); \
    hevcdsp->upsample_filter_block_cr_v[1]   = FUNC(upsample_filter_block_cr_v_x2, depth); \
    hevcdsp->upsample_filter_block_cr_v[2]   = FUNC(upsample_filter_block_cr_v_x1_5, depth); \
    
    switch (bit_depth) {
    case 9:
        HEVC_DSP_UP(9);
        break;
    case 10:
        HEVC_DSP_UP(10);
        break;
    case 12:
        HEVC_DSP_UP(12);
        break;
    case 14:
        HEVC_DSP_UP(14);
        break;
    default:
        HEVC_DSP_UP(8);
        break;
    }
#endif
    if (ARCH_X86) ff_hevcdsp_init_x86(hevcdsp, bit_depth);
    if (ARCH_ARM) ff_hevcdsp_init_arm(hevcdsp, bit_depth);
}
