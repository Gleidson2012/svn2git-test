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

#include "stdafx.h"

#include "TheoraDecodeFilter.h"



//COM Factory Template
CFactoryTemplate g_Templates[] = 
{
    { 
		L"Theora Decode Filter",					// Name
	    &CLSID_TheoraDecodeFilter,				// CLSID
	    TheoraDecodeFilter::CreateInstance,		// Method to create an instance of Theora Decoder
        NULL,									// Initialization function
        NULL									// Set-up information (for filters)
    }

};

// Generic way of determining the number of items in the template
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]); 



TheoraDecodeFilter::TheoraDecodeFilter() 
	:	CTransformFilter( NAME("Theora Decode Filter"), NULL, CLSID_TheoraDecodeFilter)
	,	mHeight(0)
	,	mWidth(0)
	,	mFrameSize(0)
	,	mFrameCount(0)
	,	mYOffset(0)
	,	mXOffset(0)
	,	mFrameDuration(0)
	,	mBegun(false)
	,	mSeekTimeBase(0)
	,	mLastSeenStartGranPos(0)
	,	mTheoraFormatInfo(NULL)
{
#ifdef OGGCODECS_LOGGING
	debugLog.open("G:\\logs\\newtheofilter.log", ios_base::out);
#endif
	mTheoraDecoder = new TheoraDecoder;
	mTheoraDecoder->initCodec();

}

TheoraDecodeFilter::~TheoraDecodeFilter() {
	delete mTheoraDecoder;
	mTheoraDecoder = NULL;

	delete mTheoraFormatInfo;
	mTheoraFormatInfo = NULL;
	debugLog.close();

}

CUnknown* WINAPI TheoraDecodeFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
{
	//This routine is the COM implementation to create a new Filter
	TheoraDecodeFilter *pNewObject = new TheoraDecodeFilter();
    if (pNewObject == NULL) {
        *pHr = E_OUTOFMEMORY;
    }
	return pNewObject;
} 
void TheoraDecodeFilter::FillMediaType(CMediaType* outMediaType, unsigned long inSampleSize) {
	outMediaType->SetType(&MEDIATYPE_Video);
	outMediaType->SetSubtype(&MEDIASUBTYPE_YV12);
	outMediaType->SetFormatType(&FORMAT_VideoInfo);
	outMediaType->SetTemporalCompression(FALSE);
	outMediaType->SetSampleSize(inSampleSize);		

}
bool TheoraDecodeFilter::FillVideoInfoHeader(VIDEOINFOHEADER* inFormatBuffer) {
	TheoraDecodeFilter* locFilter = this;

	inFormatBuffer->AvgTimePerFrame = (UNITS * locFilter->mTheoraFormatInfo->frameRateDenominator) / locFilter->mTheoraFormatInfo->frameRateNumerator;
	inFormatBuffer->dwBitRate = locFilter->mTheoraFormatInfo->targetBitrate;
	
	inFormatBuffer->bmiHeader.biBitCount = 12;   //12 bits per pixel
	inFormatBuffer->bmiHeader.biClrImportant = 0;   //All colours important
	inFormatBuffer->bmiHeader.biClrUsed = 0;        //Use max colour depth
	inFormatBuffer->bmiHeader.biCompression = MAKEFOURCC('Y','V','1','2');
	inFormatBuffer->bmiHeader.biHeight = locFilter->mTheoraFormatInfo->pictureHeight;   //Not sure
	inFormatBuffer->bmiHeader.biPlanes = 1;    //Must be 1
	inFormatBuffer->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);    //????? Size of what ?
	inFormatBuffer->bmiHeader.biSizeImage = ((locFilter->mTheoraFormatInfo->pictureHeight * locFilter->mTheoraFormatInfo->pictureWidth) * 3)/2;    //Size in bytes of image ??
	inFormatBuffer->bmiHeader.biWidth = locFilter->mTheoraFormatInfo->pictureWidth;
	inFormatBuffer->bmiHeader.biXPelsPerMeter = 2000;   //Fuck knows
	inFormatBuffer->bmiHeader.biYPelsPerMeter = 2000;   //" " " " " 
	
	inFormatBuffer->rcSource.top = 0;
	inFormatBuffer->rcSource.bottom = locFilter->mTheoraFormatInfo->pictureHeight;
	inFormatBuffer->rcSource.left = 0;
	inFormatBuffer->rcSource.right = locFilter->mTheoraFormatInfo->pictureWidth;

	inFormatBuffer->rcTarget.top = 0;
	inFormatBuffer->rcTarget.bottom = locFilter->mTheoraFormatInfo->pictureHeight;
	inFormatBuffer->rcTarget.left = 0;
	inFormatBuffer->rcTarget.right = locFilter->mTheoraFormatInfo->pictureWidth;

	inFormatBuffer->dwBitErrorRate=0;
	return true;
}

