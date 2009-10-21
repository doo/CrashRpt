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

// Funtion prototype
int crpSetErrorMsg(PTSTR pszErrorMsg);

#define STACK_TRACE_TBL_IDS 32768 // Dynamic table ids for stack traces are starting from this number

TCHAR* exctypes[13] =
{
  _T("structured exception"),
  _T("terminate call"),
  _T("unexpected call"),
  _T("pure virtual call"),
  _T("new operator fault"),
  _T("buffer overrun"),
  _T("invalid parameter"),
  _T("SIGABRT signal"),
  _T("SIGFPE signal"),
  _T("SIGILL signal"),
  _T("SIGINT signal"),
  _T("SIGSEGV signal"),
  _T("SIGTERM signal"),
};

// CrpReportData
// This structure is used internally for storing report data
struct CrpReportData
{
  CrpReportData()
  {
    m_hZip = 0;
    m_pDescReader = NULL;
    m_pDmpReader = NULL;
  }

  HZIP m_hZip; // Handle to the ZIP archive
  CCrashDescReader* m_pDescReader; // Pointer to the crash descriptor reader object
  CMiniDumpReader* m_pDmpReader;   // Pointer to the minidump reader object
  CString m_sMiniDumpTempName;     // The name of the tmp file to store extracted minidump in
  CString m_sSymSearchPath;        // Symbol files search path
};

// The list of opened handles
std::map<int, CrpReportData> g_OpenedHandles;

// GetBaseFileName
// This helper function returns file name without extension
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

// GetFileExtension
// This helper function returns file extension by file name
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

// CalcFileMD5Hash
// Calculates the MD5 hash for the given file
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
  LPCWSTR pszSymSearchPath,
  DWORD dwFlags,
  CrpHandle* pHandle)
{   
  UNREFERENCED_PARAMETER(dwFlags);

  int status = -1;
  int nNewHandle = 0;
  CrpReportData report_data;  
  ZRESULT zr = 0;
  ZIPENTRY ze;
  int xml_index = -1;
  int dmp_index = -1;
  CString sCalculatedMD5Hash;

  crpSetErrorMsg(_T("Unspecified error."));
  *pHandle = 0;
  
  report_data.m_sSymSearchPath = pszSymSearchPath;
  report_data.m_pDescReader = new CCrashDescReader;
  report_data.m_pDmpReader = new CMiniDumpReader;

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
  report_data.m_hZip = OpenZip(pszFileName, 0);
  if(report_data.m_hZip==NULL)
  {
    crpSetErrorMsg(_T("Error opening ZIP archive."));
    goto exit;
  }

  // Look for v1.1 crash descriptor XML
  zr = FindZipItem(report_data.m_hZip, _T("crashrpt.xml"), false, &xml_index, &ze);
  
  // Look for v1.1 crash dump 
  zr = FindZipItem(report_data.m_hZip, _T("crashdump.dmp"), false, &dmp_index, &ze);
  
  // If xml and dmp still not found, assume it is v1.0
  if(xml_index==-1 && dmp_index==-1)  
  {    
    // Look for .dmp file
    int i=0;
    for(i=0;;i++)
    {
      zr = GetZipItem(report_data.m_hZip, i, &ze);
      if(zr!=ZR_OK)
        break;

      CString sExt = GetFileExtension(ze.name);
      if(sExt.CompareNoCase(_T("dmp"))==0)
      {
        // DMP found
        dmp_index = i;
        break;
      }
    }

    // Assume the name of XML is the same as DMP
    CString sXmlName = GetBaseFileName(ze.name) + _T(".xml");
    zr = FindZipItem(report_data.m_hZip, sXmlName, false, &xml_index, &ze);
    if(zr!=ZR_OK)
    {
      xml_index = -1;
    }
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
    zr = GetZipItem(report_data.m_hZip, xml_index, &ze);
    if(zr!=ZR_OK)
    {
      crpSetErrorMsg(_T("Error retrieving ZIP item."));
      goto exit; // Can't get ZIP element
    }

    CString sTempFile = Utility::getTempFileName();
    zr = UnzipItem(report_data.m_hZip, xml_index, sTempFile);
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
    CString sTempFile = Utility::getTempFileName();
    zr = UnzipItem(report_data.m_hZip, dmp_index, sTempFile);
    if(zr!=ZR_OK)
    {
      crpSetErrorMsg(_T("Error extracting ZIP item."));
      goto exit; // Can't unzip ZIP element
    }

    report_data.m_sMiniDumpTempName = sTempFile;
  } 

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

    if(report_data.m_hZip!=0) 
      CloseZip(report_data.m_hZip);
  }

  
  return status;
}

