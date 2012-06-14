/*
 *
 *  gtk2 spectrum analyzer
 *
 *      Copyright (C) 2004-2012 Monty
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

#include "analyzer.h"
#include "io.h"

static float *window=NULL;
static float *freqbuffer=0;
static fftwf_plan plan=NULL;
static int prev_total_ch=-1;

static pthread_mutex_t feedback_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t bw_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static float *feedback_count=NULL;
static float *process_work=NULL;

static float **mag_acc=NULL;
static float **mag_max=NULL;
static float **mag_instant=NULL;

static float **phI_acc=NULL;
static float **phQ_acc=NULL;
static float **phI_max=NULL;
static float **phQ_max=NULL;
static float **phI_instant=NULL;
static float **phQ_instant=NULL;

static float **xmappingL=NULL;
static float **xmappingM=NULL;
static float **xmappingH=NULL;
static int metascale = -1;
static int metawidth = -1;
static int metareload = 0;

sig_atomic_t acc_rewind=0;
sig_atomic_t acc_loop=0;

sig_atomic_t process_active=0;
sig_atomic_t process_exit=0;

static void init_process(void){
  int i;

  if(!window || prev_total_ch != total_ch){

    if(plan)fftwf_destroy_plan(plan);

    if(mag_acc){
      for(i=0;i<prev_total_ch;i++)
        if(mag_acc[i])free(mag_acc[i]);
      free(mag_acc);
    }
    if(mag_max){
      for(i=0;i<prev_total_ch;i++)
        if(mag_max[i])free(mag_max[i]);
      free(mag_max);
    }
    if(mag_instant){
      for(i=0;i<prev_total_ch;i++)
        if(mag_instant[i])free(mag_instant[i]);
      free(mag_instant);
    }

    if(phI_acc){
      for(i=0;i<prev_total_ch;i++)
        if(phI_acc[i])free(phI_acc[i]);
      free(phI_acc);
    }
    if(phQ_acc){
      for(i=0;i<prev_total_ch;i++)
        if(phQ_acc[i])free(phQ_acc[i]);
      free(phQ_acc);
    }
    if(phI_max){
      for(i=0;i<prev_total_ch;i++)
        if(phI_max[i])free(phI_max[i]);
      free(phI_max);
    }
    if(phQ_max){
      for(i=0;i<prev_total_ch;i++)
        if(phQ_max[i])free(phQ_max[i]);
      free(phQ_max);
    }
    if(phI_instant){
      for(i=0;i<prev_total_ch;i++)
        if(phI_instant[i])free(phI_instant[i]);
      free(phI_instant);
    }
    if(phQ_instant){
      for(i=0;i<prev_total_ch;i++)
        if(phQ_instant[i])free(phQ_instant[i]);
      free(phQ_instant);
    }

    if(process_work)free(process_work);
    if(feedback_count)free(feedback_count);

    if(freqbuffer)free(freqbuffer);
    if(window)free(window);

    process_work=calloc(blocksize+2,sizeof(*process_work));
    feedback_count=calloc(total_ch,sizeof(*feedback_count));

    mag_acc=calloc(total_ch,sizeof(*mag_acc));
    mag_max=calloc(total_ch,sizeof(*mag_max));
    mag_instant=calloc(total_ch,sizeof(*mag_instant));

    phI_acc=calloc(total_ch,sizeof(*phI_acc));
    phQ_acc=calloc(total_ch,sizeof(*phQ_acc));
    phI_max=calloc(total_ch,sizeof(*phI_max));
    phQ_max=calloc(total_ch,sizeof(*phQ_max));
    phI_instant=calloc(total_ch,sizeof(*phI_instant));
    phQ_instant=calloc(total_ch,sizeof(*phQ_instant));

    freqbuffer=fftwf_malloc((blocksize+2)*sizeof(*freqbuffer));
    for(i=0;i<total_ch;i++){

      mag_acc[i]=calloc(blocksize/2+1,sizeof(**mag_acc));
      mag_max[i]=calloc(blocksize/2+1,sizeof(**mag_max));
      mag_instant[i]=calloc(blocksize/2+1,sizeof(**mag_instant));

      if(i>0){
        phI_acc[i]=calloc(blocksize/2+1,sizeof(**phI_acc));
        phI_max[i]=calloc(blocksize/2+1,sizeof(**phI_max));
        phI_instant[i]=calloc(blocksize/2+1,sizeof(**phI_instant));
        phQ_acc[i]=calloc(blocksize/2+1,sizeof(**phQ_acc));
        phQ_max[i]=calloc(blocksize/2+1,sizeof(**phQ_max));
        phQ_instant[i]=calloc(blocksize/2+1,sizeof(**phQ_instant));
      }
    }

    prev_total_ch = total_ch;

    plan=fftwf_plan_dft_r2c_1d(blocksize,freqbuffer,
                               (fftwf_complex *)freqbuffer,
                               FFTW_ESTIMATE);

    /* construct proper window (sin^4 I'd think) */
    window = calloc(blocksize,sizeof(*window));
    for(i=0;i<blocksize;i++)window[i]=sin(M_PIl*i/blocksize);
    for(i=0;i<blocksize;i++)window[i]*=window[i];
    for(i=0;i<blocksize;i++)window[i]=sin(window[i]*M_PIl*.5);
    for(i=0;i<blocksize;i++)window[i]*=window[i]/(blocksize/4)*.778;
  }
}

