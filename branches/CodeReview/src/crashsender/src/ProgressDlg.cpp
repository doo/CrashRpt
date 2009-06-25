#include "stdafx.h"
#include "ProgressDlg.h"


LRESULT CProgressDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{ 
  DlgResize_Init();

  HICON hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
  SetIcon(hIcon, FALSE);
  SetIcon(hIcon, TRUE);

  m_prgProgress = GetDlgItem(IDC_PROGRESS);
  m_prgProgress.SetRange(0, 100);

  m_listBox = GetDlgItem(IDC_LIST);  

  return TRUE;
}

LRESULT CProgressDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{    
 #if _MSC_VER>=1300
  AnimateWindow(m_hWnd, 200, AW_HIDE|AW_BLEND); 
#else
  ShowWindow(SW_HIDE);
#endif

  return 0;
}


LRESULT CProgressDlg::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{ 
  CancelSenderThread();
  CButton m_btnCancel = GetDlgItem(IDCANCEL);
  m_btnCancel.EnableWindow(0);
  return 0;
}

void CProgressDlg::Start()
{  
  // center the dialog on the screen
	CenterWindow();

  ShowWindow(SW_SHOW); 
  SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
  FlashWindow(FALSE);

  SetTimer(1, 3000);
  SetTimer(0, 200);
}

LRESULT CProgressDlg::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  WORD wTimerId = (WORD)wParam;

  if(wTimerId==0)
  {
    int nProgressPct = 0;
    std::vector<CString> messages;

    GetSenderThreadStatus(nProgressPct, messages);
    
    m_prgProgress.SetPos(nProgressPct);

    unsigned i;
    for(i=0; i<messages.size(); i++)
    {
      int count = m_listBox.GetCount();
      int indx = m_listBox.InsertString(count, messages[i]);
      m_listBox.SetTopIndex(indx);

    }
  }

  if(wTimerId==1)
  {
#if _MSC_VER>=1300
    AnimateWindow(m_hWnd, 200, AW_HIDE|AW_BLEND); 
#else
    ShowWindow(SW_HIDE);
#endif
    KillTimer(1);
  }

  return 0;
}

