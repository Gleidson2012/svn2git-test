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

#include "StdAfx.h"
#include "FLACDecodeFilter.h"


//COM Factory Template
CFactoryTemplate g_Templates[] = 
{
    { 
		L"FLAC Decode Filter",						// Name
	    &CLSID_FLACDecodeFilter,            // CLSID
	    FLACDecodeFilter::CreateInstance,	// Method to create an instance of MyComponent
        NULL,									// Initialization function
        NULL									// Set-up information (for filters)
    }

};

// Generic way of determining the number of items in the template
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]); 

FLACDecodeFilter::FLACDecodeFilter(void)
	:	AbstractTransformFilter(NAME("FLAC Audio Decoder"), CLSID_FLACDecodeFilter)
	,	mFLACFormatBlock(NULL)
{
	bool locWasConstructed = ConstructPins();
}

FLACDecodeFilter::~FLACDecodeFilter(void)
{
	delete mFLACFormatBlock;
	mFLACFormatBlock = NULL;
}

bool FLACDecodeFilter::ConstructPins() 
{
	//Vector to hold our set of media types we want to accept.
	vector<CMediaType*> locAcceptableTypes;

	//Setup the media types for the output pin.
	CMediaType* locAcceptMediaType = new CMediaType(&MEDIATYPE_Audio);		//Deleted in pin destructor
	locAcceptMediaType->subtype = MEDIASUBTYPE_PCM;
	locAcceptMediaType->formattype = FORMAT_WaveFormatEx;
	
	locAcceptableTypes.push_back(locAcceptMediaType);

	//Output pin must be done first because it's passed to the input pin.
	mOutputPin = new FLACDecodeOutputPin(this, m_pLock, locAcceptableTypes);			//Deleted in base class destructor

	//Clear out the vector, now we've already passed it to the output pin.
	locAcceptableTypes.clear();

	//Setup the media Types for the input pin.
	locAcceptMediaType = NULL;
	locAcceptMediaType = new CMediaType(&MEDIATYPE_Audio);			//Deleted by pin

	locAcceptMediaType->subtype = MEDIASUBTYPE_FLAC;
	locAcceptMediaType->formattype = FORMAT_FLAC;

	locAcceptableTypes.push_back(locAcceptMediaType);
	
	mInputPin = new FLACDecodeInputPin(this, m_pLock, mOutputPin, locAcceptableTypes);	//Deleted in base class filter destructor.
	return true;

}

CUnknown* WINAPI FLACDecodeFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
{
	//This routine is the COM implementation to create a new Filter
	FLACDecodeFilter *pNewObject = new FLACDecodeFilter();
    if (pNewObject == NULL) {
        *pHr = E_OUTOFMEMORY;
    }
	return pNewObject;
} 

sFLACFormatBlock* FLACDecodeFilter::getFLACFormatBlock() 
{
	return mFLACFormatBlock;
}
void FLACDecodeFilter::setFLACFormatBlock(sFLACFormatBlock* inFormatBlock) 
{
	delete mFLACFormatBlock;
	mFLACFormatBlock = new sFLACFormatBlock;		//Deleted in destructor.
	*mFLACFormatBlock = *inFormatBlock;
}