// CrashSender.cpp : main source file for CrashSender.exe
//

#include "stdafx.h"
#include "CrashSender.h"
#include "resource.h"
#include "MainDlg.h"
#include "tinyxml.h"
#include "smtpclient.h"
#include "httpsend.h"
#include "unzip.h"
#include "utility.h"
#include "CrashRpt.h"
#include "base64.h"
#include "strconv.h"

CAppModule _Module;
CCrashInfo g_CrashInfo;
CMainDlg dlgMain;
CProgressDlg dlgProgress;

int CCrashInfo::ParseCrashInfo(CString sCrashInfoFileName)
{ 
  strconv_t strconv;

  TiXmlDocument doc;
  bool bOpen = doc.LoadFile(strconv.t2a(sCrashInfoFileName));
  if(!bOpen)
    return 1;

  TiXmlHandle hRoot = doc.FirstChild("CrashRptInternal");
  if(hRoot.ToElement()==NULL)
    return 1;

  TiXmlHandle hReportFolder = hRoot.FirstChild("ReportFolder");
  const char* szReportFolder = hReportFolder.FirstChild().ToText()->Value();
  if(szReportFolder!=NULL)
    m_sErrorReportDirName = szReportFolder;
  
  TiXmlHandle hCrashGUID = hRoot.FirstChild("CrashGUID");
  const char* szCrashGUID = hCrashGUID.FirstChild().ToText()->Value();
  if(szCrashGUID!=NULL)
    m_sCrashGUID = szCrashGUID;

  TiXmlHandle hDbgHelpPath = hRoot.FirstChild("DbgHelpPath");
  const char* szDbgHelpPath = hDbgHelpPath.FirstChild().ToText()->Value();
  if(szDbgHelpPath!=NULL)
    m_sDbgHelpPath = szDbgHelpPath;

  TiXmlHandle hMinidumpType = hRoot.FirstChild("MinidumpType");
  const char* szMinidumpType = hMinidumpType.FirstChild().ToText()->Value();
  if(szMinidumpType!=NULL)
    m_MinidumpType = (MINIDUMP_TYPE)atol(szMinidumpType);

  TiXmlHandle hUrl = hRoot.FirstChild("Url");
  const char* szUrl = hUrl.FirstChild().ToText()->Value();
  if(szUrl!=NULL)
    m_sUrl = szUrl;

  TiXmlHandle hPrivacyPolicyUrl = hRoot.FirstChild("PrivacyPolicyUrl");
  const char* szPrivacyPolicyUrl = hPrivacyPolicyUrl.FirstChild().ToText()->Value();
  if(szPrivacyPolicyUrl!=NULL)
    m_sPrivacyPolicyURL = szPrivacyPolicyUrl;

  TiXmlHandle hHttpPriority = hRoot.FirstChild("HttpPriority");
  const char* szHttpPriority = hHttpPriority.FirstChild().ToText()->Value();
  if(szHttpPriority!=NULL)
    m_uPriorities[CR_HTTP] = atoi(szHttpPriority);

  TiXmlHandle hSmtpPriority = hRoot.FirstChild("SmtpPriority");
  const char* szSmtpPriority = hSmtpPriority.FirstChild().ToText()->Value();
  if(szSmtpPriority!=NULL)
    m_uPriorities[CR_SMTP] = atoi(szSmtpPriority);

  TiXmlHandle hMapiPriority = hRoot.FirstChild("MapiPriority");
  const char* szMapiPriority = hMapiPriority.FirstChild().ToText()->Value();
  if(szMapiPriority!=NULL)
    m_uPriorities[CR_SMAPI] = atoi(szMapiPriority);

  TiXmlHandle hProcessId = hRoot.FirstChild("ProcessId");
  const char* szProcessId = hProcessId.FirstChild().ToText()->Value();
  if(szProcessId!=NULL)
    m_dwProcessId = strtoul(szProcessId, NULL, 10);

  TiXmlHandle hThreadId = hRoot.FirstChild("ThreadId");
  const char* szThreadId = hThreadId.FirstChild().ToText()->Value();
  if(szThreadId!=NULL)
    m_dwThreadId = strtoul(szThreadId, NULL, 10);

  TiXmlHandle hExceptionPointers = hRoot.FirstChild("ExceptionPointers");
  if(hExceptionPointers.ToElement()!=NULL)
  {
    TiXmlHandle hContext = hExceptionPointers.FirstChild("Context");
    const char* szContext = hContext.FirstChild().ToText()->Value();
    if(szContext!=NULL)
    {
      std::string sContext = base64_decode(szContext);
      g_CrashInfo.m_ExInfo.ContextRecord = new CONTEXT;
      memcpy(g_CrashInfo.m_ExInfo.ContextRecord, sContext.c_str(), sizeof(CONTEXT));
    }

    int n = 0;
    EXCEPTION_RECORD* pExRec = NULL;
    for(;;n++)
    {
      CString sName;
      sName.Format(_T("ExceptionRecord%d"), n);
      TiXmlHandle hExRec = hExceptionPointers.FirstChild(strconv.t2a(sName));
      if(hExRec.ToElement()==NULL)
        break;
      
      const char* szExRec = hExRec.FirstChild().ToText()->Value();
      if(szExRec)
      {
        EXCEPTION_RECORD* pNewRec = new EXCEPTION_RECORD;
        if(pExRec==NULL)
          g_CrashInfo.m_ExInfo.ExceptionRecord = pNewRec;
        else
          pNewRec->ExceptionRecord = pNewRec;

        pExRec = pNewRec;

        std::string sExRec = base64_decode(szExRec);
        memcpy(pExRec, sExRec.c_str(), sizeof(EXCEPTION_RECORD));           
      }      
    }
  }

  ParseCrashDescriptor(m_sErrorReportDirName + _T("\\crashrpt.xml"));

  return ParseFileList(hRoot);
}

