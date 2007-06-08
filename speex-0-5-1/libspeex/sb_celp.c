/* Copyright (C) 2002 Jean-Marc Valin 
   File: sb_celp.c

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "nb_celp.h"
#include "sb_celp.h"
#include "stdlib.h"
#include "filters.h"
#include <math.h>
#include "lpc.h"
#include "lsp.h"
#include <stdio.h>
#include "stack_alloc.h"
#include "cb_search.h"
#include "quant_lsp.h"
#include "vq.h"
#include <string.h>
#include "ltp.h"
#include <stdlib.h>

#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#define sqr(x) ((x)*(x))

#define SUBMODE(x) st->submodes[st->submodeID]->x


#if 0
#define QMF_ORDER 32
static float h0[32] = {
   0.0006910579, -0.001403793,
   -0.001268303, 0.004234195,
   0.001414246, -0.009458318,
   -0.0001303859, 0.01798145,
   -0.004187483, -0.03123862,
   0.01456844, 0.05294745,
   -0.03934878, -0.09980243,
   0.1285579, 0.4664053,
   0.4664053, 0.1285579,
   -0.09980243, -0.03934878,
   0.05294745, 0.01456844,
   -0.03123862, -0.004187483,
   0.01798145, -0.0001303859,
   -0.009458318, 0.001414246,
   0.004234195, -0.001268303,
   -0.001403793, 0.0006910579
};

static float h1[32] = {
   0.0006910579, 0.001403793,
   -0.001268303, -0.004234195,
   0.001414246, 0.009458318,
   -0.0001303859, -0.01798145,
   -0.004187483, 0.03123862,
   0.01456844, -0.05294745,
   -0.03934878, 0.09980243,
   0.1285579, -0.4664053,
   0.4664053, -0.1285579,
   -0.09980243, 0.03934878,
   0.05294745, -0.01456844,
   -0.03123862, 0.004187483,
   0.01798145, 0.0001303859,
   -0.009458318, -0.001414246,
   0.004234195, 0.001268303,
   -0.001403793, -0.0006910579
};
#else 
#define QMF_ORDER 64
static float h0[64] = {
   3.596189e-05, -0.0001123515,
   -0.0001104587, 0.0002790277,
   0.0002298438, -0.0005953563,
   -0.0003823631, 0.00113826,
   0.0005308539, -0.001986177,
   -0.0006243724, 0.003235877,
   0.0005743159, -0.004989147,
   -0.0002584767, 0.007367171,
   -0.0004857935, -0.01050689,
   0.001894714, 0.01459396,
   -0.004313674, -0.01994365,
   0.00828756, 0.02716055,
   -0.01485397, -0.03764973,
   0.026447, 0.05543245,
   -0.05095487, -0.09779096,
   0.1382363, 0.4600981,
   0.4600981, 0.1382363,
   -0.09779096, -0.05095487,
   0.05543245, 0.026447,
   -0.03764973, -0.01485397,
   0.02716055, 0.00828756,
   -0.01994365, -0.004313674,
   0.01459396, 0.001894714,
   -0.01050689, -0.0004857935,
   0.007367171, -0.0002584767,
   -0.004989147, 0.0005743159,
   0.003235877, -0.0006243724,
   -0.001986177, 0.0005308539,
   0.00113826, -0.0003823631,
   -0.0005953563, 0.0002298438,
   0.0002790277, -0.0001104587,
   -0.0001123515, 3.596189e-05
};

static float h1[64] = {
   3.596189e-05, 0.0001123515,
   -0.0001104587, -0.0002790277,
   0.0002298438, 0.0005953563,
   -0.0003823631, -0.00113826,
   0.0005308539, 0.001986177,
   -0.0006243724, -0.003235877,
   0.0005743159, 0.004989147,
   -0.0002584767, -0.007367171,
   -0.0004857935, 0.01050689,
   0.001894714, -0.01459396,
   -0.004313674, 0.01994365,
   0.00828756, -0.02716055,
   -0.01485397, 0.03764973,
   0.026447, -0.05543245,
   -0.05095487, 0.09779096,
   0.1382363, -0.4600981,
   0.4600981, -0.1382363,
   -0.09779096, 0.05095487,
   0.05543245, -0.026447,
   -0.03764973, 0.01485397,
   0.02716055, -0.00828756,
   -0.01994365, 0.004313674,
   0.01459396, -0.001894714,
   -0.01050689, 0.0004857935,
   0.007367171, 0.0002584767,
   -0.004989147, -0.0005743159,
   0.003235877, 0.0006243724,
   -0.001986177, -0.0005308539,
   0.00113826, 0.0003823631,
   -0.0005953563, -0.0002298438,
   0.0002790277, 0.0001104587,
   -0.0001123515, -3.596189e-05
};
#endif

void *sb_encoder_init(SpeexMode *m)
{
   int i;
   SBEncState *st;
   SpeexSBMode *mode;

   st = malloc(sizeof(SBEncState));
   st->mode = m;
   mode = m->mode;

   st->st_low = nb_encoder_init(mode->nb_mode);
   st->full_frame_size = 2*mode->frameSize;
   st->frame_size = mode->frameSize;
   st->subframeSize = mode->subframeSize;
   st->nbSubframes = mode->frameSize/mode->subframeSize;
   st->windowSize = st->frame_size*3/2;
   st->lpcSize=mode->lpcSize;
   st->bufSize=mode->bufSize;

   st->submodes=mode->submodes;
   st->submodeID=mode->defaultSubmode;
   {
      int mod=6;
      speex_encoder_ctl(st->st_low, SPEEX_SET_MODE, &mod);
   }

   st->lag_factor = mode->lag_factor;
   st->lpc_floor = mode->lpc_floor;
   st->gamma1=mode->gamma1;
   st->gamma2=mode->gamma2;
   st->first=1;
   st->stack = calloc(10000, sizeof(float));

   st->x0=calloc(st->full_frame_size, sizeof(float));
   st->x1=calloc(st->full_frame_size, sizeof(float));
   st->x0d=calloc(st->frame_size, sizeof(float));
   st->x1d=calloc(st->frame_size, sizeof(float));
   st->high=calloc(st->full_frame_size, sizeof(float));
   st->y0=calloc(st->full_frame_size, sizeof(float));
   st->y1=calloc(st->full_frame_size, sizeof(float));

   st->h0_mem=calloc(QMF_ORDER, sizeof(float));
   st->h1_mem=calloc(QMF_ORDER, sizeof(float));
   st->g0_mem=calloc(QMF_ORDER, sizeof(float));
   st->g1_mem=calloc(QMF_ORDER, sizeof(float));

   st->buf=calloc(st->windowSize, sizeof(float));
   st->excBuf=calloc(st->bufSize, sizeof(float));
   st->exc = st->excBuf + st->bufSize - st->windowSize;
   /*st->exc=st->excBuf+st->frame_size;*/

   st->res=calloc(st->frame_size, sizeof(float));
   st->sw=calloc(st->frame_size, sizeof(float));
   st->target=calloc(st->frame_size, sizeof(float));
   /*Asymetric "pseudo-Hamming" window*/
   {
      int part1, part2;
      part1 = st->subframeSize*7/2;
      part2 = st->subframeSize*5/2;
      st->window = malloc(st->windowSize*sizeof(float));
      for (i=0;i<part1;i++)
         st->window[i]=.54-.46*cos(M_PI*i/part1);
      for (i=0;i<part2;i++)
         st->window[part1+i]=.54+.46*cos(M_PI*i/part2);
   }

   st->lagWindow = malloc((st->lpcSize+1)*sizeof(float));
   for (i=0;i<st->lpcSize+1;i++)
      st->lagWindow[i]=exp(-.5*sqr(2*M_PI*st->lag_factor*i));

   st->rc = malloc(st->lpcSize*sizeof(float));
   st->autocorr = malloc((st->lpcSize+1)*sizeof(float));
   st->lpc = malloc((st->lpcSize+1)*sizeof(float));
   st->bw_lpc1 = malloc((st->lpcSize+1)*sizeof(float));
   st->bw_lpc2 = malloc((st->lpcSize+1)*sizeof(float));
   st->lsp = malloc(st->lpcSize*sizeof(float));
   st->qlsp = malloc(st->lpcSize*sizeof(float));
   st->old_lsp = malloc(st->lpcSize*sizeof(float));
   st->old_qlsp = malloc(st->lpcSize*sizeof(float));
   st->interp_lsp = malloc(st->lpcSize*sizeof(float));
   st->interp_qlsp = malloc(st->lpcSize*sizeof(float));
   st->interp_lpc = malloc((st->lpcSize+1)*sizeof(float));
   st->interp_qlpc = malloc((st->lpcSize+1)*sizeof(float));

   st->mem_sp = calloc(st->lpcSize, sizeof(float));
   st->mem_sp2 = calloc(st->lpcSize, sizeof(float));
   st->mem_sw = calloc(st->lpcSize, sizeof(float));


   return st;
}

