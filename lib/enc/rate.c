/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2007                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function:
  last mod: $Id$

 ********************************************************************/
#include <stdlib.h>
#include <string.h>
#include "encint.h"



void oc_enc_calc_lambda(oc_enc_ctx *_enc,int _frame_type){
  ogg_int64_t l;
  int         qi;
  qi=_enc->state.qis[0];
  /*For now, lambda is fixed depending on the qi value and frame type:
      lambda=scale[qti]*(qavg[qti][qi]**1.5),
     where scale={2.25,1.125}.
    A more adaptive scheme might perform better, but Theora's behavior does not
     seem to conform to existing models in the literature.*/
  /*If rate control is active, use the lambda for the _target_ quantizer.
    This allows us to scale to rates slightly lower than we'd normally be able
     to reach, and give the rate control a semblance of "fractional qi"
     precision.*/
  if(_enc->state.info.target_bitrate>0)l=_enc->rc.log_qtarget;
  else l=_enc->log_qavg[_frame_type][qi];
  /*Raise to the 1.5 power.*/
  l+=l>>1;
  /*Multiply by 1.125.*/
  l+=0x00570068E7EF5A1ELL;
  /*And multiply by an extra factor of 2 for INTRA frames.*/
  if(!_frame_type)l+=OC_Q57(1);
  /*The upper bound here is 0x48000.*/
  _enc->lambda=(int)oc_bexp64(l);
}



void oc_rc_state_init(oc_rc_state *_rc,const oc_enc_ctx *_enc){
  ogg_int64_t npixels;
  ogg_int64_t ibpp;
  /*TODO: These parameters should be exposed in a th_encode_ctl() API.*/
  _rc->bits_per_frame=(_enc->state.info.target_bitrate*
   (ogg_int64_t)_enc->state.info.fps_denominator)/
   _enc->state.info.fps_numerator;
  /*Insane framerates or frame sizes mean insane bitrates.
    Let's not get carried away.*/
  if(_rc->bits_per_frame>0x400000000000LL){
    _rc->bits_per_frame=(ogg_int64_t)0x400000000000LL;
  }
  else if(_rc->bits_per_frame<32)_rc->bits_per_frame=32;
  /*The buffer size is set equal to the keyframe interval, clamped to the range
     [8,256] frames.
    The 8 frame minimum gives us some chance to distribute bit estimation
     errors.
    The 256 frame maximum means we'll require 8-10 seconds of pre-buffering at
     24-30 fps, which is not unreasonable.*/
  _rc->buf_delay=_enc->keyframe_frequency_force>256?
   256:_enc->keyframe_frequency_force;
  _rc->buf_delay=OC_MAXI(_rc->buf_delay,12);
  _rc->max=_rc->bits_per_frame*_rc->buf_delay;
  /*Start with a buffer fullness of 75%.
    We can require fully half the buffer for a keyframe, and so this initial
     level gives us maximum flexibility for over/under-shooting in subsequent
     frames.*/
  _rc->target=_rc->fullness=(_rc->max+1>>1)+(_rc->max+2>>2);
  /*Pick exponents and initial scales for quantizer selection.*/
  npixels=_enc->state.info.frame_width*
   (ogg_int64_t)_enc->state.info.frame_height;
  _rc->log_npixels=oc_blog64(npixels);
  ibpp=npixels/_rc->bits_per_frame;
  if(ibpp<1){
    _rc->exp[0]=59;
    _rc->log_scale[0]=oc_blog64(1997)-OC_Q57(8);
  }
  else if(ibpp<2){
    _rc->exp[0]=55;
    _rc->log_scale[0]=oc_blog64(1604)-OC_Q57(8);
  }
  else{
    _rc->exp[0]=48;
    _rc->log_scale[0]=oc_blog64(834)-OC_Q57(8);
  }
  if(ibpp<4){
    _rc->exp[1]=100;
    _rc->log_scale[1]=oc_blog64(2249)-OC_Q57(8);
  }
  else if(ibpp<8){
    _rc->exp[1]=95;
    _rc->log_scale[1]=oc_blog64(1751)-OC_Q57(8);
  }
  else{
    _rc->exp[1]=73;
    _rc->log_scale[1]=oc_blog64(1260)-OC_Q57(8);
  }
  _rc->prev_drop_count=0;
  _rc->log_drop_scale=OC_Q57(0);
}

