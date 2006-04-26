/* Copyright (C) 2002 Jean-Marc Valin 
   File: filters.c
   Various analysis/synthesis filters

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "filters.h"
#include "stack_alloc.h"
#include "misc.h"
#include "math_approx.h"
#include "ltp.h"
#include <math.h>

#ifdef _USE_SSE
#include "filters_sse.h"
#elif defined (ARM4_ASM) || defined(ARM5E_ASM)
#include "filters_arm4.h"
#elif defined (BFIN_ASM)
#include "filters_bfin.h"
#endif



void bw_lpc(spx_word16_t gamma, const spx_coef_t *lpc_in, spx_coef_t *lpc_out, int order)
{
   int i;
   spx_word16_t tmp=gamma;
   for (i=0;i<order;i++)
   {
      lpc_out[i] = MULT16_16_P15(tmp,lpc_in[i]);
      tmp = MULT16_16_P15(tmp, gamma);
   }
}


#ifdef FIXED_POINT

/* FIXME: These functions are ugly and probably introduce too much error */
void signal_mul(const spx_sig_t *x, spx_sig_t *y, spx_word32_t scale, int len)
{
   int i;
   for (i=0;i<len;i++)
   {
      y[i] = SHL32(MULT16_32_Q14(EXTRACT16(SHR32(x[i],7)),scale),7);
   }
}

void signal_div(const spx_sig_t *x, spx_sig_t *y, spx_word32_t scale, int len)
{
   int i;
   if (scale > SHL32(EXTEND32(SIG_SCALING), 8))
   {
      spx_word16_t scale_1;
      scale = PSHR32(scale, SIG_SHIFT);
      scale_1 = EXTRACT16(DIV32_16(SHL32(EXTEND32(SIG_SCALING),7),scale));
      for (i=0;i<len;i++)
      {
         y[i] = SHR32(MULT16_16(scale_1, EXTRACT16(SHR32(x[i],SIG_SHIFT))),7);
      }
   } else {
      spx_word16_t scale_1;
      scale = PSHR32(scale, SIG_SHIFT-5);
      scale_1 = DIV32_16(SHL32(EXTEND32(SIG_SCALING),3),scale);
      for (i=0;i<len;i++)
      {
         y[i] = MULT16_16(scale_1, EXTRACT16(SHR32(x[i],SIG_SHIFT-2)));
      }
   }
}

#else

void signal_mul(const spx_sig_t *x, spx_sig_t *y, spx_word32_t scale, int len)
{
   int i;
   for (i=0;i<len;i++)
      y[i] = scale*x[i];
}

void signal_div(const spx_sig_t *x, spx_sig_t *y, spx_word32_t scale, int len)
{
   int i;
   float scale_1 = 1/scale;
   for (i=0;i<len;i++)
      y[i] = scale_1*x[i];
}
#endif



#ifdef FIXED_POINT



spx_word16_t compute_rms(const spx_sig_t *x, int len)
{
   int i;
   spx_word32_t sum=0;
   spx_sig_t max_val=1;
   int sig_shift;

   for (i=0;i<len;i++)
   {
      spx_sig_t tmp = x[i];
      if (tmp<0)
         tmp = -tmp;
      if (tmp > max_val)
         max_val = tmp;
   }

   sig_shift=0;
   while (max_val>16383)
   {
      sig_shift++;
      max_val >>= 1;
   }

   for (i=0;i<len;i+=4)
   {
      spx_word32_t sum2=0;
      spx_word16_t tmp;
      tmp = EXTRACT16(SHR32(x[i],sig_shift));
      sum2 = MAC16_16(sum2,tmp,tmp);
      tmp = EXTRACT16(SHR32(x[i+1],sig_shift));
      sum2 = MAC16_16(sum2,tmp,tmp);
      tmp = EXTRACT16(SHR32(x[i+2],sig_shift));
      sum2 = MAC16_16(sum2,tmp,tmp);
      tmp = EXTRACT16(SHR32(x[i+3],sig_shift));
      sum2 = MAC16_16(sum2,tmp,tmp);
      sum = ADD32(sum,SHR32(sum2,6));
   }
   
   return EXTRACT16(PSHR32(SHL32(EXTEND32(spx_sqrt(1+DIV32(sum,len))),(sig_shift+3)),SIG_SHIFT));
}

