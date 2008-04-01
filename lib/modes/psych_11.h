/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2007             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: 11kHz settings 
 last mod: $Id$

 ********************************************************************/

/* tonal masking curve level adjustments *************************/
static vp_adjblock _vp_tonemask_adj_11[4]={
  /* adjust for mode zero */
  /* 63     125     250     500     1     2     4     8    16 */
  {{-16,-16,-16,-16,-16,-16,-10,-8,-4, 0, 0, 0, 3, 3,99,99,99}}, /* -2 */
  {{-16,-16,-16,-16,-16,-16,-10,-8,-4, 0, 0, 0, 0, 0,99,99,99}}, /* -1 */
  {{-20,-20,-20,-20,-20,-16,-10,-8,-4,-2, 0, 0, 0, 0,99,99,99}}, /*  0 */
  {{-20,-20,-20,-20,-20,-16,-10,-8,-4,-2, 0, 0, 0, 0,99,99,99}}, /* 10 */
};

/* noise bias */
static noise3 _psy_noisebias_11[4]={
  /*  63     125     250     500      1k       2k      4k      8k     16k*/
  {{{-10,-10,-10,-10, -5, -5, -4,  2,  4, 10, 10, 12, 12, 12, 99, 99, 99},
    {-12,-12,-12,-12, -7, -7, -2,  3,  3,  4,  4,  5,  6, 12, 99, 99, 99},
    {-30,-30,-30,-30,-30,-24,-20,-14,-10, -6, -8, -8, -6, -6, 99, 99, 99}}},

  {{{-10,-10,-10,-10, -5, -5, -5,  0,  4, 10, 10, 12, 12, 12, 99, 99, 99},
    {-15,-15,-15,-15,-10,-10, -5,  0,  0,  4,  4,  5,  5, 10, 99, 99, 99},
    {-30,-30,-30,-30,-30,-24,-20,-14,-10, -6, -8, -8, -6, -6, 99, 99, 99}}},

  {{{-10,-10,-10,-10, -5, -5, -5,  0,  4, 10, 10, 12, 12, 12, 99, 99, 99},
    {-15,-15,-15,-15,-10,-10, -5, -5, -5,  0,  0,  0,  0,  0, 99, 99, 99},
    {-30,-30,-30,-30,-30,-24,-20,-14,-10, -6, -8, -8, -6, -6, 99, 99, 99}}},

  {{{-15,-15,-15,-15,-15,-12,-10, -8,  0,  2,  4,  4,  5,  5, 99, 99, 99},
    {-30,-30,-30,-30,-26,-22,-20,-14,-12,-12,-10,-10,-10,-10, 99, 99, 99},
    {-30,-30,-30,-30,-26,-26,-26,-26,-26,-26,-26,-26,-26,-24, 99, 99, 99}}},
};

/* tone master attenuation by base quality mode and bitrate tweak */
static att3 _psy_tone_masteratt_11[4]={
  {{ 31,  28,  13},  0,   0},  /* -2 */
  {{ 30,  28,  12},  0,   0},  /* -1 */
  {{ 30,  26,  12},  0,   0},  /*  0 */
  {{ 20,   0, -14},  0,   0},  /* 10 */
};

/* lowpass by mode **************/
static double _psy_lowpass_11[4]={4.1,4.5,5.5,30.,};

/* noise normalization **********/
static double _noise_thresh_11[3]={ .3,.3,.5 };