void rundata_clear(){
  int i;
  pthread_mutex_lock(&feedback_mutex);
  for(i=0;i<total_ch;i++){
    feedback_count[i]=0;
    memset(mag_acc[i],0,(blocksize/2+1)*sizeof(**mag_acc));
    memset(mag_max[i],0,(blocksize/2+1)*sizeof(**mag_max));
    memset(mag_instant[i],0,(blocksize/2+1)*sizeof(**mag_instant));

    if(i>0){
      memset(phI_acc[i],0,(blocksize/2+1)*sizeof(**phI_acc));
      memset(phQ_acc[i],0,(blocksize/2+1)*sizeof(**phQ_acc));
      memset(phI_max[i],0,(blocksize/2+1)*sizeof(**phI_max));
      memset(phQ_max[i],0,(blocksize/2+1)*sizeof(**phQ_max));
      memset(phI_instant[i],0,(blocksize/2+1)*sizeof(**phI_instant));
      memset(phQ_instant[i],0,(blocksize/2+1)*sizeof(**phQ_instant));
    }
  }
  pthread_mutex_unlock(&feedback_mutex);
}

char *bw_entries[]=
  {"native","display",
   ".1Hz",".3Hz","1Hz","3Hz","10Hz","30Hz","100Hz",NULL};
float bw_values[]=
  {0., 0., .1, .3, 1, 3, 10, 30, 100};

//static int bandwidth_choice=0;
//static float bandwidth=-1;
//static int detector_mode=0;

void set_bandwidth_detector(int bw, int det){
#if 0
  pthread_mutex_lock(&bw_mutex);
  bandwidth_choice=bw;
  detector_mode=det;
  compute_bandwidth();
  pthread_mutex_unlock(&bw_mutex);

  pthread_mutex_lock(&feedback_mutex);
  rundata_clear();
  accumulate_feedback();
  pthread_mutex_unlock(&feedback_mutex);
#endif
}

