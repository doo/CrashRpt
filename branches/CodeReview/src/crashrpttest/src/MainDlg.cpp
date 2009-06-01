// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "aboutdlg.h"
#include "MainDlg.h"
#include "CrashThread.h"
#include <assert.h>

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	return TRUE;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	CloseDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnExceptionInMainThread(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  int type = 0;

  switch(wID)
  {
  case IDC_MAIN_NOEXC: return 0;
  case IDC_MAIN_WIN32: type = CR_WIN32_NULL_POINTER_EXCEPTION; break;                              
  case IDC_MAIN_TERM: type = CR_CPP_TERMINATE_CALL; break;                              
  case IDC_MAIN_UNEXP: type = CR_CPP_UNEXPECTED_CALL; break;                              
  case IDC_MAIN_PURECALL: type = CR_CPP_PURE_CALL; break;
  case IDC_MAIN_SECURITY: type = CR_CPP_SECURITY_ERROR; break;
  case IDC_MAIN_INVPAR: type = CR_CPP_INVALID_PARAMETER; break;
  case IDC_MAIN_NEW: type = CR_CPP_NEW_OPERATOR_ERROR; break;
  case IDC_MAIN_SIGABRT: type = CR_CPP_SIGABRT; break;
  case IDC_MAIN_SIGILL: type = CR_CPP_SIGILL; break;
  case IDC_MAIN_SIGINT: type = CR_CPP_SIGINT; break;
  case IDC_MAIN_SIGSEGV: type = CR_CPP_SIGSEGV; break;
  case IDC_MAIN_SIGTERM: type = CR_CPP_SIGTERM; break;
  default: assert(0); break;
  }

  int nResult = crEmulateCrash(type);
  if(nResult!=0)
  {
    MessageBox(_T("Error creating exception situation!"));
  }

  return 0;
}

LRESULT CMainDlg::OnExceptionInWorkingThread(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  int type = 0;

  switch(wID)
  {
  case IDC_THREAD_NOEXC: return 0;
  case IDC_THREAD_WIN32: type = CR_WIN32_NULL_POINTER_EXCEPTION; break;                              
  case IDC_THREAD_TERM: type = CR_CPP_TERMINATE_CALL; break;                              
  case IDC_THREAD_UNEXP: type = CR_CPP_UNEXPECTED_CALL; break;                              
  case IDC_THREAD_PURECALL: type = CR_CPP_PURE_CALL; break;
  case IDC_THREAD_SECURITY: type = CR_CPP_SECURITY_ERROR; break;
  case IDC_THREAD_INVPAR: type = CR_CPP_INVALID_PARAMETER; break;
  case IDC_THREAD_NEW: type = CR_CPP_NEW_OPERATOR_ERROR; break;
  case IDC_THREAD_SIGABRT: type = CR_CPP_SIGABRT; break;
  case IDC_THREAD_SIGILL: type = CR_CPP_SIGILL; break;
  case IDC_THREAD_SIGINT: type = CR_CPP_SIGINT; break;
  case IDC_THREAD_SIGSEGV: type = CR_CPP_SIGSEGV; break;
  case IDC_THREAD_SIGTERM: type = CR_CPP_SIGTERM; break;
  default: assert(0); break;
  }

  extern CrashThreadInfo g_CrashThreadInfo;
  g_CrashThreadInfo.m_ExceptionType = type;
  SetEvent(g_CrashThreadInfo.m_hWakeUpEvent); // wake up the working thread

  return 0;
}