void oc_enc_update_rc_state(oc_enc_ctx *_enc,
 long _bits,int _qti,int _qi,int _trial){
  /*Note, setting OC_SCALE_SMOOTHING[1] to 0x80 (0.5), which one might expect
     to be a reasonable value, actually causes a feedback loop with, e.g., 12
     fps content encoded at 24 fps; use values near 0 or near 1 for now.
    TODO: Should probably revisit using an exponential moving average in the
     first place at some point; dup tracking should help as well.*/
  static const unsigned OC_SCALE_SMOOTHING[2]={0x13,0x00};
  if(_bits>0){
    ogg_int64_t log_scale;
    ogg_int64_t log_bits;
    ogg_int64_t log_qexp;
    /*Compute the estimated scale factor for this frame type.*/
    log_bits=oc_blog64(_bits);
    log_qexp=_enc->log_qavg[_qti][_qi]-OC_Q57(2);
    log_qexp=(log_qexp>>6)*(_enc->rc.exp[_qti]);
    log_scale=OC_MINI(log_bits-_enc->rc.log_npixels+log_qexp,OC_Q57(16));
    /*Use it to set that factor directly if this was a trial.*/
    if(_trial)_enc->rc.log_scale[_qti]=log_scale;
    else{
      /*Otherwise update an exponential moving average.*/
      _enc->rc.log_scale[_qti]=log_scale
       +(_enc->rc.log_scale[_qti]-log_scale+128>>8)*OC_SCALE_SMOOTHING[_qti];
      /*And update a simple exponential moving average to estimate the "real"
         frame rate taking drops and duplicates into account.*/
      _enc->rc.log_drop_scale=_enc->rc.log_drop_scale
       +oc_blog64(_enc->rc.prev_drop_count+1)>>1;
      _enc->rc.prev_drop_count=_enc->dup_count;
    }
  }
  else{
    /*We dropped this frame.*/
    /*Add it to the previous frame's dup count.*/
    _enc->rc.prev_drop_count+=1+_enc->dup_count;
    /*If this was the first frame of this type, lower the expected scale, but
       don't set it to zero outright.*/
    if(_trial)_enc->rc.log_scale[_qti]>>=1;
  }
  if(!_trial){
    /*And update the buffer fullness level.*/
    _enc->rc.fullness+=_enc->rc.bits_per_frame*(1+_enc->dup_count)-_bits;
    /*If we're too quick filling the buffer, that rate is lost forever.*/
    if(_enc->rc.fullness>_enc->rc.max)_enc->rc.fullness=_enc->rc.max;
  }
}

