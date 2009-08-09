// CrashRptProbe.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "CrashRptProbe.h"
#include <map>
#include "tinyxml.h"
#include "unzip.h"

WTL::CAppModule _Module;

struct CrpReportData
{
  HZIP m_hZip;
  CDescReader m_descReader;
  CMinidumpReader m_dmpReader;  
};

std::map<int, CrpReportData> g_OpenedHandles;

int
crpOpenCrashReportW(
  LPCWSTR pszFileName,
  CrpHandle* pHandle)
{


  return 0;
}

int
crpOpenCrashReportA(
  LPCSTR pszFileName,
  CrpHandle* pHandle)
{
  USES_CONVERSION;
  return crpOpenCrashReportW(A2W(pszFileName), pHandle);  
}

int
crpCloseCrashReport(
  CrpHandle handle)
{
  return 0;
}

int
crpGetStrPropertyW(
  UINT uHandle,
  LPCWSTR lpszPropName,
  LPWSTR lpszBuffer,
  UINT uBuffSize
)
{
  return 0;
}

int
crpGetStrPropertyA(
  CrpHandle handle,
  LPCSTR lpszPropName,
  LPSTR lpszBuffer,
  UINT uBuffSize
)
{
  return 0;
}

int
crpGetLongPropertyW(
  CrpHandle handle,
  LPCWSTR lpszPropName,
  PLONG lpnPropVal)
{
  return 0;
}

int
crpGetLongPropertyA(
  CrpHandle handle,
  LPCSTR lpszPropName,
  PLONG lpnPropVal)
{
  return 0;
}


int
crpExtractFileW(
  CrpHandle handle,
  LPCWSTR lpszFileName,
  LPCWSTR lpszFileSaveAs
);

int
crpExtractFileA(
  CrpHandle handle,
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

