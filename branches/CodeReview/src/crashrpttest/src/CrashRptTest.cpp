// CrashRptTest.cpp : main source file for CrashRptTest.exe
//

#include "stdafx.h"

#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "CrashThread.h"
#include <atlstr.h>

CAppModule _Module;

LPVOID g_pCrashRptState = NULL;
HANDLE g_hWorkingThread = NULL;
CrashThreadInfo g_CrashThreadInfo;

BOOL WINAPI CrashCallback(LPVOID lpvState)
{
   AddFile(lpvState, _T("dummy.log"), _T("Dummy Log File"));
   AddFile(lpvState, _T("dummy.ini"), _T("Dummy INI File"));

   return TRUE;
}

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainDlg dlgMain;

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

  // Install crash reporting
#ifdef TEST_DEPRECATED_FUNCS

  g_pCrashRptState = Install(
    CrashCallback, 
    _T("zexspectrum_1980@mail.ru"), 
    _T("Crash"));

#else

  CString szSubject;
  szSubject.Format(_T("%s %s Error Report"), APP_NAME, APP_VERSION);

  CRASHRPT_INFO info;
  memset(&info, 0, sizeof(CRASHRPT_INFO));
  info.cb = sizeof(CRASHRPT_INFO);  
  info.pszAppName = APP_NAME;
  info.pszAppVersion = APP_VERSION;
  info.pszEmailSubject = szSubject;
  info.pszEmailTo = _T("zexspectrum_1980@mail.ru");
  info.pfnCrashCallback = CrashCallback;  

  g_pCrashRptState = NULL;
  int nInstResult = crInstall(&info, &g_pCrashRptState);
  ATLASSERT(nInstResult==0 && g_pCrashRptState!=NULL);

#endif //TEST_DEPRECATED_FUNCS

  /* Create another thread */
  g_CrashThreadInfo.m_pCrashRptState = g_pCrashRptState;
  g_CrashThreadInfo.m_hWakeUpEvent = CreateEvent(NULL, FALSE, FALSE, _T("WakeUpEvent"));
  ATLASSERT(g_CrashThreadInfo.m_hWakeUpEvent!=NULL);

  DWORD dwThreadId = 0;
  g_hWorkingThread = CreateThread(NULL, 0, CrashThread, (LPVOID)&g_CrashThreadInfo, 0, &dwThreadId);
  ATLASSERT(g_hWorkingThread!=NULL);

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();

  // Uninstall crash reporting
  Uninstall(g_pCrashRptState);

#ifdef TEST_DEPRECATED_FUNCS

  Uninstall(g_pCrashRptState);

#else
  
  int nUninstResult = crUninstall(g_pCrashRptState);
  ATLASSERT(nUninstResult==0);

#endif //TEST_DEPRECATED_FUNCS

	::CoUninitialize();

	return nRet;
}