int oc_enc_select_qi(oc_enc_ctx *_enc,int _qti,int _clamp){
  ogg_int64_t  rate_total;
  ogg_uint32_t next_key_frame;
  int          nframes[2];
  int          buf_delay;
  ogg_int64_t  log_qtarget;
  int          best_qi;
  ogg_int64_t  best_qdiff;
  int          old_qi;
  int          qi;
  /*Figure out how to re-distribute bits so that we hit our fullness target
     before the last keyframe in our current buffer window (after the current
     frame), or the end of the buffer window, whichever comes first.*/
  next_key_frame=_qti?_enc->keyframe_frequency_force
   -(_enc->state.curframe_num-_enc->state.keyframe_num):0;
  nframes[0]=(_enc->rc.buf_delay-OC_MINI(next_key_frame,_enc->rc.buf_delay)
   +_enc->keyframe_frequency_force-1)/_enc->keyframe_frequency_force;
  if(nframes[0]+_qti>1){
    buf_delay=next_key_frame+(nframes[0]-1)*_enc->keyframe_frequency_force;
    nframes[0]--;
  }
  else buf_delay=_enc->rc.buf_delay;
  nframes[1]=buf_delay-nframes[0];
  rate_total=_enc->rc.fullness-_enc->rc.target
   +buf_delay*_enc->rc.bits_per_frame;
  /*Downgrade the delta frame rate to correspond to the recent drop count
     history.*/
  if(_enc->rc.prev_drop_count>0||_enc->rc.log_drop_scale>OC_Q57(0)){
    ogg_int64_t dup_scale;
    dup_scale=oc_bexp64((_enc->rc.log_drop_scale
     +oc_blog64(_enc->rc.prev_drop_count+1)>>1)+OC_Q57(8));
    if(dup_scale<nframes[1]<<8){
      int dup_scalei;
      dup_scalei=(int)dup_scale;
      if(dup_scalei>0)nframes[1]=((nframes[1]<<8)+dup_scalei-1)/dup_scalei;
    }
    else nframes[1]=!!nframes[1];
  }
  /*If there aren't enough bits to achieve our desired fullness level, use the
     minimum quality permitted.*/
  if(rate_total<=buf_delay)log_qtarget=OC_QUANT_MAX_LOG;
  else{
    static const unsigned char KEY_RATIO[2]={32,17};
    ogg_int64_t   log_scale0;
    ogg_int64_t   log_scale1;
    ogg_int64_t   prevr;
    ogg_int64_t   curr;
    ogg_int64_t   realr;
    int           i;
    log_scale0=_enc->rc.log_scale[_qti]+_enc->rc.log_npixels;
    log_scale1=_enc->rc.log_scale[1-_qti]+_enc->rc.log_npixels;
    curr=(rate_total+(buf_delay>>1))/buf_delay;
    realr=curr*KEY_RATIO[_qti]+16>>5;
    for(i=0;i<10;i++){
      ogg_int64_t rdiff;
      ogg_int64_t rderiv;
      ogg_int64_t log_rpow;
      ogg_int64_t rscale;
      ogg_int64_t drscale;
      ogg_int64_t bias;
      prevr=curr;
      log_rpow=oc_blog64(prevr)-log_scale0;
      log_rpow=(log_rpow+(_enc->rc.exp[_qti]>>1))/_enc->rc.exp[_qti]*
       _enc->rc.exp[1-_qti];
      rscale=nframes[1-_qti]*KEY_RATIO[1-_qti]*
       oc_bexp64(log_scale1+log_rpow);
      rdiff=nframes[_qti]*KEY_RATIO[_qti]*prevr+rscale-(rate_total<<5);
      drscale=(rscale+(_enc->rc.exp[_qti]>>1))/_enc->rc.exp[_qti]*
       _enc->rc.exp[1-_qti]/prevr;
      rderiv=nframes[_qti]*KEY_RATIO[_qti]+drscale;
      if(rderiv==0)break;
      bias=rderiv+OC_SIGNMASK(rdiff^rderiv)^OC_SIGNMASK(rdiff^rderiv);
      curr=prevr-((rdiff<<1)+bias)/(rderiv<<1);
      realr=curr*KEY_RATIO[_qti]+16>>5;
      if(curr<=0||realr>rate_total||prevr==curr)break;
    }
    log_qtarget=OC_Q57(2)-((oc_blog64(realr)-log_scale0+(_enc->rc.exp[_qti]>>1))/
     _enc->rc.exp[_qti]<<6);
    log_qtarget=OC_MINI(log_qtarget,OC_QUANT_MAX_LOG);
  }
  /*If this was not one of the initial frames, limit the change in quality.*/
  old_qi=_enc->state.qis[0];
  if(_clamp){
    ogg_int64_t log_qmin;
    ogg_int64_t log_qmax;
    /*Clamp the target quantizer to within [0.8*Q,1.2*Q], where Q is the
       current quantizer.
      TODO: With user-specified quant matrices, we need to enlarge these limits
       if they don't actually let us change qi values.*/
    log_qmin=_enc->log_qavg[_qti][old_qi]-0x00A4D3C25E68DC58LL;
    log_qmax=_enc->log_qavg[_qti][old_qi]+0x00A4D3C25E68DC58LL;
    log_qtarget=OC_CLAMPI(log_qmin,log_qtarget,log_qmax);
  }
  /*Search for the quantizer that matches the target most closely.
    We don't assume a linear ordering, but when there are ties we do pick the
     quantizer closest to the current one.*/
  best_qi=_enc->state.info.quality;
  best_qdiff=_enc->log_qavg[_qti][best_qi]-log_qtarget;
  best_qdiff=best_qdiff+OC_SIGNMASK(best_qdiff)^OC_SIGNMASK(best_qdiff);
  for(qi=_enc->state.info.quality+1;qi<64;qi++){
    ogg_int64_t qdiff;
    qdiff=_enc->log_qavg[_qti][qi]-log_qtarget;
    qdiff=qdiff+OC_SIGNMASK(qdiff)^OC_SIGNMASK(qdiff);
    if(qdiff<best_qdiff||
     qdiff==best_qdiff&&abs(qi-old_qi)<abs(best_qi-old_qi)){
      best_qi=qi;
      best_qdiff=qdiff;
    }
  }
  /*Save the quantizer target for lambda calculations.*/
  _enc->rc.log_qtarget=log_qtarget;
  return best_qi;
}
