// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <windows.h>
#include "resource.h"
#include "MainDlg.h"
#include "Utility.h"
#include "tinyxml.h"
#include "zip.h"
#include "unzip.h"
#include "CrashSender.h"

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
  CString sRTL = Utility::GetINIString(_T("Settings"), _T("RTLReading"));
  if(sRTL.CompareNoCase(_T("1"))==0)
  {
    Utility::SetLayoutRTL(m_hWnd);
  }

  SetWindowText(Utility::GetINIString(_T("MainDlg"), _T("DlgCaption")));

	// center the dialog on the screen
	CenterWindow();
	
  // Set window icon
  SetIcon(::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME)), 0);

  // Load heading icon
  HMODULE hExeModule = LoadLibrary(g_CrashInfo.m_sImageName);
  if(hExeModule)
  {
    // Use IDR_MAINFRAME icon which is the default one for the crashed application.
    m_HeadingIcon = ::LoadIcon(hExeModule, MAKEINTRESOURCE(IDR_MAINFRAME));
  }  

  // If there is no IDR_MAINFRAME icon in crashed EXE module, use IDI_APPLICATION system icon
  if(m_HeadingIcon == NULL)
  {
    m_HeadingIcon = ::LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
  }  

  CStatic statSubHeader = GetDlgItem(IDC_SUBHEADER);
  statSubHeader.SetWindowText(Utility::GetINIString(_T("MainDlg"), _T("SubHeaderText")));
 
  m_link.SubclassWindow(GetDlgItem(IDC_LINK));   
  m_link.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON);
  m_link.SetLabel(Utility::GetINIString(_T("MainDlg"), _T("WhatDoesReportContain")));
  
  m_linkMoreInfo.SubclassWindow(GetDlgItem(IDC_MOREINFO));
  m_linkMoreInfo.SetHyperLinkExtendedStyle(HLINK_COMMANDBUTTON);
  m_linkMoreInfo.SetLabel(Utility::GetINIString(_T("MainDlg"), _T("ProvideAdditionalInfo")));
  
  m_statEmail = GetDlgItem(IDC_STATMAIL);
  m_statEmail.SetWindowText(Utility::GetINIString(_T("MainDlg"), _T("YourEmail")));

  m_editEmail = GetDlgItem(IDC_EMAIL);
  
  m_statDesc = GetDlgItem(IDC_DESCRIBE);
  m_statDesc.SetWindowText(Utility::GetINIString(_T("MainDlg"), _T("DescribeProblem")));

  m_editDesc = GetDlgItem(IDC_DESCRIPTION);

  m_statIndent =  GetDlgItem(IDC_INDENT);
  
  m_statConsent = GetDlgItem(IDC_CONSENT);

  LOGFONT lf;
  memset(&lf, 0, sizeof(LOGFONT));
  lf.lfHeight = 11;
  lf.lfWeight = FW_NORMAL;
  lf.lfQuality = ANTIALIASED_QUALITY;
  _TCSCPY_S(lf.lfFaceName, 32, _T("Tahoma"));
  CFontHandle hConsentFont;
  hConsentFont.CreateFontIndirect(&lf);
  m_statConsent.SetFont(hConsentFont);

  if(g_CrashInfo.m_sPrivacyPolicyURL.IsEmpty())
    m_statConsent.SetWindowText(Utility::GetINIString(_T("MainDlg"), _T("MyConsent2")));
  else
    m_statConsent.SetWindowText(Utility::GetINIString(_T("MainDlg"), _T("MyConsent")));

  m_linkPrivacyPolicy.SubclassWindow(GetDlgItem(IDC_PRIVACYPOLICY));
  m_linkPrivacyPolicy.SetHyperLink(g_CrashInfo.m_sPrivacyPolicyURL);
  m_linkPrivacyPolicy.SetLabel(Utility::GetINIString(_T("MainDlg"), _T("PrivacyPolicy")));
  
  BOOL bShowPrivacyPolicy = !g_CrashInfo.m_sPrivacyPolicyURL.IsEmpty();  
  m_linkPrivacyPolicy.ShowWindow(bShowPrivacyPolicy?SW_SHOW:SW_HIDE);
  
  m_statCrashRpt = GetDlgItem(IDC_CRASHRPT);
  m_statHorzLine = GetDlgItem(IDC_HORZLINE);  

  m_btnOk = GetDlgItem(IDOK);
  m_btnOk.SetWindowText(Utility::GetINIString(_T("MainDlg"), _T("SendReport")));

  m_btnCancel = GetDlgItem(IDCANCEL);
  m_btnCancel.SetWindowText(Utility::GetINIString(_T("MainDlg"), _T("CloseTheProgram")));

  CRect rc1, rc2, rc3;
  m_linkMoreInfo.GetWindowRect(&rc1);
  m_statConsent.GetWindowRect(&rc2);
  m_nDeltaY = rc2.top-rc1.bottom;
  m_statHorzLine.GetWindowRect(&rc3);
  m_nDeltaY2 = rc3.top-rc2.bottom;
  
  memset(&lf, 0, sizeof(LOGFONT));
  lf.lfHeight = 25;
  lf.lfWeight = FW_NORMAL;
  lf.lfQuality = ANTIALIASED_QUALITY;
  _TCSCPY_S(lf.lfFaceName, 32, _T("Tahoma"));
  m_HeadingFont.CreateFontIndirect(&lf);

  ShowMoreInfo(FALSE);

  m_dlgProgress.Create(m_hWnd);
  m_dlgProgress.Start(TRUE);

  DWORD dwThreadId = 0;
  m_hSenderThread = CreateThread(NULL, 0, CollectorThread, (LPVOID)&m_ctx, 0, &dwThreadId);

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
  m_statIndent.ShowWindow(bShow?SW_SHOW:SW_HIDE);
  
  int k = bShow?1:-1;

  m_statConsent.GetWindowRect(&rc1);
  ::MapWindowPoints(0, m_hWnd, (LPPOINT)&rc1, 2);
  rc1.OffsetRect(0, k*m_nDeltaY);
  m_statConsent.MoveWindow(&rc1);

  m_linkPrivacyPolicy.GetWindowRect(&rc1);
  ::MapWindowPoints(0, m_hWnd, (LPPOINT)&rc1, 2);
  rc1.OffsetRect(0, k*m_nDeltaY);
  m_linkPrivacyPolicy.MoveWindow(&rc1);

  int nDeltaY = m_nDeltaY;
  if(!m_linkPrivacyPolicy.IsWindowVisible())
  {
    if(!bShow)
      nDeltaY = m_nDeltaY+m_nDeltaY2;
    else
      nDeltaY = m_nDeltaY-m_nDeltaY2;
  }

  m_statHorzLine.GetWindowRect(&rc1);
  ::MapWindowPoints(0, m_hWnd, (LPPOINT)&rc1, 2);
  rc1.OffsetRect(0, k*nDeltaY);
  m_statHorzLine.MoveWindow(&rc1);

  m_statCrashRpt.GetWindowRect(&rc1);
  ::MapWindowPoints(0, m_hWnd, (LPPOINT)&rc1, 2);
  rc1.OffsetRect(0, k*nDeltaY);
  m_statCrashRpt.MoveWindow(&rc1);

  m_btnOk.GetWindowRect(&rc1);
  ::MapWindowPoints(0, m_hWnd, (LPPOINT)&rc1, 2);
  rc1.OffsetRect(0, k*nDeltaY);
  m_btnOk.MoveWindow(&rc1);

  m_btnCancel.GetWindowRect(&rc1);
  ::MapWindowPoints(0, m_hWnd, (LPPOINT)&rc1, 2);
  rc1.OffsetRect(0, k*nDeltaY);
  m_btnCancel.MoveWindow(&rc1);

  GetClientRect(&rc1);
  rc1.bottom += k*nDeltaY;
  ResizeClient(rc1.Width(), rc1.Height());

  if(bShow)
    m_editEmail.SetFocus();
  else
    m_btnOk.SetFocus();
}

