//============================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//----------------------------------------------------------------------------

#include "duck_mem.h"
#include <memory.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>   
#include <stdio.h>  
#include <MacMemory.h>

void *duck_malloc(size_t theSize, dmemType /*fred*/)
{   
	void *temp;

#ifdef USE_SYSTEM_MEMORY
	temp = NewPtrSys(theSize); 
#else
//	temp = NewPtr(theSize);
temp = malloc(theSize);

#endif
	
	return temp;
}


void *duck_calloc(size_t n, size_t theSize, dmemType /*fred*/)
{   
	void *temp;
	
#ifdef USE_SYSTEM_MEMORY
	temp = NewPtrSysClear(n * theSize);
#else
//	temp = NewPtrClear(n * theSize);

temp = calloc(n, theSize);
#endif

	return temp;
}

void duck_free(void *old_blk)
{  
#ifdef USE_SYSTEM_MEMORY
	DisposePtr( (char *)old_blk );
#else
//	DisposePtr( (char *)old_blk );
	free(old_blk);
#endif
}



void *duck_memcpy(void *dest, const void *source, size_t length)
{
	BlockMoveData(source, dest, length);
	
	return dest;
}


void *duck_memset(void *dest, int val , size_t length)
{
	return memset(dest, val, length);
}


int  duck_strcmp(const char *one, const char *two)
{
	return strcmp(one, two);
}


