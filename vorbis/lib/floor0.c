/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and The XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: floor backend 0 implementation
 last mod: $Id: floor0.c,v 1.11.2.1.2.3 2000/04/06 16:47:45 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "registry.h"
#include "lpc.h"
#include "lsp.h"
#include "bookinternal.h"
#include "scales.h"

typedef struct {
  long n;
  int ln;
  int  m;
  int *linearmap;
  double *subcurve;

  vorbis_info_floor0 *vi;
  lpc_lookup lpclook;
} vorbis_look_floor0;

static void free_info(vorbis_info_floor *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_floor0));
    free(i);
  }
}

static void free_look(vorbis_look_floor *i){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  if(i){
    if(look->linearmap)free(look->linearmap);
    if(look->subcurve)free(look->subcurve);
    lpc_clear(&look->lpclook);
    memset(look,0,sizeof(vorbis_look_floor0));
    free(look);
  }
}

static void pack (vorbis_info_floor *i,oggpack_buffer *opb){
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  int j;
  _oggpack_write(opb,info->order,8);
  _oggpack_write(opb,info->rate,16);
  _oggpack_write(opb,info->barkmap,16);
  _oggpack_write(opb,info->ampbits,6);
  _oggpack_write(opb,info->ampdB,8);
  for(j=0;j<27;j++)
    _oggpack_write(opb,(int)info->subcurve[j],8);
  _oggpack_write(opb,info->numbooks-1,4);
  for(j=0;j<info->numbooks;j++)
    _oggpack_write(opb,info->books[j],8);
}

static vorbis_info_floor *unpack (vorbis_info *vi,oggpack_buffer *opb){
  int j;
  vorbis_info_floor0 *info=malloc(sizeof(vorbis_info_floor0));
  info->order=_oggpack_read(opb,8);
  info->rate=_oggpack_read(opb,16);
  info->barkmap=_oggpack_read(opb,16);
  info->ampbits=_oggpack_read(opb,6);
  info->ampdB=_oggpack_read(opb,8);
  for(j=0;j<27;j++)
    info->subcurve[j]=_oggpack_read(opb,8);
  info->numbooks=_oggpack_read(opb,4)+1;
  
  if(info->order<1)goto err_out;
  if(info->rate<1)goto err_out;
  if(info->barkmap<1)goto err_out;
  if(info->numbooks<1)goto err_out;

  for(j=0;j<info->numbooks;j++){
    info->books[j]=_oggpack_read(opb,8);
    if(info->books[j]<0 || info->books[j]>=vi->books)goto err_out;
  }
  return(info);  
 err_out:
  free_info(info);
  return(NULL);
}

/* initialize Bark scale and normalization lookups.  We could do this
   with static tables, but Vorbis allows a number of possible
   combinations, so it's best to do it computationally.

   The below is authoritative in terms of defining scale mapping.
   Note that the scale depends on the sampling rate as well as the
   linear block and mapping sizes */

static vorbis_look_floor *look (vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_floor *i){
  int j;
  double scale;
  vorbis_info        *vi=vd->vi;
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  vorbis_look_floor0 *look=malloc(sizeof(vorbis_look_floor0));
  look->m=info->order;
  look->n=vi->blocksizes[mi->blockflag]/2;
  look->ln=info->barkmap;
  look->vi=info;
  lpc_init(&look->lpclook,look->ln,look->m);

  /* we choose a scaling constant so that:
     floor(bark(rate/2-1)*C)=mapped-1
     floor(bark(rate/2)*C)=mapped */
  scale=look->ln/toBARK(info->rate/2.);

  /* the mapping from a linear scale to a smaller bark scale is
     straightforward.  We do *not* make sure that the linear mapping
     does not skip bark-scale bins; the decoder simply skips them and
     the encoder may do what it wishes in filling them.  They're
     necessary in some mapping combinations to keep the scale spacing
     accurate */
  look->linearmap=malloc(look->n*sizeof(int));
  for(j=0;j<look->n;j++){
    int val=floor( toBARK((info->rate/2.)/look->n*j) 
		   *scale); /* bark numbers represent band edges */
    if(val>look->ln)val=look->ln; /* guard against the approximation */
    look->linearmap[j]=val;
  }
  look->subcurve=malloc(look->ln*sizeof(double));
  {
    double max=toBARK(info->rate/2.);
    for(j=0;j<look->ln;j++){
      double bark=max/look->ln*j;
      int f=floor(bark);
      double del=bark-f;

      if(f>=27){
	f=26;
	del=1.;
      }

      look->subcurve[j]=info->subcurve[f]*(1.-del)+info->subcurve[f+1]*del;
    }
  }

  return look;
}