LRESULT CMainDlg::OnEraseBkgnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  CDCHandle dc((HDC)wParam);

  RECT rcClient;
  GetClientRect(&rcClient);

  RECT rc;
  CStatic statUpperHorzLine = GetDlgItem(IDC_UPPERHORZ);
  statUpperHorzLine.GetWindowRect(&rc);
  ScreenToClient(&rc);

  COLORREF cr = GetSysColor(COLOR_3DFACE);
  CBrush brush;
  brush.CreateSolidBrush(cr);  

  RECT rcHeading = {0, 0, rcClient.right, rc.bottom};
  dc.FillRect(&rcHeading, (HBRUSH)GetStockObject(WHITE_BRUSH));

  RECT rcBody = {0, rc.bottom, rcClient.right, rcClient.bottom};
  dc.FillRect(&rcBody, brush);

  rcHeading.left = 60;
  rcHeading.right -= 10;

  CString sHeading;
  sHeading.Format(Utility::GetINIString(_T("MainDlg"), _T("HeaderText")), g_CrashInfo.m_sAppName);
  dc.SelectFont(m_HeadingFont);
  dc.DrawTextEx(sHeading.GetBuffer(0), sHeading.GetLength(), &rcHeading, 
    DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);  

  if(m_HeadingIcon)
  {
    ICONINFO ii;
    m_HeadingIcon.GetIconInfo(&ii);
    dc.DrawIcon(16, rcHeading.bottom/2 - ii.yHotspot, m_HeadingIcon);
  }

  return TRUE;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);  
	return 0;
}

