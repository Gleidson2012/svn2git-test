//===========================================================================
//Copyright (C) 2003, 2004, 2005 Zentaro Kavanagh
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
#include "StdAfx.h"
#include ".\OggDemuxPacketsourcepin.h"

OggDemuxPacketSourcePin::	OggDemuxPacketSourcePin(		TCHAR* inObjectName
												,	OggDemuxPacketSourceFilter* inParentFilter
												,	CCritSec* inFilterLock
												,	OggPacket* inIdentHeader
												,	unsigned long inSerialNo)
	:	CBaseOutputPin(			NAME("Ogg Demux Output Pin")
							,	inParentFilter
							,	inFilterLock
							,	&mFilterHR
							,	L"Ogg Stream" )
	,	mIdentHeader(inIdentHeader)
	,	mSerialNo(inSerialNo)
	,	mIsStreamReady(false)
	,	mAcceptingData(false)
	,	mNumBuffers(0)
	,	mDataQueue(NULL)
{

	
		//(BYTE*)inBOSPage->createRawPageData();
	mPacketiser.setPacketSink(this);
}

OggDemuxPacketSourcePin::~OggDemuxPacketSourcePin(void)
{
	//delete[] mBOSAsFormatBlock;
	//delete mBOSPage;
	delete mIdentHeader;
}

bool OggDemuxPacketSourcePin::acceptOggPage(OggPage* inOggPage)
{
	if (mIsStreamReady) {
		mAcceptingData = true;
		return mPacketiser.acceptOggPage(inOggPage);
	}
	return false;
}
BYTE* OggDemuxPacketSourcePin::getIdentAsFormatBlock()
{
	return (BYTE*)mIdentHeader->packetData();
}

unsigned long OggDemuxPacketSourcePin::getIdentSize()
{
	return mIdentHeader->packetSize();
}

unsigned long OggDemuxPacketSourcePin::getSerialNo()
{
	return mSerialNo;//mBOSPage->header()->StreamSerialNo();
}

IOggDecoder* OggDemuxPacketSourcePin::getDecoderInterface()
{
	if (mDecoderInterface == NULL) {
		IOggDecoder* locDecoder = NULL;
		if (IsConnected()) {
			IPin* locPin = GetConnected();
			if (locPin != NULL) {
				locPin->QueryInterface(IID_IOggDecoder, (void**)&locDecoder);
			}
		}

		mDecoderInterface = locDecoder;
	}
	return mDecoderInterface;
	
}
HRESULT OggDemuxPacketSourcePin::GetMediaType(int inPosition, CMediaType* outMediaType) 
{
	//Put it in from the info we got in the constructor.
	if (inPosition == 0) {
		AM_MEDIA_TYPE locAMMediaType;
		locAMMediaType.majortype = MEDIATYPE_OggPacketStream;

		locAMMediaType.subtype = MEDIASUBTYPE_None;
		locAMMediaType.formattype = FORMAT_OggIdentHeader;
		locAMMediaType.cbFormat = getIdentSize();
		locAMMediaType.pbFormat = getIdentAsFormatBlock();
		locAMMediaType.pUnk = NULL;
	
			
	
		CMediaType locMediaType(locAMMediaType);		
		*outMediaType = locMediaType;
		return S_OK;
	} else {
		return VFW_S_NO_MORE_ITEMS;
	}
}
HRESULT OggDemuxPacketSourcePin::CheckMediaType(const CMediaType* inMediaType) {
	if (		(inMediaType->majortype == MEDIATYPE_OggPacketStream) 
			&&	(inMediaType->subtype == MEDIASUBTYPE_None)
			&&	(inMediaType->formattype == FORMAT_OggIdentHeader)) {
			//&&	(inMediaType->cbFormat == mBOSPage->pageSize()) {

		return S_OK;
	} else {
		return E_FAIL;
	}
}
HRESULT OggDemuxPacketSourcePin::DecideBufferSize(IMemAllocator* inoutAllocator, ALLOCATOR_PROPERTIES* inoutInputRequest) 
{
	HRESULT locHR = S_OK;

	ALLOCATOR_PROPERTIES locReqAlloc = *inoutInputRequest;
	ALLOCATOR_PROPERTIES locActualAlloc;

	//locReqAlloc.cbAlign = 1;
	//locReqAlloc.cbBuffer = 65536; //BUFFER_SIZE;
	//locReqAlloc.cbPrefix = 0;
	//locReqAlloc.cBuffers = NUM_PAGE_BUFFERS; //NUM_BUFFERS;

	locHR = inoutAllocator->SetProperties(&locReqAlloc, &locActualAlloc);

	if (locHR != S_OK) {
		return locHR;
	}

	mNumBuffers = locActualAlloc.cBuffers;
	
	locHR = inoutAllocator->Commit();

	return locHR;

}


