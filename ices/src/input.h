/* input.h
 * - Input functions
 *
 * $Id: input.h,v 1.8 2003/03/16 14:21:48 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __INPUT_H__
#define __INPUT_H__

#include <shout/shout.h>
#include <vorbis/codec.h>

#include "config.h"
#include "inputmodule.h"
#include "stream.h"
#include "reencode.h"
#include "encode.h"
#include "audio.h"

typedef struct {
    instance_t *stream;
    input_module_t *input;
    reencode_state *reenc;
    encoder_state *enc;
    downmix_state *downmix;
    resample_state *resamp;
    shout_t *shout;
    vorbis_comment vc;
} stream_description;


void input_loop(void);
void input_flush_queue(buffer_queue *queue, int keep_critical);
void input_sleep(void);
int  input_calculate_ogg_sleep(ogg_page *og);
int  input_calculate_pcm_sleep(unsigned bytes, unsigned bytes_per_sec);


#endif /* __INPUT_H__ */


