/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail for basic thread mutexing
 last mod: $Id: mutex.h,v 1.1.2.1 2002/12/31 01:18:02 xiphmont Exp $

 ********************************************************************/

/* the thread mutexing here is for internal Ogg use only; the
   zero-copy code in libogg2 centralizes buffer management in one
   place where libogg1 spreads it across abstraction layers. 

   Mutexing code isn't requird for a platform, it's only required to
   use threads with libogg and oggfile. */

#ifndef _OGG_MUTEX_H_
#define _OGG_MUTEX_H_

#include <ogg/ogg.h>

#ifdef USE_POSIX_THREADS
#define _REENTRANT 1
#include <pthread.h>
static typedef ogg_mutex_t pthread_mutex_t;
#define ogg_mutex_init(m) (pthread_mutex_init(m,NULL))
#define ogg_mutex_clear(m) (pthread_mutex_destroy(m))
#define ogg_mutex_lock(m) (pthread_mutex_lock(m))
#define ogg_mutex_unlock(m) (pthread_mutex_unlock(m))

#else
#ifdef USE_NO_THREADS
static typedef ogg_mutex_t int;
static void noop(void){return};
#define ogg_mutex_init(m) (noop())
#define ogg_mutex_clear(m) (noop())
#define ogg_mutex_lock(m) (noop())
#define ogg_mutex_unlock(m) (noop())

#else
#error "this platform has no threading primitive wrappers in ogg/mutex.c"

#endif


#endif 