LRESULT CMainDlg::OnCompleteCollectCrashInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  ShowWindow(SW_SHOW);
  return 0;
}

LRESULT CMainDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  CreateTrayIcon(FALSE, m_hWnd);
  CloseDialog(0);  
  return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}

LRESULT CMainDlg::OnLinkClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{  
  CDetailDlg dlg;
  dlg.DoModal();
  return 0;
}

LRESULT CMainDlg::OnMoreInfoClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
 m_linkMoreInfo.EnableWindow(0);
 ShowMoreInfo(TRUE);
 return 0;
}

LRESULT CMainDlg::OnSend(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{  
  HWND     hWndEmail = GetDlgItem(IDC_EMAIL);
  HWND     hWndDesc = GetDlgItem(IDC_DESCRIPTION);
  int      nEmailLen = ::GetWindowTextLength(hWndEmail);
  int      nDescLen = ::GetWindowTextLength(hWndDesc);

  LPTSTR lpStr = g_CrashInfo.m_sEmailFrom.GetBufferSetLength(nEmailLen+1);
  ::GetWindowText(hWndEmail, lpStr, nEmailLen+1);
  g_CrashInfo.m_sEmailFrom.ReleaseBuffer();

  lpStr = g_CrashInfo.m_sDescription.GetBufferSetLength(nDescLen+1);
  ::GetWindowText(hWndDesc, lpStr, nDescLen+1);
  g_CrashInfo.m_sDescription.ReleaseBuffer();

  //
  // If an email address was entered, verify that
  // it [1] contains a @ and [2] the last . comes
  // after the @.
  //
  if (g_CrashInfo.m_sEmailFrom.GetLength() &&
      (g_CrashInfo.m_sEmailFrom.Find(_T('@')) < 0 ||
       g_CrashInfo.m_sEmailFrom.ReverseFind(_T('.')) < g_CrashInfo.m_sEmailFrom.Find(_T('@'))))
  {
    DWORD dwFlags = 0;
    CString sRTL = Utility::GetINIString(_T("Settings"), _T("RTLReading"));
    if(sRTL.CompareNoCase(_T("1"))==0)
      dwFlags = MB_RTLREADING;
  
     // alert user     
     MessageBox(
       Utility::GetINIString(_T("MainDlg"), _T("InvalidEmailText")), 
       Utility::GetINIString(_T("MainDlg"), _T("InvalidEmailCaption")), 
       MB_OK|dwFlags);
     
     // select email
     ::SetFocus(hWndEmail);

     return 0;
  }

  // Write user email and problem description to XML
  AddUserInfoToCrashDescriptorXML(g_CrashInfo.m_sEmailFrom, g_CrashInfo.m_sDescription);
  
  m_ctx.m_sAppName = g_CrashInfo.m_sAppName;
  m_ctx.m_sAppVersion = g_CrashInfo.m_sAppVersion;
  m_ctx.m_sErrorReportDirName = g_CrashInfo.m_sErrorReportDirName;
  m_ctx.m_sEmailTo = g_CrashInfo.m_sEmailTo;
  m_ctx.m_sEmailFrom = g_CrashInfo.m_sEmailFrom;
  m_ctx.m_sEmailSubject = g_CrashInfo.m_sEmailSubject;
  m_ctx.m_sEmailText = g_CrashInfo.m_sDescription;
  m_ctx.m_sUrl = g_CrashInfo.m_sUrl;
  memcpy(&m_ctx.m_uPriorities, &g_CrashInfo.m_uPriorities, 3*sizeof(UINT));

  DWORD dwThreadId = 0;
  m_hSenderThread = CreateThread(NULL, 0, SenderThread, (LPVOID)&m_ctx, NULL, &dwThreadId);

  ShowWindow(SW_HIDE);
  CreateTrayIcon(true, m_hWnd);
  m_dlgProgress.m_pctx = &m_ctx;
  m_dlgProgress.Start(FALSE);    
  SetTimer(0, 500);
    
  return 0;
}


LRESULT CMainDlg::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  if(WaitForSingleObject(m_hSenderThread, 0)==WAIT_OBJECT_0 )
  {
    KillTimer(0);    
  }
  
  return 0;
}