spx_word16_t compute_rms16(const spx_word16_t *x, int len)
{
   int i;
   spx_word32_t sum=0;
   for (i=0;i<len;i+=4)
   {
      spx_word32_t sum2=0;
      sum2 = MAC16_16(sum2,x[i],x[i]);
      sum2 = MAC16_16(sum2,x[i+1],x[i+1]);
      sum2 = MAC16_16(sum2,x[i+2],x[i+2]);
      sum2 = MAC16_16(sum2,x[i+3],x[i+3]);
      sum = ADD32(sum,SHR32(sum2,6));
   }
   
   return SHL16(spx_sqrt(1+DIV32(sum,len)),3);
}

#ifndef OVERRIDE_NORMALIZE16
int normalize16(const spx_sig_t *x, spx_word16_t *y, spx_sig_t max_scale, int len)
{
   int i;
   spx_sig_t max_val=1;
   int sig_shift;
   
   for (i=0;i<len;i++)
   {
      spx_sig_t tmp = x[i];
      if (tmp<0)
         tmp = NEG32(tmp);
      if (tmp >= max_val)
         max_val = tmp;
   }

   sig_shift=0;
   while (max_val>max_scale)
   {
      sig_shift++;
      max_val >>= 1;
   }

   for (i=0;i<len;i++)
      y[i] = EXTRACT16(SHR32(x[i], sig_shift));
   
   return sig_shift;
}
#endif

#else

spx_word16_t compute_rms(const spx_sig_t *x, int len)
{
   int i;
   float sum=0;
   for (i=0;i<len;i++)
   {
      sum += x[i]*x[i];
   }
   return sqrt(.1+sum/len);
}
spx_word16_t compute_rms16(const spx_word16_t *x, int len)
{
   return compute_rms(x, len);
}
#endif



#ifndef OVERRIDE_FILTER_MEM2
#ifdef PRECISION16
void filter_mem2(const spx_sig_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_sig_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word16_t xi,yi,nyi;

   for (i=0;i<N;i++)
   {
      xi= EXTRACT16(PSHR32(SATURATE(x[i],536870911),SIG_SHIFT));
      yi = EXTRACT16(PSHR32(SATURATE(ADD32(x[i], SHL32(mem[0],1)),536870911),SIG_SHIFT));
      nyi = NEG16(yi);
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_16(MAC16_16(mem[j+1], num[j],xi), den[j],nyi);
      }
      mem[ord-1] = ADD32(MULT16_16(num[ord-1],xi), MULT16_16(den[ord-1],nyi));
      y[i] = SHL32(EXTEND32(yi),SIG_SHIFT);
   }
}
#else
void filter_mem2(const spx_sig_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_sig_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_sig_t xi,yi,nyi;

   for (i=0;i<ord;i++)
      mem[i] = SHR32(mem[i],1);   
   for (i=0;i<N;i++)
   {
      xi=SATURATE(x[i],805306368);
      yi = SATURATE(ADD32(xi, SHL32(mem[0],2)),805306368);
      nyi = NEG32(yi);
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_32_Q15(MAC16_32_Q15(mem[j+1], num[j],xi), den[j],nyi);
      }
      mem[ord-1] = SUB32(MULT16_32_Q15(num[ord-1],xi), MULT16_32_Q15(den[ord-1],yi));
      y[i] = yi;
   }
   for (i=0;i<ord;i++)
      mem[i] = SHL32(mem[i],1);   
}
#endif
#endif

#ifdef FIXED_POINT
void filter_mem16(const spx_word16_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word16_t xi,yi,nyi;
   for (i=0;i<N;i++)
   {
      xi= x[i];
      yi = EXTRACT16(SATURATE(ADD32(EXTEND32(x[i]),PSHR32(mem[0],LPC_SHIFT)),32767));
      nyi = NEG16(yi);
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_16(MAC16_16(mem[j+1], num[j],xi), den[j],nyi);
      }
      mem[ord-1] = ADD32(MULT16_16(num[ord-1],xi), MULT16_16(den[ord-1],nyi));
      y[i] = yi;
   }
}
#else
void filter_mem16(const spx_word16_t *x, const spx_coef_t *num, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem)
{
   filter_mem2(x, num, den, y, N, ord, mem);
}
#endif


