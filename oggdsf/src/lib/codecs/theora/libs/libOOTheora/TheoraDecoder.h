//===========================================================================
//Copyright (C) 2003, 2004 Zentaro Kavanagh
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//- Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//- Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//- Neither the name of Zentaro Kavanagh nor the names of contributors 
//  may be used to endorse or promote products derived from this software 
//  without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//===========================================================================

#pragma once
#include "libootheora.h"
#include <libOOOgg/dllstuff.h>
#include <libOOOgg/StampedOggPacket.h>
extern "C" {
#include "theora_cdecl.h"
}
class LIBOOTHEORA_API TheoraDecoder
{
public:
	TheoraDecoder(void);
	~TheoraDecoder(void);

	/// Initialise the internal theora decoder.
	bool initCodec();

	//bool resetPackCount();
	//bool clearAll();

	/// Decode a theora packet returning a yuv_buffer struct. Now owns your packet.
	yuv_buffer* decodeTheora(StampedOggPacket* inPacket);

	/// Returns true if the packet is a keyframe.
	bool isKeyFrame(StampedOggPacket* inPacket);

	theora_info mTheoraInfo;
protected:

	/// Moves the pointers around to make it look like a xiph ogg packet.
	ogg_packet* simulateOldOggPacket(StampedOggPacket* inPacket);

	/// Decodes a header packet and adjusts the internal state based on it.
	bool decodeHeader(StampedOggPacket* inHeaderPacket);

	//theora_info mTheoraInfo;
	theora_comment mTheoraComment;
	theora_state mTheoraState;
	yuv_buffer mYUVBuffer;

	StampedOggPacket* mPartialPacket; //TEMP !!
	unsigned long mHeadersSeen;
	bool mFirstPacket;
	bool mFirstHeader;
	unsigned long mPacketCount;
	
};
