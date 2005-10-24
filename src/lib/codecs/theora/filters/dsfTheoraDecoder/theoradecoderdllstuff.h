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
#include <streams.h>
#include <pullpin.h>
#include <initguid.h>

//#ifdef DSFABSTRACTOGGVIDEODECODER_EXPORTS
//#pragma message("----> Exporting from Abstract Video Library...")
//#define ABS_VIDEO_DEC_API __declspec(dllexport)
//#else
//#pragma message("<---- Importing from Abstract Video Library...")
//#define ABS_VIDEO_DEC_API __declspec(dllimport)
//#endif

#ifdef LIBOOOGG_EXPORTS
#define LIBOOOGG_API __declspec(dllexport)
#else
#define LIBOOOGG_API __declspec(dllimport)
#endif

// {05187161-5C36-4324-A734-22BF37509F2D}
DEFINE_GUID(CLSID_TheoraDecodeFilter, 
0x5187161, 0x5c36, 0x4324, 0xa7, 0x34, 0x22, 0xbf, 0x37, 0x50, 0x9f, 0x2d);

//// {D124B2B1-8968-4ae8-B288-FE16EA34B0CE}
//DEFINE_GUID(MEDIASUBTYPE_Theora, 
//0xd124b2b1, 0x8968, 0x4ae8, 0xb2, 0x88, 0xfe, 0x16, 0xea, 0x34, 0xb0, 0xce);
//
//// {A99F116C-DFFA-412c-95DE-725F99874826}
//DEFINE_GUID(FORMAT_Theora, 
//0xa99f116c, 0xdffa, 0x412c, 0x95, 0xde, 0x72, 0x5f, 0x99, 0x87, 0x48, 0x26);


// {60891713-C24F-4767-B6C9-6CA05B3338FC}
DEFINE_GUID(MEDIATYPE_OggPacketStream, 
0x60891713, 0xc24f, 0x4767, 0xb6, 0xc9, 0x6c, 0xa0, 0x5b, 0x33, 0x38, 0xfc);

// {95388704-162C-42a9-8149-C3577C12AAF9}
DEFINE_GUID(FORMAT_OggIdentHeader, 
0x95388704, 0x162c, 0x42a9, 0x81, 0x49, 0xc3, 0x57, 0x7c, 0x12, 0xaa, 0xf9);

// {43F0F818-10B0-4c86-B9F1-F6B6E2D33462}
DEFINE_GUID(IID_IOggDecoder, 
0x43f0f818, 0x10b0, 0x4c86, 0xb9, 0xf1, 0xf6, 0xb6, 0xe2, 0xd3, 0x34, 0x62);

const REGPINTYPES TheoraDecodeOutputTypes = {
    &MEDIATYPE_Video,
	&MEDIASUBTYPE_YV12
};

const REGPINTYPES TheoraDecodeInputTypes = {
	&MEDIATYPE_OggPacketStream,
	&MEDIASUBTYPE_None
};
const REGFILTERPINS TheoraDecodePinReg[] = {
	{
    L"Theora Input",						//Name (obsoleted)
	FALSE,								//Renders from this pin ?? Not sure about this.
	FALSE,								//Not an output pin
	FALSE,								//Cannot have zero instances of this pin
	FALSE,								//Cannot have more than one instance of this pin
	NULL,								//Connects to filter (obsoleted)
	NULL,								//Connects to pin (obsoleted)
	1,									//upport two media type
	&TheoraDecodeInputTypes				//Pointer to media type (Video/Theora)
	} ,

	{
	L"YV12 Output",						//Name (obsoleted)
	FALSE,								//Renders from this pin ?? Not sure about this.
	TRUE,								//Is an output pin
	FALSE,								//Cannot have zero instances of this pin
	FALSE,								//Cannot have more than one instance of this pin
	NULL,								//Connects to filter (obsoleted)
	NULL,								//Connects to pin (obsoleted)
	1,									//Only support one media type
	&TheoraDecodeOutputTypes					//Pointer to media type (YV12)

	}
};



const REGFILTER2 TheoraDecodeFilterReg = {
		1,
		MERIT_NORMAL,
		2,
        TheoraDecodePinReg
		
};
	   

struct sTheoraFormatBlock {
	unsigned long theoraVersion;
	unsigned long outerFrameWidth;
	unsigned long outerFrameHeight;
	unsigned long pictureWidth;
	unsigned long pictureHeight;
	unsigned long frameRateNumerator;
	unsigned long frameRateDenominator;
	unsigned long aspectNumerator;
	unsigned long aspectDenominator;
	unsigned long maxKeyframeInterval;
	unsigned long targetBitrate;
	unsigned char targetQuality;
	unsigned char xOffset;
	unsigned char yOffset;
	unsigned char colourSpace;
};