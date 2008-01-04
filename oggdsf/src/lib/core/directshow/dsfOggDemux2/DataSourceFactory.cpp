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
#include "datasourcefactory.h"

DataSourceFactory::DataSourceFactory(void)
{
}

DataSourceFactory::~DataSourceFactory(void)
{
}

IFilterDataSource* DataSourceFactory::createDataSource(wstring inSourceLocation) {
	wstring locType = identifySourceType(inSourceLocation);

	if(locType.length() == 1) {
		//File...
		return new FilterFileSource;
	} else if (locType == L"\\\\") {
		//Network share...
		return new FilterFileSource;
	} else if (locType == L"http") {
		//Http stream
		//return new HTTPFileSource;
		return new HTTPStreamingFileSource;
#ifdef WINCE
	} else if (locType == "\\") {
		//WinCE absolute file path
		return new FilterFileSource;
#endif
	} else {
		//Something else
		return NULL;
	}
}

wstring DataSourceFactory::identifySourceType(wstring inSourceLocation) {
	size_t locPos = inSourceLocation.find(':');
	if (locPos == string::npos) {
		//No colon... not a normal file. See if it's a network share...

		//Make sure it's long enough...
		if (inSourceLocation.length() > 2) {
			wstring retStr = inSourceLocation.substr(0,2);
			if (retStr == L"\\\\") {
				//A "\\" is a network share
				return retStr;
			} else {
#ifdef WINCE
				retStr = inSourceLocation.substr(0,1);

				if (retStr == L"\\") {
					//WinCE absolute path
					return retStr;
				}
#endif
				//Not a network share.
				return L"";
			}
		} else {
			//Too short
			return L"";
		}
	} else {
		wstring retStr = inSourceLocation.substr(0,locPos);
		return retStr;
	}
	
}