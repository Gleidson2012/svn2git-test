/* im_alsa.h
 * - read pcm data from oss devices
 *
 * $Id: im_alsa.h,v 1.1 2002/12/29 10:28:30 msmith Exp $
 *
 * by Jason Chu  <jchu@uvic.ca>, based
 * on im_oss.c which is...
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __IM_ALSA_H__
#define __IM_ALSA_H__

#include <alsa/asoundlib.h>
#include "inputmodule.h"
#include "thread.h"
#include <ogg/ogg.h>

typedef struct
{
	int rate;
	int channels;

	snd_pcm_t *fd;
	char **metadata;
	int newtrack;
	mutex_t metadatalock;
} im_alsa_state; 

input_module_t *alsa_open_module(module_param_t *params);

#endif  /* __IM_ALSA */
