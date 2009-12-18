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

CAppModule _Module;
CCrashInfo g_CrashInfo;
CMainDlg dlgMain;
CProgressDlg dlgProgress;

int CCrashInfo::ParseCrashInfo(LPCSTR text)
{  
  TiXmlDocument doc;
  doc.Parse(text);
  if(doc.Error())
    return 1;

  TiXmlHandle hRoot = doc.FirstChild("crashrpt");
  if(hRoot.ToElement()==NULL)
    return 1;

  const char* pszAppName = hRoot.ToElement()->Attribute("appname");
  const char* pszAppVersion= hRoot.ToElement()->Attribute("appver");
  const char* pszImageName = hRoot.ToElement()->Attribute("imagename");
  const char* pszSubject = hRoot.ToElement()->Attribute("subject");
  const char* pszMailTo = hRoot.ToElement()->Attribute("mailto");
  const char* pszUrl = hRoot.ToElement()->Attribute("url");
  const char* pszErrorReportDirName = hRoot.ToElement()->Attribute("errorreportdirname");
  const char* pszHttpPriority = hRoot.ToElement()->Attribute("http_priority");
  const char* pszSmtpPriority = hRoot.ToElement()->Attribute("smtp_priority");
  const char* pszMapiPriority = hRoot.ToElement()->Attribute("mapi_priority");
  const char* pszPrivacyPolicyURL = hRoot.ToElement()->Attribute("privacy_policy_url");

  if(pszAppName)
    m_sAppName = pszAppName;

  if(pszAppVersion)
    m_sAppVersion = pszAppVersion;

  if(pszImageName)
    m_sImageName = pszImageName;

  if(pszSubject)
    m_sEmailSubject = pszSubject;

  if(pszMailTo!=NULL)
    m_sEmailTo = pszMailTo;

  if(pszUrl!=NULL)
    m_sUrl = pszUrl;

  if(pszErrorReportDirName!=NULL)
    m_sErrorReportDirName = pszErrorReportDirName;

  if(pszHttpPriority!=NULL)
    m_uPriorities[CR_HTTP] = atoi(pszHttpPriority);
  else
    m_uPriorities[CR_HTTP] = 0;

  if(pszSmtpPriority!=NULL)
    m_uPriorities[CR_SMTP] = atoi(pszSmtpPriority);
  else
    m_uPriorities[CR_SMTP] = 0;

  if(pszMapiPriority!=NULL)
    m_uPriorities[CR_SMAPI] = atoi(pszMapiPriority);
  else
    m_uPriorities[CR_SMAPI] = 0;
  
  if(pszPrivacyPolicyURL!=NULL)
    m_sPrivacyPolicyURL = pszPrivacyPolicyURL;

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

int GetCrashInfoThroughPipe()
{
  // Create named pipe to get crash information from client process.
  
  DWORD dwProcessId = GetCurrentProcessId();
  CString szPipeName;
  szPipeName.Format(_T("\\\\.\\pipe\\CrashRpt_%lu"), dwProcessId);
  
  HANDLE hPipe = CreateNamedPipe(
    szPipeName, PIPE_ACCESS_INBOUND|FILE_FLAG_OVERLAPPED, 
    0, 1, 0, 1024, 0, NULL);
  
  if(hPipe==INVALID_HANDLE_VALUE)
  {
    ATLASSERT(hPipe!=INVALID_HANDLE_VALUE);
    return 1; // Couldn't create pipe
  }

  // Connect pipe
  OVERLAPPED overlapped;
  memset(&overlapped, 0, sizeof(OVERLAPPED));
  overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  BOOL bConnected = ConnectNamedPipe(hPipe, &overlapped);
  DWORD dwLastError = GetLastError();
  if(!bConnected && 
     dwLastError!=ERROR_IO_PENDING &&
     dwLastError!=ERROR_PIPE_CONNECTED)
  {
    ATLASSERT(bConnected);
    CloseHandle(hPipe);
    return 3; // Couldn't connect 
  }

  DWORD dwWaitResult = WaitForSingleObject(overlapped.hEvent, 60*1000);
  if(dwWaitResult!=WAIT_OBJECT_0)
  {    
    ATLASSERT(dwWaitResult==WAIT_OBJECT_0);
    CloseHandle(hPipe);
    return 4; // Time out interval exceeded
  }

  // Check connection result
  DWORD dwBytesTransferred = 0;
  BOOL bConnectionResult = GetOverlappedResult(hPipe, &overlapped, &dwBytesTransferred, TRUE); 
  if(!bConnectionResult)
  {
    ATLASSERT(bConnectionResult!=0);
    return 5; // Connection failed
  }

  // Read incoming data
  std::string sDataA;
  for(;;)
  {
    DWORD dwBytesRead = 0;
    BYTE buffer[1024];
    BOOL bRead = ReadFile(hPipe, buffer, 1024, &dwBytesRead, NULL);
    if(!bRead)
      break;
    sDataA += std::string((char*)buffer, dwBytesRead);
  }

  // Disconnect
  BOOL bDisconnected = DisconnectNamedPipe(hPipe);
  ATLASSERT(bDisconnected);
  bDisconnected;

  CloseHandle(hPipe);
  CloseHandle(overlapped.hEvent); 

  // Parse text  
  int nParseResult = g_CrashInfo.ParseCrashInfo(sDataA.c_str());
  if(nParseResult!=0)
  {
    ATLASSERT(nParseResult==0);
    return 6;
  }
  
  // Success
  return 0;
}

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
  // Check window mirroring settings 
  CString sRTL = Utility::GetINIString(_T("Settings"), _T("RTLReading"));
  if(sRTL.CompareNoCase(_T("1"))==0)
  {
  	SetProcessDefaultLayout(LAYOUT_RTL);  
  }  

  CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);
  
  int nGetCrashInfoThroughPipe = GetCrashInfoThroughPipe();
  if(nGetCrashInfoThroughPipe!=0)
  {
    ATLASSERT(nGetCrashInfoThroughPipe==0);
    return 1; 
  }
    
	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}
  
	//dlgMain.ShowWindow(nCmdShow);

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