//Pin Conenction Methods
HRESULT OggDemuxPacketSourcePin::BreakConnect()
{
	delete mDataQueue;
	mDataQueue = NULL;
	return CBaseOutputPin::BreakConnect();
}
HRESULT OggDemuxPacketSourcePin::CompleteConnect(IPin *inReceivePin)
{
	IOggDecoder* locDecoder = NULL;
	inReceivePin->QueryInterface(IID_IOggDecoder, (void**)&locDecoder);
	if (locDecoder != NULL) {
		mDecoderInterface = locDecoder;

		IOggDecoder::eAcceptHeaderResult locResult = mDecoderInterface->showHeaderPacket(mIdentHeader->clone());
		if (locResult == IOggDecoder::AHR_ALL_HEADERS_RECEIVED) {
			mIsStreamReady = true;

		} else {
			OggPacketiser locPacketiser;
			locPacketiser.setPacketSink(this);
			OggDemuxPacketSourceFilter* locParent = (OggDemuxPacketSourceFilter*)m_pFilter;
			vector<OggPage*> locList = locParent->getMatchingBufferedPages(mSerialNo);
			
			for (size_t i = 0; i < locList.size(); i++) {
				locPacketiser.acceptOggPage(locList[i]);
			}

			locParent->removeMatchingBufferedPages(mSerialNo);

			
		}

		if (mIsStreamReady) {
			mDataQueue = new COutputQueue (inReceivePin, &mFilterHR, FALSE, TRUE,1,TRUE, mNumBuffers);
			return CBaseOutputPin::CompleteConnect(inReceivePin);
		}	

		
	}
	return E_FAIL;
	
}

bool OggDemuxPacketSourcePin::dispatchPacket(StampedOggPacket* inPacket)
{
	CAutoLock locStreamLock(((OggDemuxPacketSourceFilter*)m_pFilter)->streamLock());


	//Set up the sample info
	IMediaSample* locSample = NULL;
	REFERENCE_TIME locStart = inPacket->startTime();
	REFERENCE_TIME locStop = inPacket->endTime();
	
	//Get a delivery buffer
	HRESULT	locHR = GetDeliveryBuffer(&locSample, &locStart, &locStop, NULL);
	
	//Error checks
	if (locHR != S_OK) {
		//Stopping, fluching or error

		delete inPacket;
		return false;
	}

	//Set time stamps. These are granule pos, and may be -1
	locSample->SetTime(&locStart, &locStop);
	
	locSample->SetMediaTime(&locStart, &locStop);
	locSample->SetSyncPoint(TRUE);
	

	// Create a pointer for the samples buffer
	BYTE* locBuffer = NULL;
	locSample->GetPointer(&locBuffer);

	if (locSample->GetSize() >= inPacket->packetSize()) {

		memcpy((void*)locBuffer, (const void*)inPacket->packetData(), inPacket->packetSize());
		locSample->SetActualDataLength(inPacket->packetSize());

		locHR = mDataQueue->Receive(locSample);

		if (locHR != S_OK) {
			//debugLog << "Failure... Queue rejected sample..."<<endl;
			//Stopping ??

			delete inPacket;
			return false;
		} else {
			delete inPacket;
			return true;
		}
	} else {
		//DbgLog((LOG_TRACE, 2, "* BUFFER TOO SMALL... FATALITY !!"));
		throw 0;
	}	
}
bool OggDemuxPacketSourcePin::acceptStampedOggPacket(StampedOggPacket* inPacket)
{
	if (mAcceptingData) {
		return dispatchPacket(inPacket);
	} else {
		//This handles callbacks with header packets
		IOggDecoder::eAcceptHeaderResult locResult;
		if ((mDecoderInterface != NULL) && (!mIsStreamReady)) {
			locResult = mDecoderInterface->showHeaderPacket(inPacket);
			if (locResult == IOggDecoder::AHR_ALL_HEADERS_RECEIVED) {
				mIsStreamReady = true;
			}
		}
		delete inPacket;
		return true;
	}
}

//Pin streaming methods
HRESULT OggDemuxPacketSourcePin::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	NewSegment(tStart, tStop, dRate);

	mDataQueue->NewSegment(tStart, tStop, dRate);

	return S_OK;
}
HRESULT OggDemuxPacketSourcePin::DeliverEndOfStream(void)
{
	mDataQueue->EOS();
    return S_OK;
}

HRESULT OggDemuxPacketSourcePin::DeliverEndFlush(void)
{
	mDataQueue->EndFlush();
    return S_OK;
}

HRESULT OggDemuxPacketSourcePin::DeliverBeginFlush(void)
{
	mDataQueue->BeginFlush();
    return S_OK;
}