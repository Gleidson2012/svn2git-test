#include "StdAfx.h"
#include ".\oggstreammapper.h"

OggStreamMapper::OggStreamMapper(OggDemuxPacketSourceFilter* inParentFilter, CCritSec* inParentFilterLock)
	:	mStreamState(eStreamState::STRMAP_READY)
	,	mParentFilter(inParentFilter)
	,	mParentFilterLock(inParentFilterLock)
	,	mFishHeadPacket(NULL)
	,	mSkeletonSerialNo(0)
{
}

OggStreamMapper::~OggStreamMapper(void)
{
	for (size_t i = 0; i < mPins.size(); i++) {
		delete mPins[i];
	}
}
OggDemuxPacketSourcePin* OggStreamMapper::getPinByIndex(unsigned long inIndex)
{
	if (inIndex < mPins.size()) {
		return mPins[inIndex];
	} else {
		return NULL;
	}
}
bool OggStreamMapper::acceptOggPage(OggPage* inOggPage)
{
	
	switch (mStreamState) {
		case STRMAP_READY:
			//WARNING::: Partial fall through
			if (inOggPage->header()->isBOS()) {
				mStreamState = STRMAP_PARSING_BOS_PAGES;
			} else {
				mStreamState = STRMAP_ERROR;
				delete inOggPage;
				return false;
			}
			//Partial fall through
		case STRMAP_PARSING_BOS_PAGES:
			//WARNING::: Partial fall through
			if (!allStreamsReady()) {
				if (inOggPage->header()->isBOS()) {
					return addNewPin(inOggPage);
				} else {
					mStreamState = STRMAP_DATA;
				}
			}
			//Partial fall through
		case STRMAP_DATA:
			{
				if (mFishHeadPacket != NULL) {
					if (inOggPage->header()->StreamSerialNo() == mSkeletonSerialNo) {
						int x= 2;
					}
				}
				OggDemuxPacketSourcePin* locPin = getMatchingPin(inOggPage->header()->StreamSerialNo());
				if (locPin != NULL) {
					return locPin->acceptOggPage(inOggPage);
				} else {
					//Ignore unknown streams
					delete inOggPage;
					return true;
				}
				
			}
			break;
		case STRMAP_FINISHED:
			return false;
		case STRMAP_ERROR:
			return false;
			
	}
	

}

bool OggStreamMapper::allStreamsReady()
{
	bool locAllReady = true;
	//OggDemuxPacketSourcePin* locPin = NULL;
	for (size_t i = 0; i < mPins.size(); i++) {
		locAllReady = locAllReady && (mPins[i]->isStreamReady());
	}	

	return locAllReady && (mPins.size() > 0);
}

bool OggStreamMapper::isFishHead(OggPage* inOggPage)
{
	StampedOggPacket* locPacket = inOggPage->getStampedPacket(0);

	if (locPacket == NULL) {
		return false;
	} else {
		if ((strncmp((const char*)locPacket->packetData(), "fishead\0", 8)) == 0) {
			return true;
		}
	}
	return false;
}

bool OggStreamMapper::handleFishHead(OggPage* inOggPage)
{
	mFishHeadPacket = inOggPage->getStampedPacket(0)->clone();
	mSkeletonSerialNo = inOggPage->header()->StreamSerialNo();
	delete inOggPage;
	return true;
}
bool OggStreamMapper::addNewPin(OggPage* inOggPage)
{
	//FISH::: Catch the fishead here.

	if (isFishHead(inOggPage)) {
		return handleFishHead(inOggPage);
	} else {
		OggDemuxPacketSourcePin* locNewPin = new OggDemuxPacketSourcePin(NAME("OggPageSourcePin"), mParentFilter, mParentFilterLock, inOggPage->getPacket(0)->clone(), inOggPage->header()->StreamSerialNo());
		//locNewPin->AddRef();
		delete inOggPage;
		mPins.push_back(locNewPin);
		return true;
	}
}

OggDemuxPacketSourcePin* OggStreamMapper::getMatchingPin(unsigned long inSerialNo)
{
	OggDemuxPacketSourcePin* locPin = NULL;
	for (size_t i = 0; i < mPins.size(); i++) {
		locPin = mPins[i];
		if (locPin->getSerialNo() == inSerialNo) {
			return locPin;
		}
	}
	return NULL;
}