int
CRASHRPTPROBE_API
crpOpenErrorReportA(
  LPCSTR pszFileName,
  LPCSTR pszMd5Hash,
  LPCSTR pszSymSearchPath,
  DWORD dwFlags,
  CrpHandle* pHandle)
{
  strconv_t strconv;
  return crpOpenErrorReportW(
    strconv.a2w(pszFileName), 
    strconv.a2w(pszMd5Hash), 
    strconv.a2w(pszSymSearchPath), 
    dwFlags, 
    pHandle);  
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

  delete it->second.m_pDescReader;
  delete it->second.m_pDmpReader;

  if(it->second.m_hZip)
    CloseZip(it->second.m_hZip);

  // Remove from the list of opened handles
  g_OpenedHandles.erase(it);

  // OK.
  crpSetErrorMsg(_T("Success."));
  return 0;
}

int
CRASHRPTPROBE_API
crpGetPropertyW(
  CrpHandle hReport,
  CRP_TableId TableId, 
  CRP_ColumnId ColumnId,
  INT nRowIndex,
  LPWSTR lpszBuffer,
  ULONG cchBuffSize,
  PULONG pcchCount)
{
  crpSetErrorMsg(_T("Unspecified error."));

  LPCWSTR pszPropVal = NULL;
  const int BUFF_SIZE = 4096; 
  TCHAR szBuff[BUFF_SIZE]; // Internal buffer to store property value
  strconv_t strconv; // String convertor object
  
  // Validate input parameters
  if(nRowIndex<0 || // Check we have non-negative row index
     (lpszBuffer==NULL && cchBuffSize!=0) || // Check that we have a valid buffer
     (lpszBuffer!=NULL && cchBuffSize==0)
    )
  {
    crpSetErrorMsg(_T("Invalid argument specified."));
    return -1;
  }

  std::map<int, CrpReportData>::iterator it = g_OpenedHandles.find(hReport);
  if(it==g_OpenedHandles.end())
  {
    crpSetErrorMsg(_T("Invalid handle specified."));
    return -1;
  }

  CCrashDescReader* pDescReader = it->second.m_pDescReader;
  CMiniDumpReader* pDmpReader = it->second.m_pDmpReader;

  // Check if we need to load minidump file to be able to get the property
  if( (TableId==CRP_TBL_META && nRowIndex>=CRP_TBL_MDMP_MODULES) || 
    TableId>=CRP_TBL_MDMP_MISC)
  {
    // Load the minidump
    pDmpReader->Open(it->second.m_sMiniDumpTempName, it->second.m_sSymSearchPath);
   
    // Walk the stack if this is needed to get the property
    if(TableId>=STACK_TRACE_TBL_IDS && TableId<(int)(STACK_TRACE_TBL_IDS+pDmpReader->m_DumpData.m_Threads.size()))
    {
      pDmpReader->StackWalk(pDmpReader->m_DumpData.m_Threads[TableId-STACK_TRACE_TBL_IDS].m_dwThreadId);
    }

    if(TableId==CRP_TBL_META && 
      nRowIndex>=STACK_TRACE_TBL_IDS && nRowIndex<(int)(STACK_TRACE_TBL_IDS+pDmpReader->m_DumpData.m_Threads.size()))
    {
      pDmpReader->StackWalk(pDmpReader->m_DumpData.m_Threads[nRowIndex-STACK_TRACE_TBL_IDS].m_dwThreadId);
    }
  }  

  if(TableId==CRP_TBL_META)
  {
    if(ColumnId==CRP_COL_ROW_COUNT)
    {
      int nRowCount = -1;
      if(nRowIndex==CRP_TBL_META)
        nRowCount = CRP_TBL_MDMP_THREADS+1;
      else if(nRowIndex==CRP_TBL_XMLDESC_MISC)
        nRowCount = 1;
      else if(nRowIndex==CRP_TBL_XMLDESC_FILE_ITEMS)
        nRowCount = pDescReader->m_aFileItems.size();
      else if(nRowIndex==CRP_TBL_XMLDESC_FILE_ITEMS)
        nRowCount = pDescReader->m_aFileItems.size();
      else if(nRowIndex==CRP_TBL_MDMP_MISC)
        nRowCount = 1;
      else if(nRowIndex==CRP_TBL_MDMP_MODULES)
        nRowCount = pDmpReader->m_DumpData.m_Modules.size();
      else if(nRowIndex==CRP_TBL_MDMP_THREADS)
        nRowCount = pDmpReader->m_DumpData.m_Threads.size();
      else if(nRowIndex>=STACK_TRACE_TBL_IDS && nRowIndex<(int)(STACK_TRACE_TBL_IDS+pDmpReader->m_DumpData.m_Threads.size()))
        nRowCount = pDmpReader->m_DumpData.m_Threads[nRowIndex-STACK_TRACE_TBL_IDS].m_StackTrace.size();
      else
      {
        crpSetErrorMsg(_T("Invalid row index specified."));
        return -4;    
      }   

      _ultot_s(nRowCount, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }   
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else if(TableId==CRP_TBL_XMLDESC_MISC)
  {
    // This table contains single row.
    if(nRowIndex!=0)
    {
      crpSetErrorMsg(_T("Invalid row index specified."));
      return -4;    
    }

    if(ColumnId==CRP_COL_CRASHRPT_VERSION)
    {    
      _ultot_s(pDescReader->m_dwGeneratorVersion, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }  
    else if(ColumnId==CRP_COL_CRASH_GUID)
    {
      pszPropVal = strconv.t2w(pDescReader->m_sCrashGUID);    
    }
    else if(ColumnId==CRP_COL_APP_NAME)
    {
      pszPropVal = strconv.t2w(pDescReader->m_sAppName);    
    }
    else if(ColumnId==CRP_COL_APP_VERSION)
    {
      pszPropVal = strconv.t2w(pDescReader->m_sAppVersion);    
    }
    else if(ColumnId==CRP_COL_IMAGE_NAME)
    {
      pszPropVal = strconv.t2w(pDescReader->m_sImageName);    
    }
    else if(ColumnId==CRP_COL_OPERATING_SYSTEM)
    {
      pszPropVal = strconv.t2w(pDescReader->m_sOperatingSystem);    
    }
    else if(ColumnId==CRP_COL_SYSTEM_TIME_UTC)
    {
      pszPropVal = strconv.t2w(pDescReader->m_sSystemTimeUTC);    
    }
    else if(ColumnId==CRP_COL_INVPARAM_EXPRESSION)
    {     
      pszPropVal = strconv.t2w(pDescReader->m_sInvParamExpression);    
    }
    else if(ColumnId==CRP_COL_INVPARAM_FILE)
    {
      pszPropVal = strconv.t2w(pDescReader->m_sInvParamFile);    
    }
    else if(ColumnId==CRP_COL_EXCEPTION_TYPE)
    { 
      _ultot_s(pDescReader->m_dwExceptionType, szBuff, BUFF_SIZE, 10);
      _tcscat_s(szBuff, BUFF_SIZE, _T(" "));
      _tcscat_s(szBuff, BUFF_SIZE, exctypes[pDescReader->m_dwExceptionType]);
      pszPropVal = szBuff;      
    }
    else if(ColumnId==CRP_COL_EXCEPTION_CODE)
    {  
      _ultot_s(pDescReader->m_dwExceptionCode, szBuff, BUFF_SIZE, 16);
      _tcscat_s(szBuff, BUFF_SIZE, _T(" "));
      CString msg = Utility::FormatErrorMsg(pDescReader->m_dwExceptionCode);
      _tcscat_s(szBuff, BUFF_SIZE, msg);
      pszPropVal = szBuff;    
    }
    else if(ColumnId==CRP_COL_FPE_SUBCODE)
    { 
      _ultot_s(pDescReader->m_dwFPESubcode, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;        
    }
    else if(ColumnId==CRP_COL_INVPARAM_LINE)
    {       
      _ultot_s(pDescReader->m_dwInvParamLine, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;            
    }
    else if(ColumnId==CRP_COL_USER_EMAIL)
    {
      pszPropVal = strconv.t2w(pDescReader->m_sUserEmail);    
    }
    else if(ColumnId==CRP_COL_PROBLEM_DESCRIPTION)
    {
      pszPropVal = strconv.t2w(pDescReader->m_sProblemDescription);    
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else if(TableId==CRP_TBL_XMLDESC_FILE_ITEMS)
  {
    if(nRowIndex>=(int)pDescReader->m_aFileItems.size())
    {
      crpSetErrorMsg(_T("Invalid row index specified."));
      return -4;    
    }
  
    if(ColumnId==CRP_COL_FILE_ITEM_NAME || 
       ColumnId==CRP_COL_FILE_ITEM_DESCRIPTION)
    {      
      std::map<CString, CString>::iterator it = pDescReader->m_aFileItems.begin();
      int i;
      for(i=0; i<nRowIndex; i++) it++;

      if(ColumnId==CRP_COL_FILE_ITEM_NAME)
        pszPropVal = strconv.t2w(it->first);    
      else
        pszPropVal = strconv.t2w(it->second);    
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }  
  else if(TableId==CRP_TBL_MDMP_MISC)
  {
    if(nRowIndex!=0)
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;    
    }

    if(ColumnId==CRP_COL_CPU_ARCHITECTURE)
    {
      _ultot_s(pDmpReader->m_DumpData.m_uProcessorArchitecture, szBuff, BUFF_SIZE, 10);
      _tcscat_s(szBuff, BUFF_SIZE, _T(" "));

      TCHAR* szDescription = _T("unknown processor type");
      if(pDmpReader->m_DumpData.m_uProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
        szDescription = _T("x64 (AMD or Intel)");
      if(pDmpReader->m_DumpData.m_uProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA32_ON_WIN64)
        szDescription = _T("WOW");
      if(pDmpReader->m_DumpData.m_uProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64)
        szDescription = _T("Intel Itanium Processor Family (IPF)");
      if(pDmpReader->m_DumpData.m_uProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL)
        szDescription = _T("x86");

      _tcscat_s(szBuff, BUFF_SIZE, szDescription);

      pszPropVal = szBuff;        
    }
    else if(ColumnId==CRP_COL_CPU_COUNT)
    {
      _ultot_s(pDmpReader->m_DumpData.m_uchNumberOfProcessors, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;        
    }
    else if(ColumnId==CRP_COL_SYSTEM_TYPE)
    {
      _ultot_s(pDmpReader->m_DumpData.m_uchProductType, szBuff, BUFF_SIZE, 10);
      _tcscat_s(szBuff, BUFF_SIZE, _T(" "));

      TCHAR* szDescription = _T("unknown product type");
      if(pDmpReader->m_DumpData.m_uchProductType==VER_NT_DOMAIN_CONTROLLER)
        szDescription = _T("domain controller");
      if(pDmpReader->m_DumpData.m_uchProductType==VER_NT_SERVER)
        szDescription = _T("server");
      if(pDmpReader->m_DumpData.m_uchProductType==VER_NT_WORKSTATION)
        szDescription = _T("workstation");
      
      _tcscat_s(szBuff, BUFF_SIZE, szDescription);


      pszPropVal = szBuff;        
    }
    else if(ColumnId==CRP_COL_OS_VER_MAJOR)
    {      
      _ultot_s(pDmpReader->m_DumpData.m_ulVerMajor, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;        
    }
    else if(ColumnId==CRP_COL_OS_VER_MINOR)
    {
      _ultot_s(pDmpReader->m_DumpData.m_ulVerMinor, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;        
    }
    else if(ColumnId==CRP_COL_OS_VER_BUILD)
    {
      _ultot_s(pDmpReader->m_DumpData.m_ulVerBuild, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;        
    }
    else if(ColumnId==CRP_COL_OS_VER_CSD)
    {      
      pszPropVal = strconv.t2w(pDmpReader->m_DumpData.m_sCSDVer);    
    }
    else if(ColumnId==CRP_COL_EXCPTRS_EXCEPTION_CODE)
    {      
      _stprintf_s(szBuff, BUFF_SIZE, _T("0x%x"), pDmpReader->m_DumpData.m_uExceptionCode); 
      _tcscat_s(szBuff, BUFF_SIZE, _T(" "));
      CString msg = Utility::FormatErrorMsg(pDmpReader->m_DumpData.m_uExceptionCode);
      _tcscat_s(szBuff, BUFF_SIZE, msg);
      pszPropVal = szBuff;
    }
    else if(ColumnId==CRP_COL_EXCEPTION_ADDRESS)
    {      
      _stprintf_s(szBuff, BUFF_SIZE, _T("0x%I64x"), pDmpReader->m_DumpData.m_uExceptionAddress); 
      pszPropVal = szBuff;
    }
    else if(ColumnId==CRP_COL_EXCEPTION_THREAD_ROWID)
    {      
      _stprintf_s(szBuff, BUFF_SIZE, _T("%d"), pDmpReader->GetThreadRowIdByThreadId(pDmpReader->m_DumpData.m_uExceptionThreadId)); 
      pszPropVal = szBuff;
    }
    else if(ColumnId==CRP_COL_EXCEPTION_MODULE_ROWID)
    {      
      _stprintf_s(szBuff, BUFF_SIZE, _T("%d"), pDmpReader->GetModuleRowIdByAddress(pDmpReader->m_DumpData.m_uExceptionAddress)); 
      pszPropVal = szBuff;
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else if(TableId==CRP_TBL_MDMP_MODULES)
  {  
    if(nRowIndex>=(int)pDmpReader->m_DumpData.m_Modules.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;    
    }

    if(ColumnId==CRP_COL_MODULE_NAME)
    {      
      pszPropVal = strconv.t2w(pDmpReader->m_DumpData.m_Modules[nRowIndex].m_sModuleName);    
    }
    else if(ColumnId==CRP_COL_MODULE_BASE_ADDRESS)
    {      
      _stprintf_s(szBuff, BUFF_SIZE, _T("0x%I64x"), pDmpReader->m_DumpData.m_Modules[nRowIndex].m_uBaseAddr); 
      pszPropVal = szBuff;
    }
    else if(ColumnId==CRP_COL_MODULE_SIZE)
    {
      _stprintf_s(szBuff, BUFF_SIZE, _T("%I64u"), pDmpReader->m_DumpData.m_Modules[nRowIndex].m_uImageSize); 
      pszPropVal = szBuff;
    }    
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else if(TableId==CRP_TBL_MDMP_THREADS)
  {  
    if(nRowIndex>=(int)pDmpReader->m_DumpData.m_Threads.size())
    {
      crpSetErrorMsg(_T("Invalid row index specified."));
      return -4;    
    }

    if(ColumnId==CRP_COL_THREAD_ID)
    {
      _stprintf_s(szBuff, BUFF_SIZE, _T("0x%x"), pDmpReader->m_DumpData.m_Threads[nRowIndex].m_dwThreadId); 
      pszPropVal = szBuff;
    }
    else if(ColumnId==CRP_COL_THREAD_STACK_TABLEID)
    {
      _stprintf_s(szBuff, BUFF_SIZE, _T("%d"), STACK_TRACE_TBL_IDS+nRowIndex); 
      pszPropVal = szBuff;
    }    
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }

  }  
  else if(TableId>=STACK_TRACE_TBL_IDS && TableId<(int)(STACK_TRACE_TBL_IDS+pDmpReader->m_DumpData.m_Threads.size()))
  {
    int nEntryIndex = TableId - STACK_TRACE_TBL_IDS;
    
    if(nRowIndex>=(int)pDmpReader->m_DumpData.m_Threads[nEntryIndex].m_StackTrace.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;    
    }

    if(ColumnId==CRP_COL_STACK_OFFSET_IN_SYMBOL)
    {      
      _stprintf_s(szBuff, BUFF_SIZE, _T("0x%I64x"), pDmpReader->m_DumpData.m_Threads[nEntryIndex].m_StackTrace[nRowIndex].m_dw64OffsInSymbol);      
      pszPropVal = szBuff;                  
    }
    else if(ColumnId==CRP_COL_STACK_ADDR_PC_OFFSET)
    {       
      _stprintf_s(szBuff, BUFF_SIZE, _T("0x%I64x"), pDmpReader->m_DumpData.m_Threads[nEntryIndex].m_StackTrace[nRowIndex].m_dwAddrPCOffset);
      pszPropVal = szBuff;                
    }
    else if(ColumnId==CRP_COL_STACK_SOURCE_LINE)
    {       
      _ultot_s(pDmpReader->m_DumpData.m_Threads[nEntryIndex].m_StackTrace[nRowIndex].m_nSrcLineNumber, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;                
    }  
    else if(ColumnId==CRP_COL_STACK_MODULE_ROWID)
    {     
      _ultot_s(pDmpReader->m_DumpData.m_Threads[nEntryIndex].m_StackTrace[nRowIndex].m_nModuleRowID, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;       
    }
    else if(ColumnId==CRP_COL_STACK_SYMBOL_NAME)
    {      
      pszPropVal = strconv.t2w(pDmpReader->m_DumpData.m_Threads[nEntryIndex].m_StackTrace[nRowIndex].m_sSymbolName);    
    }
    else if(ColumnId==CRP_COL_STACK_SOURCE_FILE)
    {      
      pszPropVal = strconv.t2w(pDmpReader->m_DumpData.m_Threads[nEntryIndex].m_StackTrace[nRowIndex].m_sSrcFileName);    
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else
  {
    crpSetErrorMsg(_T("Invalid table ID specified."));
    return -3;
  }


  // Check the provided buffer size
  if(lpszBuffer==NULL || cchBuffSize==0)
  {
    // User wants us to get the required length of the buffer
    if(pcchCount!=NULL)
    {
      *pcchCount = (ULONG)wcslen(pszPropVal);
    }
  }
  else
  {
    // User wants us to return the property value 
    ULONG uRequiredLen = (ULONG)wcslen(pszPropVal);
    if(uRequiredLen>(cchBuffSize))
    {
      crpSetErrorMsg(_T("Buffer is too small."));
      return uRequiredLen;
    }

    // Copy the property to the buffer
    wcscpy_s(lpszBuffer, cchBuffSize, pszPropVal);

    if(pcchCount!=NULL)
    {
      *pcchCount = uRequiredLen;
    }
  }

  // Done.
  crpSetErrorMsg(_T("Success."));
  return 0;
}

int
CRASHRPTPROBE_API
crpGetPropertyA(
  CrpHandle hReport,
  CRP_TableId TableId,
  CRP_ColumnId ColumnId,
  INT nRowIndex,
  LPSTR lpszBuffer,
  ULONG cchBuffSize,
  PULONG pcchCount)
{
  crpSetErrorMsg(_T("Unspecified error."));

  WCHAR* szBuffer = new WCHAR[cchBuffSize];
  strconv_t strconv;

  int result = crpGetPropertyW(hReport, TableId, ColumnId, 
    nRowIndex, szBuffer, cchBuffSize, pcchCount);

  LPCSTR aszResult = strconv.w2a(szBuffer);
  delete [] szBuffer;
  strcpy_s(lpszBuffer, cchBuffSize, aszResult);
  return result;
}

int
CRASHRPTPROBE_API
crpExtractFileW(
  CrpHandle hReport,
  LPCWSTR lpszFileName,
  LPCWSTR lpszFileSaveAs,
  BOOL bOverwriteExisting)
{
  crpSetErrorMsg(_T("Unspecified error."));
  
  strconv_t strconv;
  ZRESULT zr;
  ZIPENTRY ze;
  int index = -1;
  HZIP hZip = 0;
  
  std::map<int, CrpReportData>::iterator it = g_OpenedHandles.find(hReport);
  if(it==g_OpenedHandles.end())
  {
    crpSetErrorMsg(_T("Invalid handle specified."));
    return -1;
  }
  
  hZip = it->second.m_hZip;

  zr = FindZipItem(hZip, strconv.w2t(lpszFileName), true, &index, &ze);
  if(zr!=ZR_OK)
  {
    crpSetErrorMsg(_T("Couldn't find the specified zip item."));
    return -2;
  }

  if(!bOverwriteExisting)
  {
    // Check if such file already exists
    DWORD dwFileAttrs = GetFileAttributes(lpszFileSaveAs);
    if(dwFileAttrs!=INVALID_FILE_ATTRIBUTES && // such object exists
       dwFileAttrs!=FILE_ATTRIBUTE_DIRECTORY)  // and it is not a directory
    {
      crpSetErrorMsg(_T("Such file already exists."));
      return -3;
    }
  }

  zr = UnzipItem(hZip, index, strconv.w2t(lpszFileSaveAs));
  if(zr!=ZR_OK)
  {
    crpSetErrorMsg(_T("Error extracting the specified zip item."));
    return -4;
  }  

  crpSetErrorMsg(_T("Success."));
  return 0;
}

int
CRASHRPTPROBE_API
crpExtractFileA(
  CrpHandle hReport,
  LPCSTR lpszFileName,
  LPCSTR lpszFileSaveAs,
  BOOL bOverwriteExisting)
{
  strconv_t strconv;
  LPCWSTR pwszFileName = strconv.a2w(lpszFileName);
  LPCWSTR pwszFileSaveAs = strconv.a2w(lpszFileSaveAs);

  return crpExtractFileW(hReport, pwszFileName, pwszFileSaveAs, bOverwriteExisting);
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
  
  LPCWSTR pwszErrorMsg = strconv.t2w(it->second.GetBuffer(0));
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