#ifndef OVERRIDE_IIR_MEM2
#ifdef PRECISION16
void iir_mem2(const spx_sig_t *x, const spx_coef_t *den, spx_sig_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word16_t yi,nyi;

   for (i=0;i<N;i++)
   {
      yi = EXTRACT16(PSHR32(SATURATE(x[i] + SHL32(mem[0],1),536870911),SIG_SHIFT));
      nyi = NEG16(yi);
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_16(mem[j+1],den[j],nyi);
      }
      mem[ord-1] = MULT16_16(den[ord-1],nyi);
      y[i] = SHL32(EXTEND32(yi),SIG_SHIFT);
   }
}
#else
void iir_mem2(const spx_sig_t *x, const spx_coef_t *den, spx_sig_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word32_t xi,yi,nyi;

   for (i=0;i<ord;i++)
      mem[i] = SHR32(mem[i],1);   
   for (i=0;i<N;i++)
   {
      xi=SATURATE(x[i],805306368);
      yi = SATURATE(xi + SHL32(mem[0],2),805306368);
      nyi = NEG32(yi);
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_32_Q15(mem[j+1],den[j],nyi);
      }
      mem[ord-1] = MULT16_32_Q15(den[ord-1],nyi);
      y[i] = yi;
   }
   for (i=0;i<ord;i++)
      mem[i] = SHL32(mem[i],1);   
}
#endif
#endif

#ifdef FIXED_POINT
void iir_mem16(const spx_word16_t *x, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word16_t yi,nyi;

   for (i=0;i<N;i++)
   {
      yi = EXTRACT16(SATURATE(ADD32(EXTEND32(x[i]),PSHR32(mem[0],LPC_SHIFT)),32767));
      nyi = NEG16(yi);
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_16(mem[j+1],den[j],nyi);
      }
      mem[ord-1] = MULT16_16(den[ord-1],nyi);
      y[i] = yi;
   }
}
#else
void iir_mem16(const spx_word16_t *x, const spx_coef_t *den, spx_word16_t *y, int N, int ord, spx_mem_t *mem)
{
   iir_mem2(x, den, y, N, ord, mem);
}
#endif


#ifndef OVERRIDE_FIR_MEM2
#ifdef PRECISION16
void fir_mem2(const spx_sig_t *x, const spx_coef_t *num, spx_sig_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word16_t xi,yi;

   for (i=0;i<N;i++)
   {
      xi= EXTRACT16(PSHR32(SATURATE(x[i],536870911),SIG_SHIFT));
      yi = EXTRACT16(PSHR32(SATURATE(x[i] + SHL32(mem[0],1),536870911),SIG_SHIFT));
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_16(mem[j+1], num[j],xi);
      }
      mem[ord-1] = MULT16_16(num[ord-1],xi);
      y[i] = SHL32(EXTEND32(yi),SIG_SHIFT);
   }
}
#else
void fir_mem2(const spx_sig_t *x, const spx_coef_t *num, spx_sig_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word32_t xi,yi;

   for (i=0;i<ord;i++)
      mem[i] = SHR32(mem[i],1);   
   for (i=0;i<N;i++)
   {
      xi=SATURATE(x[i],805306368);
      yi = xi + SHL32(mem[0],2);
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_32_Q15(mem[j+1], num[j],xi);
      }
      mem[ord-1] = MULT16_32_Q15(num[ord-1],xi);
      y[i] = SATURATE(yi,805306368);
   }
   for (i=0;i<ord;i++)
      mem[i] = SHL32(mem[i],1);   
}
#endif
#endif

#ifdef FIXED_POINT
void fir_mem16(const spx_word16_t *x, const spx_coef_t *num, spx_word16_t *y, int N, int ord, spx_mem_t *mem)
{
   int i,j;
   spx_word16_t xi,yi;

   for (i=0;i<N;i++)
   {
      xi=x[i];
      yi = EXTRACT16(SATURATE(ADD32(EXTEND32(x[i]),PSHR32(mem[0],LPC_SHIFT)),32767));
      for (j=0;j<ord-1;j++)
      {
         mem[j] = MAC16_16(mem[j+1], num[j],xi);
      }
      mem[ord-1] = MULT16_16(num[ord-1],xi);
      y[i] = yi;
   }
}
#else
void fir_mem16(const spx_word16_t *x, const spx_coef_t *num, spx_word16_t *y, int N, int ord, spx_mem_t *mem)
{
   fir_mem2(x, num, y, N, ord, mem);
}
#endif






