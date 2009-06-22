#include "stdafx.h"
#include "ProgressDlg.h"

LRESULT CProgressDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{  
  return TRUE;
}

LRESULT CProgressDlg::OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  EndDialog(0);
  return 0;
}

int CProgressDlg::CreateTrayIcon(bool bCreate, HWND hWndParent)
{
  NOTIFYICONDATA nf;
	memset(&nf,0,sizeof(NOTIFYICONDATA));
	nf.cbSize = sizeof(NOTIFYICONDATA);
	nf.hWnd = hWndParent;
	nf.uID = 0;
	
	if(bCreate==true) // add icon to tray
	{
		nf.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nf.uCallbackMessage = WM_TRAYICON;
		nf.uVersion = NOTIFYICON_VERSION;

		nf.hIcon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
	  _tcscpy_s(nf.szTip, 128, _T("Sending Error Report"));
		
		Shell_NotifyIcon(NIM_ADD, &nf);
	}
	else // delete icon
	{
		Shell_NotifyIcon(NIM_DELETE, &nf);
	}
  return 0;
}

void CProgressDlg::Start()
{  
  // center the dialog on the screen
	CenterWindow();

  ShowWindow(SW_SHOW); 
  SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
  FlashWindow(FALSE);

  CreateTrayIcon(true, m_hWnd); 
  SetTimer(0, 3000, NULL);
  
}

LRESULT CProgressDlg::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  AnimateWindow(m_hWnd, 200, AW_HIDE|AW_BLEND); 
  KillTimer(0);
  //ShowWindow(SW_HIDE);

  return 0;
}