HRESULT TheoraDecodeFilter::CheckInputType(const CMediaType* inMediaType) 
{

	
	if	( (inMediaType->majortype == MEDIATYPE_OggPacketStream) &&
			(inMediaType->subtype == MEDIASUBTYPE_None) && (inMediaType->formattype == FORMAT_OggIdentHeader)
		)
	{
		if (inMediaType->cbFormat == THEORA_IDENT_HEADER_SIZE) {
			if (strncmp((char*)inMediaType->pbFormat, "\200theora", 7) == 0) {
				//TODO::: Possibly verify version
				return S_OK;
			}
		}

	}
	return S_FALSE;
	
}
HRESULT TheoraDecodeFilter::CheckTransform(const CMediaType* inInputMediaType, const CMediaType* inOutputMediaType) {
	if ((CheckInputType(inInputMediaType) == S_OK) &&
		((inOutputMediaType->majortype == MEDIATYPE_Video) && (inOutputMediaType->subtype == MEDIASUBTYPE_YV12) && (inOutputMediaType->formattype == FORMAT_VideoInfo)
		)) {
		VIDEOINFOHEADER* locVideoHeader = (VIDEOINFOHEADER*)inOutputMediaType->Format();

	//	if ((locVideoHeader->bmiHeader.biHeight == mTheoraFormatInfo->pictureHeight) && (locVideoHeader->bmiHeader.biWidth == mTheoraFormatInfo->pictureWidth)) {

			mHeight = (unsigned long)abs(locVideoHeader->bmiHeader.biHeight);
			mWidth = (unsigned long)abs(locVideoHeader->bmiHeader.biWidth);
			return S_OK;
	//	} else {
	//		return S_FALSE;
	//	}
	} else {
		return S_FALSE;
	}
}
HRESULT TheoraDecodeFilter::DecideBufferSize(IMemAllocator* inAllocator, ALLOCATOR_PROPERTIES* inPropertyRequest) {
	//debugLog<<endl;
	//debugLog<<"DecideBufferSize :"<<endl;
	//FIX::: Abstract this out properly	

	//debugLog<<"Allocator is "<<(unsigned long)inAllocator<<endl;
	//Our error variable
	HRESULT locHR = S_OK;

	//Create the structures for setproperties to use
	ALLOCATOR_PROPERTIES locReqAlloc;
	ALLOCATOR_PROPERTIES locActualAlloc;

	//debugLog<<"DecideBufferSize : Requested :"<<endl;
	//debugLog<<"DecideBufferSize : Align     : "<<inPropertyRequest->cbAlign<<endl;
	//debugLog<<"DecideBufferSize : BuffSize  : "<<inPropertyRequest->cbBuffer<<endl;
	//debugLog<<"DecideBufferSize : Prefix    : "<<inPropertyRequest->cbPrefix<<endl;
	//debugLog<<"DecideBufferSize : NumBuffs  : "<<inPropertyRequest->cBuffers<<endl;


	const unsigned long MIN_BUFFER_SIZE = 16*16;			//What should this be ????
	const unsigned long DEFAULT_BUFFER_SIZE = 1024*1024 * 2;
	const unsigned long MIN_NUM_BUFFERS = 1;
	const unsigned long DEFAULT_NUM_BUFFERS = 1;

	
	//Validate and change what we have been requested to do.
	//Allignment of data
	if (inPropertyRequest->cbAlign <= 0) {
		locReqAlloc.cbAlign = 1;
	} else {
		locReqAlloc.cbAlign = inPropertyRequest->cbAlign;
	}

	//Size of each buffer
	if (inPropertyRequest->cbBuffer < MIN_BUFFER_SIZE) {
		locReqAlloc.cbBuffer = DEFAULT_BUFFER_SIZE;
	} else {
		locReqAlloc.cbBuffer = inPropertyRequest->cbBuffer;
	}

	//How many prefeixed bytes
	if (inPropertyRequest->cbPrefix < 0) {
			locReqAlloc.cbPrefix = 0;
	} else {
		locReqAlloc.cbPrefix = inPropertyRequest->cbPrefix;
	}

	//Number of buffers in the allcoator
	if (inPropertyRequest->cBuffers < MIN_NUM_BUFFERS) {
		locReqAlloc.cBuffers = DEFAULT_NUM_BUFFERS;
	} else {

		locReqAlloc.cBuffers = inPropertyRequest->cBuffers;
	}

	//debugLog<<"DecideBufferSize : Modified Request :"<<endl;
	//debugLog<<"DecideBufferSize : Align     : "<<locReqAlloc.cbAlign<<endl;
	//debugLog<<"DecideBufferSize : BuffSize  : "<<locReqAlloc.cbBuffer<<endl;
	//debugLog<<"DecideBufferSize : Prefix    : "<<locReqAlloc.cbPrefix<<endl;
	//debugLog<<"DecideBufferSize : NumBuffs  : "<<locReqAlloc.cBuffers<<endl;


	//Set the properties in the allocator
	locHR = inAllocator->SetProperties(&locReqAlloc, &locActualAlloc);

	//debugLog<<"DecideBufferSize : SetProperties returns "<<locHR<<endl;
	//debugLog<<"DecideBufferSize : Actual Params :"<<endl;
	//debugLog<<"DecideBufferSize : Align     : "<<locActualAlloc.cbAlign<<endl;
	//debugLog<<"DecideBufferSize : BuffSize  : "<<locActualAlloc.cbBuffer<<endl;
	//debugLog<<"DecideBufferSize : Prefix    : "<<locActualAlloc.cbPrefix<<endl;
	//debugLog<<"DecideBufferSize : NumBuffs  : "<<locActualAlloc.cBuffers<<endl;

	//Check the response
	switch (locHR) {
		case E_POINTER:
			//debugLog<<"DecideBufferSize : SetProperties - NULL POINTER"<<endl;
			return locHR;
			

		case VFW_E_ALREADY_COMMITTED:
			//debugLog<<"DecideBufferSize : SetProperties - Already COMMITED"<<endl;
			return locHR;
			
		case VFW_E_BADALIGN:
			//debugLog<<"DecideBufferSize : SetProperties - Bad ALIGN"<<endl;
			return locHR;
			
		case VFW_E_BUFFERS_OUTSTANDING:
			//debugLog<<"DecideBufferSize : SetProperties - BUFFS OUTSTANDING"<<endl;
			return locHR;
			

		case S_OK:

			break;
		default:
			//debugLog<<"DecideBufferSize : SetProperties - UNKNOWN ERROR"<<endl;
			break;

	}

	
	//TO DO::: Do we commit ?
	//RESOLVED ::: Yep !
	
	locHR = inAllocator->Commit();
	//debugLog<<"DecideBufferSize : Commit Returned "<<locHR<<endl;


	switch (locHR) {
		case E_FAIL:
			//debugLog<<"DecideBufferSize : Commit - FAILED "<<endl;
			return locHR;
		case E_POINTER:
			//debugLog<<"DecideBufferSize : Commit - NULL POINTER "<<endl;
			return locHR;
		case E_INVALIDARG:
			//debugLog<<"DecideBufferSize : Commit - INVALID ARG "<<endl;
			return locHR;
		case E_NOTIMPL:
			//debugLog<<"DecideBufferSize : Commit - NOT IMPL"<<endl;
			return locHR;
		case S_OK:
			//debugLog<<"DecideBufferSize : Commit - ** SUCCESS **"<<endl;
			break;
		default:
			//debugLog<<"DecideBufferSize : Commit - UNKNOWN ERROR "<<endl;
			return locHR;
	}


	return S_OK;
}
HRESULT TheoraDecodeFilter::GetMediaType(int inPosition, CMediaType* outOutputMediaType) {
	if (inPosition < 0) {
		return E_INVALIDARG;
	}
	
	if (inPosition == 0) {
		
		VIDEOINFOHEADER* locVideoFormat = (VIDEOINFOHEADER*)outOutputMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
		FillVideoInfoHeader(locVideoFormat);
		FillMediaType(outOutputMediaType, locVideoFormat->bmiHeader.biSizeImage);
		//debugLog<<"Vid format size "<<locVideoFormat->bmiHeader.biSizeImage<<endl;
		//outMediaType->SetSampleSize(locVideoFormat->bmiHeader.biSizeImage);
		//debugLog<<"Returning from GetMediaType"<<endl;
		return S_OK;
	} else {
		return VFW_S_NO_MORE_ITEMS;
	}
}

