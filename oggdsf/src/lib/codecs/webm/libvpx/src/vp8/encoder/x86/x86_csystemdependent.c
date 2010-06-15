/*
 *  Copyright (c) 2010 The VP8 project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license and patent
 *  grant that can be found in the LICENSE file in the root of the source
 *  tree. All contributing project authors may be found in the AUTHORS
 *  file in the root of the source tree.
 */


#include "vpx_ports/config.h"
#include "vpx_ports/x86.h"
#include "variance.h"
#include "onyx_int.h"


#if HAVE_MMX
void vp8_short_fdct8x4_mmx(short *input, short *output, int pitch)
{
    vp8_short_fdct4x4_mmx(input,   output,    pitch);
    vp8_short_fdct4x4_mmx(input + 4, output + 16, pitch);
}

void vp8_fast_fdct8x4_mmx(short *input, short *output, int pitch)
{
    vp8_fast_fdct4x4_mmx(input,   output   , pitch);
    vp8_fast_fdct4x4_mmx(input + 4, output + 16, pitch);
}

int vp8_fast_quantize_b_impl_mmx(short *coeff_ptr, short *zbin_ptr,
                                 short *qcoeff_ptr, short *dequant_ptr,
                                 short *scan_mask, short *round_ptr,
                                 short *quant_ptr, short *dqcoeff_ptr);
void vp8_fast_quantize_b_mmx(BLOCK *b, BLOCKD *d)
{
    short *scan_mask    = vp8_default_zig_zag_mask;//d->scan_order_mask_ptr;
    short *coeff_ptr  = &b->coeff[0];
    short *zbin_ptr   = &b->zbin[0][0];
    short *round_ptr  = &b->round[0][0];
    short *quant_ptr  = &b->quant[0][0];
    short *qcoeff_ptr = d->qcoeff;
    short *dqcoeff_ptr = d->dqcoeff;
    short *dequant_ptr = &d->dequant[0][0];

    d->eob = vp8_fast_quantize_b_impl_mmx(
                 coeff_ptr,
                 zbin_ptr,
                 qcoeff_ptr,
                 dequant_ptr,
                 scan_mask,

                 round_ptr,
                 quant_ptr,
                 dqcoeff_ptr
             );
}

int vp8_mbblock_error_mmx_impl(short *coeff_ptr, short *dcoef_ptr, int dc);
int vp8_mbblock_error_mmx(MACROBLOCK *mb, int dc)
{
    short *coeff_ptr =  mb->block[0].coeff;
    short *dcoef_ptr =  mb->e_mbd.block[0].dqcoeff;
    return vp8_mbblock_error_mmx_impl(coeff_ptr, dcoef_ptr, dc);
}

int vp8_mbuverror_mmx_impl(short *s_ptr, short *d_ptr);
int vp8_mbuverror_mmx(MACROBLOCK *mb)
{
    short *s_ptr = &mb->coeff[256];
    short *d_ptr = &mb->e_mbd.dqcoeff[256];
    return vp8_mbuverror_mmx_impl(s_ptr, d_ptr);
}

void vp8_subtract_b_mmx_impl(unsigned char *z,  int src_stride,
                             short *diff, unsigned char *predictor,
                             int pitch);
void vp8_subtract_b_mmx(BLOCK *be, BLOCKD *bd, int pitch)
{
    unsigned char *z = *(be->base_src) + be->src;
    unsigned int  src_stride = be->src_stride;
    short *diff = &be->src_diff[0];
    unsigned char *predictor = &bd->predictor[0];
    vp8_subtract_b_mmx_impl(z, src_stride, diff, predictor, pitch);
}

#endif

#if HAVE_SSE2
void vp8_short_fdct8x4_wmt(short *input, short *output, int pitch)
{
    vp8_short_fdct4x4_wmt(input,   output,    pitch);
    vp8_short_fdct4x4_wmt(input + 4, output + 16, pitch);
}

int vp8_fast_quantize_b_impl_sse(short *coeff_ptr, short *zbin_ptr,
                                 short *qcoeff_ptr, short *dequant_ptr,
                                 short *scan_mask, short *round_ptr,
                                 short *quant_ptr, short *dqcoeff_ptr);
void vp8_fast_quantize_b_sse(BLOCK *b, BLOCKD *d)
{
    short *scan_mask    = vp8_default_zig_zag_mask;//d->scan_order_mask_ptr;
    short *coeff_ptr  = &b->coeff[0];
    short *zbin_ptr   = &b->zbin[0][0];
    short *round_ptr  = &b->round[0][0];
    short *quant_ptr  = &b->quant[0][0];
    short *qcoeff_ptr = d->qcoeff;
    short *dqcoeff_ptr = d->dqcoeff;
    short *dequant_ptr = &d->dequant[0][0];

    d->eob = vp8_fast_quantize_b_impl_sse(
                 coeff_ptr,
                 zbin_ptr,
                 qcoeff_ptr,
                 dequant_ptr,
                 scan_mask,

                 round_ptr,
                 quant_ptr,
                 dqcoeff_ptr
             );
}

