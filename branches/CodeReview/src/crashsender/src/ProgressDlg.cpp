#include "stdafx.h"
#include "ProgressDlg.h"


LRESULT CProgressDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{  
  m_prgProgress = GetDlgItem(IDC_PROGRESS);
  m_prgProgress.SetRange(0, 100);

  m_listView = GetDlgItem(IDC_LIST);
  m_listView.InsertColumn(0, _T("Message"), LVCFMT_LEFT, 300);

  return TRUE;
}

LRESULT CProgressDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{  
  CloseDialog(0);
  return 0;
}


LRESULT CProgressDlg::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{  
  CloseDialog(0);
  return 0;
}

void CProgressDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}


void CProgressDlg::Start()
{  
  // center the dialog on the screen
	CenterWindow();

  ShowWindow(SW_SHOW); 
  SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
  FlashWindow(FALSE);

  SetTimer(0, 3000, NULL);
  SetTimer(1, 200, NULL);
}

LRESULT CProgressDlg::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  WORD wTimerId = wParam;

  if(wTimerId==0)
  {
    int nProgressPct = 0;
    std::vector<CString> messages;

    m_pctx->m_cs.Lock();
    nProgressPct = m_pctx->m_nProgressPct;
    messages = m_pctx->m_Messages;
    m_pctx->m_Messages.clear();
    m_pctx->m_cs.Unlock();

    m_prgProgress.SetPos(nProgressPct);

    unsigned i;
    for(i=0; i<messages.size(); i++)
    {
      m_listView.InsertItem(0, messages[i], 0);
    }
  }

  if(wTimerId==1)
  {
    AnimateWindow(m_hWnd, 200, AW_HIDE|AW_BLEND); 
    KillTimer(1);
  }

  return 0;
}