void TheoraDecodeFilter::ResetFrameCount() 
{
	//XTODO::: Maybe not needed
	mFrameCount = 0;
	
}

HRESULT TheoraDecodeFilter::NewSegment(REFERENCE_TIME inStart, REFERENCE_TIME inEnd, double inRate) 
{
	debugLog<<"Resetting frame count"<<endl;
	ResetFrameCount();
	return CTransformFilter::NewSegment(inStart, inEnd, inRate);

}
HRESULT TheoraDecodeFilter::Transform(IMediaSample* inInputSample, IMediaSample* outOutputSample) {

	//CAutoLock locLock(mStreamLock);
	//debugLog<<endl<<"Transform "<<endl;
	//debugLog<<"outOutputSample Size = "<<outOutputSample->
	HRESULT locHR;
	BYTE* locBuff = NULL;
	//Get a source poitner into the input buffer
	locHR = inInputSample->GetPointer(&locBuff);

	//TODO::: This should be after the return value check !!
	BYTE* locNewBuff = new unsigned char[inInputSample->GetActualDataLength()];		//This gets put into a packet.
	memcpy((void*)locNewBuff, (const void*)locBuff, inInputSample->GetActualDataLength());


	if (locHR != S_OK) {
		//debugLog<<"Receive : Get pointer failed..."<<locHR<<endl;	
		return S_FALSE;
	} else {
		//debugLog<<"Receive : Get pointer succeeds..."<<endl;	
		//New start time hacks
		REFERENCE_TIME locStart = 0;
		REFERENCE_TIME locEnd = 0;
		inInputSample->GetTime(&locStart, &locEnd);
		//Error chacks needed here
		//debugLog<<"Input Sample Time - "<<locStart<<" to "<<locEnd<<endl;
		
		//More work arounds for that stupid granule pos scheme in theora!
		REFERENCE_TIME locTimeBase = 0;
		REFERENCE_TIME locDummy = 0;
		inInputSample->GetMediaTime(&locTimeBase, &locDummy);
		mSeekTimeBase = locTimeBase;
		//

		//debugLog<<"SeekTimeBase = "<<mSeekTimeBase<<endl;
		
		if ((mLastSeenStartGranPos != locStart) && (locStart != -1)) {
			//debugLog<<"Resetting frame count"<<endl;

			//FIXXX:::
			//ResetFrameCount();
			//

			mLastSeenStartGranPos = locStart;
			//debugLog<<"Setting base gran pos to "<<locStart<<endl;
		}
		
		//End of additions



		AM_MEDIA_TYPE* locMediaType = NULL;
		inInputSample->GetMediaType(&locMediaType);
		if (locMediaType == NULL) {
			//debugLog<<"No dynamic change..."<<endl;
		} else {
			//debugLog<<"Attempting dynamic change..."<<endl;
		}
		
		//This packet is given to the decoder.
		StampedOggPacket* locPacket = new StampedOggPacket(locNewBuff, inInputSample->GetActualDataLength(), false, false, locStart, locEnd, StampedOggPacket::OGG_END_ONLY);

		bool locIsKeyFrame = mTheoraDecoder->isKeyFrame(locPacket);
		yuv_buffer* locYUV = mTheoraDecoder->decodeTheora(locPacket);		//This accept the packet and deletes it
		if (locYUV != NULL) {
			if (TheoraDecoded(locYUV, outOutputSample, locIsKeyFrame) != 0) {
				//debugLog<<"Decoded *** FALSE ***"<<endl;
				return S_FALSE;
			}
		} else {
			//debugLog<<"!@&#^()!&@#!()*@#&)!(*@#&()!*@# NULL Decode"<<endl;
			return S_FALSE;
		}

		return S_OK;
		
	}
	
}

