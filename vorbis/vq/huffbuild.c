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

 function: hufftree builder
 last mod: $Id: huffbuild.c,v 1.5.2.4 2000/11/04 06:22:10 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "bookutil.h"

static int nsofar=0;
static int getval(FILE *in,int begin,int n,int group,int max){
  float v;
  int i;
  long val=0;

  if(nsofar>=n || get_line_value(in,&v)){
    reset_next_value();
    nsofar=0;
    if(get_next_value(in,&v))
      return(-1);
    for(i=1;i<=begin;i++)
      get_line_value(in,&v);
  }

  val=(int)v;
  nsofar++;

  for(i=1;i<group;i++,nsofar++)
    if(nsofar>=n || get_line_value(in,&v))
      return(getval(in,begin,n,group,max));
    else
      val = val*max+(int)v;
  return(val);
}

static void usage(){
  fprintf(stderr,
	  "usage:\n" 
	  "huffbuild <input>.vqd <begin,n,group> [noguard]\n"
	  "   where begin,n,group is first scalar, \n"
	  "                          number of scalars of each in line,\n"
	  "                          number of scalars in a group\n"
	  "eg: huffbuild reslongaux.vqd 0,1024,4\n"
	  "produces reslongaux.vqh\n\n");
  exit(1);
}

int main(int argc, char *argv[]){
  char *base;
  char *infile;
  int i,j,k,begin,n,subn,guard=1;
  FILE *file;
  int maxval=0;

  if(argc<3)usage();
  if(argc==4)guard=0;

  infile=strdup(argv[1]);
  base=strdup(infile);
  if(strrchr(base,'.'))
    strrchr(base,'.')[0]='\0';

  {
    char *pos=strchr(argv[2],',');
    begin=atoi(argv[2]);
    if(!pos)
      usage();
    else
      n=atoi(pos+1);
    pos=strchr(pos+1,',');
    if(!pos)
      usage();
    else
      subn=atoi(pos+1);
    if(n/subn*subn != n){
      fprintf(stderr,"n must be divisible by group\n");
      exit(1);
    }
  }

  /* scan the file for maximum value */
  file=fopen(infile,"r");
  if(!file){
    fprintf(stderr,"Could not open file %s\n",infile);
    exit(1);
  }
  i=0;
  while(1){
    long v;
    if(get_next_ivalue(file,&v))break;
    if(v>maxval)maxval=v;

    if(!(i++&0xff))spinnit("loading... ",i);
  }
  rewind(file);
  maxval++;

  {
    long vals=pow(maxval,subn);
    long *hist=malloc(vals*sizeof(long));
    long *lengths=malloc(vals*sizeof(long));
    
    for(j=0;j<vals;j++)hist[j]=guard;
    
    reset_next_value();
    i/=subn;
    while(!feof(file)){
      long val=getval(file,begin,n,subn,maxval);
      if(val==-1)break;
      hist[val]++;
      if(!(i--&0xff))spinnit("loading... ",i*subn);
    }
    fclose(file);
 
    /* we have the probabilities, build the tree */
    fprintf(stderr,"Building tree for %ld entries\n",vals);
    build_tree_from_lengths0(vals,hist,lengths);

    /* save the book */
    {
      char *buffer=alloca(strlen(base)+5);
      strcpy(buffer,base);
      strcat(buffer,".vqh");
      file=fopen(buffer,"w");
      if(!file){
	fprintf(stderr,"Could not open file %s\n",buffer);
	exit(1);
      }
    }
    
    fprintf(file,
 "/********************************************************************\n"
 " *                                                                  *\n"
 " * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *\n"
 " * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *\n"
 " * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *\n"
 " * PLEASE READ THESE TERMS DISTRIBUTING.                            *\n"
 " *                                                                  *\n"
 " * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *\n"
 " * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *\n"
 " * http://www.xiph.org/                                             *\n"
 " *                                                                  *\n"
 " ********************************************************************\n"
 "\n"
 " function: static codebook autogenerated by huff/huffbuld\n"
 "\n"
 " ********************************************************************/\n\n");

    fprintf(file,"#ifndef _V_%s_VQH_\n#define _V_%s_VQH_\n",base,base);
    fprintf(file,"#include \"codebook.h\"\n\n");
    
    /* first, the static vectors, then the book structure to tie it together. */
    /* lengthlist */
    fprintf(file,"static long _huff_lengthlist_%s[] = {\n",base);
    for(j=0;j<vals;){
      fprintf(file,"\t");
      for(k=0;k<16 && j<vals;k++,j++)
	fprintf(file,"%2ld,",lengths[j]);
      fprintf(file,"\n");
    }
    fprintf(file,"};\n\n");
    
    /* the toplevel book */
    fprintf(file,"static static_codebook _huff_book_%s = {\n",base);
    fprintf(file,"\t%d, %ld,\n",subn,vals);
    fprintf(file,"\t_huff_lengthlist_%s,\n",base);
    fprintf(file,"\t0, 0, 0, 0, 0,\n");
    fprintf(file,"\tNULL,\n");
    fprintf(file,"\tNULL,\n");
    fprintf(file,"\tNULL,\n");
    fprintf(file,"\t0\n};\n\n");
    
    fprintf(file,"\n#endif\n");
    fclose(file);
    fprintf(stderr,"Done.                                \n\n");
  }
  exit(0);
}