int CCrashInfo::ParseFileList(TiXmlHandle& hRoot)
{
  strconv_t strconv;
   
  TiXmlHandle fl = hRoot.FirstChild("FileItems");
  if(fl.ToElement()==0)
  {    
    return 1;
  }

  TiXmlHandle fi = fl.FirstChild("FileItem");
  while(fi.ToElement()!=0)
  {
    const char* pszDestFile = fi.ToElement()->Attribute("destfile");
    const char* pszSrcFile = fi.ToElement()->Attribute("srcfile");
    const char* pszDesc = fi.ToElement()->Attribute("description");
    const char* pszMakeCopy = fi.ToElement()->Attribute("makecopy");

    if(pszDestFile!=NULL)
    {
	    CString sDestFile = pszDestFile;      
      FileItem item;
      item.m_sDestFile = sDestFile;
      if(pszSrcFile)
        item.m_sSrcFile = pszSrcFile;
      if(pszDesc)
        item.m_sDesc = pszDesc;

      if(pszMakeCopy)
      {
        if(strcmp(pszMakeCopy, "1")==0)
          item.m_bMakeCopy = TRUE;
        else
          item.m_bMakeCopy = FALSE;
      }
      else
        item.m_bMakeCopy = FALSE;
      
      m_FileItems[sDestFile] = item;
    }

    fi = fi.ToElement()->NextSibling("FileItem");
  }

  return 0;
}

int CCrashInfo::ParseCrashDescriptor(CString sFileName)
{
  strconv_t strconv;

  TiXmlDocument doc;
  bool bOpen = doc.LoadFile(strconv.t2a(sFileName));
  if(!bOpen)
    return 1;

  TiXmlHandle hRoot = doc.FirstChild("CrashRpt");
  if(hRoot.ToElement()==NULL)
    return 1;

  TiXmlHandle hAppName = hRoot.FirstChild("AppName");
  const char* szAppName = hAppName.FirstChild().ToText()->Value();
  if(szAppName!=NULL)
    m_sAppName = szAppName;

  TiXmlHandle hAppVersion = hRoot.FirstChild("AppVersion");
  const char* szAppVersion = hAppVersion.FirstChild().ToText()->Value();
  if(szAppVersion!=NULL)
    m_sAppVersion = szAppVersion;

  TiXmlHandle hImageName = hRoot.FirstChild("ImageName");
  const char* szImageName = hAppName.FirstChild().ToText()->Value();
  if(szImageName!=NULL)
    m_sImageName = szImageName;

  return 0;
}

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int /*nCmdShow*/ = SW_SHOWDEFAULT)
{
  LPCWSTR szCommandLine = GetCommandLineW();  
  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(szCommandLine, &argc);
 
  ATLASSERT(0);

  if(argc==1)
  {
    g_CrashInfo.ParseCrashInfo(CString(argv[0]));
  }

  // Notify the parent process that we have finished,
  // so the parent process unblock
  CString sEventName;
  sEventName.Format(_T("Local\\CrashRptEvent_%s"), g_CrashInfo.m_sCrashGUID);
  HANDLE hEvent = OpenEvent(SYNCHRONIZE, FALSE, sEventName);
  if(hEvent!=NULL)
    SetEvent(hEvent);


  // Check window mirroring settings 
  CString sRTL = Utility::GetINIString(_T("Settings"), _T("RTLReading"));
  if(sRTL.CompareNoCase(_T("1"))==0)
  {
  	SetProcessDefaultLayout(LAYOUT_RTL);  
  }  

  CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);
  
	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}
  
	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();

	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

  int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
