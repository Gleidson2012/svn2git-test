// axAnxOggPlayer.cpp : Implementation of CaxAnxOggPlayerApp and DLL registration.

#include "stdafx.h"
#include "axAnxOggPlayer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CaxAnxOggPlayerApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0x375E2E46, 0x3968, 0x41FA, { 0x99, 0xBE, 0x35, 0x52, 0x3D, 0xC5, 0x7B, 0x4E } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;



// CaxAnxOggPlayerApp::InitInstance - DLL initialization

BOOL CaxAnxOggPlayerApp::InitInstance()
{
	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{
		// TODO: Add your own module initialization code here.
	}

	return bInit;
}



// CaxAnxOggPlayerApp::ExitInstance - DLL termination

int CaxAnxOggPlayerApp::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}



// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}



// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid, _wVerMajor, _wVerMinor))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}