int TheoraDecodeFilter::TheoraDecoded (yuv_buffer* inYUVBuffer, IMediaSample* outSample, bool inIsKeyFrame) 
{
	//debugLog<<"TheoraDecoded... #################### "<<endl;
	
		
	if (!mBegun) {
		//debugLog<<"First time..."<<endl;
		mBegun = true;
		
		//How many UNITS does one frame take.
		mFrameDuration = (UNITS * mTheoraFormatInfo->frameRateDenominator) / (mTheoraFormatInfo->frameRateNumerator);

		mFrameSize = (mHeight * mWidth * 3) / 2;
		mFrameCount = 0;
		//debugLog<<"Frame Durn = "<<mFrameDuration<<endl;
		//debugLog<<"FrameSize = "<<mFrameSize<<endl;
		
		
	}


	////TO DO::: Fix this up... needs to move around order and some only needs to be done once, move it into the block aboce and use member data


	//-----------------------
	//OLD CODE... FIXXX:::
	//Timestamp hacks start here...
			//unsigned long locMod = (unsigned long)pow(2, mTheoraFormatInfo->maxKeyframeInterval);
			//unsigned long locInterFrameNo = (mLastSeenStartGranPos) % locMod;
			//LONGLONG locAbsFramePos = ((mLastSeenStartGranPos >> mTheoraFormatInfo->maxKeyframeInterval)) + locInterFrameNo;
			//REFERENCE_TIME locTimeBase = (locAbsFramePos * mFrameDuration) - mSeekTimeBase;
			//REFERENCE_TIME locFrameStart = locTimeBase + (mFrameCount * mFrameDuration);
			////Increment the frame counter
			//mFrameCount++;
			////Make the end frame counter
			//REFERENCE_TIME locFrameEnd = locTimeBase + (mFrameCount * mFrameDuration);
	//------------------------


	REFERENCE_TIME locFrameStart = (mFrameCount * mFrameDuration);
	mFrameCount++;
	REFERENCE_TIME locFrameEnd = (mFrameCount * mFrameDuration);

	
	debugLog<<"Sample times = "<<locFrameStart<<" to "<<locFrameEnd<<"  frame "<<mFrameCount<<" KF = "<<((inIsKeyFrame) ? "YES" : "NO")<<endl;
	
	//FILTER_STATE locFS;
	//GetState(0, &locFS);
	//debugLog<<"State Before = "<<locFS<<endl;
	//HRESULT locHR = mOutputPin->GetDeliveryBuffer(&locSample, &locFrameStart, &locFrameEnd, locFlags);
	//GetState(0, &locFS);
	//debugLog<<"State After = "<<locFS<<endl;
	
	

	//Debuggin code
	AM_MEDIA_TYPE* locMediaType = NULL;
	outSample->GetMediaType(&locMediaType);
	if (locMediaType == NULL) {
		//debugLog<<"No dynamic change..."<<endl;
	} else {
		//debugLog<<"Attempting dynamic change..."<<endl;
		if (locMediaType->majortype == MEDIATYPE_Video) {
			//debugLog<<"Still MEDIATYPE_Video"<<endl;
		}

		if (locMediaType->subtype == MEDIASUBTYPE_YV12) {
			//debugLog<<"Still MEDIASUBTYPE_YV12"<<endl;
		}

		if (locMediaType->formattype == FORMAT_VideoInfo) {
			//debugLog<<"Still FORMAT_VideoInfo"<<endl;
			VIDEOINFOHEADER* locVF = (VIDEOINFOHEADER*)locMediaType->pbFormat;
			//debugLog<<"Size = "<<locVF->bmiHeader.biSizeImage<<endl;
			//debugLog<<"Dim   = "<<locVF->bmiHeader.biWidth<<" x " <<locVF->bmiHeader.biHeight<<endl;
		}

		//debugLog<<"Major  : "<<DSStringer::GUID2String(&locMediaType->majortype);
		//debugLog<<"Minor  : "<<DSStringer::GUID2String(&locMediaType->subtype);
		//debugLog<<"Format : "<<DSStringer::GUID2String(&locMediaType->formattype);
		//debugLog<<"Form Sz: "<<locMediaType->cbFormat;


	}
	//

	////Create pointers for the samples buffer to be assigned to
	BYTE* locBuffer = NULL;
	
	//
	////Make our pointers set to point to the samples buffer
	outSample->GetPointer(&locBuffer);
	
	


	//Fill the buffer with yuv data...
	//	



	//Set up the pointers
	unsigned char* locDestUptoPtr = locBuffer;
	char* locSourceUptoPtr = inYUVBuffer->y;

	//Strides from theora are generally -'ve
	long locYStride = inYUVBuffer->y_stride;
	long locUVStride = inYUVBuffer->uv_stride;

	//debugLog<<"Y Stride = "<<locYStride<<endl;
	//debugLog<<"UV Stride = "<<locUVStride<<endl;
	//
	//Y DATA
	//

	//NEW WAY with offsets Y Data
	long locTopPad = inYUVBuffer->y_height - mHeight - mYOffset;
	//debugLog<<"--------- PAD = "<<locTopPad<<endl;
	ASSERT(locTopPad >= 0);
	if (locTopPad < 0) {
		locTopPad = 0;
	} else {
		
	}

	//Skip the top padding
	locSourceUptoPtr += (mYOffset * locYStride);

	for (unsigned long line = 0; line < mHeight; line++) {
		memcpy((void*)(locDestUptoPtr), (const void*)(locSourceUptoPtr + mXOffset), mWidth);
		locSourceUptoPtr += locYStride;
		locDestUptoPtr += mWidth;
	}

	locSourceUptoPtr += (locTopPad * locYStride);

	//debugLog<<"Dest Distance(y) = "<<(unsigned long)(locDestUptoPtr - locBuffer)<<endl;

	//Source advances by (y_height * y_stride)
	//Dest advances by (mHeight * mWidth)

	//
	//V DATA
	//

	//Half the padding for uv planes... is this correct ? 
	locTopPad = locTopPad /2;
	
	locSourceUptoPtr = inYUVBuffer->v;

	//Skip the top padding
	locSourceUptoPtr += ((mYOffset/2) * locYStride);

	for (unsigned long line = 0; line < mHeight / 2; line++) {
		memcpy((void*)(locDestUptoPtr), (const void*)(locSourceUptoPtr + (mXOffset / 2)), mWidth / 2);
		locSourceUptoPtr += locUVStride;
		locDestUptoPtr += (mWidth / 2);
	}
	locSourceUptoPtr += (locTopPad * locUVStride);

	//Source advances by (locTopPad + mYOffset/2 + mHeight /2) * uv_stride
	//where locTopPad for uv = (inYUVBuffer->y_height - mHeight - mYOffset) / 2
	//						=	(inYUVBuffer->yheight/2 - mHeight/2 - mYOffset/2)
	// so source advances by (y_height/2) * uv_stride
	//Dest advances by (mHeight * mWidth) /4


	//debugLog<<"Dest Distance(V) = "<<(unsigned long)(locDestUptoPtr - locBuffer)<<endl;
	//
	//U DATA
	//

	locSourceUptoPtr = inYUVBuffer->u;

	//Skip the top padding
	locSourceUptoPtr += ((mYOffset/2) * locYStride);

	for (unsigned long line = 0; line < mHeight / 2; line++) {
		memcpy((void*)(locDestUptoPtr), (const void*)(locSourceUptoPtr + (mXOffset / 2)), mWidth / 2);
		locSourceUptoPtr += locUVStride;
		locDestUptoPtr += (mWidth / 2);
	}
	locSourceUptoPtr += (locTopPad * locUVStride);

	//debugLog<<"Dest Distance(U) = "<<(unsigned long)(locDestUptoPtr - locBuffer)<<endl;
	//debugLog<<"Frame Size = "<<mFrameSize<<endl;

	//Set the sample parameters.
	//BOOL locIsKeyFrame = (locInterFrameNo == 0);
	BOOL locIsKeyFrame = FALSE;
	if (inIsKeyFrame) {
		locIsKeyFrame = TRUE;
	};
	SetSampleParams(outSample, mFrameSize, &locFrameStart, &locFrameEnd, locIsKeyFrame);

	
	
	return 0;


}