void sb_encoder_destroy(void *state)
{
   SBEncState *st=state;

   nb_encoder_destroy(st->st_low);
   free(st->x0);
   free(st->x0d);
   free(st->x1);
   free(st->x1d);
   free(st->high);
   free(st->y0);
   free(st->y1);
   free(st->h0_mem);
   free(st->h1_mem);
   free(st->g0_mem);
   free(st->g1_mem);
   
   free(st->buf);
   free(st->window);
   free(st->excBuf);
   free(st->sw);
   free(st->res);
   free(st->target);
   free(st->lagWindow);
   free(st->rc);
   free(st->autocorr);
   free(st->lpc);
   free(st->bw_lpc1);
   free(st->bw_lpc2);
   free(st->lsp);
   free(st->qlsp);
   free(st->old_lsp);
   free(st->old_qlsp);
   free(st->interp_lsp);
   free(st->interp_qlsp);
   free(st->interp_lpc);
   free(st->interp_qlpc);

   free(st->mem_sp);
   free(st->mem_sp2);
   free(st->mem_sw);

   free(st->stack);

   free(st);
}


void sb_encode(void *state, float *in, SpeexBits *bits)
{
   SBEncState *st;
   int i, roots, sub;

   st = state;

   /* Compute the two sub-bands by filtering with h0 and h1*/
   fir_mem(in, h0, st->x0, st->full_frame_size, QMF_ORDER, st->h0_mem);
   fir_mem(in, h1, st->x1, st->full_frame_size, QMF_ORDER, st->h1_mem);
   /* Down-sample x0 and x1 */
   for (i=0;i<st->frame_size;i++)
   {
      st->x0d[i]=st->x0[i<<1];
      st->x1d[i]=st->x1[i<<1];
   }
   /* Encode the narrowband part*/
   nb_encode(st->st_low, st->x0d, bits);

   speex_bits_pack(bits, st->submodeID, SB_SUBMODE_BITS);

   /* High-band buffering / sync with low band */
#if 0
   for (i=0;i<st->frame_size;i++)
   {
      /*st->excBuf[i]=st->exc[i];*/
      st->high[i]=st->high[st->frame_size+i];
      st->high[st->frame_size+i]=st->x1d[i];
   }
#else
   for (i=0;i<st->windowSize-st->frame_size;i++)
      st->high[i] = st->high[st->frame_size+i];
   for (i=0;i<st->frame_size;i++)
      st->high[st->windowSize-st->frame_size+i]=st->x1d[i];
#endif

   memmove(st->excBuf, st->excBuf+st->frame_size, (st->bufSize-st->frame_size)*sizeof(float));

   /* Start encoding the high-band */

   for (i=0;i<st->windowSize;i++)
      st->buf[i] = st->high[i] * st->window[i];

   /* Compute auto-correlation */
   autocorr(st->buf, st->autocorr, st->lpcSize+1, st->windowSize);

   st->autocorr[0] += 1;        /* prevents NANs */
   st->autocorr[0] *= st->lpc_floor; /* Noise floor in auto-correlation domain */
   /* Lag windowing: equivalent to filtering in the power-spectrum domain */
   for (i=0;i<st->lpcSize+1;i++)
      st->autocorr[i] *= st->lagWindow[i];

   /* Levinson-Durbin */
   wld(st->lpc+1, st->autocorr, st->rc, st->lpcSize);
   st->lpc[0]=1;

   /* LPC to LSPs (x-domain) transform */
   roots=lpc_to_lsp (st->lpc, st->lpcSize, st->lsp, 6, 0.002, st->stack);
   if (roots!=st->lpcSize)
   {
      fprintf (stderr, "roots!=st->lpcSize (found only %d roots)\n", roots);
      exit(1);
   }

   /* x-domain to angle domain*/
   for (i=0;i<st->lpcSize;i++)
      st->lsp[i] = acos(st->lsp[i]);

   /* LSP quantization */
   SUBMODE(lsp_quant)(st->lsp, st->qlsp, st->lpcSize, bits);
   
   /*printf ("high_lsp:");
   for (i=0;i<st->lpcSize;i++)
      printf (" %f", st->lsp[i]);
      printf ("\n");*/
   /*for (i=0;i<st->lpcSize;i++)
     st->qlsp[i]=st->lsp[i];*/
   

   if (st->first)
   {
      for (i=0;i<st->lpcSize;i++)
         st->old_lsp[i] = st->lsp[i];
      for (i=0;i<st->lpcSize;i++)
         st->old_qlsp[i] = st->qlsp[i];
   }
   
   for (sub=0;sub<st->nbSubframes;sub++)
   {
      float *exc, *sp, *mem, *res, *target, *sw, tmp, filter_ratio;
      int offset;
      float rl, rh, eh=0, el=0;
      int fold;

      offset = st->subframeSize*sub;
      sp=st->high+offset;
      exc=st->excBuf+offset;
      res=st->res+offset;
      target=st->target+offset;
      sw=st->sw+offset;

      mem=PUSH(st->stack, st->lpcSize);
      
      /* LSP interpolation (quantized and unquantized) */
      tmp = (1.0 + sub)/st->nbSubframes;
      for (i=0;i<st->lpcSize;i++)
         st->interp_lsp[i] = (1-tmp)*st->old_lsp[i] + tmp*st->lsp[i];
      for (i=0;i<st->lpcSize;i++)
         st->interp_qlsp[i] = (1-tmp)*st->old_qlsp[i] + tmp*st->qlsp[i];
      
      /* Compute interpolated LPCs (quantized and unquantized) */
      for (i=0;i<st->lpcSize;i++)
         st->interp_lsp[i] = cos(st->interp_lsp[i]);
      for (i=0;i<st->lpcSize;i++)
         st->interp_qlsp[i] = cos(st->interp_qlsp[i]);

      lsp_enforce_margin(st->interp_lsp, st->lpcSize, .002);
      lsp_enforce_margin(st->interp_qlsp, st->lpcSize, .002);

      lsp_to_lpc(st->interp_lsp, st->interp_lpc, st->lpcSize,st->stack);
      lsp_to_lpc(st->interp_qlsp, st->interp_qlpc, st->lpcSize, st->stack);

      bw_lpc(st->gamma1, st->interp_lpc, st->bw_lpc1, st->lpcSize);
      bw_lpc(st->gamma2, st->interp_lpc, st->bw_lpc2, st->lpcSize);

      /* Compute mid-band (4000 Hz for wideband) response of low-band and high-band
         filters */
      rl=rh=0;
      tmp=1;
      for (i=0;i<=st->lpcSize;i++)
      {
         rh += tmp*st->interp_qlpc[i];
         tmp = -tmp;
      }
      rl = ((EncState*)st->st_low)->pi_gain[sub];
      rl=1/(fabs(rl)+.01);
      rh=1/(fabs(rh)+.01);
      /* Compute ratio, will help predict the gain */
      filter_ratio=fabs(.01+rh)/(.01+fabs(rl));

      fold = filter_ratio<5;
      /*printf ("filter_ratio %f\n", filter_ratio);*/
      fold=0;

      /* Compute "real excitation" */
      residue_mem(sp, st->interp_qlpc, exc, st->subframeSize, st->lpcSize, st->mem_sp2);
      /* Compute energy of low-band and high-band excitation */
      for (i=0;i<st->subframeSize;i++)
         eh+=sqr(exc[i]);
      for (i=0;i<st->subframeSize;i++)
         el+=sqr(((EncState*)st->st_low)->exc[offset+i]);

      if (!SUBMODE(innovation_quant)) {/* 1 for spectral folding excitation, 0 for stochastic */
         float g;
         /*speex_bits_pack(bits, 1, 1);*/

#if 1
         /* Gain to use if we want to use the low-band excitation for high-band */
         g=eh/(.01+el);
         g=sqrt(g);

         g *= filter_ratio;
         /* Gain quantization */
         {
            int quant = (int) floor(.5 + 9.4 * log(10*(g+.0001)));
            if (quant<0)
               quant=0;
            if (quant>31)
               quant=31;
            speex_bits_pack(bits, quant, 5);
            g= .1*exp(quant/9.4);
         }
         /*printf ("folding gain: %f\n", g);*/
         g /= filter_ratio;

#if 0
         {
            float noise_gain;
            float *noise;
            float *alias;
            static int init=0;
            if (!init)
            {
               srand48(0);
               init=1;
            }
            noise_gain = 3*sqrt(el/st->subframeSize);
            noise = PUSH(st->stack, st->subframeSize);

            for (i=0;i<st->subframeSize;i++)
               noise[i]=noise_gain*(drand48()-.5);

            alias = ((EncState*)st->st_low)->exc+offset;
            /* Too lazy to do the memory properly */
            exc[0]=.5*.35*g*(alias[0]+noise[0]);
            for (i=1;i<st->subframeSize;i++)
            {
               exc[i]=.7*.5*g* (.8*(noise[i]+.8*noise[i-1]) + alias[i]-.8*alias[i-1]);
            }
            POP(st->stack);
         }
#else
         /* High-band excitation using the low-band excitation and a gain */
         /*FIXME: Should we replace the excitation in the encoder of just in the decoder?*/
         /*                  for (i=0;i<st->subframeSize;i++)
            exc[i]=g*((EncState*)st->st_low)->exc[offset+i];
         */
#endif

#endif
         /* Update the input signal using the non-coded memory */
         /* FIXME: is that right? */
         /*syn_filt_mem(exc, st->interp_qlpc, sp, st->subframeSize, st->lpcSize, st->mem_sp);*/

         /* FIXME: Update perceptually weighted signal in case we switch to the
            other mode */
      } else {
         float gc, scale, scale_1;
         float *innov;

         /*speex_bits_pack(bits, 0, 1);*/
         innov = PUSH(st->stack, st->subframeSize);


         gc = sqrt(1+eh)*filter_ratio/sqrt((1+el)*st->subframeSize);

         {
            int qgc = (int)floor(.5+3.7*(log(gc)+2));
            if (qgc<0)
               qgc=0;
            if (qgc>15)
               qgc=15;
            speex_bits_pack(bits, qgc, 4);
            gc = exp((1/3.7)*qgc-2);
         }

         scale = gc*sqrt(1+el)/filter_ratio;
         scale_1 = 1/scale;
         if (0 && rand()%5==0)
         {
            float sc = 1/sqrt(.1+eh/st->subframeSize);
            if (rand()&1)
               sc=-sc;
            for (i=0;i<st->subframeSize;i++)
            {
               float tmp=exc[i]*sc;
               if (i%8==0)
                  printf ("\nhexc");
               printf (" %f", tmp);
            }
         }

         /* Reset excitation */
         for (i=0;i<st->subframeSize;i++)
            exc[i]=0;
         
         /* Compute zero response (ringing) of A(z/g1) / ( A(z/g2) * Aq(z) ) */
         for (i=0;i<st->lpcSize;i++)
            mem[i]=st->mem_sp[i];
         syn_filt_mem(exc, st->interp_qlpc, exc, st->subframeSize, st->lpcSize, mem);
         for (i=0;i<st->lpcSize;i++)
            mem[i]=st->mem_sp[i];
         residue_mem(exc, st->bw_lpc1, res, st->subframeSize, st->lpcSize, mem);
         for (i=0;i<st->lpcSize;i++)
            mem[i]=st->mem_sw[i];
         syn_filt_mem(res, st->bw_lpc2, res, st->subframeSize, st->lpcSize, mem);
         
         /* Compute weighted signal */
         for (i=0;i<st->lpcSize;i++)
            mem[i]=st->mem_sp[i];
         residue_mem(sp, st->bw_lpc1, sw, st->subframeSize, st->lpcSize, mem);
         for (i=0;i<st->lpcSize;i++)
            mem[i]=st->mem_sw[i];
         syn_filt_mem(sw, st->bw_lpc2, sw, st->subframeSize, st->lpcSize, mem);
         
         /* Compute target signal */
         for (i=0;i<st->subframeSize;i++)
            target[i]=sw[i]-res[i];

         for (i=0;i<st->subframeSize;i++)
           exc[i]=0;


         for (i=0;i<st->subframeSize;i++)
            target[i]*=scale_1;
         
         /* Reset excitation */
         for (i=0;i<st->subframeSize;i++)
            innov[i]=0;

         /*print_vec(target, st->subframeSize, "\ntarget");*/
         SUBMODE(innovation_quant)(target, st->interp_qlpc, st->bw_lpc1, st->bw_lpc2, 
                                SUBMODE(innovation_params), st->lpcSize, st->subframeSize, 
                                innov, bits, st->stack);
         /*print_vec(target, st->subframeSize, "after");*/

         for (i=0;i<st->subframeSize;i++)
            exc[i] += innov[i]*scale;

         if (0) {
            float en=0;
            for (i=0;i<st->subframeSize;i++)
               en+=exc[i]*exc[i];
            en=sqrt(eh/(1+en));
            for (i=0;i<st->subframeSize;i++)
               exc[i]*=en;
            printf ("correction high: %f\n", en);
         }

         POP(st->stack);
      }
#if 1
         /*Keep the previous memory*/
         for (i=0;i<st->lpcSize;i++)
            mem[i]=st->mem_sp[i];
         /* Final signal synthesis from excitation */
         syn_filt_mem(exc, st->interp_qlpc, sp, st->subframeSize, st->lpcSize, st->mem_sp);
         
         /* Compute weighted signal again, from synthesized speech (not sure it's the right thing) */
         residue_mem(sp, st->bw_lpc1, sw, st->subframeSize, st->lpcSize, mem);
         syn_filt_mem(sw, st->bw_lpc2, sw, st->subframeSize, st->lpcSize, st->mem_sw);
#endif
      
      
      POP(st->stack);
   }


#ifndef RELEASE
   /* Up-sample coded low-band and high-band*/
   for (i=0;i<st->frame_size;i++)
   {
      st->x0[(i<<1)]=st->x0d[i];
      st->x1[(i<<1)]=st->high[i];
      st->x0[(i<<1)+1]=0;
      st->x1[(i<<1)+1]=0;
   }
   /* Reconstruct the original */
   fir_mem(st->x0, h0, st->y0, st->full_frame_size, QMF_ORDER, st->g0_mem);
   fir_mem(st->x1, h1, st->y1, st->full_frame_size, QMF_ORDER, st->g1_mem);
   for (i=0;i<st->full_frame_size;i++)
      in[i]=2*(st->y0[i]-st->y1[i]);
#endif
   for (i=0;i<st->lpcSize;i++)
      st->old_lsp[i] = st->lsp[i];
   for (i=0;i<st->lpcSize;i++)
      st->old_qlsp[i] = st->qlsp[i];

   st->first=0;
}