void syn_percep_zero(const spx_sig_t *xx, const spx_coef_t *ak, const spx_coef_t *awk1, const spx_coef_t *awk2, spx_sig_t *y, int N, int ord, char *stack)
{
   int i;
   VARDECL(spx_mem_t *mem);
   ALLOC(mem, ord, spx_mem_t);
   for (i=0;i<ord;i++)
     mem[i]=0;
   iir_mem2(xx, ak, y, N, ord, mem);
   for (i=0;i<ord;i++)
      mem[i]=0;
   filter_mem2(y, awk1, awk2, y, N, ord, mem);
}

void residue_percep_zero(const spx_sig_t *xx, const spx_coef_t *ak, const spx_coef_t *awk1, const spx_coef_t *awk2, spx_sig_t *y, int N, int ord, char *stack)
{
   int i;
   VARDECL(spx_mem_t *mem);
   ALLOC(mem, ord, spx_mem_t);
   for (i=0;i<ord;i++)
      mem[i]=0;
   filter_mem2(xx, ak, awk1, y, N, ord, mem);
   for (i=0;i<ord;i++)
     mem[i]=0;
   fir_mem2(y, awk2, y, N, ord, mem);
}

#ifndef OVERRIDE_COMPUTE_IMPULSE_RESPONSE
void compute_impulse_response(const spx_coef_t *ak, const spx_coef_t *awk1, const spx_coef_t *awk2, spx_word16_t *y, int N, int ord, char *stack)
{
   int i,j;
   spx_word16_t y1, ny1i, ny2i;
   VARDECL(spx_mem_t *mem1);
   VARDECL(spx_mem_t *mem2);
   ALLOC(mem1, ord, spx_mem_t);
   ALLOC(mem2, ord, spx_mem_t);
   
   y[0] = LPC_SCALING;
   for (i=0;i<ord;i++)
      y[i+1] = awk1[i];
   i++;
   for (;i<N;i++)
      y[i] = VERY_SMALL;
   
   for (i=0;i<ord;i++)
      mem1[i] = mem2[i] = 0;
   for (i=0;i<N;i++)
   {
      y1 = ADD16(y[i], EXTRACT16(PSHR32(mem1[0],LPC_SHIFT)));
      ny1i = NEG16(y1);
      y[i] = ADD16(SHL16(y1,1), EXTRACT16(PSHR32(mem2[0],LPC_SHIFT)));
      ny2i = NEG16(y[i]);
      for (j=0;j<ord-1;j++)
      {
         mem1[j] = MAC16_16(mem1[j+1], awk2[j],ny1i);
         mem2[j] = MAC16_16(mem2[j+1], ak[j],ny2i);
      }
      mem1[ord-1] = MULT16_16(awk2[ord-1],ny1i);
      mem2[ord-1] = MULT16_16(ak[ord-1],ny2i);
   }
}
#endif

void qmf_decomp(const spx_word16_t *xx, const spx_word16_t *aa, spx_sig_t *y1, spx_sig_t *y2, int N, int M, spx_word16_t *mem, char *stack)
{
   int i,j,k,M2;
   VARDECL(spx_word16_t *a);
   VARDECL(spx_word16_t *x);
   spx_word16_t *x2;
   
   ALLOC(a, M, spx_word16_t);
   ALLOC(x, N+M-1, spx_word16_t);
   x2=x+M-1;
   M2=M>>1;
   for (i=0;i<M;i++)
      a[M-i-1]= aa[i];

   for (i=0;i<M-1;i++)
      x[i]=mem[M-i-2];
   for (i=0;i<N;i++)
      x[i+M-1]=SATURATE(PSHR(xx[i],1),16383);
   for (i=0,k=0;i<N;i+=2,k++)
   {
      y1[k]=0;
      y2[k]=0;
      for (j=0;j<M2;j++)
      {
         y1[k]=ADD32(y1[k],MULT16_16(a[j],ADD16(x[i+j],x2[i-j])));
         y2[k]=SUB32(y2[k],MULT16_16(a[j],SUB16(x[i+j],x2[i-j])));
         j++;
         y1[k]=ADD32(y1[k],MULT16_16(a[j],ADD16(x[i+j],x2[i-j])));
         y2[k]=ADD32(y2[k],MULT16_16(a[j],SUB16(x[i+j],x2[i-j])));
      }
      y1[k] = SHR32(y1[k],1);
      y2[k] = SHR32(y2[k],1);
   }
   for (i=0;i<M-1;i++)
     mem[i]=SATURATE(PSHR(xx[N-i-1],1),16383);
}


