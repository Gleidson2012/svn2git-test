#include "StdAfx.h"
#include "vorbiscomments.h"

VorbisComments::VorbisComments(void)
{
}

VorbisComments::~VorbisComments(void)
{
}

string VorbisComments::vendorString() {
	return mVendorString;
}
bool VorbisComments::setVendorString(string inVendorString) {
	//FIX::: Validation needed
	mVendorString = inVendorString;
	return true;
}

unsigned long VorbisComments::numUserComments() {
	return mCommentList.size();
}
SingleVorbisComment VorbisComments::getUserComment(unsigned long inIndex) {
	//FIX::: Bounds checking
	return mCommentList[inIndex];
}
	
vector<SingleVorbisComment> VorbisComments::getCommentsByKey(string inKey) {
	//FIX::: Probably faster not to iterate... but who cares for now.. there aren't many.
	vector<SingleVorbisComment> retComments;
	SingleVorbisComment locCurrComment;

	for (int i = 0; i < mCommentList.size(); i++) {
		locCurrComment = mCommentList[i];
		//FIX::: Need to upcase everything
		if (locCurrComment.key() == inKey) {
			retComments.push_back(locCurrComment);
		}
	}
	return retComments;
}

bool VorbisComments::addComment(SingleVorbisComment inComment) {
	mCommentList.push_back(inComment);
	return true;
}
bool VorbisComments::addComment(string inKey, string inValue) {
	SingleVorbisComment locComment;
	locComment.setKey(inKey);
	locComment.setValue(inValue);
	mCommentList.push_back(locComment);
	return true;
}

bool VorbisComments::parseOggPacket(OggPacket* inPacket) {
	//FIX::: Validate it is a comment packet
	unsigned long locPackSize = inPacket->packetSize();
	unsigned long locUpto = 0;
	unsigned long locVendorLength = 0;
	string locVendorString;
	char* tempBuff = NULL;
	unsigned char* locPackBuff = inPacket->packetData();
	unsigned long locNumComments = 0;
	vector<SingleVorbisComment> locCommentList;

	if (locPackSize < locUpto + 4 - 1) {
		//FAILED - No vendor length
		return false;
	}

	locVendorLength = OggMath::charArrToULong(inPacket->packetData());
	locUpto+=4;

	if (locPackSize < locUpto + locVendorLength - 1) {
		//FAILED - Vendor string not present
		return false;
	}

	tempBuff = new char[locVendorLength + 1];

	if (tempBuff == NULL) {
		//FAILED - Vendor length too big, out of memory
		return false;
	}

	memcpy((void*)tempBuff, (const void*)(locPackBuff + locUpto), locVendorLength);
	tempBuff[locVendorLength] = '\0';

	locVendorString = tempBuff;
	delete tempBuff;
	tempBuff = NULL;

	locUpto += locVendorLength;

	if (locPackSize < locUpto + 4 - 1) {
		//FAILED - User comment list length not present
		return false;
	}

	locNumComments = OggMath::charArrToULong(locPackBuff + locUpto);
	locUpto += 4;

	unsigned long locUserCommentLength = 0;
	bool locFailed = false;
	string locUserComment;
	unsigned long i = 0;
	while (!locFailed && (i < locNumComments)) {
		if (locPackSize < locUpto + 4 -1) {
			//FAILED - User comment string length not present
			return false;
		}

		locUserCommentLength = OggMath::charArrToULong(locPackBuff + locUpto);
		locUpto += 4;


		if (locPackSize < locUpto + locUserCommentLength - 1) {
			//FAILED - User comment string not present
			return false;
		}

		memcpy((void*)tempBuff, (const void*)(locPackBuff + locUpto), locUserCommentLength);
		tempBuff[locUserCommentLength] = '\0';

        locUserComment = tempBuff;
		delete tempBuff;
		locUpto += locVendorLength;


		SingleVorbisComment locComment;
		if (locComment.parseComment(locUserComment)) {
			locCommentList.push_back(locComment);
		} else {
			//FAILED - Comment not parsable
			return false;
		}

		i++;

	}

	//Check the bit.
	if (locPackSize < locUpto) {
		//FAILED - No check bit
		return false;
	}

	if ((locPackBuff[locUpto] & 1) == 1) {
		//OK
		mVendorString = locVendorString;
		
		for (int j = 0; j < locCommentList.size(); j++) {
			mCommentList.push_back(locCommentList[j]);	
		}
	} else {
		//FAILED - Check bit not set
		return false;
	}

	return true;



	

	
}

string VorbisComments::toString() {
	return "";

}
unsigned long VorbisComments::size() {
	unsigned long locPackSize = 0;

	locPackSize = mVendorString.size() + 4;

	for (int i = 0; i < mCommentList.size(); i++) {
		locPackSize += mCommentList[i].length() + 4;
	}

	//Check bit
	locPackSize++;
	
	return locPackSize;
}
OggPacket* VorbisComments::toOggPacket() {
	unsigned long locPackSize = size();
	unsigned long locUpto = 0;
	unsigned char* locPackData = new unsigned char[locPackSize];

	OggMath::ULongToCharArr(mVendorString.length(), locPackData);
	locUpto += 4;

	memcpy((void*)(locPackData + locUpto), (const void*)mVendorString.c_str(), mVendorString.length());
	locUpto += mVendorString.length();

	OggMath::ULongToCharArr(mCommentList.size(), locPackData + locUpto);
	locUpto += 4;

	for (int i = 0; i < mCommentList.size(); i++) {
		OggMath::ULongToCharArr(mCommentList[i].length(), locPackData + locUpto);
		locUpto += 4;

		memcpy((void*)(locPackData + locUpto), (const void*)mCommentList[i].toString().c_str(), mCommentList[i].length());
		locUpto += mCommentList[i].length();
	}

	locPackData[locUpto] = 1;

	OggPacket* locPacket = NULL;
	locPacket = new OggPacket(locPackData, locPackSize, true);

	return locPacket;

}
