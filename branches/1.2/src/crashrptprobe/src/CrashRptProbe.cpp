// CrashRptProbe.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "CrashRptProbe.h"

WTL::CAppModule _Module;

int
craOpenCrashReportA(
  LPCSTR lpszFileName,
  LPUINT lpuHandle
)
{
  return 0;
}

// DllMain
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
  HRESULT hRes = _Module.Init(NULL, hModule);
  ATLASSERT(SUCCEEDED(hRes));
  hRes;

  return TRUE;
}