static void process(void){
  /* by channel */
  int i,j,fi,ch=0;
  for(fi=0;fi<inputs;fi++){
    if(blockbuffernew[fi]){
      blockbuffernew[fi]=0;
      for(i=ch;i<ch+channels[fi];i++){

        pthread_mutex_lock(&bw_mutex);
	float *data=blockbuffer[i];

	/* window the blockbuffer into the FFT buffer, save a copy of
           current frame for BW changes */
	for(j=0;j<blocksize;j++){
	  freqbuffer[j]=window[j]*data[j];
	}

	/* transform */
	fftwf_execute(plan);

        pthread_mutex_unlock(&bw_mutex);

	pthread_mutex_lock(&feedback_mutex);

	/* perform desired accumulations */
	for(j=0;j<blocksize+2;j+=2){
	  float R = freqbuffer[j];
	  float I = freqbuffer[j+1];
	  float sqR = R*R;
	  float sqI = I*I;
	  float sqM = sqR+sqI;
          //float M = sqrtf(sqM);

	  /* deal with phase accumulate/rotate */
	  if(i==ch){
	    /* normalize/store ref for later rotation */
	    process_work[j] = R;
	    process_work[j+1] = -I;

	  }else{
	    /* rotate signed square phase according to ref for phase calculation */
	    float pR;
	    float pI;
	    float rR = process_work[j];
	    float rI = process_work[j+1];
	    pR = (rR*R - rI*I);
	    pI = (rR*I + rI*R);

	    phI_instant[i][j>>1]=pR;
	    phQ_instant[i][j>>1]=pI;

	    phI_acc[i][j>>1]+=pR;
	    phQ_acc[i][j>>1]+=pI;

	    if(mag_max[i][j>>1]<sqM){
	      phI_max[i][j>>1]=pR;
	      phQ_max[i][j>>1]=pI;
	    }
	  }

	  mag_instant[i][j>>1]=sqM;
	  mag_acc[i][j>>1]+=sqM;

	  if(mag_max[i][j>>1]<sqM)
	    mag_max[i][j>>1]=sqM;

	}
	feedback_count[i]++;

	pthread_mutex_unlock(&feedback_mutex);
      }
    }
    ch+=channels[fi];
  }
}

void *process_thread(void *dummy){
  int ret;
  pthread_mutex_lock(&feedback_mutex);
  init_process();
  pthread_mutex_unlock(&feedback_mutex);

  while(!process_exit){

    if(acc_rewind) rewind_files();
    acc_rewind=0;

    ret=input_read(acc_loop,0);
    if(ret==0) break;
    if(ret==-1){
      /* a pipe returned EOF; attempt reopen */
      pthread_mutex_lock(&feedback_mutex);
      if(pipe_reload()){
        init_process();
        rundata_clear();
        metareload=1;
        pthread_mutex_unlock(&feedback_mutex);
        write(eventpipe[1],"",1);
        continue;
      }else{
        pthread_mutex_unlock(&feedback_mutex);
        break;
      }
    }

    process();
    write(eventpipe[1],"",1);
  }

  /* eof on all inputs */
  process_active=0;
  write(eventpipe[1],"",1);
  return NULL;
}

void process_dump(int mode){
  int fi,i,j,ch;
  FILE *out;

  pthread_mutex_lock(&feedback_mutex);

  out=fopen("accumulate.m","w");
  ch = 0;
  for(fi=0;fi<inputs;fi++){
    for(i=0;i<blocksize/2+1;i++){
      fprintf(out,"%f ",(double)i*rate[fi]/blocksize);

      for(j=ch;j<ch+channels[fi];j++)
        fprintf(out,"%f ",todB(mag_acc[j][i])*.5);
      fprintf(out,"\n");
    }
    fprintf(out,"\n");
    ch+=channels[fi];
  }
  fclose(out);

  out=fopen("max.m","w");
  ch = 0;
  for(fi=0;fi<inputs;fi++){
    for(i=0;i<blocksize/2+1;i++){
      fprintf(out,"%f ",(double)i*rate[fi]/blocksize);

      for(j=ch;j<ch+channels[fi];j++)
        fprintf(out,"%f ",todB(mag_max[j][i])*.5);
      fprintf(out,"\n");
    }
    fprintf(out,"\n");
    ch+=channels[fi];
  }
  fclose(out);

  out=fopen("instant.m","w");
  ch = 0;
  for(fi=0;fi<inputs;fi++){
    for(i=0;i<blocksize/2+1;i++){
      fprintf(out,"%f ",(double)i*rate[fi]/blocksize);

      for(j=ch;j<ch+channels[fi];j++)
        fprintf(out,"%f ",todB(mag_instant[j][i])*.5);
      fprintf(out,"\n");
    }
    fprintf(out,"\n");
    ch+=channels[fi];
  }
  fclose(out);

  out=fopen("accphase.m","w");
  ch = 0;
  for(fi=0;fi<inputs;fi++){

    /* phase */
    for(i=0;i<blocksize+2;i+=2){
      fprintf(out,"%f ",(double)i*.5*rate[fi]/blocksize);
      fprintf(out,"%f ",atan2(phQ_acc[ch+1][i>>1],phI_acc[ch+1][i>>1])*57.29);
      fprintf(out,"\n");
    }
    fprintf(out,"\n");
    ch+=channels[fi];
  }
  fclose(out);

  pthread_mutex_unlock(&feedback_mutex);
}

