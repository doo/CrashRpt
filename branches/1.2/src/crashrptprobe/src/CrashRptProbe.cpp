// CrashRptProbe.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "CrashRptProbe.h"
#include "CrashRpt.h"
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
  CCrashDescReader* m_pDescReader;
  CMiniDumpReader* m_pDmpReader;  
  CString m_sMiniDumpTempName;
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
crpOpenErrorReportW(
  LPCWSTR pszFileName,
  LPCWSTR pszMd5Hash,
  DWORD dwFlags,
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
  *pHandle = 0;
  
  report_data.m_pDescReader = new CCrashDescReader();
  report_data.m_pDmpReader = new CMiniDumpReader();

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

    int result = report_data.m_pDescReader->Load(sTempFile);    
    DeleteFile(sTempFile);
    if(result!=0)
    {
      crpSetErrorMsg(_T("Crash descriptor is not a valid XML file."));
      goto exit; // Corrupted XML
    }
  }  

  // Extract minidump file
  if(dmp_index>=0)
  {
    CString sTempFile = CUtility::getTempFileName();
    zr = UnzipItem(hZip, dmp_index, sTempFile);
    if(zr!=ZR_OK)
    {
      crpSetErrorMsg(_T("Error extracting ZIP item."));
      goto exit; // Can't unzip ZIP element
    }

    report_data.m_sMiniDumpTempName = sTempFile;
  }

  // Check integrity of minidump

  // Add handle to the list of opened handles
  nNewHandle = (int)g_OpenedHandles.size()+1;
  g_OpenedHandles[nNewHandle] = report_data;
  *pHandle = nNewHandle;
  
  crpSetErrorMsg(_T("Success."));
  status = 0;

exit:

  if(status!=0)
  {
    delete report_data.m_pDescReader;
    delete report_data.m_pDmpReader;
  }

  if(hZip!=0) 
    CloseZip(hZip);

  return status;
}

int
CRASHRPTPROBE_API
crpOpenErrorReportA(
  LPCSTR pszFileName,
  LPCSTR pszMd5Hash,
  DWORD dwFlags,
  CrpHandle* pHandle)
{
  strconv_t strconv;
  return crpOpenErrorReportW(strconv.a2w(pszFileName), strconv.a2w(pszMd5Hash), dwFlags, pHandle);  
}

int
CRASHRPTPROBE_API
crpCloseErrorReport(
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
  delete it->second.m_pDescReader;
  delete it->second.m_pDmpReader;
  g_OpenedHandles.erase(it);

  // OK.
  crpSetErrorMsg(_T("Success."));
  return 0;
}

