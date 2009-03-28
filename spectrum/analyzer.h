/*
 *
 *  gtk2 spectrum analyzer
 *    
 *      Copyright (C) 2004 Monty
 *
 *  This analyzer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  The analyzer is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#ifndef _ANALYZER_H_
#define _ANALYZER_H_

#define _GNU_SOURCE
#define _ISOC99_SOURCE
#define _FILE_OFFSET_BITS 64
#define _REENTRANT 1
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define __USE_GNU 1
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>
#include <fftw3.h>

#define MAX_FILES 16
#define readbuffersize 8192
/* blocksize for the FFT */
extern int blocksize;

static inline float todB(float x){
  return logf((x)*(x)+1e-30f)*4.34294480f;
}

#ifdef UGLY_IEEE754_FLOAT32_HACK

static inline float todB_a(const float *x){
  union {
    int32_t i;
    float f;
  } ix;
  ix.f = *x;
  ix.i = ix.i&0x7fffffff;
  return (float)(ix.i * 7.17711438e-7f -764.6161886f);
}

#else

static inline float todB_a(const float *x){
  return todB(*x);
}

#endif

#ifndef max
#define max(x,y) ((x)>(y)?(x):(y))
#endif


#define toOC(n)     (log(n)*1.442695f-5.965784f)
#define fromOC(o)   (exp(((o)+5.965784f)*.693147f))

extern int eventpipe[2];

extern void panel_go(int argc,char *argv[]);
extern int input_load();
extern void *process_thread(void *dummy);
extern void process_dump(int mode);
extern void rundata_clear();
extern void clear_noise_floor();
extern float **process_fetch(int res, int scale, int mode, int link, 
			     int *active, int width, 
			     float *ymax, float *pmax, float *pmin,
			     float **floor, int noise);

extern int inputs;
extern int total_ch;
extern int bits[MAX_FILES];
extern int bigendian[MAX_FILES];
extern int channels[MAX_FILES];
extern int rate[MAX_FILES];
extern int signedp[MAX_FILES];
extern char *inputname[MAX_FILES];
extern int seekable[MAX_FILES];
extern int global_seekable;

extern pthread_mutex_t feedback_mutex;
extern int feedback_increment;
extern float **feedback_acc;
extern float **feedback_max;
extern float **feedback_instant;
extern float **feedback_rp;
extern sig_atomic_t acc_clear;
extern sig_atomic_t acc_rewind;
extern sig_atomic_t acc_loop;

extern sig_atomic_t process_active;
extern sig_atomic_t process_exit;

#define LINKS 12
static char *link_entries[LINKS]={"independent","sum","subtract ref","subtract from",
				  "imp .1\xCE\xA9 shunt","imp 1\xCE\xA9 shunt",
				  "imp 10\xCE\xA9 shunt",
				  "response/phase","THD","THD (-2nd)","THD+N","THD+N (-2nd)"};
#define LINK_INDEPENDENT  0
#define LINK_SUMMED       1
#define LINK_SUB_REF      2
#define LINK_SUB_FROM     3
#define LINK_IMPEDENCE_p1 4
#define LINK_IMPEDENCE_1  5
#define LINK_IMPEDENCE_10 6
#define LINK_PHASE        7
#define LINK_THD          8
#define LINK_THD2         9
#define LINK_THDN        10
#define LINK_THDN2       11

#endif
