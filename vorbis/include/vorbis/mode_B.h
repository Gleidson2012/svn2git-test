/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: predefined encoding modes
 last mod: $Id: mode_B.h,v 1.1.6.1 2000/11/04 06:21:40 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_B_H_
#define _V_MODES_B_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "vorbis/backends.h"

#include "vorbis/book/lsp12_0.vqh"
#include "vorbis/book/lsp30_0.vqh"

#include "vorbis/book/resaux0_128a_160.vqh"
#include "vorbis/book/resaux0_1024a_160.vqh"

#include "vorbis/book/res0_128a_160_1.vqh"
#include "vorbis/book/res0_128a_160_2.vqh"
#include "vorbis/book/res0_128a_160_3.vqh"
#include "vorbis/book/res0_128a_160_4.vqh"
#include "vorbis/book/res0_128a_160_5.vqh"
#include "vorbis/book/res0_1024a_160_1.vqh"
#include "vorbis/book/res0_1024a_160_2.vqh"
#include "vorbis/book/res0_1024a_160_3.vqh"
#include "vorbis/book/res0_1024a_160_4.vqh"
#include "vorbis/book/res0_1024a_160_5.vqh"
#include "vorbis/book/res0_1024a_160_6.vqh"
#include "vorbis/book/res0_1024a_160_7.vqh"
#include "vorbis/book/res0_1024a_160_8.vqh"
#include "vorbis/book/res0_1024a_160_9.vqh"

static vorbis_info_psy _psy_set_B={
  1,/*athp*/
  1,/*decayp*/
  1,/*smoothp*/
  0,.1,

  -100.,
  -140.,

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

  1,/* tonemaskp */
  /*  0   10   20   30   40   50   60   70   80   90   100 */
  {{-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*63*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*88*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*125*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*175*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*250*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*350*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*500*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*700*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*1000*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*1400*/
   {-40.,-40.,-40.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*2000*/
   {-40.,-40.,-40.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*2800*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*4000*/
   {-30.,-30.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*5600*/
   {-30.,-30.,-35.,-35.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*8000*/
   {-30.,-30.,-35.,-33.,-35.,-45.,-55.,-65.,-75.,-90.,-100.}, /*11500*/
   {-30.,-28.,-30.,-32.,-35.,-45.,-55.,-65.,-75.,-90.,-100.}, /*16000*/
  },

  1,/* peakattp */
  {{-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-22.,-22.}, /*63*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-22.,-22.}, /*88*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-22.,-22.}, /*125*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-16.,-17.}, /*8000*/
   { -6., -7., -9., -9., -9., -9.,-10.,-11.,-12.,-13.,-14.}, /*11500*/
   { -6., -6., -9., -9., -9., -9., -9., -9.,-10.,-11.,-12.}, /*16000*/
  },

  1,/*noisemaskp */
  /*  0   10   20   30   40   50   60    70    80    90   100 */
  {{-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*63*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*88*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*125*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*175*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*250*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*350*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*500*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*700*/

   {  4.,  4.,  4.,  4.,  4.,  2.,  1.,   0.,  -1.,  -2.,  -3.}, /*1000*/
   {  4.,  4.,  4.,  4.,  4.,  2.,  1.,   0.,   0.,  -2.,  -3.}, /*1400*/
   {  4.,  4.,  4.,  4.,  2.,  2.,  1.,   0.,   0.,  -2.,  -3.}, /*2000*/
   {  4.,  4.,  4.,  4.,  2.,  2.,  1.,   0.,   0.,  -2.,  -3.}, /*2800*/
   {  4.,  4.,  4.,  4.,  4.,  4.,  2.,   0.,   0.,   0.,   0.}, /*4000*/
   {  4.,  4.,  4.,  4.,  4.,  4.,  2.,   2.,   2.,   2.,   2.}, /*5600*/
   {  6.,  6.,  6.,  6.,  4.,  4.,  4.,   4.,   4.,   2.,   2.}, /*8000*/
   {  8.,  8.,  8.,  8.,  6.,  6.,  6.,   6.,   6.,   4.,   4.}, /*11500*/
   {  8.,  8.,  8.,  8.,  8.,  8.,  8.,   6.,   6.,   6.,   4.}, /*16000*/
  },
 
  110.,

  -0., -.004   /* attack/decay control */
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0B={0};
static vorbis_info_floor0 _floor_set0B={12, 44100,  64, 12,150, 1, {0} };
static vorbis_info_floor0 _floor_set1B={30, 44100, 256, 12,150, 1, {1} };
static vorbis_info_residue0 _residue_set0B={0,128, 32,6,2,
					    {0,1,1,1,1,1},
					    {4,5,6,7,8},
					    
					    {0,99999,9999,9999,9999},
					    {99,1.5,2.5,3.5,5},
					    {5,5,5,5,5},
					    {99,99,99,99,99}};

static vorbis_info_residue0 _residue_set1B={0,1024, 32,10,3,
					    {0,1,1,1,1,1,1,1,1,1},
					    {9,10,11,12,13,14,15,16,17},
					   
					    {0,9,9999,17,9999,
					     27,9999,9999,9999},
					    {99,1.5,1.5,2.5,2.5,3.5,3.5,7,13},
					    {5,5,5,5,5,5,5,5,5},
					    {99,99,99,99,99,99,99,99,99}};

static vorbis_info_mapping0 _mapping_set0B={1, {0,0}, {0}, {0}, {0}, {0}};
static vorbis_info_mapping0 _mapping_set1B={1, {0,0}, {0}, {1}, {1}, {0}};
static vorbis_info_mode _mode_set0B={0,0,0,0};
static vorbis_info_mode _mode_set1B={1,0,0,1};

/* CD quality stereo, no channel coupling */
vorbis_info info_B={
  /* channels, sample rate, upperkbps, nominalkbps, lowerkbps */
  0, 2, 44100, 0,0,0,
  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   18,   1,
  /* modes */
  {&_mode_set0B,&_mode_set1B},
  /* maps */
  {0,0},{&_mapping_set0B,&_mapping_set1B},
  /* times */
  {0,0},{&_time_set0B},
  /* floors */
  {0,0},{&_floor_set0B,&_floor_set1B},
  /* residue */
  {0,0},{&_residue_set0B,&_residue_set1B},
  /* books */
  {&_vq_book_lsp12_0,      /* 0 */
   &_vq_book_lsp30_0,      /* 1 */

   &_huff_book_resaux0_128a_160,
   &_huff_book_resaux0_1024a_160,

   &_vq_book_res0_128a_160_1,
   &_vq_book_res0_128a_160_2,
   &_vq_book_res0_128a_160_3,
   &_vq_book_res0_128a_160_4,
   &_vq_book_res0_128a_160_5,
   &_vq_book_res0_1024a_160_1,
   &_vq_book_res0_1024a_160_2,
   &_vq_book_res0_1024a_160_3,
   &_vq_book_res0_1024a_160_4,
   &_vq_book_res0_1024a_160_5,
   &_vq_book_res0_1024a_160_6,
   &_vq_book_res0_1024a_160_7,
   &_vq_book_res0_1024a_160_8,
   &_vq_book_res0_1024a_160_9,

  },
  /* psy */
  {&_psy_set_B},
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, 32, 6, -96.
};

#define PREDEF_INFO_MAX 0

#endif