#include <stdio.h>


/* less efficient than the decode side (written for clarity).  We're
   not bottlenecked here anyway */
static double _curve_to_lpc(double *curve,double *lpc,vorbis_look_floor0 *l,
			    long frameno){
  /* map the input curve to a bark-scale curve for encoding */
  
  int mapped=l->ln;
  double *work=alloca(sizeof(double)*mapped);
  int i,j,last=0;

  memset(work,0,sizeof(double)*mapped);
  
  /* Only the decode side is behavior-specced; for now in the encoder,
     we select the maximum value of each band as representative (this
     helps make sure peaks don't go out of range.  In error terms,
     selecting min would make more sense, but the codebook is trained
     numerically, so we don't actually lose.  We'd still want to
     use the original curve for error and noise estimation */
  
  for(i=0;i<l->n;i++){
    int bark=l->linearmap[i];
    if(work[bark]<curve[i])work[bark]=curve[i];
    if(bark>last+1){
      /* If the bark scale is climbing rapidly, some bins may end up
         going unused.  This isn't a waste actually; it keeps the
         scale resolution even so that the LPC generator has an easy
         time.  However, if we leave the bins empty we lose energy.
         So, fill 'em in.  The decoder does not do anything with  he
         unused bins, so we can fill them anyway we like to end up
         with a better spectral curve */

      /* we'll always have a bin zero, so we don't need to guard init */
      long span=bark-last;
      for(j=1;j<span;j++){
	double del=(double)j/span;
	work[j+last]=work[bark]*del+work[last]*(1.-del);
      }
    }
    last=bark;
  }
  for(i=0;i<l->ln;i++)work[i]-=l->subcurve[i];

#if 1
    { /******************/
      FILE *of;
      char buffer[80];
      int i;

      sprintf(buffer,"mask_%d.m",frameno);
      of=fopen(buffer,"w");
      for(i=0;i<mapped;i++)
	fprintf(of,"%g\n",work[i]);
      fclose(of);
    }
#endif

  return vorbis_lpc_from_curve(work,lpc,&(l->lpclook));
}

/* generate the whole freq response curve of an LPC IIR filter */

static void _lpc_to_curve(double *curve,double *lpc,double amp,
			  vorbis_look_floor0 *l,char *name,long frameno){
  double *lcurve=alloca(sizeof(double)*(l->ln*2));
  int i;

  if(amp==0){
    memset(curve,0,sizeof(double)*l->n);
    return;
  }
  vorbis_lpc_to_curve(lcurve,lpc,amp,&(l->lpclook));

#if 1
    { /******************/
      FILE *of;
      char buffer[80];
      int i;

      sprintf(buffer,"%s_%d.m",name,frameno);
      of=fopen(buffer,"w");
      for(i=0;i<l->ln;i++)
	fprintf(of,"%g\n",lcurve[i]);
      fclose(of);
    }
#endif

  for(i=0;i<l->ln;i++)lcurve[i]+=l->subcurve[i];
  for(i=0;i<l->n;i++)curve[i]=lcurve[l->linearmap[i]];

}