/* By segher */
void fir_mem_up(const spx_sig_t *x, const spx_word16_t *a, spx_sig_t *y, int N, int M, spx_word32_t *mem, char *stack)
   /* assumptions:
      all odd x[i] are zero -- well, actually they are left out of the array now
      N and M are multiples of 4 */
{
   int i, j;
   VARDECL(spx_word16_t *xx);
   
   ALLOC(xx, M+N-1, spx_word16_t);

   for (i = 0; i < N/2; i++)
      xx[2*i] = PSHR(x[N/2-1-i],SIG_SHIFT+1);
   for (i = 0; i < M - 1; i += 2)
      xx[N+i] = mem[i+1];

   for (i = 0; i < N; i += 4) {
      spx_sig_t y0, y1, y2, y3;
      spx_word16_t x0;

      y0 = y1 = y2 = y3 = 0;
      x0 = xx[N-4-i];

      for (j = 0; j < M; j += 4) {
         spx_word16_t x1;
         spx_word16_t a0, a1;

         a0 = a[j];
         a1 = a[j+1];
         x1 = xx[N-2+j-i];

         y0 = ADD32(y0,SHR(MULT16_16(a0, x1),1));
         y1 = ADD32(y1,SHR(MULT16_16(a1, x1),1));
         y2 = ADD32(y2,SHR(MULT16_16(a0, x0),1));
         y3 = ADD32(y3,SHR(MULT16_16(a1, x0),1));

         a0 = a[j+2];
         a1 = a[j+3];
         x0 = xx[N+j-i];

         y0 = ADD32(y0,SHR(MULT16_16(a0, x0),1));
         y1 = ADD32(y1,SHR(MULT16_16(a1, x0),1));
         y2 = ADD32(y2,SHR(MULT16_16(a0, x1),1));
         y3 = ADD32(y3,SHR(MULT16_16(a1, x1),1));
      }
      y[i] = y0;
      y[i+1] = y1;
      y[i+2] = y2;
      y[i+3] = y3;
   }

   for (i = 0; i < M - 1; i += 2)
      mem[i+1] = xx[i];
}

#ifdef NEW_ENHANCER

#ifdef FIXED_POINT
#if 0
spx_word16_t shift_filt[3][7] = {{-33,    1043,   -4551,   19959,   19959,   -4551,    1043},
                                 {-98,    1133,   -4425,   29179,    8895,   -2328,     444},
                                 {444,   -2328,    8895,   29179,   -4425,    1133,     -98}};
#else
spx_word16_t shift_filt[3][7] = {{-390,    1540,   -4993,   20123,   20123,   -4993,    1540},
                                {-1064,    2817,   -6694,   31589,    6837,    -990,    -209},
                                 {-209,    -990,    6837,   31589,   -6694,    2817,   -1064}};
#endif
#else
#if 0
float shift_filt[3][7] = {{-9.9369e-04, 3.1831e-02, -1.3889e-01, 6.0910e-01, 6.0910e-01, -1.3889e-01, 3.1831e-02},
                          {-0.0029937, 0.0345613, -0.1350474, 0.8904793, 0.2714479, -0.0710304, 0.0135403},
                          {0.0135403, -0.0710304, 0.2714479, 0.8904793, -0.1350474, 0.0345613,  -0.0029937}};
#else
float shift_filt[3][7] = {{-0.011915, 0.046995, -0.152373, 0.614108, 0.614108, -0.152373, 0.046995},
                          {-0.0324855, 0.0859768, -0.2042986, 0.9640297, 0.2086420, -0.0302054, -0.0063646},
                          {-0.0063646, -0.0302054, 0.2086420, 0.9640297, -0.2042986, 0.0859768, -0.0324855}};