void CMainDlg::AddUserInfoToCrashDescriptorXML(CString sEmail, CString sDesc)
{ 
  strconv_t strconv;

  TiXmlDocument doc;
  
  CString sFileName = g_CrashInfo.m_sErrorReportDirName + _T("\\crashrpt.xml");
  bool bLoad = doc.LoadFile(strconv.t2a(sFileName.GetBuffer(0)));
  if(!bLoad)
    return;

  TiXmlNode* root = doc.FirstChild("CrashRpt");
  if(!root)
    return;

  // Write user e-mail

  TiXmlElement* email = new TiXmlElement("UserEmail");
  root->LinkEndChild(email);

  LPCSTR lpszEmail = strconv.t2a(sEmail.GetBuffer(0));
  TiXmlText* email_text = new TiXmlText(lpszEmail);
  email->LinkEndChild(email_text);              

  // Write problem description

  TiXmlElement* desc = new TiXmlElement("ProblemDescription");
  root->LinkEndChild(desc);

  LPCSTR lpszDesc = strconv.t2a(sDesc.GetBuffer(0));
  TiXmlText* desc_text = new TiXmlText(lpszDesc);
  desc->LinkEndChild(desc_text);              

  doc.SaveFile();        
}

LRESULT CMainDlg::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  if((HWND)lParam!=m_statIcon)
    return 0;

  HDC hDC = (HDC)wParam;
  SetBkColor(hDC, RGB(0, 255, 255));
  SetTextColor(hDC, RGB(0, 255, 255));
  return (LRESULT)TRUE;
}

int CMainDlg::CreateTrayIcon(bool bCreate, HWND hWndParent)
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
#if _MSC_VER>=1300		
		nf.uVersion = NOTIFYICON_VERSION;
#endif

		nf.hIcon = LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME));
	  _TCSCPY_S(nf.szTip, 128, _T("Sending Error Report"));
		
		Shell_NotifyIcon(NIM_ADD, &nf);
	}
	else // delete icon
	{
		Shell_NotifyIcon(NIM_DELETE, &nf);
	}
  return 0;
}

               
LRESULT CMainDlg::OnTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
  UINT uMouseMsg = (UINT)lParam; 

	if(uMouseMsg==WM_LBUTTONDBLCLK)
	{
	  m_dlgProgress.ShowWindow(SW_SHOW);  	
    m_dlgProgress.SetFocus();
	}	
  return 0;
}