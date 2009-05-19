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

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
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
  eExceptionType type = ET_NO_EXCEPTION;

  switch(wID)
  {
  case IDC_MAIN_NOEXC: break;
  case IDC_MAIN_WIN32: type = ET_UNHANDLED_WIN32_EXCEPTION; break;                              
  case IDC_MAIN_TERM: type = ET_CPP_TERMINATE; break;                              
  case IDC_MAIN_UNEXP: type = ET_CPP_UNEXPECTED; break;                              
  case IDC_MAIN_PURECALL: type = ET_CPP_PURECALL; break;
  case IDC_MAIN_SECURITY: type = ET_CPP_SECURITY; break;
  case IDC_MAIN_INVPAR: type = ET_CPP_INVALIDPARAM; break;
  case IDC_MAIN_NEW: type = ET_CPP_NEW; break;
  case IDC_MAIN_SIGABRT: type = ET_CPP_SIGABRT; break;
  case IDC_MAIN_SIGILL: type = ET_CPP_SIGILL; break;
  case IDC_MAIN_SIGINT: type = ET_CPP_SIGINT; break;
  case IDC_MAIN_SIGSEGV: type = ET_CPP_SIGSEGV; break;
  case IDC_MAIN_SIGTERM: type = ET_CPP_SIGTERM; break;
  default: assert(0); break;
  }

  int nResult = GenerateException(type);
  if(nResult!=0)
  {
    MessageBox(_T("Error creating exception situation!"));
  }

  return 0;
}