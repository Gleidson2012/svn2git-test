/***
 * CopyPolicy: GNU Public License 2 applies
 * Copyright (C) by Monty (xiphmont@mit.edu)
 ***/

#ifndef _p_block_h_
#define _p_block_h_

#include "../interface/cdda_interface.h"

#define MIN_WORDS_OVERLAP    64     /* 16 bit words */
#define MIN_WORDS_SEARCH     64     /* 16 bit words */
#define MIN_WORDS_RIFT       16     /* 16 bit words */
#define MAX_SECTOR_OVERLAP   32     /* sectors */
#define MIN_SECTOR_EPSILON  128     /* words */
#define MIN_SECTOR_BACKUP    16     /* sectors */
#define JIGGLE_MODULO         8     /* sectors */

#define min(x,y) ((x)>(y)?(y):(x))
#define max(x,y) ((x)<(y)?(y):(x))

typedef struct linked_list{
  /* linked list */
  struct linked_element *head;
  struct linked_element *tail;

  void *(*new_poly)();
  void (*free_poly)(void *poly);
  long current;
  long active;

} linked_list;

typedef struct linked_element{
  void *ptr;
  struct linked_element *prev;
  struct linked_element *next;
  
  struct linked_list *list;
  int stamp;
} linked_element;

extern linked_list *new_list(void *(*new)(void),void (*free)(void *));
extern linked_element *new_elem(linked_list *list);
extern linked_element *add_elem(linked_list *list,void *elem);
extern void free_list(linked_list *list,int free_ptr); /* unlink or free */
extern void free_elem(linked_element *e,int free_ptr); /* unlink or free */
extern void *get_elem(linked_element *e);
extern linked_list *copy_list(linked_list *list); /* shallow; doesn't copy
						     contained structures */

typedef struct c_block{
  /* The buffer */
  void *vector;  /* (is_vector *) */

  /* auxiliary support structures */
  unsigned char *flags; /* 1    known boundaries in read data
			   2    known blanked data
			   4    used sample
			   8    reserved
			   16   reserved
			   32   reserved
			   64   reserved
			   128  reserved
			 */

  /* end of session cases */
  long lastsector;
  struct cdrom_paranoia *p;
  struct linked_element *e;

} c_block;

extern void free_c_block(c_block *c);
extern c_block *new_c_block(struct cdrom_paranoia *p);

typedef struct v_fragment{
  c_block *one;
  
  long begin;
  long size;

  /* end of session cases */
  long lastsector;

  /* linked list */
  struct cdrom_paranoia *p;
  struct linked_element *e;

} v_fragment;

extern void free_v_fragment(v_fragment *c);
extern v_fragment *new_v_fragment(struct cdrom_paranoia *p,c_block *one,
				  long begin, long end, int lastsector);
extern size16 *v_buffer(v_fragment *v);

extern c_block *c_first(struct cdrom_paranoia *p);
extern c_block *c_last(struct cdrom_paranoia *p);
extern c_block *c_next(c_block *c);
extern c_block *c_prev(c_block *c);

extern v_fragment *v_first(struct cdrom_paranoia *p);
extern v_fragment *v_last(struct cdrom_paranoia *p);
extern v_fragment *v_next(v_fragment *v);
extern v_fragment *v_prev(v_fragment *v);

typedef struct root_block{
  long returnedlimit;   
  long lastsector;
  struct cdrom_paranoia *p;

  void *vector;
} root_block;

typedef struct offsets{
  
  long offpoints;
  long newpoints;
  long offaccum;
  long offdiff;
  long offmin;
  long offmax;

} offsets;

typedef struct cdrom_paranoia{
  cdrom_drive *d;

  root_block root;        /* verified/reconstructed cached data */
  linked_list *cache;     /* our data as read from the cdrom */
  long cache_limit;
  linked_list *fragments; /* fragments of blocks that have been 'verified' */

  int readahead;          /* sectors of readahead in each readop */
  int jitter;           
  long lastread;

  int enable;
  long cursor;
  long current_lastsector;
  long current_firstsector;

  /* statistics for drift/overlap */
  struct offsets stage1;
  struct offsets stage2;

  long dynoverlap;
  long dyndrift;

  /* statistics for verification */

} cdrom_paranoia;

extern void recover_cache(cdrom_paranoia *p);
extern void i_paranoia_firstlast(cdrom_paranoia *p);

#define CDP_COMPILE
#endif

