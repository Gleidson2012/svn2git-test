/* libOggFLAC - Free Lossless Audio Codec + Ogg library
 * Copyright (C) 2002,2003  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#ifndef OggFLAC__PROTECTED__SEEKABLE_STREAM_DECODER_H
#define OggFLAC__PROTECTED__SEEKABLE_STREAM_DECODER_H

#include "OggFLAC/seekable_stream_decoder.h"

typedef struct OggFLAC__SeekableStreamDecoderProtected {
	OggFLAC__SeekableStreamDecoderState state;
} OggFLAC__SeekableStreamDecoderProtected;

#endif
