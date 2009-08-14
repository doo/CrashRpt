// CrashRptProbe.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "CrashRptProbe.h"
#include <map>
#include "unzip.h"
#include "CrashDescReader.h"
#include "MinidumpReader.h"
#include "md5.h"
#include "Utility.h"

WTL::CAppModule _Module;

struct CrpReportData
{
  HZIP m_hZip;
  CCrashDescReader m_descReader;
  CMiniDumpReader m_dmpReader;  
};

std::map<int, CrpReportData> g_OpenedHandles;

CString GetBaseName(CString sFileName)
{
  CString sBase;
  int pos = sFileName.ReverseFind('.');
  if(pos>=0)
  {
    sBase = sFileName.Mid(0, pos);
  }
  return sBase;
}

CString GetExtension(CString sFileName)
{
  CString sExt;
  int pos = sFileName.ReverseFind('.');
  if(pos>=0)
  {
    sExt = sFileName.Mid(pos+1);
  }
  return sExt;
}

int CalcFileMD5Hash(CString sFileName, CString& sMD5Hash)
{  
  BYTE buff[512];
  MD5 md5;
  MD5_CTX md5_ctx;
  unsigned char md5_hash[16];
  FILE* f = NULL;

#if _MSC_VER<1400
  f = _tfopen(sFileName, _T("rb"));
#else
  _tfopen_s(&f, sFileName, _T("rb"));
#endif

  if(f==NULL)
  {
    return -1; // Couldn't open ZIP file
  }

  md5.MD5Init(&md5_ctx);
  
  while(!feof(f))
  {
    size_t count = fread(buff, 1, 512, f);
    if(count>0)
    {
      md5.MD5Update(&md5_ctx, buff, (unsigned int)count);
    }
  }

  fclose(f);
  md5.MD5Final(md5_hash, &md5_ctx);

  int i;
  for(i=0; i<16; i++)
  {
    CString number;
    number.Format(_T("%02x"), md5_hash[i]);
    sMD5Hash += number;
  } 
 
  return 0;
}

int
crpOpenCrashReportW(
  LPCWSTR pszFileName,
  LPCWSTR pszMd5Hash,
  CrpHandle* pHandle)
{ 
  int status = -1;
  int nNewHandle = 0;
  CrpReportData report_data;
  HZIP hZip = 0;
  ZRESULT zr = -1;
  int index = -1;
  ZIPENTRY ze;
  int xml_index = -1;
  int dmp_index = -1;
  CString sCalculatedMD5Hash;
  
  // Check ZIP integrity
  if(pszMd5Hash!=NULL)
  {
    int result = CalcFileMD5Hash(pszFileName, sCalculatedMD5Hash);
    if(result!=0 || sCalculatedMD5Hash.CompareNoCase(pszMd5Hash)!=0)
    {
      goto exit; // Invalid hash
    }
  }

  // Open ZIP archive
  hZip = OpenZip(pszFileName, 0);
  if(hZip==NULL)
    goto exit;

  // Look for v1.1 crash descriptor XML
  zr = FindZipItem(hZip, _T("crashrpt.xml"), false, &xml_index, &ze);
  
  // Look for v1.1 crash dump 
  zr = FindZipItem(hZip, _T("crashdump.dmp"), false, &dmp_index, &ze);
  
  // If xml and dmp still not found, assume it is v1.0
  if(xml_index==-1 && dmp_index==-1)  
  {    
    // Look for .dmp file
    int i=0;
    for(i=0;;i++)
    {
      zr = GetZipItem(hZip, index, &ze);
      if(zr!=ZR_OK)
        break;

      CString sExt = GetExtension(ze.name);
      if(sExt.CompareNoCase(_T("dmp")))
      {
        // DMP found
        dmp_index = index;
        break;
      }
    }

    // Assume the name of XML is the same as DMP
    CString sXmlName = GetBaseName(ze.name) + _T(".xml");
    zr = FindZipItem(hZip, sXmlName, false, &xml_index, &ze);
  }

  // Check that both xml and dmp found
  if(xml_index<0 || dmp_index<0)
  {
    goto exit; // XML or DMP not found 
  }

  // Load crash descriptor data
  if(xml_index>=0)
  {
    zr = GetZipItem(hZip, xml_index, &ze);
    if(zr!=ZR_OK)
    {
      goto exit; // Can't get ZIP element
    }

    CString sTempFile = CUtility::getTempFileName();
    zr = UnzipItem(hZip, xml_index, sTempFile);
    if(zr!=ZR_OK)
    {
      goto exit; // Can't unzip ZIP element
    }

    int result = report_data.m_descReader.Load(sTempFile);    
    DeleteFile(sTempFile);
    if(result!=0)
    {
      goto exit; // Corrupted XML
    }
  }  

  // Check integrity of minidump


  // Add handle to the list of opened handles
  nNewHandle = (int)g_OpenedHandles.size()+1;
  g_OpenedHandles[nNewHandle] = report_data;

exit:

  if(hZip!=0) 
    CloseZip(hZip);

  return status;
}

int
crpOpenCrashReportA(
  LPCSTR pszFileName,
  LPCSTR pszMd5Hash,
  CrpHandle* pHandle)
{
  USES_CONVERSION;
  return crpOpenCrashReportW(A2W(pszFileName), A2W(pszMd5Hash), pHandle);  
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