HRESULT TheoraDecodeFilter::SetMediaType(PIN_DIRECTION inDirection, const CMediaType* inMediaType) {

	if (inDirection == PINDIR_INPUT) {
		if (CheckInputType(inMediaType) == S_OK) {
			//debugLog<<"Setting format block"<<endl;
			setTheoraFormat(inMediaType->pbFormat);
			
			//Set some other stuff here too...
			mXOffset = mTheoraFormatInfo->xOffset;
			mYOffset = mTheoraFormatInfo->xOffset;
		} else {
			//Failed... should never be here !
			throw 0;
		}
		return CTransformFilter::SetMediaType(PINDIR_INPUT, inMediaType);//CVideoTransformFilter::SetMediaType(PINDIR_INPUT, inMediaType);
	} else {
		//debugLog<<"Setting Output Stuff"<<endl;
		//Output pin SetMediaType
		//VIDEOINFOHEADER* locVideoHeader = (VIDEOINFOHEADER*)inMediaType->Format();
		//mHeight = (unsigned long)abs(locVideoHeader->bmiHeader.biHeight);
		//mWidth = (unsigned long)abs(locVideoHeader->bmiHeader.biWidth);


		//mFrameSize = (unsigned long)locVideoHeader->bmiHeader.biSizeImage;

		//debugLog<<"Size = "<<mWidth<<" x "<<mHeight<<" ("<<mFrameSize<<")"<<endl;
		//debugLog<<"Size in Format = "<<locVideoHeader->bmiHeader.biWidth<<" x "<<locVideoHeader->bmiHeader.biHeight<<endl;
		return CTransformFilter::SetMediaType(PINDIR_OUTPUT, inMediaType);//CVideoTransformFilter::SetMediaType(PINDIR_OUTPUT, inMediaType);
	}
}


