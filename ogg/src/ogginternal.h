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
 last mod: $Id: ogginternal.h,v 1.1.2.2 2003/01/21 08:18:34 xiphmont Exp $

 ********************************************************************/

#ifndef _OGGI_H
#define _OGGI_H

#include <ogg/ogg.h>
#include "mutex.h"

struct ogg_buffer_state{
  ogg_buffer *unused_pool;
  int         outstanding;
  ogg_mutex_t mutex;
};

typedef struct{
  ogg_buffer_reference *segment;
  int                   cursor;
} fragmented_cursor;

struct ogg_buffer {
  unsigned char     *data;
  int                used;
  struct ogg_buffer *next;
  int                size;
  int                refcount;
};

struct oggpack_buffer {
  int               headbit;
  unsigned char    *headptr;
  long              headend;

  /* memory management */
  ogg_buffer       *head;
  ogg_buffer       *tail;  
  ogg_buffer_state *owner; /* centralized mem management; buffer fragment 
			      memory is owned and managed by the physical
			      stream abstraction */

  /* render the byte/bit counter API constant time */
  long length; /* meaningful only in decode */
  long count;  /* doesn't count the tail */

};

typedef struct ogg_packet_chain {
  ogg_buffer_reference     packet;
  ogg_int64_t              granulepos;

  struct ogg_packet_chain *next;
} ogg_packet_chain;

struct ogg_sync_state {

  /* stream buffers */
  ogg_buffer *unused_fifo;
  ogg_buffer *fifo_head;
  ogg_buffer *fifo_tail;
  ogg_buffer *fifo_returned;
  int         fifo_returned_pos;
  int         fifo_fill;

  /* stream sync management */
  int         unsynced;
  int         headerbytes;
  int         bodybytes;

};

struct ogg_stream_state {
  ogg_packet_chain *unused;
  ogg_packet_chain *head;
  ogg_packet_chain *tail;


  long              body_len;

  unsigned char    *header;       /* working space for header encode */
  int               header_fill;

  int     e_o_s;        /* set when we have buffered the last packet in the
                           logical bitstream */
  int     b_o_s;        /* set after we've written the initial page
                           of a logical bitstream */
  int     headers;      /* how many setup headers? */
  long    serialno;
  long    pageno;
  ogg_int64_t packetno; /* sequence number for decode; the framing
                           knows where there's a hole in the data,
                           but we need coupling so that the codec
                           (which is in a seperate abstraction
                           layer) also knows about the gap */

  /* for sync memory management use */
  struct ogg_sync_state   *sync;
  struct ogg_stream_state *next;

};

#endif
