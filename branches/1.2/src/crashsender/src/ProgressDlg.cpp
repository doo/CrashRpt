#include "stdafx.h"
#include "ProgressDlg.h"
#include "Utility.h"
#include "CrashSender.h"


LRESULT CProgressDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{   
  // Check if current UI language is an RTL language
  CString sRTL = Utility::GetINIString(_T("Settings"), _T("RTLReading"));
  if(sRTL.CompareNoCase(_T("1"))==0)
  {
    // Mirror this window
    Utility::SetLayoutRTL(m_hWnd);
  }

  HICON hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
  SetIcon(hIcon, FALSE);
  SetIcon(hIcon, TRUE);

  m_statText = GetDlgItem(IDC_TEXT);

  m_prgProgress = GetDlgItem(IDC_PROGRESS);
  m_prgProgress.SetRange(0, 100);
  
  m_listView = GetDlgItem(IDC_LIST); 
  m_listView.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
  m_listView.InsertColumn(0, _T("Status"), LVCFMT_LEFT, 2048);
  
  m_btnCancel = GetDlgItem(IDCANCEL);
  m_btnCancel.SetWindowText(Utility::GetINIString(_T("ProgressDlg"), _T("Cancel")));

  DlgResize_Init();

  return TRUE;
}

LRESULT CProgressDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{    
  if(m_bFinished)
  {
	  HWND hWndParent = ::GetParent(m_hWnd);
	  ::PostMessage(hWndParent, WM_CLOSE, 0, 0);
    return 0;
  }

  AnimateWindow(m_hWnd, 200, AW_HIDE|AW_BLEND); 
  return 0;
}


LRESULT CProgressDlg::OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{ 
  if(m_bFinished)
  {
	  HWND hWndParent = ::GetParent(m_hWnd);
	  ::PostMessage(hWndParent, WM_CLOSE, 0, 0);
    return 0;
  }

  // Start cancelling the worker thread
  CancelSenderThread();  

  // Disable Cancel button
  m_btnCancel.EnableWindow(0);

  return 0;
}

void CProgressDlg::Start(BOOL bCollectInfo)
{ 
  if(bCollectInfo)
  {
    CString sCaption;
    sCaption.Format(Utility::GetINIString(_T("ProgressDlg"), _T("DlgCaption2")), g_CrashInfo.m_sAppName);
    SetWindowText(sCaption);    
  }
  else
  {
    CString sCaption;
    sCaption.Format(Utility::GetINIString(_T("ProgressDlg"), _T("DlgCaption")), g_CrashInfo.m_sAppName);
    SetWindowText(sCaption);    
  }

  SetWindowPos(HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
  // center the dialog on the screen
	CenterWindow();
  ShowWindow(SW_SHOW); 
  SetFocus();    

  if(!bCollectInfo)
  {
    SetTimer(1, 3000); // Hide this dialog in 3 sec.
  }

  SetTimer(0, 200); // Update this dialog each 200 ms.

  m_bFinished = FALSE;
}

LRESULT CProgressDlg::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  WORD wTimerId = (WORD)wParam;

  if(wTimerId==0) // Dialog update timer
  {
    // Get current progress
    int nProgressPct = 0;
    std::vector<CString> messages;
    GetSenderThreadStatus(nProgressPct, messages);
    
    // Update progress bar
    m_prgProgress.SetPos(nProgressPct);

    int attempt = 0; // Sending attempt

    unsigned i;
    for(i=0; i<messages.size(); i++)
    {  
      if(messages[i].CompareNoCase(_T("[collecting_crash_info]"))==0)
      { 
        m_bFinished = TRUE;
        m_statText.SetWindowText(Utility::GetINIString(_T("ProgressDlg"), _T("CollectingCrashInfo")));        
      }
      else if(messages[i].CompareNoCase(_T("[completed_collecting_crash_info]"))==0)
      { 
        m_bFinished = TRUE;
        ShowWindow(SW_HIDE);
        HWND hWndParent = ::GetParent(m_hWnd);        
        ::PostMessage(hWndParent, WM_COMPLETECOLLECT, 0, 0);
      }
      if(messages[i].CompareNoCase(_T("[compressing_files]"))==0)
      {         
        m_statText.SetWindowText(Utility::GetINIString(_T("ProgressDlg"), _T("CompressingFiles")));        
      }
      else if(messages[i].CompareNoCase(_T("[status_success]"))==0)
      { 
        m_bFinished = TRUE;        
        m_statText.SetWindowText(_T("Completed successfuly!"));        
        HWND hWndParent = ::GetParent(m_hWnd);        
        ::PostMessage(hWndParent, WM_CLOSE, 0, 0);
      }
      else if(messages[i].CompareNoCase(_T("[status_failed]"))==0)
      { 
        m_bFinished = TRUE;
        KillTimer(1);
        m_statText.SetWindowText(Utility::GetINIString(_T("ProgressDlg"), _T("CompletedWithErrors")));
                
        m_btnCancel.EnableWindow(1);
        m_btnCancel.SetWindowText(Utility::GetINIString(_T("ProgressDlg"), _T("Close")));
        ShowWindow(SW_SHOW);
      }
      else if(messages[i].CompareNoCase(_T("[cancelled_by_user]"))==0)
      { 
        m_statText.SetWindowText(Utility::GetINIString(_T("ProgressDlg"), _T("Cancelling")));
      }
      else if(messages[i].CompareNoCase(_T("[sending_attempt]"))==0)
      {
        attempt ++;      
        CString str;
        str.Format(Utility::GetINIString(_T("ProgressDlg"), _T("StatusText")), attempt);
        m_statText.SetWindowText(str);
      }
      else if(messages[i].CompareNoCase(_T("[confirm_launch_email_client]"))==0)
      {       
        KillTimer(1);        
        ShowWindow(SW_SHOW);

        DWORD dwFlags = 0;
        CString sRTL = Utility::GetINIString(_T("Settings"), _T("RTLReading"));
        if(sRTL.CompareNoCase(_T("1"))==0)
          dwFlags = MB_RTLREADING;

        CString sMailClientName;        
        CMailMsg::DetectMailClient(sMailClientName);
        CString msg;
        msg.Format(Utility::GetINIString(_T("ProgressDlg"), _T("ConfirmLaunchEmailClient")), sMailClientName);

        INT_PTR result = MessageBox(msg, 
          Utility::GetINIString(_T("ProgressDlg"), _T("DlgCaption")),
          MB_OKCANCEL|MB_ICONQUESTION|dwFlags);

        FeedbackReady(result==IDOK?0:1);       
        ShowWindow(SW_HIDE);
      }

      int count = m_listView.GetItemCount();
      int indx = m_listView.InsertItem(count, messages[i]);
      m_listView.EnsureVisible(indx, TRUE);

    }
  }
  else if(wTimerId==1) // The timer that hides this window
  {
    AnimateWindow(m_hWnd, 200, AW_HIDE|AW_BLEND); 
    KillTimer(1);
  }

  return 0;
}

