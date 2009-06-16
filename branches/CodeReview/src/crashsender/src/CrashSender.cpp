// CrashSender.cpp : main source file for CrashSender.exe
//

#include "stdafx.h"
#include "resource.h"
#include "MainDlg.h"
#include "tinyxml.h"
#include "smtpclient.h"

CAppModule _Module;

int ParseFileList(CStringA& text, CString& sAppName, CString& sImageName,
  CString& sSubject, CString& sMailTo, std::map<CStringA, CStringA>& file_list)
{
  TiXmlDocument doc;
  doc.Parse(text.GetBuffer());
  if(doc.Error())
    return 1;

  TiXmlHandle hRoot = doc.FirstChild("crashrpt");
  if(hRoot.ToElement()==NULL)
    return 1;

  const char* pszAppName = hRoot.ToElement()->Attribute("appname");
  const char* pszImageName = hRoot.ToElement()->Attribute("imagename");
  const char* pszSubject = hRoot.ToElement()->Attribute("subject");
  const char* pszMailTo = hRoot.ToElement()->Attribute("mailto");

  if(pszAppName)
    sAppName = pszAppName;

  if(pszImageName)
    sImageName = pszImageName;

  if(pszSubject)
    sSubject = pszSubject;

  if(pszMailTo!=NULL)
    sMailTo = pszMailTo;

  TiXmlHandle hFile = hRoot.ToElement()->FirstChild("file");
  while(hFile.ToElement()!=NULL)
  {
    const char* szFileName = hFile.ToElement()->Attribute("name");
    const char* szFileDesc = hFile.ToElement()->Attribute("description");

    if(szFileName && szFileDesc)
    {      
      file_list[szFileName] = szFileDesc;
    }

    hFile = hFile.ToElement()->NextSibling("file");
  }

  return 0;
}

int 
GetCrashInfoThroughPipe(
  CString& sAppName,
  CString& sImageName,
  CString& sSubject,
  CString& sMailTo,
  std::map<CStringA, CStringA>& file_list)
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
  CStringW data;
  for(;;)
  {
    DWORD dwBytesRead = 0;
    BYTE buffer[1024];
    BOOL bRead = ReadFile(hPipe, buffer, 1024, &dwBytesRead, NULL);
    if(!bRead)
      break;
    data += CStringW((wchar_t*)buffer, dwBytesRead/2);
  }

  // Parse text
  int nParseResult = ParseFileList(CStringA(data), sAppName, sImageName, 
    sSubject, sMailTo, file_list);
  if(nParseResult!=0)
  {
    ATLASSERT(nParseResult==0);
    return 6;
  }

  // Disconnect
  BOOL bDisconnected = DisconnectNamedPipe(hPipe);
  ATLASSERT(bDisconnected);

  CloseHandle(hPipe);
  CloseHandle(overlapped.hEvent); 

  // Success
  return 0;
}

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

  CMainDlg dlgMain;

  if(GetCrashInfoThroughPipe(
    dlgMain.m_sAppName,
    dlgMain.m_sImageName,
    dlgMain.m_sSubject, 
    dlgMain.m_sEmail, 
    dlgMain.m_pUDFiles)!=0)
    return 1;  

	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

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

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