static int forward(vorbis_block *vb,vorbis_look_floor *i,
		    double *in,double *out){
  long j,k,stage;
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;
  double amp;
  long bits=0;

  /* our floor comes in on a linear scale; go to a [-Inf...0] dB
     scale.  The curve has to be positive, so we offset it. */
  for(j=0;j<look->n;j++)in[j]=todB(in[j])+info->ampdB;
  
  /* use 'out' as temp storage */
  /* Convert our floor to a set of lpc coefficients */ 
  amp=sqrt(_curve_to_lpc(in,out,look,vb->sequence));
  
  /* amp is in the range (0. to ampdB].  Encode that range using
     ampbits bits */
 
  {
    long maxval=(1<<info->ampbits)-1;
    
    long val=amp/info->ampdB*maxval+1;

    if(val<0)val=0;           /* likely */
    if(val>maxval)val=maxval; /* not bloody likely */

    _oggpack_write(&vb->opb,val,info->ampbits);
    if(val>0)
      amp=(val-.5)/maxval*info->ampdB;
    else
      amp=0;
  }

  if(amp>0){

    /* LSP <-> LPC is orthogonal and LSP quantizes more stably  */
    vorbis_lpc_to_lsp(out,out,look->m);

#ifdef TRAIN
    {
      int j;
      FILE *of;
      char buffer[80];
      sprintf(buffer,"lsp0coeff_%d.vqd",vb->mode);
      of=fopen(buffer,"a");
      for(j=0;j<look->m;j++)
	fprintf(of,"%g, ",out[j]);
      fprintf(of,"\n");
      fclose(of);
    }
#endif

#if 1
    { /******************/
      vorbis_lsp_to_lpc(out,in,look->m); 
      _lpc_to_curve(in,in,amp,look,"prefloor",vb->sequence);
    }
#endif

    /* code the spectral envelope, and keep track of the actual
       quantized values; we don't want creeping error as each block is
       nailed to the last quantized value of the previous block. */

    /* the spec supports using one of a number of codebooks.  Right
       now, encode using this lib supports only one */
    _oggpack_write(&vb->opb,0,_ilog(info->numbooks));

    {
      codebook *b=vb->vd->fullbooks+info->books[0];
      double last=0.;
      for(j=0;j<look->m;){
	for(k=0;k<b->dim;k++)out[j+k]-=last;
	bits+=vorbis_book_encodev(b,out+j,&vb->opb);
	for(k=0;k<b->dim;k++,j++)out[j]+=last;
	last=out[j-1];
      }
    }

    /* take the coefficients back to a spectral envelope curve */
    vorbis_lsp_to_lpc(out,out,look->m); 
    _lpc_to_curve(out,out,amp,look,"floor",vb->sequence);
    for(j=0;j<look->n;j++)out[j]= fromdB(out[j]-info->ampdB);
    return(1);
  }

  memset(out,0,sizeof(double)*look->n);
  return(0);
}

static int inverse(vorbis_block *vb,vorbis_look_floor *i,double *out){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;
  int j,k,stage;
  
  long ampraw=_oggpack_read(&vb->opb,info->ampbits);
  if(ampraw>0){
    long maxval=(1<<info->ampbits)-1;
    double amp=(ampraw-.5)/maxval*info->ampdB;
    int booknum=_oggpack_read(&vb->opb,_ilog(info->numbooks));
    codebook *b=vb->vd->fullbooks+info->books[booknum];
    double last=0.;

    memset(out,0,sizeof(double)*look->m);    

    for(j=0;j<look->m;j+=b->dim)
      vorbis_book_decodev(b,out+j,&vb->opb);
    for(j=0;j<look->m;){
      for(k=0;k<b->dim;k++,j++)out[j]+=last;
      last=out[j-1];
    }

    /* take the coefficients back to a spectral envelope curve */
    vorbis_lsp_to_lpc(out,out,look->m); 
    _lpc_to_curve(out,out,amp,look,"",0);

    for(j=0;j<look->n;j++)out[j]= fromdB(out[j]-info->ampdB);
    return(1);
  }else
    memset(out,0,sizeof(double)*look->n);

  return(0);
}

/* export hooks */
vorbis_func_floor floor0_exportbundle={
  &pack,&unpack,&look,&free_info,&free_look,&forward,&inverse
};