/* how many bins to 'trim' off the edge of calculated data when we
   know we've hit a boundary of marginal measurement */
#define binspan 5

static fetchdata fetch_ret;

/* the data returned is now 2 vals per bin; a min and a max.  The spec
   plot merely draws a vertical line between. */
fetchdata *process_fetch(int scale, int mode, int link,
                         float bw, int bwmode,
                         int *process_in, Plot *plot){
  int ch,ci,i,j,fi;
  float **data;
  float **phI;
  float **phQ;
  float *normptr;
  float maxrate=-1.;
  float nyq;
  int *process;
  int width=-1;

  pthread_mutex_lock(&feedback_mutex);
  init_process();
  process = alloca(total_ch*sizeof(*process));

  if(total_ch!=fetch_ret.total_ch){
    if(fetch_ret.data){
      for(i=0;i<fetch_ret.total_ch;i++)
        if(fetch_ret.data[i])free(fetch_ret.data[i]);
      free(fetch_ret.data);
      fetch_ret.data=NULL;
    }
    if(fetch_ret.active){
      free(fetch_ret.active);
      fetch_ret.active=NULL;
    }
  }

  if(!fetch_ret.data)
    fetch_ret.data = calloc(total_ch,sizeof(*fetch_ret.data));

  if(!fetch_ret.active)
    fetch_ret.active = calloc(total_ch,sizeof(*fetch_ret.active));

  /* the passed in process array doesn't necesarily match the
     current channel structure.  Copy group by group. */
  {
    int ch_now=0;
    int ch_in=0;
    for(i=0;i<inputs;i++){
      int ci;
      for(ci=0;ci<channels[i] && ci<fetch_ret.channels[i];ci++)
        process[ch_now+ci] = process_in[ch_in+ci];
      for(;ci<channels[i];ci++)
        process[ch_now+ci] = 0;
      ch_now+=channels[i];
      ch_in+=fetch_ret.channels[i];
    }

    memcpy(fetch_ret.active,process,total_ch*sizeof(*process));
  }

  fetch_ret.phase_active=0;
  if(link == LINK_PHASE){
    int cho=0;
    int gi;
    for(gi=0;gi<inputs;gi++)
      if(channels[gi]>1 && fetch_ret.active[cho+1]){
        fetch_ret.phase_active=1;
        break;
      }
  }

  fetch_ret.groups=inputs;
  fetch_ret.scale=scale;
  fetch_ret.mode=mode;
  fetch_ret.link=link;

  fetch_ret.height=plot_height(plot);
  fetch_ret.width=width=plot_width(plot,fetch_ret.phase_active);
  fetch_ret.total_ch=total_ch;

  for(fi=0;fi<inputs;fi++)
    if(rate[fi]>maxrate)maxrate=rate[fi];

  memcpy(fetch_ret.bits,bits,sizeof(fetch_ret.bits));
  memcpy(fetch_ret.channels,channels,sizeof(fetch_ret.channels));
  memcpy(fetch_ret.rate,rate,sizeof(fetch_ret.rate));

  nyq=maxrate/2.;
  fetch_ret.maxrate=maxrate;
  fetch_ret.reload=metareload;
  metareload=0;

  /* are our scale mappings up to date? */
  if(scale != metascale || width != metawidth || fetch_ret.reload){
    if(!xmappingL) xmappingL = calloc(inputs, sizeof(*xmappingL));
    if(!xmappingM) xmappingM = calloc(inputs, sizeof(*xmappingM));
    if(!xmappingH) xmappingH = calloc(inputs, sizeof(*xmappingH));

    for(fi=0;fi<inputs;fi++){

      /* if mapping preexists, resize it */
      if(xmappingL[fi]){
	xmappingL[fi] = realloc(xmappingL[fi],(width+1)*sizeof(**xmappingL));
      }else{
	xmappingL[fi] = malloc((width+1)*sizeof(**xmappingL));
      }
      if(xmappingM[fi]){
	xmappingM[fi] = realloc(xmappingM[fi],(width+1)*sizeof(**xmappingM));
      }else{
	xmappingM[fi] = malloc((width+1)*sizeof(**xmappingM));
      }
      if(xmappingH[fi]){
	xmappingH[fi] = realloc(xmappingH[fi],(width+1)*sizeof(**xmappingH));
      }else{
	xmappingH[fi] = malloc((width+1)*sizeof(**xmappingH));
      }

      metascale = scale;
      metawidth = width;

      /* generate new numbers */
      for(i=0;i<width;i++){
	float off=.5;
	float loff=1.;
	float hoff=1.;
	float lfreq,mfreq,hfreq;

        off=.5;

	switch(scale){
	case 0: /* log */
	  lfreq= pow(10.,(i-off)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.)) * loff;
	  mfreq= pow(10.,((float)i)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.));
	  hfreq= pow(10.,(i+off)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.)) * hoff;
	  break;
	case 1: /* ISO */
	  lfreq= pow(2.,(i-off)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.)) * loff;
	  mfreq= pow(2.,((float)i)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.));
	  hfreq= pow(2.,(i+off)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.)) *hoff;
	  break;
	case 2: /* screen-resolution linear */
	  lfreq=(i-off)*nyq/(width-1)*loff;
	  mfreq=((float)i)*nyq/(width-1);
	  hfreq=(i+off)*nyq/(width-1)*hoff;
	  break;
	}

	xmappingL[fi][i]=lfreq/(rate[fi]*.5)*(blocksize/2);
	xmappingM[fi][i]=mfreq/(rate[fi]*.5)*(blocksize/2);
	xmappingH[fi][i]=hfreq/(rate[fi]*.5)*(blocksize/2);

      }

      for(i=0;i<width;i++){
	if(xmappingL[fi][i]<0.)xmappingL[fi][i]=0.;
	if(xmappingL[fi][i]>blocksize/2.)xmappingL[fi][i]=blocksize/2.;
	if(xmappingM[fi][i]<0.)xmappingM[fi][i]=0.;
	if(xmappingM[fi][i]>blocksize/2.)xmappingM[fi][i]=blocksize/2.;
	if(xmappingH[fi][i]<0.)xmappingH[fi][i]=0.;
	if(xmappingH[fi][i]>blocksize/2.)xmappingH[fi][i]=blocksize/2.;
      }
    }

    for(i=0;i<total_ch;i++){
      if(fetch_ret.data[i]){
	fetch_ret.data[i] = realloc
          (fetch_ret.data[i],(width+1)*2*sizeof(**fetch_ret.data));
      }else{
	fetch_ret.data[i] = malloc((width+1)*2*sizeof(**fetch_ret.data));
      }
    }
  }

  /* mode selects the base data set */
  normptr=NULL;
  switch(mode){
  case 0: /* independent / instant */
    data=mag_instant;
    phI=phI_instant;
    phQ=phQ_instant;
    break;
  case 1: /* independent / max */
    data=mag_max;
    phI=phI_max;
    phQ=phQ_max;
    break;
  case 2: /* independent / accumulate */
    data=mag_acc;
    phI=phI_acc;
    phQ=phQ_acc;
    break;
  case 3: /* independent / average */
    data=mag_acc;
    phI=phI_acc;
    phQ=phQ_acc;
    normptr=feedback_count;
    break;
  }

  ch=0;
  fetch_ret.ymax = -210.;
  fetch_ret.pmax = -180.;
  fetch_ret.pmin = 180.;

  for(fi=0;fi<inputs;fi++){
    float *L = xmappingL[fi];
    float *M = xmappingM[fi];
    float *H = xmappingH[fi];
    float normalize = normptr ? 1./normptr[fi] : 1.;

    switch(link){
    case LINK_INDEPENDENT:

      for(ci=0;ci<channels[fi];ci++){
	if(process[ch+ci]){

          float *y = fetch_ret.data[ci+ch];
          float *m = data[ci+ch];
          int prevbin;
          float prevy;
	  for(i=0;i<width;i++){
	    int first=ceil(L[i]);
	    int last=ceil(H[i]);
	    float firsty,lasty,min,max;

            /* don't allow roundoff error to skip a bin entirely */
            if(i>0 && prevbin<first)first=prevbin;
            prevbin=last;

	    if(first==last){
	      float del=M[i]-floor(M[i]);
              int mid = floor(M[i]);
              float a = todB(m[mid]*normalize);
              float b = todB(m[mid+1]*normalize);
	      firsty=lasty=min=max=(a+(b-a)*del);

	    }else{
	      firsty=min=max=m[first];
	      for(j=first+1;j<last;j++){
                if(m[j]<min)min=m[j];
                if(m[j]>max)max=m[j];
              }
              lasty=todB(m[j-1]*normalize);
              firsty=todB(firsty*normalize);
              min=todB(min*normalize);
              max=todB(max*normalize);
	    }

            max*=.5;
            min*=.5;
	    if(max>fetch_ret.ymax)fetch_ret.ymax=max;

            /* link non-overlapping bins into contiguous lines */
            if(i>0){
              float midpoint = (prevy+firsty)*.25;

              if(midpoint<min)min=midpoint;
              if(midpoint>max)max=midpoint;

              if(midpoint<y[i*2-2])y[i*2-2]=midpoint;
              if(midpoint>y[i*2-1])y[i*2-1]=midpoint;
            }

	    y[i*2]=min;
	    y[i*2+1]=max;

            prevy=lasty;
	  }
	}
      }
      break;

    case LINK_SUMMED:

      /* display first channel, but only if any channels in the group
         are processed */
      {
        int any=0;
        for(i=ch;i<ch+channels[fi];i++){
          if(fetch_ret.active[i])any=1;
          fetch_ret.active[i]=0;
        }
        fetch_ret.active[ch]=any;
      }

      {
        float *y = fetch_ret.data[ch];
        float **m = data+ch;
        int prevbin;
        float prevy;
        for(i=0;i<width;i++){
          int first=ceil(L[i]);
          int last=ceil(H[i]);
          float firsty,lasty,min,max;

          /* don't allow roundoff error to skip a bin entirely */
          if(i>0 && prevbin<first)first=prevbin;
          prevbin=last;

          if(first==last){
            float a=0.;
            float b=0.;
            int mid = floor(M[i]);
            float del=M[i]-floor(M[i]);
            for(ci=0;ci<channels[fi];ci++){
              if(process[ch+ci]){
                a+=m[ci][mid];
                b+=m[ci][mid+1];
              }
            }
            a=todB(a*normalize);
            b=todB(b*normalize);
            firsty=lasty=min=max=(a+(b-a)*del);
          }else{
            float a=0.;
            for(ci=0;ci<channels[fi];ci++){
              if(process[ch+ci]) a+=m[ci][first];
            }
            firsty=min=max=a;

            for(j=first+1;j<last;j++){
              a=0.;
              for(ci=0;ci<channels[fi];ci++){
                if(process[ch+ci]) a+=m[ci][j];
              }
              if(a<min)min=a;
              if(a>max)max=a;
            }

            lasty=todB(a*normalize);
            firsty=todB(firsty*normalize);
            min=todB(min*normalize);
            max=todB(max*normalize);
          }

          min*=.5;
          max*=.5;

          if(max>fetch_ret.ymax)fetch_ret.ymax=max;

          /* link non-overlapping bins into contiguous lines */
          if(i>0){
            float midpoint = (prevy+firsty)*.25;

            if(midpoint<min)min=midpoint;
            if(midpoint>max)max=midpoint;

            if(midpoint<y[i*2-2])y[i*2-2]=midpoint;
            if(midpoint>y[i*2-1])y[i*2-1]=midpoint;
          }

          y[i*2]=min;
          y[i*2+1]=max;

          prevy=lasty;
        }
      }
      break;

    case LINK_SUB_FROM:

      for(i=ch;i<ch+channels[fi];i++)
        fetch_ret.active[i]=0;

      if(process[ch]==0){
        float *y = fetch_ret.data[ch];
        for(i=0;i<width*2+2;i++)
          y[i]=-300;
      }else{
        float *y = fetch_ret.data[ch];
        float **m = data+ch;
        int prevbin;
        float prevy;

        fetch_ret.active[ch]=1;

        for(i=0;i<width;i++){
          int first=ceil(L[i]);
          int last=ceil(H[i]);
          float firsty,lasty,min,max;

          /* don't allow roundoff error to skip a bin entirely */
          if(i>0 && prevbin<first)first=prevbin;
          prevbin=last;

          if(first==last){
            int mid = floor(M[i]);
            float del=M[i]-floor(M[i]);
            float a=m[0][mid];
            float b=m[0][mid+1];
            for(ci=1;ci<channels[fi];ci++){
              if(process[ch+ci]){
                a-=m[ci][mid];
                b-=m[ci][mid+1];
              }
            }
            a=todB(a*normalize);
            b=todB(b*normalize);
            firsty=lasty=min=max=(a+(b-a)*del);
          }else{
            float a=m[0][first];
            for(ci=1;ci<channels[fi];ci++){
              if(process[ch+ci]) a-=m[ci][first];
            }
            firsty=min=max=a;

            for(j=first+1;j<last;j++){
              a=m[0][j];
              for(ci=1;ci<channels[fi];ci++){
                if(process[ch+ci]) a-=m[ci][j];
              }
              if(a<min)min=a;
              if(a>max)max=a;
            }

            lasty=todB(a*normalize);
            firsty=todB(firsty*normalize);
            min=todB(min*normalize);
            max=todB(max*normalize);
          }

          min*=.5;
          max*=.5;

          if(max>fetch_ret.ymax)fetch_ret.ymax=max;

          /* link non-overlapping bins into contiguous lines */
          if(i>0){
            float midpoint = (prevy+firsty)*.25;

            if(midpoint<min)min=midpoint;
            if(midpoint>max)max=midpoint;

            if(midpoint<y[i*2-2])y[i*2-2]=midpoint;
            if(midpoint>y[i*2-1])y[i*2-1]=midpoint;
          }

          y[i*2]=min;
          y[i*2+1]=max;

          prevy=lasty;
        }
      }
      break;
    case LINK_SUB_REF:

      {
        float *r = data[ch];
        float *y = fetch_ret.data[ch];
        for(i=0;i<width*2+2;i++)
          y[i]=-300;

        /* first channel in each display group not shown; used as a
           reference */
        fetch_ret.active[ch]=0;

        /* process 1->n */
        for(ci=1;ci<channels[fi];ci++){
          if(process[ch+ci]){
            float *y = fetch_ret.data[ci+ch];
            float *m = data[ci+ch];
            int prevbin;
            float prevy;
            for(i=0;i<width;i++){
              int first=ceil(L[i]);
              int last=ceil(H[i]);
              float firsty,lasty,min,max;

              /* don't allow roundoff error to skip a bin entirely */
              if(i>0 && prevbin<first)first=prevbin;
              prevbin=last;

              if(first==last){
                float del=M[i]-floor(M[i]);
                int mid = floor(M[i]);
                float a = todB((m[mid]-r[mid])*normalize);
                float b = todB((m[mid+1]-r[mid])*normalize);
                firsty=lasty=min=max=(a+(b-a)*del);

              }else{
                firsty=min=max=m[first]-r[first];
                for(j=first+1;j<last;j++){
                  if(m[j]<min)min=m[j]-r[j];
                  if(m[j]>max)max=m[j]-r[j];
                }
                lasty=todB((m[j-1]-r[j-1])*normalize);
                firsty=todB(firsty*normalize);
                min=todB(min*normalize);
                max=todB(max*normalize);
              }

              max*=.5;
              min*=.5;
              if(max>fetch_ret.ymax)fetch_ret.ymax=max;

              /* link non-overlapping bins into contiguous lines */
              if(i>0){
                float midpoint = (prevy+firsty)*.25;

                if(midpoint<min)min=midpoint;
                if(midpoint>max)max=midpoint;

                if(midpoint<y[i*2-2])y[i*2-2]=midpoint;
                if(midpoint>y[i*2-1])y[i*2-1]=midpoint;
              }

              y[i*2]=min;
              y[i*2+1]=max;

              prevy=lasty;
            }
          }
        }
      }
      break;

    case LINK_PHASE: /* response/phase */

      for(i=ch+2;i<ch+channels[fi];i++)
        fetch_ret.active[i]=0;

      if(channels[fi]>=2){
	float *om = fetch_ret.data[ch];
	float *op = fetch_ret.data[ch+1];

	float *r = data[ch];
	float *m = data[ch+1];
	float *pI = phI[ch+1];
	float *pQ = phQ[ch+1];

	if(feedback_count[ch]==0){
	  memset(om,0,width*2*sizeof(*om));
	  memset(op,0,width*2*sizeof(*op));
	}else{
	  /* two vectors only; response and phase */
	  /* response is a standard minmax vector */
          /* phase is averaged to screen resolution */
	  if(process[ch] || process[ch+1]){

            int prevbin;
            float prevy;
            float prevP=0;
            for(i=0;i<width;i++){
              int first=ceil(L[i]);
              int last=ceil(H[i]);
              float firsty,lasty,min,max;
              float P,R,I;

              /* don't allow roundoff error to skip a bin entirely */
              if(i>0 && prevbin<first)first=prevbin;
              prevbin=last;

              if(first==last){
                float del=M[i]-floor(M[i]);
                int mid = floor(M[i]);
                float a = todB(m[mid]/r[mid]);
                float b = todB(m[mid+1]/r[mid+1]);
                firsty=lasty=min=max=(a+(b-a)*del);

                if(process[ch+1]){
                  float aP = (isnan(a) ? NAN : atan2f(pQ[mid],pI[mid]));
                  float bP = (isnan(b) ? NAN : atan2f(pQ[mid+1],pI[mid+1]));
                  P=(aP+(bP-aP)*del)*57.29;
                }

              }else{
                firsty=min=max=m[first]/r[first];
                R = pI[first];
                I = pQ[first];

                for(j=first+1;j<last;j++){
                  float a = m[j]/r[j];
                  if(a<min)min=a;
                  if(a>max)max=a;
                  R += pI[j];
                  I += pQ[j];
                }

                lasty=todB(m[j-1]/r[j-1]);
                firsty=todB(firsty);
                min=todB(min);
                max=todB(max);

                if(process[ch+1])
                  P = atan2f(I,R)*57.29;
              }

              max*=.5;
              min*=.5;
              if(max>fetch_ret.ymax)fetch_ret.ymax=max;
              if(P>fetch_ret.pmax)fetch_ret.pmax = P;
	      if(P<fetch_ret.pmin)fetch_ret.pmin = P;

              if(process[ch+1] && min>-70){
                float midpoint = (prevP+P)*.5;
                op[i*2]=P;
                op[i*2+1]=P;

                /* link phase into contiguous line */
                if(i){
                  if(midpoint<P) op[i*2]=midpoint;
                  if(midpoint>P) op[i*2+1]=midpoint;
                  if(midpoint<op[i*2-2]) op[i*2-2]=midpoint;
                  if(midpoint>op[i*2-1]) op[i*2-1]=midpoint;
                }
              }else{
                op[i*2]=op[i*2+1]=NAN;
              }

              /* link non-overlapping bins into contiguous lines */
              if(i>0){
                float midpoint = (prevy+firsty)*.25;

                if(midpoint<min)min=midpoint;
                if(midpoint>max)max=midpoint;

                if(midpoint<om[i*2-2])om[i*2-2]=midpoint;
                if(midpoint>om[i*2-1])om[i*2-1]=midpoint;
              }

              om[i*2]=min;
              om[i*2+1]=max;

              prevy=lasty;
              prevP=P;
            }
          }
	}
      }
      break;
    }
    ch+=channels[fi];
  }

  pthread_mutex_unlock(&feedback_mutex);
  return &fetch_ret;
}
