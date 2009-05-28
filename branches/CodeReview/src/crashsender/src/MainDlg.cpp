// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"
#include "DeadLink.h"
#include "Utility.h"

//
// RTF load callback
//
DWORD CALLBACK LoadRTFString(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
   CString *sText = (CString*)dwCookie;
   LONG lLen = sText->GetLength();

   for (*pcb = 0; *pcb < cb && *pcb < lLen; (*pcb)++)
   {  
      pbBuff[*pcb] = CStringA(*sText).GetAt(*pcb);
   }

   return 0;
}


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
	/*HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);*/
  
  //
  // Use app icon
  //
  m_statIcon = GetDlgItem(IDI_APPICON);
  
  HICON hIcon = NULL;

  HMODULE hModule = LoadLibrary(m_sImageName);
  if(hModule)
  {
    // Use IDR_MAINFRAME icon which is the default one for the application.
    hIcon = ::LoadIcon(hModule, MAKEINTRESOURCE(IDR_MAINFRAME));
  }  

  // If there is no IDR_MAINFRAME icon, use IDI_APPLICATION system icon
  if(hIcon==NULL)
  {
    hIcon = ::LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
  }

  m_statIcon.SetIcon(hIcon);                  
  SetIcon(::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME)), 0);



  //
  // Set failure heading
  //
  EDITSTREAM es;
  es.pfnCallback = LoadRTFString;

  CString sText;
  sText.Format(IDS_HEADER, m_sAppName);
  es.dwCookie = (DWORD_PTR)&sText;

  CRichEditCtrl re;
  re.Attach(GetDlgItem(IDC_HEADING_TEXT));
  re.StreamIn(SF_RTF, es);
  re.Detach();

  //
  // Hook dead link
  //
  m_link.SubclassWindow(GetDlgItem(IDC_LINK));   
  m_linkMoreInfo.SubclassWindow(GetDlgItem(IDC_MOREINFO));
  m_linkMoreInfo.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON);

  m_statEmail = GetDlgItem(IDC_STATMAIL);
  m_editEmail = GetDlgItem(IDC_EMAIL);
  m_statDesc = GetDlgItem(IDC_DESCRIBE);
  m_editDesc = GetDlgItem(IDC_DESCRIPTION);
  m_statCrashRpt = GetDlgItem(IDC_CRASHRPT);
  m_statHorzLine = GetDlgItem(IDC_HORZLINE);
  m_btnOk = GetDlgItem(IDOK);
  m_btnCancel = GetDlgItem(IDCANCEL);

  CRect rc1, rc2;
  m_linkMoreInfo.GetWindowRect(&rc1);
  m_statHorzLine.GetWindowRect(&rc2);
  m_nDeltaY = rc1.bottom+15-rc2.top;

  ShowMoreInfo(FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	return TRUE;
}

void CMainDlg::ShowMoreInfo(BOOL bShow)
{
    CRect rc1, rc2;

    m_statEmail.ShowWindow(bShow?SW_SHOW:SW_HIDE);
    m_editEmail.ShowWindow(bShow?SW_SHOW:SW_HIDE);
    m_statDesc.ShowWindow(bShow?SW_SHOW:SW_HIDE);
    m_editDesc.ShowWindow(bShow?SW_SHOW:SW_HIDE);
    
    int k = bShow?-1:1;

    m_statHorzLine.GetWindowRect(&rc1);
    ScreenToClient(&rc1);
    rc1.OffsetRect(0, k*m_nDeltaY);
    m_statHorzLine.MoveWindow(&rc1);

    m_statCrashRpt.GetWindowRect(&rc1);
    ScreenToClient(&rc1);
    rc1.OffsetRect(0, k*m_nDeltaY);
    m_statCrashRpt.MoveWindow(&rc1);

    m_btnOk.GetWindowRect(&rc1);
    ScreenToClient(&rc1);
    rc1.OffsetRect(0, k*m_nDeltaY);
    m_btnOk.MoveWindow(&rc1);

    m_btnCancel.GetWindowRect(&rc1);
    ScreenToClient(&rc1);
    rc1.OffsetRect(0, k*m_nDeltaY);
    m_btnCancel.MoveWindow(&rc1);

    GetClientRect(&rc1);
    rc1.bottom += k*m_nDeltaY;
    ResizeClient(rc1.Width(), rc1.Height());
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
  PostQuitMessage(0);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
  PostQuitMessage(0);
	return 0;
}