int
CRASHRPTPROBE_API
crpGetPropertyW(
  CrpHandle hReport,
  INT nPropId, 
  INT nIndex,
  LPWSTR lpszBuffer,
  ULONG cchBuffSize,
  PULONG pcchCount)
{
  crpSetErrorMsg(_T("Unspecified error."));

  LPCWSTR pszPropVal = NULL;
  const int BUFF_SIZE = 1024;
  TCHAR szBuff[BUFF_SIZE];
  strconv_t strconv;
  
  std::map<int, CrpReportData>::iterator it = g_OpenedHandles.find(hReport);
  if(it==g_OpenedHandles.end())
  {
    crpSetErrorMsg(_T("Invalid handle specified."));
    return -1;
  }

  CCrashDescReader* pDescReader = it->second.m_pDescReader;
  CMiniDumpReader* pDmpReader = it->second.m_pDmpReader;

  if(nPropId>=CRP_PROP_STACK_FRAME_COUNT)
  {
    int result = pDmpReader->Open(it->second.m_sMiniDumpTempName);
  }

  if(nPropId==CRP_PROP_CRASH_GUID)
  {
    pszPropVal = strconv.t2w(pDescReader->m_sCrashGUID);    
  }
  else if(nPropId==CRP_PROP_APP_NAME)
  {
    pszPropVal = strconv.t2w(pDescReader->m_sAppName);    
  }
  else if(nPropId==CRP_PROP_APP_VERSION)
  {
    pszPropVal = strconv.t2w(pDescReader->m_sAppVersion);    
  }
  else if(nPropId==CRP_PROP_IMAGE_NAME)
  {
    pszPropVal = strconv.t2w(pDescReader->m_sImageName);    
  }
  else if(nPropId==CRP_PROP_OPERATING_SYSTEM)
  {
    pszPropVal = strconv.t2w(pDescReader->m_sOperatingSystem);    
  }
  else if(nPropId==CRP_PROP_SYSTEM_TIME_UTC)
  {
    pszPropVal = strconv.t2w(pDescReader->m_sSystemTimeUTC);    
  }
  else if(nPropId==CRP_PROP_INVPARAM_EXPRESSION)
  {
    if(pDescReader->m_dwExceptionType!=CR_CPP_INVALID_PARAMETER)
    {
      crpSetErrorMsg(_T("This property is valid for invalid parameter exceptions only."));
      return -3;
    }
    pszPropVal = strconv.t2w(pDescReader->m_sInvParamExpression);    
  }
  else if(nPropId==CRP_PROP_INVPARAM_FILE)
  {
    if(pDescReader->m_dwExceptionType!=CR_CPP_INVALID_PARAMETER)
    {
      crpSetErrorMsg(_T("This property is valid for invalid parameter exceptions only."));
      return -3;
    }
    pszPropVal = strconv.t2w(pDescReader->m_sInvParamFile);    
  }
  else if(nPropId==CRP_PROP_USER_EMAIL)
  {
    pszPropVal = strconv.t2w(pDescReader->m_sUserEmail);    
  }
  else if(nPropId==CRP_PROP_PROBLEM_DESCRIPTION)
  {
    pszPropVal = strconv.t2w(pDescReader->m_sProblemDescription);    
  }
  else if(nPropId==CRP_PROP_FILE_ITEM_NAME || 
    nPropId==CRP_PROP_FILE_ITEM_DESCRIPTION)
  {
    if(nIndex<0 || nIndex>=(int)pDescReader->m_aFileItems.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;    
    }
    
    std::map<CString, CString>::iterator it = pDescReader->m_aFileItems.begin();
    int i;
    for(i=0; i<nIndex; i++) it++;

    if(nPropId==CRP_PROP_FILE_ITEM_NAME)
      pszPropVal = strconv.t2w(it->first);    
    else
      pszPropVal = strconv.t2w(it->second);    
  }
  else if(nPropId==CRP_PROP_STACK_MODULE_NAME)
  {
    if(nIndex<0 || nIndex>=(int)pDmpReader->m_DumpData.m_StackTrace.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;    
    }
    pszPropVal = strconv.t2w(pDmpReader->m_DumpData.m_StackTrace[nIndex].m_sModuleName);    
  }
  else if(nPropId==CRP_PROP_STACK_SYMBOL_NAME)
  {
    if(nIndex<0 || nIndex>=(int)pDmpReader->m_DumpData.m_StackTrace.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;    
    }
    pszPropVal = strconv.t2w(pDmpReader->m_DumpData.m_StackTrace[nIndex].m_sSymbolName);    
  }
  else if(nPropId==CRP_PROP_STACK_SOURCE_FILE)
  {
    if(nIndex<0 || nIndex>=(int)pDmpReader->m_DumpData.m_StackTrace.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;    
    }
    pszPropVal = strconv.t2w(pDmpReader->m_DumpData.m_StackTrace[nIndex].m_sSrcFileName);    
  }
  else if(nPropId==CRP_PROP_CRASHRPT_VERSION)
  {    
    _ultot_s(pDescReader->m_dwGeneratorVersion, szBuff, BUFF_SIZE, 10);
    pszPropVal = szBuff;
  }
  else if(nPropId==CRP_PROP_FILE_COUNT)
  {   
    _ltot_s((long)pDescReader->m_aFileItems.size(), szBuff, BUFF_SIZE, 10);
    pszPropVal = szBuff;    
  }
  else if(nPropId==CRP_PROP_EXCEPTION_TYPE)
  {   
    _ultot_s(pDescReader->m_dwExceptionType, szBuff, BUFF_SIZE, 10);
    pszPropVal = szBuff;
    
  }
  else if(nPropId==CRP_PROP_EXCEPTION_CODE)
  {  
    if(pDescReader->m_dwExceptionType!=CR_WIN32_STRUCTURED_EXCEPTION)
    {
      crpSetErrorMsg(_T("This property is valid for strucutred exceptions only."));
      return -3;
    }

    _ultot_s(pDescReader->m_dwExceptionCode, szBuff, BUFF_SIZE, 16);
    pszPropVal = szBuff;    
  }
  else if(nPropId==CRP_PROP_FPE_SUBCODE)
  { 
    if(pDescReader->m_dwExceptionType!=CR_CPP_SIGFPE)
    {
      crpSetErrorMsg(_T("This property is valid for floating point exceptions only."));
      return -3;
    }
    _ultot_s(pDescReader->m_dwFPESubcode, szBuff, BUFF_SIZE, 10);
    pszPropVal = szBuff;        
  }
  else if(nPropId==CRP_PROP_INVPARAM_LINE)
  { 
    if(pDescReader->m_dwExceptionType!=CR_CPP_INVALID_PARAMETER)
    {
      crpSetErrorMsg(_T("This property is valid for invalid parameter exceptions only."));
      return -3;
    }
    _ultot_s(pDescReader->m_dwInvParamLine, szBuff, BUFF_SIZE, 10);
    pszPropVal = szBuff;            
  }
  else if(nPropId==CRP_PROP_STACK_FRAME_COUNT)
  {     
    _ultot_s((LONG)pDmpReader->m_DumpData.m_StackTrace.size(), szBuff, BUFF_SIZE, 10);
    pszPropVal = szBuff;            
  }
  else if(nPropId==CRP_PROP_STACK_OFFSET_IN_SYMBOL)
  {     
    if(nIndex<0 || nIndex>=(int)pDmpReader->m_DumpData.m_StackTrace.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;    
    }
    _ui64tot_s(pDmpReader->m_DumpData.m_StackTrace[nIndex].m_dw64OffsInSymbol, szBuff, BUFF_SIZE, 16);
    pszPropVal = szBuff;            
    
  }
  else if(nPropId==CRP_PROP_STACK_SOURCE_LINE)
  {     
    if(nIndex<0 || nIndex>=(int)pDmpReader->m_DumpData.m_StackTrace.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;    
    }
    _ultot_s(pDmpReader->m_DumpData.m_StackTrace[nIndex].m_nSrcLineNumber, szBuff, BUFF_SIZE, 10);
    pszPropVal = szBuff;                
  }
  else
  {
    crpSetErrorMsg(_T("Invalid property ID."));
    return -2;
  }

  if(cchBuffSize==0)
  {
    if(pcchCount!=NULL)
    {
      *pcchCount = (ULONG)wcslen(pszPropVal);
    }
  }
  else
  {
    ULONG uRequiredLen = (ULONG)wcslen(pszPropVal);
    if(uRequiredLen>(cchBuffSize))
    {
      crpSetErrorMsg(_T("Buffer is too small."));
      return uRequiredLen;
    }

    wcscpy_s(lpszBuffer, cchBuffSize, pszPropVal);

    if(pcchCount!=NULL)
    {
      *pcchCount = uRequiredLen;
    }
  }

  crpSetErrorMsg(_T("Successs."));
  return 0;
}

int
CRASHRPTPROBE_API
crpGetPropertyA(
  CrpHandle hReport,
  INT nPropId,
  INT nIndex,
  LPSTR lpszBuffer,
  ULONG cchBuffSize,
  PULONG pcchCount)
{
  crpSetErrorMsg(_T("Unspecified error."));

  WCHAR* szBuffer = new WCHAR[cchBuffSize];
  strconv_t strconv;

  int result = crpGetPropertyW(hReport, nPropId, nIndex, szBuffer, cchBuffSize, pcchCount);

  LPCSTR aszResult = strconv.w2a(szBuffer);
  delete [] szBuffer;
  strcpy_s(lpszBuffer, cchBuffSize, aszResult);
  return result;
}

int
CRASHRPTPROBE_API
crpExtractFileW(
  CrpHandle handle,
  LPCWSTR lpszFileName,
  LPCWSTR lpszFileSaveAs
)
{
  return 0;
}

int
CRASHRPTPROBE_API
crpExtractFileA(
  CrpHandle handle,
  LPCSTR lpszFileName,
  LPCSTR lpszFileSaveAs
)
{
  return 0;
}

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


