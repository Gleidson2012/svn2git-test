/* Copyright (C) 2002 Jean-Marc Valin*/
/**
  @file speex_callbacks.h
  @brief Describes callback handling and in-band signalling
*/
/*
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef SPEEX_CALLBACKS_H
#define SPEEX_CALLBACKS_H

#include "speex.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPEEX_MAX_CALLBACKS 16

/* Describes all the in-band requests */

/*These are 1-bit requests*/
#define SPEEX_INBAND_ENH_REQUEST         0
#define SPEEX_INBAND_VBR_REQUEST         1

/*These are 4-bit requests*/
#define SPEEX_INBAND_MODE_REQUEST        2
#define SPEEX_INBAND_LOW_MODE_REQUEST    3
#define SPEEX_INBAND_HIGH_MODE_REQUEST   4
#define SPEEX_INBAND_VBR_QUALITY_REQUEST 5
#define SPEEX_INBAND_ACKNOWLEDGE_REQUEST 6

/*These are 8-bit requests*/
/** Send a character in-band */
#define SPEEX_INBAND_CHAR                8

#define SPEEX_INBAND_MAX_BITRATE         10

#define SPEEX_INBAND_ACKNOWLEDGE         12

/** Callback function type */
typedef int (*speex_callback_func)(SpeexBits *bits, void *state, void *data);

typedef struct SpeexCallback {
   int callback_id;
   speex_callback_func func;
   void *data;
} SpeexCallback;

int speex_inband_handler(SpeexBits *bits, SpeexCallback *callback_list, void *state);

int speex_std_mode_request_handler(SpeexBits *bits, void *state, void *data);

int speex_std_high_mode_request_handler(SpeexBits *bits, void *state, void *data);

int speex_std_char_handler(SpeexBits *bits, void *state, void *data);


int speex_default_user_handler(SpeexBits *bits, void *state, void *data);

#ifdef __cplusplus
}
#endif


#endif