void *sb_decoder_init(SpeexMode *m)
{
   int i;
   SBDecState *st;
   SpeexSBMode *mode;
   st = malloc(sizeof(SBDecState));
   st->mode = m;
   mode=m->mode;

   st->st_low = nb_decoder_init(mode->nb_mode);
   st->full_frame_size = 2*mode->frameSize;
   st->frame_size = mode->frameSize;
   st->subframeSize = 40;
   st->nbSubframes = 4;
   st->lpcSize=8;
   st->pf_order=15;
   st->pf_gamma=.05;

   st->submodes=mode->submodes;
   st->submodeID=mode->defaultSubmode;

   st->first=1;
   st->stack = calloc(10000, sizeof(float));

   st->x0=calloc(st->full_frame_size, sizeof(float));
   st->x1=calloc(st->full_frame_size, sizeof(float));
   st->x0d=calloc(st->frame_size, sizeof(float));
   st->x1d=calloc(st->frame_size, sizeof(float));
   st->high=calloc(st->full_frame_size, sizeof(float));
   st->y0=calloc(st->full_frame_size, sizeof(float));
   st->y1=calloc(st->full_frame_size, sizeof(float));

   st->h0_mem=calloc(QMF_ORDER, sizeof(float));
   st->h1_mem=calloc(QMF_ORDER, sizeof(float));
   st->g0_mem=calloc(QMF_ORDER, sizeof(float));
   st->g1_mem=calloc(QMF_ORDER, sizeof(float));

   st->pf_exc=calloc(st->full_frame_size, sizeof(float));
   st->exc=calloc(st->frame_size, sizeof(float));
   st->pf_window=calloc(st->full_frame_size, sizeof(float));
   st->pf_autocorr=calloc(st->pf_order+1, sizeof(float));
   st->pf_lpc=calloc(st->pf_order+1, sizeof(float));
   st->pf_bwlpc=calloc(st->pf_order+1, sizeof(float));
   for (i=0;i<st->full_frame_size;i++)
      st->pf_window[i]=.5*(1-cos(2*M_PI*i/st->full_frame_size));

   st->qlsp = malloc(st->lpcSize*sizeof(float));
   st->old_qlsp = malloc(st->lpcSize*sizeof(float));
   st->interp_qlsp = malloc(st->lpcSize*sizeof(float));
   st->interp_qlpc = malloc((st->lpcSize+1)*sizeof(float));

   st->mem_sp = calloc(st->lpcSize, sizeof(float));
   st->mem_pf_exc1 = calloc(st->pf_order, sizeof(float));
   st->mem_pf_exc2 = calloc(st->pf_order, sizeof(float));
   st->mem_pf_sp = calloc(st->pf_order, sizeof(float));

   return st;
}