bool TheoraDecodeFilter::SetSampleParams(IMediaSample* outMediaSample, unsigned long inDataSize, REFERENCE_TIME* inStartTime, REFERENCE_TIME* inEndTime, BOOL inIsSync) 
{
	outMediaSample->SetTime(inStartTime, inEndTime);
	outMediaSample->SetMediaTime(NULL, NULL);
	outMediaSample->SetActualDataLength(inDataSize);
	outMediaSample->SetPreroll(FALSE);
	outMediaSample->SetDiscontinuity(FALSE);
	outMediaSample->SetSyncPoint(inIsSync);
	return true;
}
//BOOL TheoraDecodeFilter::ShouldSkipFrame(IMediaSample* inSample) {
//	//m_bSkipping = FALSE;
//	debugLog<<"Don't skip"<<endl;
//	return FALSE;
//}

sTheoraFormatBlock* TheoraDecodeFilter::getTheoraFormatBlock() 
{
	return mTheoraFormatInfo;
}
void TheoraDecodeFilter::setTheoraFormat(BYTE* inFormatBlock) 
{

	delete mTheoraFormatInfo;
	mTheoraFormatInfo = new sTheoraFormatBlock;			//Deelted in destructor.

	//0		-	55			theora ident						0	-	6
	//56	-	63			ver major							7	-	7
	//64	-	71			ver minor							8	-	8
	//72	-	79			ver subversion						9	=	9
	//80	-	95			width/16							10	-	11
	//96	-	111			height/16							12	-	13
	//112	-	135			framewidth							14	-	16
	//136	-	159			frameheight							17	-	19
	//160	-	167			xoffset								20	-	20
	//168	-	175			yoffset								21	-	21
	//176	-	207			framerateNum						22	-	25
	//208	-	239			frameratedenom						26	-	29
	//240	-	263			aspectNum							30	-	32
	//264	-	287			aspectdenom							33	-	35
	//288	-	295			colourspace							36	-	36
	//296	-	319			targetbitrate						37	-	39
	//320	-	325			targetqual							40	-	40.75
	//326	-	330			keyframintlog						40.75-  41.375

	unsigned char* locIdentHeader = inFormatBlock;
	mTheoraFormatInfo->theoraVersion = (iBE_Math::charArrToULong(locIdentHeader + 7)) >>8;
	mTheoraFormatInfo->outerFrameWidth = (iBE_Math::charArrToUShort(locIdentHeader + 10)) * 16;
	mTheoraFormatInfo->outerFrameHeight = (iBE_Math::charArrToUShort(locIdentHeader + 12)) * 16;
	mTheoraFormatInfo->pictureWidth = (iBE_Math::charArrToULong(locIdentHeader + 14)) >>8;
	mTheoraFormatInfo->pictureHeight = (iBE_Math::charArrToULong(locIdentHeader + 17)) >>8;
	mTheoraFormatInfo->xOffset = locIdentHeader[20];
	mTheoraFormatInfo->yOffset = locIdentHeader[21];
	mTheoraFormatInfo->frameRateNumerator = iBE_Math::charArrToULong(locIdentHeader + 22);
	mTheoraFormatInfo->frameRateDenominator = iBE_Math::charArrToULong(locIdentHeader + 26);
	mTheoraFormatInfo->aspectNumerator = (iBE_Math::charArrToULong(locIdentHeader + 30)) >>8;
	mTheoraFormatInfo->aspectDenominator = (iBE_Math::charArrToULong(locIdentHeader + 33)) >>8;
	mTheoraFormatInfo->colourSpace = locIdentHeader[36];
	mTheoraFormatInfo->targetBitrate = (iBE_Math::charArrToULong(locIdentHeader + 37)) >>8;
	mTheoraFormatInfo->targetQuality = (locIdentHeader[40]) >> 2;

	mTheoraFormatInfo->maxKeyframeInterval= (((locIdentHeader[40]) % 4) << 3) + (locIdentHeader[41] >> 5);
}

CBasePin* TheoraDecodeFilter::GetPin(int inPinNo)
{
    HRESULT locHR = S_OK;

    // Create an input pin if necessary

    if (m_pInput == NULL) {

        m_pInput = new TheoraDecodeInputPin(this, &locHR);		//Deleted in base destructor

        
        if (m_pInput == NULL) {
            return NULL;
        }
        m_pOutput = new TheoraDecodeOutputPin(this, &locHR);	//Deleted in base destructor
			

        if (m_pOutput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Return the pin

    if (inPinNo == 0) {
        return m_pInput;
    } else if (inPinNo == 1) {
        return m_pOutput;
    } else {
        return NULL;
    }
}