#endif
#endif

int interp_pitch(
spx_word16_t *exc,          /*decoded excitation*/
spx_word16_t *interp,          /*decoded excitation*/
int pitch,               /*pitch period*/
int len
)
{
   int i,j,k;
   spx_word32_t corr[4][7];
   spx_word32_t maxcorr;
   int maxi, maxj;
   for (i=0;i<7;i++)
   {
      corr[0][i] = inner_prod(exc, exc-pitch-3+i, len);
   }
   for (i=0;i<3;i++)
   {
      for (j=0;j<7;j++)
      {
         spx_word32_t tmp=0;
         for (k=0;k<7;k++)
         {
            if (j+k-3 >= 0 && j+k-3 < 7)
               tmp += MULT16_32_Q15(shift_filt[i][k],corr[0][j+k-3]);
         }
         corr[i+1][j] = tmp;
      }
   }
   maxi=maxj=0;
   maxcorr = corr[0][0];
   for (i=0;i<4;i++)
   {
      for (j=0;j<7;j++)
      {
         if (corr[i][j] > maxcorr)
         {
            maxcorr = corr[i][j];
            maxi=i;
            maxj=j;
         }
      }
   }
   for (i=0;i<len;i++)
   {
      spx_word32_t tmp = 0;
      if (maxi>0)
      {
         for (k=0;k<7;k++)
         {
            tmp += MULT16_16(exc[i-(pitch-maxj+3)+k-3],shift_filt[maxi-1][k]);
         }
      } else {
         tmp = SHL32(exc[i-(pitch-maxj+3)],15);
      }
      interp[i] = PSHR32(tmp,15);
   }
   return pitch-maxj+3;
}
      
#ifdef FIXED_POINT
#define GSCALE 256.
#else
#define GSCALE 1.
#endif