void sb_decoder_destroy(void *state)
{
   SBDecState *st;
   st = state;
   nb_decoder_destroy(st->st_low);
   free(st->x0);
   free(st->x0d);
   free(st->x1);
   free(st->x1d);
   free(st->high);
   free(st->y0);
   free(st->y1);
   free(st->h0_mem);
   free(st->h1_mem);
   free(st->g0_mem);
   free(st->g1_mem);
   
   free(st->exc);
   free(st->pf_exc);
   free(st->pf_window);
   free(st->pf_autocorr);
   free(st->pf_lpc);
   free(st->pf_bwlpc);
   free(st->qlsp);
   free(st->old_qlsp);
   free(st->interp_qlsp);
   free(st->interp_qlpc);

   free(st->mem_sp);
   free(st->mem_pf_exc1);
   free(st->mem_pf_exc2);
   free(st->mem_pf_sp);

   free(st->stack);

   free(state);
}


void sb_decode(void *state, SpeexBits *bits, float *out, int lost)
{
   int i, sub;
   SBDecState *st;
   
   st = state;
   /* Decode the low-band */
   nb_decode(st->st_low, bits, st->x0d, lost);

   st->submodeID = speex_bits_unpack_unsigned(bits, SB_SUBMODE_BITS);

   for (i=0;i<st->frame_size;i++)
      st->exc[i]=0;

   SUBMODE(lsp_unquant)(st->qlsp, st->lpcSize, bits);
   
   if (st->first)
   {
      for (i=0;i<st->lpcSize;i++)
         st->old_qlsp[i] = st->qlsp[i];
   }
   
   for (sub=0;sub<st->nbSubframes;sub++)
   {
      float *exc, *sp, tmp, filter_ratio, gain, el=0;
      int offset;
      
      offset = st->subframeSize*sub;
      sp=st->high+offset;
      exc=st->exc+offset;
      
      /* LSP interpolation */
      tmp = (1.0 + sub)/st->nbSubframes;
      for (i=0;i<st->lpcSize;i++)
         st->interp_qlsp[i] = (1-tmp)*st->old_qlsp[i] + tmp*st->qlsp[i];

      /* LSPs to x-domain */
      for (i=0;i<st->lpcSize;i++)
         st->interp_qlsp[i] = cos(st->interp_qlsp[i]);

      lsp_enforce_margin(st->interp_qlsp, st->lpcSize, .002);

      /* LSP to LPC */
      lsp_to_lpc(st->interp_qlsp, st->interp_qlpc, st->lpcSize, st->stack);

      {
         float rl=0, rh=0;
         tmp=1;
         for (i=0;i<=st->lpcSize;i++)
         {
            rh += tmp*st->interp_qlpc[i];
            tmp = -tmp;
         }
         rl = ((DecState*)st->st_low)->pi_gain[sub];
         rl=1/(fabs(rl)+.01);
         rh=1/(fabs(rh)+.01);
         filter_ratio=fabs(.01+rh)/(.01+fabs(rl));
      }

      for (i=0;i<st->subframeSize;i++)
         el+=sqr(((DecState*)st->st_low)->exc[offset+i]);
      gain=(1+sqrt(el/st->subframeSize))/filter_ratio;
      
      for (i=0;i<st->subframeSize;i++)
         exc[i]=0;
      if (!SUBMODE(innovation_unquant))
      {
         float g;
         int quant;
         quant = speex_bits_unpack_unsigned(bits, 5);
         g= .1*exp(quant/9.4);
         
         /*printf ("unquant folding gain: %f\n", g);*/
         g /= filter_ratio;
         
         /* High-band excitation using the low-band excitation and a gain */
         for (i=0;i<st->subframeSize;i++)
            exc[i]=.6*g*((DecState*)st->st_low)->exc[offset+i];
      } else {
         float gc, scale;
         int qgc = speex_bits_unpack_unsigned(bits, 4);

         gc = exp((1/3.7)*qgc-2);

         scale = gc*sqrt(1+el)/filter_ratio;


         SUBMODE(innovation_unquant)(exc, SUBMODE(innovation_params), st->subframeSize, 
                                bits, st->stack);
         for (i=0;i<st->subframeSize;i++)
            exc[i]*=scale;
      }
      syn_filt_mem(exc, st->interp_qlpc, sp, st->subframeSize, st->lpcSize, st->mem_sp);

   }

   /* Up-sample coded low-band and high-band*/
   for (i=0;i<st->frame_size;i++)
   {
      st->x0[(i<<1)]=st->x0d[i];
      st->x1[(i<<1)]=st->high[i];
      st->x0[(i<<1)+1]=0;
      st->x1[(i<<1)+1]=0;
   }
   /* Reconstruct the original */
   fir_mem(st->x0, h0, st->y0, st->full_frame_size, QMF_ORDER, st->g0_mem);
   fir_mem(st->x1, h1, st->y1, st->full_frame_size, QMF_ORDER, st->g1_mem);
   for (i=0;i<st->full_frame_size;i++)
      out[i]=2*(st->y0[i]-st->y1[i]);


   if (0)
   {
      float tmp=1, e1=0, e2=0, g;
      for (i=0;i<st->full_frame_size;i++)
         st->pf_exc[i] = out[i] * st->pf_window[i];
      
      /* Compute auto-correlation */
      autocorr(st->pf_exc, st->pf_autocorr, st->pf_order+1, st->full_frame_size);
      
      st->pf_autocorr[0] += 1;        /* prevents NANs */
      st->pf_autocorr[0] *= 1.0001;    /* Noise floor in auto-correlation domain */
      
      /* Levinson-Durbin */
      wld(st->pf_lpc+1, st->pf_autocorr, st->pf_exc, st->pf_order);
      st->pf_lpc[0]=1;
      
      for (i=1;i<st->pf_order;i++)
      {
         tmp*=st->pf_gamma;
         st->pf_bwlpc[i] = st->pf_lpc[i]*tmp;
      }
      st->pf_bwlpc[0]=1;
      
      /*print_vec(st->pf_lpc, st->pf_order, "post-filter LPC");*/
      residue_mem(out, st->pf_lpc, st->pf_exc, st->full_frame_size, 
                  st->pf_order, st->mem_pf_exc1);
      for (i=0;i<st->full_frame_size;i++)
         e1 += st->pf_exc[i]*st->pf_exc[i];
      syn_filt_mem(st->pf_exc, st->pf_bwlpc, st->pf_exc, st->full_frame_size, 
                  st->pf_order, st->mem_pf_exc2);
      for (i=0;i<st->full_frame_size;i++)
         e2 += st->pf_exc[i]*st->pf_exc[i];
      g=sqrt(e1/(e2+.1));
      /*printf ("post-filter gain: %f\n", g);*/
      for (i=0;i<st->full_frame_size;i++)
         st->pf_exc[i]=g*st->pf_exc[i];

      syn_filt_mem(st->pf_exc, st->pf_lpc, out, st->full_frame_size, 
                  st->pf_order, st->mem_pf_sp);
   }


   for (i=0;i<st->lpcSize;i++)
      st->old_qlsp[i] = st->qlsp[i];

   st->first=0;

}


