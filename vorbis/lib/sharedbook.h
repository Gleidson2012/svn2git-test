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

 function: basic shared codebook operations
 last mod: $Id: sharedbook.h,v 1.1.2.3 2000/04/06 15:59:37 xiphmont Exp $

 ********************************************************************/

#ifndef _V_INT_SHCODEBOOK_H_
#define _V_INT_SHCODEBOOK_H_

#include "vorbis/codebook.h"

extern void vorbis_staticbook_clear(static_codebook *b);
extern int vorbis_book_init_encode(codebook *dest,const static_codebook *source);
extern int vorbis_book_init_decode(codebook *dest,const static_codebook *source);
extern void vorbis_book_clear(codebook *b);

extern double *_book_unquantize(const static_codebook *b);
extern double *_book_logdist(const static_codebook *b,double *vals);
extern double _float32_unpack(long val);
extern long   _float32_pack(double val);
extern int  _best(codebook *book, double *a, int step);
extern int  _logbest(codebook *book, double *a, int step);
extern int _ilog(unsigned int v);

#endif
