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

#include "duk_rsrc.h"
#include "regentry.h"
#include "GetSysFolder.h"
#include <Files.h>

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

short GetSystemStartVolumeName(char *name);
short Duk_OpenComponentResFile(void);
void Duk_CloseComponentResFile(short resFileID);

#ifdef __cplusplus
}
#endif

/*
For future use, do not delete
void SetPermission(RegistryAccess regAccess, short request, short permission)
{
	if(permission == fsReadWritePerm)
		regAccess->filePermission |= 1 << (request - 1);
	else
		regAccess->filePermission &= ~(1 << (request - 1));
}

short GetPermission(RegistryAccess regAccess, short request)
{
	short permission;
	
	if( regAccess->filePermission & (1 << (request - 1) )
		permission = fsReadWritePerm;
	else
		permission = fsReadPerm;
	
	return permission;
}*/

short Duk_OpenComponentResFile(void)
{
	/*
	char sysPath[256];
	short r;

	GetSystemFolderPath(kExtensionFolderType,sysPath);
//	strcat(sysPath,":TrueMotion2X");
#ifdef ARCHIVE_VERSION
	strcat(sysPath,":ConfigTM2A");
#else
	strcat(sysPath,":ConfigTM2X");
#endif
	CtoPStr(sysPath);
	r = OpenResFile((unsigned char *) sysPath);
	return r;*/
	return 0;
}

void Duk_CloseComponentResFile(short resFileID)
{
	CloseResFile(resFileID);
}


int Registry_Open(RegistryAccess regAccess, char *mode)
{
	int		retVal = 0;
	short	currentResFile;
	
	if(regAccess == NULL)
		return -1;
	
	currentResFile = CurResFile();
#if 1	
	//	Open as Read-Only
	if (!strcmp(mode,"r"))
	{
		//	If it's not open already, open it
		if(regAccess->openRequests == 0)
		{
			regAccess->resFileID = OpenComponentResFile((Component ) regAccess->theCodec);
			regAccess->oldID = currentResFile;
			regAccess->filePermission = fsRdPerm;
		}
		
		regAccess->openRequests++;
	}
	//	Open with Read-Write
	else 
	{
#endif
		//	If it's not open already, open it
		if(regAccess->openRequests == 0)
		{
			regAccess->resFileID = OpenComponentResFile((Component ) regAccess->theCodec);
			regAccess->oldID = currentResFile;
			regAccess->filePermission = fsRdWrPerm;
		}
		//	If it's open with ReadOnly, make it writable
		else if(regAccess->filePermission == fsRdPerm)
		{
			CloseComponentResFile(regAccess->resFileID);
			regAccess->resFileID = OpenComponentResFile((Component ) regAccess->theCodec);
			regAccess->filePermission = fsRdWrPerm;
		}
		
		regAccess->openRequests++;
	}
		
		
	retVal = regAccess->resFileID;

	return retVal;
}


int Registry_Close(RegistryAccess regAccess)
{
	int retVal = 0;
	
	if(regAccess == NULL)
		return -1;
	
	//	If all requests to Open have been balanced, close it for real
	if(regAccess->openRequests == 1)
	{
		//	Use CloseComponentResFile for ReadOnly Access
		if(regAccess->filePermission == fsRdPerm)
			CloseComponentResFile(regAccess->resFileID);
		
		//	Use DuckClose for ReadWrite Access
		else
			Duk_CloseComponentResFile(regAccess->resFileID);
		
		UseResFile(regAccess->oldID);
		
		regAccess->resFileID = 0;
		regAccess->oldID = 0;
		regAccess->filePermission = 0;
	}
	regAccess->openRequests--;
	
	return retVal;
}


int Registry_GetEntry(
void *data, REGISTRY_TYPE r, unsigned long *sizeItem, char *nameItem, RegistryAccess regAccess)
{
	int retVal = 0;
	
	Registry_Open(regAccess, "r");
	
	if (regAccess->resFileID > 0)
	{
	    retVal = 0;
		switch (r)
		{
			case REG_CSTRING :
				DukGetStrResource((char *) data,'STR ',nameItem);
				break;
			case REG_INTEGER :
				{
				    char temp[256];
				    if (DukGetStrResource(temp,'STR ',nameItem))
				    	sscanf(temp,"%d",(int *) data);
				    else *((int *) data) = 0;
			    }
				break;
			case REG_UNSIGNED_LONG :
		       break;
		    case REG_PROFILE :
		    	DukGetCustomResource((unsigned char *) data,'DKPR',nameItem,sizeItem);
		 	   break;
			default :
				break;
		}
		
		Registry_Close(regAccess);
	}
	else
		retVal = -1;
	
	
	return retVal;
}



int Registry_SetEntry(void *data, REGISTRY_TYPE r, 
unsigned long, char *nameItem, RegistryAccess regAccess)
{
	int retVal = 0;
	
	Registry_Open(regAccess, "w");
	
	if (regAccess->resFileID > 0)
	{
		switch (r)
		{
			case REG_CSTRING :
				DukSetStrResource((char *) data,'STR ',nameItem);
				break;
			case REG_INTEGER :
				{
				    char temp[256];
				    sprintf(temp,"%d",*(int *) data);
				    DukSetStrResource(temp,'STR ',nameItem);
			    }
				break;
			case REG_UNSIGNED_LONG :
		       break;
		    case REG_PROFILE :
		    	//DukGetCustomResource((unsigned char *) data,'DKPR',nameItem,sizeItem);
		 	   break;
			default :
				break;
		}
	    
		Registry_Close(regAccess);
	}
	else
		retVal = -1;
	
	return retVal;
}
 