void sb_encoder_ctl(void *state, int request, void *ptr)
{
   SBEncState *st;
   st=state;
   switch(request)
   {
   case SPEEX_GET_FRAME_SIZE:
      (*(int*)ptr) = st->full_frame_size;
      break;
   case SPEEX_SET_HIGH_MODE:
      st->submodeID = (*(int*)ptr);
      break;
   case SPEEX_SET_LOW_MODE:
      speex_encoder_ctl(st->st_low, SPEEX_SET_MODE, ptr);
      break;
   case SPEEX_SET_VBR:
      speex_encoder_ctl(st->st_low, SPEEX_SET_VBR, ptr);
      break;
   case SPEEX_SET_VBR_QUALITY:
      {
         int qual = (*(int*)ptr)+1;
         if (qual>10)
            qual=10;
         speex_encoder_ctl(st->st_low, SPEEX_SET_VBR_QUALITY, &qual);
         speex_encoder_ctl(state, SPEEX_SET_QUALITY, ptr);
         break;
      }
   case SPEEX_SET_QUALITY:
      {
         int nb_mode;
         int quality = (*(int*)ptr);
         switch (quality)
         {
         case 0:
            nb_mode=1;
            st->submodeID = 1;
            break;
         case 1:
            nb_mode=2;
            st->submodeID = 1;
            break;
         case 2:
            nb_mode=3;
            st->submodeID = 1;
            break;
         case 3:
            nb_mode=4;
            st->submodeID = 1;
            break;
         case 4:
            nb_mode=4;
            st->submodeID = 2;
            break;
         case 5:
            nb_mode=5;
            st->submodeID = 2;
            break;
         case 6:
         case 7:
            nb_mode=6;
            st->submodeID = 2;
            break;
         case 8:
         case 9:
         case 10:
            nb_mode=6;
            st->submodeID = 3;
            break;
         default:
            fprintf(stderr, "Unknown sb_ctl quality: %d\n", quality);
         }
         speex_encoder_ctl(st->st_low, SPEEX_SET_MODE, &nb_mode);
      }
      break;
   default:
      fprintf(stderr, "Unknown nb_ctl request: %d\n", request);
   }

}

void sb_decoder_ctl(void *state, int request, void *ptr)
{
   SBDecState *st;
   st=state;
   switch(request)
   {
   case SPEEX_GET_FRAME_SIZE:
      (*(int*)ptr) = st->full_frame_size;
      break;
   case SPEEX_SET_PF:
      speex_decoder_ctl(st->st_low, request, ptr);
      break;
   default:
      fprintf(stderr, "Unknown sb_ctl request: %d\n", request);
   }

}