LRESULT CProgressDlg::OnListRClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{  
  LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE) pnmh;

  POINT pt;
  GetCursorPos(&pt);

  CMenu popup_menu;
  popup_menu.LoadMenu(IDR_POPUPMENU);

  CMenu submenu = popup_menu.GetSubMenu(0);  

  if(lpnmitem->iItem<0)
  {    
    submenu.EnableMenuItem(ID_MENU1_COPYSEL, MF_BYCOMMAND|MF_GRAYED);
  }

  CString sCopySelLines = Utility::GetINIString(_T("ProgressDlg"), _T("CopySelectedLines"));
  CString sCopyWholeLog = Utility::GetINIString(_T("ProgressDlg"), _T("CopyTheWholeLog"));

  MENUITEMINFO mii;
  memset(&mii, 0, sizeof(MENUITEMINFO));
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask = MIIM_STRING;

  mii.dwTypeData = sCopySelLines.GetBuffer(0);
  mii.cch = sCopySelLines.GetLength();
  submenu.SetMenuItemInfo(ID_MENU1_COPYSEL, FALSE, &mii);

  mii.dwTypeData = sCopyWholeLog.GetBuffer(0);
  mii.cch = sCopyWholeLog.GetLength();
  submenu.SetMenuItemInfo(ID_MENU1_COPYLOG, FALSE, &mii);

  submenu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, m_hWnd); 
  return 0;
}

LRESULT CProgressDlg::OnCopySel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CString sData;
  int i;
  for(i=0; i<m_listView.GetItemCount(); i++)
  {
    DWORD dwState = m_listView.GetItemState(i, LVIS_SELECTED);
    if(dwState==0)
      continue;

    TCHAR buf[4096];
    buf[0]=0;
    int n = m_listView.GetItemText(i, 0, buf, 4095);
    sData += CString(buf,n);
    sData += "\r\n";
  }

  SetClipboard(sData);  

  return 0;
}

LRESULT CProgressDlg::OnCopyLog(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CString sData;
  int i;
  for(i=0; i<m_listView.GetItemCount(); i++)
  {
    TCHAR buf[4096];
    buf[0]=0;
    int n = m_listView.GetItemText(i, 0, buf, 4095);
    sData += CString(buf,n);
    sData += "\r\n";
  }

  SetClipboard(sData);  

  return 0;
}

int CProgressDlg::SetClipboard(CString& sData)
{
  if (OpenClipboard())
  {
    EmptyClipboard();
    HGLOBAL hClipboardData;
    DWORD dwSize = (sData.GetLength()+1)*sizeof(TCHAR);
    hClipboardData = GlobalAlloc(GMEM_DDESHARE, dwSize);
    TCHAR* pszData = (TCHAR*)GlobalLock(hClipboardData);
    if(pszData!=NULL)
    {      
      _TCSNCPY_S(pszData, dwSize/sizeof(TCHAR), sData, sData.GetLength()*sizeof(TCHAR));
      GlobalUnlock(hClipboardData);
#ifdef _UNICODE
      SetClipboardData(CF_UNICODETEXT, hClipboardData);
#else
      SetClipboardData(CF_TEXT, hClipboardData);    
#endif
      CloseClipboard();
      return 0;
   }
   CloseClipboard();
  }

  return 1;
}