LRESULT CMainDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  CloseDialog(0);
  PostQuitMessage(0);
  return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnLinkClick(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{  
  CDetailDlg dlg;
  dlg.m_pUDFiles = m_pUDFiles;
  dlg.DoModal();
  return 0;
}

LRESULT CMainDlg::OnMoreInfoClick(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
 m_linkMoreInfo.EnableWindow(0);
 ShowMoreInfo(TRUE);
 return 0;
}

LRESULT CMainDlg::OnSend(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  HWND     hWndEmail = GetDlgItem(IDC_EMAIL);
  HWND     hWndDesc = GetDlgItem(IDC_DESCRIPTION);
  int      nEmailLen = ::GetWindowTextLength(hWndEmail);
  int      nDescLen = ::GetWindowTextLength(hWndDesc);

  LPTSTR lpStr = m_sEmail.GetBufferSetLength(nEmailLen+1);
  ::GetWindowText(hWndEmail, lpStr, nEmailLen+1);
  m_sEmail.ReleaseBuffer();

  lpStr = m_sDescription.GetBufferSetLength(nDescLen+1);
  ::GetWindowText(hWndDesc, lpStr, nDescLen+1);
  m_sDescription.ReleaseBuffer();

  //
  // If an email address was entered, verify that
  // it [1] contains a @ and [2] the last . comes
  // after the @.
  //
  if (m_sEmail.GetLength() &&
      (m_sEmail.Find(_T('@')) < 0 ||
       m_sEmail.ReverseFind(_T('.')) < m_sEmail.Find(_T('@'))))
  {
     // alert user
     TCHAR szBuf[256];
     ::LoadString(_Module.GetResourceInstance(), IDS_INVALID_EMAIL, szBuf, 255);
     MessageBox(szBuf, _T("Invalid E-mail address"), MB_OK);
     
     // select email
     ::SetFocus(hWndEmail);

     return 0;
  }
  

  // Write user email and problem description to XML
  /*CCrashRpt
  rpt.writeUserInfo(szXMLName, mainDlg.m_sEmail, mainDlg.m_sDescription);*/

   // zip the report
   /*if (!zlib.Open(sTempFileName))
      return;*/
   
   // add report files to zip
   //TStrStrMap::iterator cur = m_files.begin();
   //for (i = 0; i < m_files.size(); i++, cur++)
   //{
   //  zlib.AddFile((char*)(LPCSTR)(*cur).first);
   //}

   //zlib.Close();

   //// Send report

   //if (m_sTo.IsEmpty() || 
   //    !MailReport(rpt, sTempFileName, mainDlg.m_sEmail, mainDlg.m_sDescription))
   //{
   //  SaveReport(rpt, sTempFileName);   
   //}

  /*CMailMsg msg;
   msg
      .SetTo(m_sTo)
      .SetFrom(lpcszEmail)
      .SetSubject(m_sSubject.IsEmpty()?_T("Incident Report"):m_sSubject)
      .SetMessage(lpcszDesc)
      .AddAttachment(lpcszFile, CUtility::getAppName() + _T(".zip"));

  DeleteFile(sTempFileName);
  */
  CloseDialog(0);
  return 0;
 }

LRESULT CMainDlg::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  if((HWND)lParam!=m_statIcon)
    return 0;

  HDC hDC = (HDC)wParam;
  //::SelectObject(hDC, GetStockObject(NULL_BRUSH));
  SetBkColor(hDC, RGB(0, 255, 255));
  SetTextColor(hDC, RGB(0, 255, 255));
  return (LRESULT)TRUE;
}