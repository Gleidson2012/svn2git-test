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

 function: toplevel libogg include
 last mod: $Id: ogg.h,v 1.1.2.1 2003/03/06 23:12:28 xiphmont Exp $

 ********************************************************************/
#ifndef _OGG_H
#define _OGG_H

#ifdef __cplusplus
extern "C" {
#endif
  
#include <ogg/os_types.h>
  
struct ogg_buffer;
struct ogg_buffer_state;
struct ogg_sync_state;
struct ogg_stream_state;
struct oggpack_buffer;

typedef struct ogg_buffer ogg_buffer;
typedef struct ogg_buffer_state ogg_buffer_state;
typedef struct ogg_sync_state ogg_sync_state;
typedef struct ogg_stream_state ogg_stream_state;
typedef struct oggpack_buffer oggpack_buffer;

typedef struct ogg_buffer_reference {
  ogg_buffer       *buffer;
  int               begin;
  int               length;
  ogg_buffer_state *owner; 
} ogg_buffer_reference;
  
typedef struct {
  ogg_buffer_reference packet;

  long         b_o_s;
  long         e_o_s;
  ogg_int64_t  granulepos;
  ogg_int64_t  packetno;     /* sequence number for decode; the framing
                                knows where there's a hole in the data,
                                but we need coupling so that the codec
                                (which is in a seperate abstraction
                                layer) also knows about the gap */
} ogg_packet;

typedef struct {
  ogg_buffer_reference header;
  ogg_buffer_reference body;
} ogg_page;

/* Ogg BITSTREAM PRIMITIVES: bitstream ************************/

extern void  oggpack_writeinit(oggpack_buffer *b,ogg_buffer_state *bs);
extern void  oggpack_writebuffer(oggpack_buffer *b,ogg_buffer_reference *r);
extern void  oggpack_writealign(oggpack_buffer *b);
extern void  oggpack_writeclear(oggpack_buffer *b);
extern void  oggpack_readinit(oggpack_buffer *b,ogg_buffer_reference *r);
extern void  oggpack_write(oggpack_buffer *b,unsigned long value,int bits);
extern int   oggpack_look(oggpack_buffer *b,int bits,unsigned long *ret);
extern long  oggpack_look1(oggpack_buffer *b);
extern void  oggpack_adv(oggpack_buffer *b,int bits);
extern void  oggpack_adv1(oggpack_buffer *b);
extern int   oggpack_read(oggpack_buffer *b,int bits,unsigned long *ret);
extern long  oggpack_read1(oggpack_buffer *b);
extern long  oggpack_bytes(oggpack_buffer *b);
extern long  oggpack_bits(oggpack_buffer *b);

extern void  oggpackB_writeinit(oggpack_buffer *b,ogg_buffer_state *bs);
extern void  oggpackB_writebuffer(oggpack_buffer *b,ogg_buffer_reference *r);
extern void  oggpackB_writealign(oggpack_buffer *b);
extern void  oggpackB_writeclear(oggpack_buffer *b);
extern void  oggpackB_readinit(oggpack_buffer *b,ogg_buffer_reference *r);
extern void  oggpackB_write(oggpack_buffer *b,unsigned long value,int bits);
extern int   oggpackB_look(oggpack_buffer *b,int bits,unsigned long *ret);
extern long  oggpackB_look1(oggpack_buffer *b);
extern void  oggpackB_adv(oggpack_buffer *b,int bits);
extern void  oggpackB_adv1(oggpack_buffer *b);
extern int   oggpackB_read(oggpack_buffer *b,int bits,unsigned long *ret);
extern long  oggpackB_read1(oggpack_buffer *b);
extern long  oggpackB_bytes(oggpack_buffer *b);
extern long  oggpackB_bits(oggpack_buffer *b);

/* Ogg BITSTREAM PRIMITIVES: encoding **************************/

extern int      ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op);
extern int      ogg_stream_pageout(ogg_stream_state *os, ogg_page *og);
extern int      ogg_stream_flush(ogg_stream_state *os, ogg_page *og);

/* Ogg BITSTREAM PRIMITIVES: decoding **************************/

extern int      ogg_sync_init(ogg_sync_state *oy);
extern int      ogg_sync_clear(ogg_sync_state *oy);
extern int      ogg_sync_reset(ogg_sync_state *oy);

extern unsigned char *ogg_sync_buffer(ogg_sync_state *oy, long size);
extern int      ogg_sync_wrote(ogg_sync_state *oy, long bytes);
extern long     ogg_sync_pageseek(ogg_sync_state *oy,ogg_page *og);
extern int      ogg_sync_pageout(ogg_sync_state *oy, ogg_page *og);
extern int      ogg_stream_pagein(ogg_stream_state *os, ogg_page *og);
extern int      ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op);
extern int      ogg_stream_packetpeek(ogg_stream_state *os,ogg_packet *op);

/* Ogg BITSTREAM PRIMITIVES: general ***************************/

extern int      ogg_stream_init(ogg_stream_state *os,int serialno);
extern int      ogg_stream_clear(ogg_stream_state *os);
extern int      ogg_stream_reset(ogg_stream_state *os);
extern int      ogg_stream_reset_serialno(ogg_stream_state *os,int serialno);
extern int      ogg_stream_destroy(ogg_stream_state *os);
extern int      ogg_stream_eos(ogg_stream_state *os);

extern void     ogg_page_checksum_set(ogg_page *og);

extern int      ogg_page_version(ogg_page *og);
extern int      ogg_page_continued(ogg_page *og);
extern int      ogg_page_bos(ogg_page *og);
extern int      ogg_page_eos(ogg_page *og);
extern ogg_int64_t  ogg_page_granulepos(ogg_page *og);
extern ogg_uint32_t ogg_page_serialno(ogg_page *og);
extern ogg_uint32_t ogg_page_pageno(ogg_page *og);
extern int      ogg_page_packets(ogg_page *og);
extern int      ogg_page_getbuffer(ogg_page *og, unsigned char **buffer);

extern void     ogg_packet_clear(ogg_packet *op);


#ifdef __cplusplus
}
#endif

#endif  /* _OGG_H */






