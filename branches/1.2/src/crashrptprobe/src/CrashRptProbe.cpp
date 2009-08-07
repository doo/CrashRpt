// CrashRptProbe.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "CrashRptProbe.h"

WTL::CAppModule _Module;

int
crpOpenCrashReportW(
  LPCSTR lpszFileName,
  LPUINT lpuHandle)
{
  return 0;
}

int
crpOpenCrashReportA(
  LPCSTR lpszFileName,
  LPUINT lpuHandle)
{
  return 0;
}

int
crpCloseCrashReport(
  UINT uHandle)
{
  return 0;
}

CRASHRPTPROBE_API int
crpGetStrPropertyW(
  UINT uHandle,
  LPCWSTR lpszPropName,
  LPWSTR lpszBuffer,
  UINT uBuffSize
)
{
  return 0;
}

CRASHRPTPROBE_API int
crpGetStrPropertyA(
  UINT uHandle,
  LPCSTR lpszPropName,
  LPSTR lpszBuffer,
  UINT uBuffSize
)
{
  return 0;
}

CRASHRPTPROBE_API int
crpGetLongPropertyW(
  UINT uHandle,
  LPCWSTR lpszPropName,
  PLONG lpnPropVal)
{
  return 0;
}

CRASHRPTPROBE_API int
crpGetLongPropertyA(
  UINT uHandle,
  LPCSTR lpszPropName,
  PLONG lpnPropVal)
{
  return 0;
}


CRASHRPTPROBE_API int
crpExtractFileW(
  UINT uHandle,
  LPCWSTR lpszFileName,
  LPCWSTR lpszFileSaveAs
);

CRASHRPTPROBE_API int
crpExtractFileA(
  UINT uHandle,
  LPCSTR lpszFileName,
  LPCSTR lpszFileSaveAs
);


// DllMain
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
  HRESULT hRes = _Module.Init(NULL, hModule);
  ATLASSERT(SUCCEEDED(hRes));
  hRes;

  return TRUE;
}