int vp8_mbblock_error_xmm_impl(short *coeff_ptr, short *dcoef_ptr, int dc);
int vp8_mbblock_error_xmm(MACROBLOCK *mb, int dc)
{
    short *coeff_ptr =  mb->block[0].coeff;
    short *dcoef_ptr =  mb->e_mbd.block[0].dqcoeff;
    return vp8_mbblock_error_xmm_impl(coeff_ptr, dcoef_ptr, dc);
}

int vp8_mbuverror_xmm_impl(short *s_ptr, short *d_ptr);
int vp8_mbuverror_xmm(MACROBLOCK *mb)
{
    short *s_ptr = &mb->coeff[256];
    short *d_ptr = &mb->e_mbd.dqcoeff[256];
    return vp8_mbuverror_xmm_impl(s_ptr, d_ptr);
}

#endif

void vp8_arch_x86_encoder_init(VP8_COMP *cpi)
{
#if CONFIG_RUNTIME_CPU_DETECT
    int flags = x86_simd_caps();
    int mmx_enabled = flags & HAS_MMX;
    int xmm_enabled = flags & HAS_SSE;
    int wmt_enabled = flags & HAS_SSE2;
    int SSE3Enabled = flags & HAS_SSE3;
    int SSSE3Enabled = flags & HAS_SSSE3;

    /* Note:
     *
     * This platform can be built without runtime CPU detection as well. If
     * you modify any of the function mappings present in this file, be sure
     * to also update them in static mapings (<arch>/filename_<arch>.h)
     */

    /* Override default functions with fastest ones for this CPU. */
#if HAVE_MMX

    if (mmx_enabled)
    {
        cpi->rtcd.variance.sad16x16              = vp8_sad16x16_mmx;
        cpi->rtcd.variance.sad16x8               = vp8_sad16x8_mmx;
        cpi->rtcd.variance.sad8x16               = vp8_sad8x16_mmx;
        cpi->rtcd.variance.sad8x8                = vp8_sad8x8_mmx;
        cpi->rtcd.variance.sad4x4                = vp8_sad4x4_mmx;

        cpi->rtcd.variance.var4x4                = vp8_variance4x4_mmx;
        cpi->rtcd.variance.var8x8                = vp8_variance8x8_mmx;
        cpi->rtcd.variance.var8x16               = vp8_variance8x16_mmx;
        cpi->rtcd.variance.var16x8               = vp8_variance16x8_mmx;
        cpi->rtcd.variance.var16x16              = vp8_variance16x16_mmx;

        cpi->rtcd.variance.subpixvar4x4          = vp8_sub_pixel_variance4x4_mmx;
        cpi->rtcd.variance.subpixvar8x8          = vp8_sub_pixel_variance8x8_mmx;
        cpi->rtcd.variance.subpixvar8x16         = vp8_sub_pixel_variance8x16_mmx;
        cpi->rtcd.variance.subpixvar16x8         = vp8_sub_pixel_variance16x8_mmx;
        cpi->rtcd.variance.subpixvar16x16        = vp8_sub_pixel_variance16x16_mmx;
        cpi->rtcd.variance.subpixmse16x16        = vp8_sub_pixel_mse16x16_mmx;

        cpi->rtcd.variance.mse16x16              = vp8_mse16x16_mmx;
        cpi->rtcd.variance.getmbss               = vp8_get_mb_ss_mmx;

        cpi->rtcd.variance.get16x16prederror     = vp8_get16x16pred_error_mmx;
        cpi->rtcd.variance.get8x8var             = vp8_get8x8var_mmx;
        cpi->rtcd.variance.get16x16var           = vp8_get16x16var_mmx;
        cpi->rtcd.variance.get4x4sse_cs          = vp8_get4x4sse_cs_mmx;

        cpi->rtcd.fdct.short4x4                  = vp8_short_fdct4x4_mmx;
        cpi->rtcd.fdct.short8x4                  = vp8_short_fdct8x4_mmx;
        cpi->rtcd.fdct.fast4x4                   = vp8_fast_fdct4x4_mmx;
        cpi->rtcd.fdct.fast8x4                   = vp8_fast_fdct8x4_mmx;
        cpi->rtcd.fdct.walsh_short4x4            = vp8_short_walsh4x4_c;

        cpi->rtcd.encodemb.berr                  = vp8_block_error_mmx;
        cpi->rtcd.encodemb.mberr                 = vp8_mbblock_error_mmx;
        cpi->rtcd.encodemb.mbuverr               = vp8_mbuverror_mmx;
        cpi->rtcd.encodemb.subb                  = vp8_subtract_b_mmx;
        cpi->rtcd.encodemb.submby                = vp8_subtract_mby_mmx;
        cpi->rtcd.encodemb.submbuv               = vp8_subtract_mbuv_mmx;

        cpi->rtcd.quantize.fastquantb            = vp8_fast_quantize_b_mmx;
    }

#endif
#if HAVE_SSE2

    if (wmt_enabled)
    {
        cpi->rtcd.variance.sad16x16              = vp8_sad16x16_wmt;
        cpi->rtcd.variance.sad16x8               = vp8_sad16x8_wmt;
        cpi->rtcd.variance.sad8x16               = vp8_sad8x16_wmt;
        cpi->rtcd.variance.sad8x8                = vp8_sad8x8_wmt;
        cpi->rtcd.variance.sad4x4                = vp8_sad4x4_wmt;

        cpi->rtcd.variance.var4x4                = vp8_variance4x4_wmt;
        cpi->rtcd.variance.var8x8                = vp8_variance8x8_wmt;
        cpi->rtcd.variance.var8x16               = vp8_variance8x16_wmt;
        cpi->rtcd.variance.var16x8               = vp8_variance16x8_wmt;
        cpi->rtcd.variance.var16x16              = vp8_variance16x16_wmt;

        cpi->rtcd.variance.subpixvar4x4          = vp8_sub_pixel_variance4x4_wmt;
        cpi->rtcd.variance.subpixvar8x8          = vp8_sub_pixel_variance8x8_wmt;
        cpi->rtcd.variance.subpixvar8x16         = vp8_sub_pixel_variance8x16_wmt;
        cpi->rtcd.variance.subpixvar16x8         = vp8_sub_pixel_variance16x8_wmt;
        cpi->rtcd.variance.subpixvar16x16        = vp8_sub_pixel_variance16x16_wmt;
        cpi->rtcd.variance.subpixmse16x16        = vp8_sub_pixel_mse16x16_wmt;

        cpi->rtcd.variance.mse16x16              = vp8_mse16x16_wmt;
        cpi->rtcd.variance.getmbss               = vp8_get_mb_ss_sse2;

        cpi->rtcd.variance.get16x16prederror     = vp8_get16x16pred_error_sse2;
        cpi->rtcd.variance.get8x8var             = vp8_get8x8var_sse2;
        cpi->rtcd.variance.get16x16var           = vp8_get16x16var_sse2;
        /* cpi->rtcd.variance.get4x4sse_cs  not implemented for wmt */;

#if 0
        /* short SSE2 DCT currently disabled, does not match the MMX version */
        cpi->rtcd.fdct.short4x4                  = vp8_short_fdct4x4_wmt;
        cpi->rtcd.fdct.short8x4                  = vp8_short_fdct8x4_wmt;
#endif
        /* cpi->rtcd.fdct.fast4x4  not implemented for wmt */;
        cpi->rtcd.fdct.fast8x4                   = vp8_fast_fdct8x4_wmt;
        cpi->rtcd.fdct.walsh_short4x4            = vp8_short_walsh4x4_sse2;

        cpi->rtcd.encodemb.berr                  = vp8_block_error_xmm;
        cpi->rtcd.encodemb.mberr                 = vp8_mbblock_error_xmm;
        cpi->rtcd.encodemb.mbuverr               = vp8_mbuverror_xmm;
        /* cpi->rtcd.encodemb.sub* not implemented for wmt */

        cpi->rtcd.quantize.fastquantb            = vp8_fast_quantize_b_sse;
    }

#endif
#if HAVE_SSE3

    if (SSE3Enabled)
    {
        cpi->rtcd.variance.sad16x16              = vp8_sad16x16_sse3;
        cpi->rtcd.variance.sad16x16x3            = vp8_sad16x16x3_sse3;
        cpi->rtcd.variance.sad16x8x3             = vp8_sad16x8x3_sse3;
        cpi->rtcd.variance.sad8x16x3             = vp8_sad8x16x3_sse3;
        cpi->rtcd.variance.sad8x8x3              = vp8_sad8x8x3_sse3;
        cpi->rtcd.variance.sad4x4x3              = vp8_sad4x4x3_sse3;
        cpi->rtcd.search.full_search             = vp8_full_search_sadx3;

        cpi->rtcd.variance.sad16x16x4d           = vp8_sad16x16x4d_sse3;
        cpi->rtcd.variance.sad16x8x4d            = vp8_sad16x8x4d_sse3;
        cpi->rtcd.variance.sad8x16x4d            = vp8_sad8x16x4d_sse3;
        cpi->rtcd.variance.sad8x8x4d             = vp8_sad8x8x4d_sse3;
        cpi->rtcd.variance.sad4x4x4d             = vp8_sad4x4x4d_sse3;
        cpi->rtcd.search.diamond_search          = vp8_diamond_search_sadx4;
    }

#endif
#if HAVE_SSSE3

    if (SSSE3Enabled)
    {
        cpi->rtcd.variance.sad16x16x3            = vp8_sad16x16x3_ssse3;
        cpi->rtcd.variance.sad16x8x3             = vp8_sad16x8x3_ssse3;
    }

#endif
#endif
}