void multicomb(
spx_sig_t *_exc,          /*decoded excitation*/
spx_word16_t *new_exc,      /*enhanced excitation*/
spx_coef_t *ak,           /*LPC filter coefs*/
int p,               /*LPC order*/
int nsf,             /*sub-frame size*/
int pitch,           /*pitch period*/
spx_word16_t *pitch_gain,   /*pitch gain (3-tap)*/
spx_word16_t  comb_gain,    /*gain of comb filter*/
char *stack
)
{
   int i; 
   spx_word16_t iexc[3*nsf];
   spx_word16_t old_ener, new_ener;
   int corr_pitch;
   
   int nol_pitch[6];
   spx_word16_t nol_pitch_coef[6];
   spx_word16_t ol_pitch_coef;
   spx_word16_t *exc;
   spx_word16_t iexc0_mag, iexc1_mag, exc_mag;
   spx_word32_t corr0, corr1;
   spx_word16_t gain0, gain1;
#ifdef FIXED_POINT
   VARDECL(spx_word16_t *exc2);
   /* FIXME: Can it get uglier than that??? */
   ALLOC(exc2, 510, spx_word16_t);
   for (i=0;i<510;i++)
      exc2[i] = 0;
   exc = exc2+300;
   for (i=-280;i<200;i++)
      exc[i] = PSHR32(_exc[i], SIG_SHIFT);
#else
   exc = _exc;
#endif
   open_loop_nbest_pitch(exc, 20, 120, nsf, 
                         nol_pitch, nol_pitch_coef, 6, stack);
   corr_pitch=nol_pitch[0];
   ol_pitch_coef = nol_pitch_coef[0];
   /*Try to remove pitch multiples*/
   for (i=1;i<6;i++)
   {
#ifdef FIXED_POINT
      if ((nol_pitch_coef[i]>MULT16_16_Q15(nol_pitch_coef[0],19661)) && 
#else
      if ((nol_pitch_coef[i]>.6*nol_pitch_coef[0]) && 
#endif
         (ABS(2*nol_pitch[i]-corr_pitch)<=2 || ABS(3*nol_pitch[i]-corr_pitch)<=3 || 
         ABS(4*nol_pitch[i]-corr_pitch)<=4 || ABS(5*nol_pitch[i]-corr_pitch)<=5))
      {
         corr_pitch = nol_pitch[i];
      }
   }
   interp_pitch(exc, iexc, corr_pitch, 80);
   if (corr_pitch>40)
      interp_pitch(exc, iexc+nsf, 2*corr_pitch, 80);
   else
      interp_pitch(exc, iexc+nsf, -corr_pitch, 80);

   interp_pitch(exc, iexc+2*nsf, 2*corr_pitch, 80);
   
   /*printf ("%d %d %f\n", pitch, corr_pitch, max_corr*ener_1);*/
   iexc0_mag = spx_sqrt(1000+inner_prod(iexc,iexc,nsf));
   iexc1_mag = spx_sqrt(1000+inner_prod(iexc+nsf,iexc+nsf,nsf));
   exc_mag = spx_sqrt(1+inner_prod(exc,exc,nsf));
   corr0  = inner_prod(iexc,exc,nsf);
   corr1 = inner_prod(iexc+nsf,exc,nsf);
   float gg1 = 1.*exc_mag/iexc0_mag;
   float gg2 = 1.*exc_mag/iexc1_mag;
   float pgain1 = 1.*corr0/iexc0_mag/exc_mag;
   float pgain2 = 1.*corr1/iexc1_mag/exc_mag;
   if (pgain1<0)
      pgain1=0;
   if (pgain2<0)
      pgain2=0;
   if (pgain1>.99)
      pgain1=.99;
   if (pgain2>.99)
      pgain2=.99;
   float c1, c2;
   float g1, g2;
   float ngain;
   if (comb_gain>0)
   {
#ifdef FIXED_POINT
      c1 = (MULT16_16_Q15(QCONST16(.4,15),comb_gain)+QCONST16(.07,15));
      c2 = .5+1.72*(c1/32768.-.07);
#else
      c1 = .4*comb_gain+.07;
      c2 = .5+1.72*(c1-.07);
#endif
   } else 
   {
      c1=c2=0;
   }
   g1 = 32768.*(1-c2*pgain1*pgain1);
   if (g1<c1)
      g1 = c1;
   g2 = 32768.*(1-c2*pgain2*pgain2);
   if (g2<c1)
      g2 = c1;
   g1 = c1/g1;
   g2 = c1/g2;
   if (corr_pitch>40)
   {
      gain0 = GSCALE*.7*g1*gg1;
      gain1 = GSCALE*.3*g2*gg2;
   } else {
      gain0 = GSCALE*.6*g1*gg1;
      gain1 = GSCALE*.6*g2*gg2;
   }
   for (i=0;i<nsf;i++)
      new_exc[i] = ADD16(exc[i], EXTRACT16(PSHR32(ADD32(MULT16_16(gain0,iexc[i]), MULT16_16(gain1,iexc[i+nsf])),8)));
   /* FIXME: compute_rms16 is currently not quite accurate enough (but close) */
   new_ener = compute_rms16(new_exc, nsf);
   old_ener = compute_rms16(exc, nsf);
   
   if (old_ener < 1)
      old_ener = 1;
   if (new_ener < 1)
      new_ener = 1;
   if (old_ener > new_ener)
      old_ener = new_ener;
   ngain = DIV32_16(SHL32(EXTEND32(old_ener),14),new_ener);
   
   for (i=0;i<nsf;i++)
      new_exc[i] = MULT16_16_Q14(ngain, new_exc[i]);
}
#endif

void comb_filter_mem_init (CombFilterMem *mem)
{
   mem->last_pitch=0;
   mem->last_pitch_gain[0]=mem->last_pitch_gain[1]=mem->last_pitch_gain[2]=0;
   mem->smooth_gain=1;
}

#ifdef FIXED_POINT
#define COMB_STEP 32767
#else
#define COMB_STEP 1.0
#endif

void comb_filter(
spx_sig_t *exc,          /*decoded excitation*/
spx_word16_t *_new_exc,      /*enhanced excitation*/
spx_coef_t *ak,           /*LPC filter coefs*/
int p,               /*LPC order*/
int nsf,             /*sub-frame size*/
int pitch,           /*pitch period*/
spx_word16_t *pitch_gain,   /*pitch gain (3-tap)*/
spx_word16_t  comb_gain,    /*gain of comb filter*/
CombFilterMem *mem
)
{
   int i;
   spx_word16_t exc_energy=0, new_exc_energy=0;
   spx_word16_t gain;
   spx_word16_t step;
   spx_word16_t fact;
   /* FIXME: This is a temporary kludge */
   spx_sig_t new_exc[40];
   
   /*Compute excitation amplitude prior to enhancement*/
   exc_energy = compute_rms(exc, nsf);
   /*for (i=0;i<nsf;i++)
     exc_energy+=((float)exc[i])*exc[i];*/

   /*Some gain adjustment if pitch is too high or if unvoiced*/
#ifdef FIXED_POINT
   {
      spx_word16_t g = gain_3tap_to_1tap(pitch_gain)+gain_3tap_to_1tap(mem->last_pitch_gain);
      if (g > 166)
         comb_gain = MULT16_16_Q15(DIV32_16(SHL32(EXTEND32(165),15),g), comb_gain);
      if (g < 64)
         comb_gain = MULT16_16_Q15(SHL16(g, 9), comb_gain);
   }
#else
   {
      float g=0;
      g = GAIN_SCALING_1*.5*(gain_3tap_to_1tap(pitch_gain)+gain_3tap_to_1tap(mem->last_pitch_gain));
      if (g>1.3)
         comb_gain*=1.3/g;
      if (g<.5)
         comb_gain*=2.*g;
   }
#endif
   step = DIV32(COMB_STEP, nsf);
   fact=0;

   /*Apply pitch comb-filter (filter out noise between pitch harmonics)*/
   for (i=0;i<nsf;i++)
   {
      spx_word32_t exc1, exc2;

      fact = ADD16(fact,step);
      
      exc1 = SHL32(MULT16_32_Q15(SHL16(pitch_gain[0],7),exc[i-pitch+1]) +
                 MULT16_32_Q15(SHL16(pitch_gain[1],7),exc[i-pitch]) +
                 MULT16_32_Q15(SHL16(pitch_gain[2],7),exc[i-pitch-1]) , 2);
      exc2 = SHL32(MULT16_32_Q15(SHL16(mem->last_pitch_gain[0],7),exc[i-mem->last_pitch+1]) +
                 MULT16_32_Q15(SHL16(mem->last_pitch_gain[1],7),exc[i-mem->last_pitch]) +
                 MULT16_32_Q15(SHL16(mem->last_pitch_gain[2],7),exc[i-mem->last_pitch-1]),2);

      new_exc[i] = exc[i] + MULT16_32_Q15(comb_gain, ADD32(MULT16_32_Q15(fact,exc1), MULT16_32_Q15(SUB16(COMB_STEP,fact), exc2)));
   }

   mem->last_pitch_gain[0] = pitch_gain[0];
   mem->last_pitch_gain[1] = pitch_gain[1];
   mem->last_pitch_gain[2] = pitch_gain[2];
   mem->last_pitch = pitch;

   /*Amplitude after enhancement*/
   new_exc_energy = compute_rms(new_exc, nsf);

   if (exc_energy > new_exc_energy)
      exc_energy = new_exc_energy;
   
   if (new_exc_energy<1)
      new_exc_energy=1;
   if (exc_energy<1)
      exc_energy=1;
   gain = DIV32_16(SUB32(SHL32(exc_energy,15),1),new_exc_energy);

#ifdef FIXED_POINT
   if (gain < 16384)
      gain = 16384;
#else
   if (gain < .5)
      gain=.5;
#endif

#ifdef FIXED_POINT
   for (i=0;i<nsf;i++)
   {
      mem->smooth_gain = ADD16(MULT16_16_Q15(31457,mem->smooth_gain), MULT16_16_Q15(1311,gain));
      new_exc[i] = MULT16_32_Q15(mem->smooth_gain, new_exc[i]);
   }
#else
   for (i=0;i<nsf;i++)
   {
      mem->smooth_gain = .96*mem->smooth_gain + .04*gain;
      new_exc[i] *= mem->smooth_gain;
   }
#endif
   for (i=0;i<nsf;i++)
      _new_exc[i] = EXTRACT16(PSHR32(new_exc[i],SIG_SHIFT));
}
