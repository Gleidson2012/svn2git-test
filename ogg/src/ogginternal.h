/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2003             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: internal/hidden data representation structures
 last mod: $Id: ogginternal.h,v 1.1.2.8 2003/03/22 05:44:51 xiphmont Exp $

 ********************************************************************/

#ifndef _OGGI_H
#define _OGGI_H

#include <ogg2/ogg.h>
#include "mutex.h"

struct ogg_buffer_state{
  ogg_buffer    *unused_buffers;
  ogg_reference *unused_references;
  int            outstanding;

  ogg_mutex_t mutex;
  int         shutdown;
};

struct ogg_buffer {
  unsigned char      *data;
  long                size;
  int                 refcount;
  
  union {
    ogg_buffer_state *owner;
    ogg_buffer       *next;
  } ptr;
};

struct ogg_reference {
  struct ogg_buffer    *buffer;
  long                  begin;
  long                  length;
  struct ogg_reference *next;
};

struct oggpack_buffer {
  int            headbit;
  unsigned char *headptr;
  long           headend;

  /* memory management */
  ogg_reference *head;
  ogg_reference *tail;

  /* render the byte/bit counter API constant time */
  long              count; /* doesn't count the tail */
  ogg_buffer_state *owner; /* useful on encode side */
};

typedef struct oggbyte_buffer {
  ogg_reference *baseref;
  long           basepos;

  ogg_reference *headref;
  unsigned char *headptr;
  long           headpos;
  long           headend;

  ogg_buffer_state *owner; /* if it's to be extensible; encode side */
} oggbyte_buffer;

struct ogg_sync_state {
  /* decode memory management pool */
  ogg_buffer_state *bufferpool;

  /* stream buffers */
  ogg_reference    *fifo_head;
  ogg_reference    *fifo_tail;
  
  long              fifo_cursor;
  long              fifo_fill;
  ogg_reference    *returned_header;
  ogg_reference    *returned_body;

  /* stream sync management */
  int               unsynced;
  int               headerbytes;
  int               bodybytes;

};

struct ogg_stream_state {
  /* encode memory management pool */
  ogg_buffer_state *bufferpool;

  ogg_reference *header_head;
  ogg_reference *header_tail;
  ogg_reference *body_head;
  ogg_reference *body_tail;
  long           body_fill;

  /* encode-side header build */
  oggpack_buffer    lacing;
  int               lacing_fill;

  ogg_reference *returned;
  ogg_reference *returned_head;

  long           header_len;
  long           body_len;

  int            e_o_s;    /* set when we have buffered the last
                              packet in the logical bitstream */
  int            b_o_s;    /* set after we've written the initial page
			      of a logical bitstream */
  int            headers;  /* how many setup headers? */
  long           serialno;
  long           pageno;
  ogg_int64_t    packetno; /* sequence number for decode; the framing
			      knows where there's a hole in the data,
			      but we need coupling so that the codec
			      (which is in a seperate abstraction
			      layer) also knows about the gap */

};

extern ogg_buffer_state *ogg_buffer_create(void);
extern void           ogg_buffer_destroy(ogg_buffer_state *bs);
extern ogg_reference *ogg_buffer_alloc(ogg_buffer_state *bs,long bytes);
extern void           ogg_buffer_realloc(ogg_reference *or,long bytes);
extern ogg_reference *ogg_buffer_dup(ogg_reference *or,long begin,long length);
extern ogg_reference *ogg_buffer_extend(ogg_reference *or,long bytes);
extern void           ogg_buffer_mark(ogg_reference *or);
extern void           ogg_buffer_release(ogg_reference *or);
extern void           ogg_buffer_release_one(ogg_reference *or);
extern ogg_reference *ogg_buffer_pretruncate(ogg_reference *or,long pos);
extern void           ogg_buffer_posttruncate(ogg_reference *or,long pos);
extern ogg_reference *ogg_buffer_cat(ogg_reference *tail, ogg_reference *head);
extern ogg_reference *ogg_buffer_walk(ogg_reference *or);
extern long           ogg_buffer_length(ogg_reference *or);

extern  int           oggbyte_init(oggbyte_buffer *b,ogg_reference *or,
				   long base,ogg_buffer_state *bs);
extern void           oggbyte_set1(oggbyte_buffer *b,unsigned char val,
				   int pos);
extern void           oggbyte_set2(oggbyte_buffer *b,int val,int pos);
extern void           oggbyte_set4(oggbyte_buffer *b,ogg_uint32_t val,int pos);
extern void           oggbyte_set8(oggbyte_buffer *b,ogg_int64_t val,int pos);
extern unsigned char  oggbyte_read1(oggbyte_buffer *b,int pos);
extern int            oggbyte_read2(oggbyte_buffer *b,int pos);
extern ogg_uint32_t   oggbyte_read4(oggbyte_buffer *b,int pos);
extern ogg_int64_t    oggbyte_read8(oggbyte_buffer *b,int pos);

#ifdef _V_SELFTEST
#define OGGPACK_CHUNKSIZE 3
#else
#define OGGPACK_CHUNKSIZE 128
#endif

#endif



