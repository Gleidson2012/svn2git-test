/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: random psychoacoustics (not including preecho)
 last mod: $Id: psy.h,v 1.24.2.4 2001/11/16 08:17:07 xiphmont Exp $

 ********************************************************************/

#ifndef _V_PSY_H_
#define _V_PSY_H_
#include "smallft.h"

#include "backends.h"

#define BLOCKTYPE_IMPULSE    0
#define BLOCKTYPE_PADDING    1
#define BLOCKTYPE_TRANSITION 0 
#define BLOCKTYPE_LONG       1


#ifndef EHMER_MAX
#define EHMER_MAX 56
#endif

/* psychoacoustic setup ********************************************/
#define MAX_BARK 27
#define P_BANDS 17
#define P_LEVELS 11

typedef struct vp_couple{
  int limit;        /* sample post */

  int outofphase_redundant_flip_p;
  float outofphase_requant_limit;

  float amppost_point;
  
} vp_couple;

typedef struct vp_couple_pass{  
  float granulem;
  float igranulem;

  vp_couple couple_pass[8];
} vp_couple_pass;

typedef struct vp_attenblock{
  float block[P_BANDS][P_LEVELS];
} vp_attenblock;

#define NOISE_COMPAND_LEVELS 40
typedef struct vorbis_info_psy{
  float  *ath;

  float  ath_adjatt;
  float  ath_maxatt;

  float tone_masteratt;
  float tone_guard;
  float tone_abs_limit;
  vp_attenblock *toneatt;

  int peakattp;
  int curvelimitp;
  vp_attenblock *peakatt;

  int noisemaskp;
  float noisemaxsupp;
  float noisewindowlo;
  float noisewindowhi;
  int   noisewindowlomin;
  int   noisewindowhimin;
  int   noisewindowfixed;
  float noiseoff[P_BANDS];
  float *noisecompand;

  float max_curve_dB;

  vp_couple_pass *couple_pass;

} vorbis_info_psy;

typedef struct{
  float     decaydBpms;
  int       eighth_octave_lines;

  /* for block long/short tuning; encode only */
  int       envelopesa;
  float     preecho_thresh[4];
  float     postecho_thresh[4];
  float     preecho_minenergy;

  float     ampmax_att_per_sec;

  /* delay caching... how many samples to keep around prior to our
     current block to aid in analysis? */
  int       delaycache;
} vorbis_info_psy_global;

typedef struct {
  float   ampmax;
  float **decay;
  int     decaylines;
  int     channels;

  vorbis_info_psy_global *gi;
} vorbis_look_psy_global;


typedef struct {
  int n;
  struct vorbis_info_psy *vi;

  float ***tonecurves;
  float *noisethresh;
  float *noiseoffset;

  float *ath;
  long  *octave;             /* in n.ocshift format */
  unsigned long *bark;

  long  firstoc;
  long  shiftoc;
  int   eighth_octave_lines; /* power of two, please */
  int   total_octave_lines;  
  long  rate; /* cache it */
} vorbis_look_psy;

extern void   _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,
			   vorbis_info_psy_global *gi,int n,long rate);
extern void   _vp_psy_clear(vorbis_look_psy *p);
extern void  *_vi_psy_dup(void *source);

extern void   _vi_psy_free(vorbis_info_psy *i);
extern vorbis_info_psy *_vi_psy_copy(vorbis_info_psy *i);

extern void _vp_remove_floor(vorbis_look_psy *p,
			     vorbis_look_psy_global *g,
			     float *logmdct, 
			     float *mdct,
			     float *codedflr,
			     float *residue,
			     float local_specmax);

extern void   _vp_compute_mask(vorbis_look_psy *p,
			       vorbis_look_psy_global *g,
			       int channel,
			       float *fft, 
			       float *mdct, 
			       float *mask, 
			       float global_specmax,
			       float local_specmax,
			       int lastsize,
			       float bitrate_noise_offset);

extern void _vp_quantize_couple(vorbis_look_psy *p,
			 vorbis_info_mapping0 *vi,
			 float **pcm,
			 float **sofar,
			 float **quantized,
			 int   *nonzero,
			 int   passno);

extern float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd);

#endif


