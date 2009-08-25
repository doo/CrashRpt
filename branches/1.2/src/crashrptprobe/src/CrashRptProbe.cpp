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

CComAutoCriticalSection g_cs; // Critical section for thread-safe accessing error messages
std::map<DWORD, CString> g_sErrorMsg; // Last error messages for each calling thread.

int crpSetErrorMsg(PTSTR pszErrorMsg);

struct CrpReportData
{
  HZIP m_hZip;
  CCrashDescReader m_descReader;
  CMiniDumpReader m_dmpReader;  
};

std::map<int, CrpReportData> g_OpenedHandles;

CString GetBaseFileName(CString sFileName)
{
  CString sBase;
  int pos = sFileName.ReverseFind('.');
  if(pos>=0)
  {
    sBase = sFileName.Mid(0, pos);
  }
  return sBase;
}

CString GetFileExtension(CString sFileName)
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
  crpSetErrorMsg(_T("Unspecified error."));

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
    crpSetErrorMsg(_T("Couldn't open ZIP file."));
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
 
  crpSetErrorMsg(_T("Success."));
  return 0;
}

int
CRASHRPTPROBE_API
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

  crpSetErrorMsg(_T("Unspecified error."));
  
  // Check ZIP integrity
  if(pszMd5Hash!=NULL)
  {
    int result = CalcFileMD5Hash(pszFileName, sCalculatedMD5Hash);
    if(result!=0)
      goto exit;

    if(sCalculatedMD5Hash.CompareNoCase(pszMd5Hash)!=0)
    {  
      crpSetErrorMsg(_T("File might be corrupted, because MD5 hash is wrong."));
      goto exit; // Invalid hash
    }
  }

  // Open ZIP archive
  hZip = OpenZip(pszFileName, 0);
  if(hZip==NULL)
  {
    crpSetErrorMsg(_T("Error opening ZIP archive."));
    goto exit;
  }

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

      CString sExt = GetFileExtension(ze.name);
      if(sExt.CompareNoCase(_T("dmp")))
      {
        // DMP found
        dmp_index = index;
        break;
      }
    }

    // Assume the name of XML is the same as DMP
    CString sXmlName = GetBaseFileName(ze.name) + _T(".xml");
    zr = FindZipItem(hZip, sXmlName, false, &xml_index, &ze);
  }

  // Check that both xml and dmp found
  if(xml_index<0 || dmp_index<0)
  {
    crpSetErrorMsg(_T("File is not a valid crash report (XML or DMP missing)."));
    goto exit; // XML or DMP not found 
  }

  // Load crash descriptor data
  if(xml_index>=0)
  {
    zr = GetZipItem(hZip, xml_index, &ze);
    if(zr!=ZR_OK)
    {
      crpSetErrorMsg(_T("Error retrieving ZIP item."));
      goto exit; // Can't get ZIP element
    }

    CString sTempFile = CUtility::getTempFileName();
    zr = UnzipItem(hZip, xml_index, sTempFile);
    if(zr!=ZR_OK)
    {
      crpSetErrorMsg(_T("Error extracting ZIP item."));
      goto exit; // Can't unzip ZIP element
    }

    int result = report_data.m_descReader.Load(sTempFile);    
    DeleteFile(sTempFile);
    if(result!=0)
    {
      crpSetErrorMsg(_T("Crash descriptor is not a valid XML file."));
      goto exit; // Corrupted XML
    }
  }  

  // Check integrity of minidump


  // Add handle to the list of opened handles
  nNewHandle = (int)g_OpenedHandles.size()+1;
  g_OpenedHandles[nNewHandle] = report_data;

  crpSetErrorMsg(_T("Success."));

exit:

  if(hZip!=0) 
    CloseZip(hZip);

  return status;
}

int
CRASHRPTPROBE_API
crpOpenCrashReportA(
  LPCSTR pszFileName,
  LPCSTR pszMd5Hash,
  CrpHandle* pHandle)
{
  strconv_t strconv;
  return crpOpenCrashReportW(strconv.a2w(pszFileName), strconv.a2w(pszMd5Hash), pHandle);  
}

int
CRASHRPTPROBE_API
crpCloseCrashReport(
  CrpHandle handle)
{
  crpSetErrorMsg(_T("Unspecified error."));

  // Look for such handle
  std::map<int, CrpReportData>::iterator it = g_OpenedHandles.find(handle);
  if(it==g_OpenedHandles.end())
  {
    crpSetErrorMsg(_T("Invalid handle specified."));
    return 1;
  }

  // Remove from the list of opened handles
  g_OpenedHandles.erase(it);

  // OK.
  crpSetErrorMsg(_T("Success."));
  return 0;
}

int
CRASHRPTPROBE_API
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
CRASHRPTPROBE_API
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
CRASHRPTPROBE_API
crpGetLongPropertyW(
  CrpHandle handle,
  LPCWSTR lpszPropName,
  PLONG lpnPropVal)
{
  return 0;
}

int
CRASHRPTPROBE_API
crpGetLongPropertyA(
  CrpHandle handle,
  LPCSTR lpszPropName,
  PLONG lpnPropVal)
{
  return 0;
}


int
CRASHRPTPROBE_API
crpExtractFileW(
  CrpHandle handle,
  LPCWSTR lpszFileName,
  LPCWSTR lpszFileSaveAs
);

int
CRASHRPTPROBE_API
crpExtractFileA(
  CrpHandle handle,
  LPCSTR lpszFileName,
  LPCSTR lpszFileSaveAs
);

int 
CRASHRPTPROBE_API
crpGetLastErrorMsgW(
  LPWSTR pszBuffer, 
  UINT uBuffSize)
{
  if(pszBuffer==NULL || uBuffSize==0)
    return -1; // Null pointer to buffer

  strconv_t strconv;

  g_cs.Lock();

  DWORD dwThreadId = GetCurrentThreadId();
  std::map<DWORD, CString>::iterator it = g_sErrorMsg.find(dwThreadId);

  if(it==g_sErrorMsg.end())
  {
    // No error message for current thread.
    CString sErrorMsg = _T("No error.");
    LPCWSTR pwszErrorMsg = strconv.t2w(sErrorMsg.GetBuffer(0));
	  WCSNCPY_S(pszBuffer, uBuffSize, pwszErrorMsg, sErrorMsg.GetLength());
    int size =  sErrorMsg.GetLength();
    g_cs.Unlock();
    return size;
  }
  
  LPWSTR pwszErrorMsg = T2W(it->second.GetBuffer(0));
  WCSNCPY_S(pszBuffer, uBuffSize, pwszErrorMsg, uBuffSize-1);
  int size = it->second.GetLength();
  g_cs.Unlock();
  return size;
}

int 
CRASHRPTPROBE_API
crpGetLastErrorMsgA(
  LPSTR pszBuffer, 
  UINT uBuffSize)
{  
  if(pszBuffer==NULL)
    return -1;

  strconv_t strconv;

  WCHAR* pwszBuffer = new WCHAR[uBuffSize];
    
  int res = crpGetLastErrorMsgW(pwszBuffer, uBuffSize);
  
  LPCSTR paszBuffer = strconv.w2a(pwszBuffer);  

  STRCPY_S(pszBuffer, uBuffSize, paszBuffer);

  delete [] pwszBuffer;

  return res;
}

int crpSetErrorMsg(PTSTR pszErrorMsg)
{  
  g_cs.Lock();
  DWORD dwThreadId = GetCurrentThreadId();
  g_sErrorMsg[dwThreadId] = pszErrorMsg;
  g_cs.Unlock();
  return 0;
}


