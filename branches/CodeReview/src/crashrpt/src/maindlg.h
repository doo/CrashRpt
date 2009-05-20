///////////////////////////////////////////////////////////////////////////////
//
//  Module: maindlg.h
//
//    Desc: Main crash report dialog, responsible for gathering additional
//          user information and allowing user to examine crash report.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAINDLG_H_
#define _MAINDLG_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Utility.h"
#include "DeadLink.h"
#include "detaildlg.h"
#include "aboutdlg.h"

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


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CMainDlg
// 
// See the module comment at top of file.
//
class CMainDlg : public CDialogImpl<CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

   CString     m_sEmail;         // Email: From
   CString     m_sDescription;   // Email: Body
   CDeadLink   m_link;           // Dead link
   TStrStrMap  *m_pUDFiles;      // Files <name,desc>

   CStatic m_statIcon;
   CHyperLink m_linkMoreInfo;
   CStatic m_statEmail;
   CEdit m_editEmail;
   CStatic m_statDesc;
   CEdit m_editDesc;
   CButton m_btnOk;
   CButton m_btnCancel;
   CStatic m_statHorzLine;
   CStatic m_statCrashRpt; 
   int m_nDeltaY;
   
	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    COMMAND_ID_HANDLER(IDC_LINK, OnLinkClick)
    COMMAND_ID_HANDLER(IDC_MOREINFO, OnMoreInfoClick)
    MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
    MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColorStatic)
		COMMAND_ID_HANDLER(IDOK, OnSend)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

   
   //-----------------------------------------------------------------------------
   // CMainDlg
   //
   // Loads RichEditCtrl library
   //
   CMainDlg() 
   {
      LoadLibrary(CRichEditCtrl::GetLibraryName());
   };

	
   //-----------------------------------------------------------------------------
   // OnInitDialog
   //
   // 
   //
   LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		// center the dialog on the screen
		CenterWindow();

	   // Add "About..." menu item to system menu.

	   // IDM_ABOUTBOX must be in the system command range.
      ATLASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX); 
      ATLASSERT(IDM_ABOUTBOX < 0xF000); 

      CMenu sysMenu;
      sysMenu.Attach(GetSystemMenu(FALSE));
      if (sysMenu.IsMenu())
      {
		   CString strAboutMenu;
		   strAboutMenu.LoadString(IDS_ABOUTBOX);
		   if (!strAboutMenu.IsEmpty())
		   {
            sysMenu.AppendMenu(MF_SEPARATOR);
			   sysMenu.AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		   }
	   }
      //
      // Set title using app name
      //
      SetWindowText(CUtility::getAppName());

      //
      // Use app icon
      //
      m_statIcon = GetDlgItem(IDI_APPICON);
      
      HICON hIcon = NULL;
      // Set window icon (use IDR_MAINFRAME icon which is the default one for the application)
      // Try to load IDR_MAINFRAME icon
      hIcon = ::LoadIcon((HINSTANCE)GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINFRAME));

      // If there is no IDR_MAINFRAME icon, use IDI_APPLICATION system icon
      if(hIcon==NULL)
        hIcon = ::LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));

      m_statIcon.SetIcon(hIcon);                  
      //SetIcon(NULL, FALSE);

      //
      // Set failure heading
      //
      EDITSTREAM es;
      es.pfnCallback = LoadRTFString;

      CString sText;
      sText.Format(IDS_HEADER, CUtility::getAppName());
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

      return TRUE;
	}

  void ShowMoreInfo(BOOL bShow)
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
	
   //-----------------------------------------------------------------------------
   // OnLinkClick
   //
   // Display details dialog instead of opening URL
   //
   LRESULT OnLinkClick(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
      CDetailDlg dlg;
      dlg.m_pUDFiles = m_pUDFiles;
      dlg.DoModal();
      return 0;
   }

   LRESULT OnMoreInfoClick(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
   {
     m_linkMoreInfo.EnableWindow(0);
     ShowMoreInfo(TRUE);
     return 0;
   }

   //-----------------------------------------------------------------------------
   // OnSysCommand
   //
   // 
   //
   LRESULT OnSysCommand(UINT, WPARAM wParam, LPARAM , BOOL& bHandled)
   {
      bHandled = FALSE;

      if ((wParam & 0xFFF0) == IDM_ABOUTBOX)
      {
         CAboutDlg dlg;
         dlg.DoModal();
         bHandled = TRUE;
      }

      return 0;
   }

	
   //-----------------------------------------------------------------------------
   // OnSend
   //
   // Send handler, validates email address entered, if one, and returns.
   //
   LRESULT OnSend(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
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
         MessageBox(szBuf, CUtility::getAppName(), MB_OK);
         // select email
         ::SetFocus(hWndEmail);
      }
      else
      {
         EndDialog(wID);
      }

      return 0;
   }

   //-----------------------------------------------------------------------------
   // OnCancel
   //
   // 
   //
   LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	{
      EndDialog(wID);
		return 0;
  }

  LRESULT OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
  {
    if((HWND)lParam!=m_statIcon)
      return 0;

    HDC hDC = (HDC)wParam;
    //::SelectObject(hDC, GetStockObject(NULL_BRUSH));
    SetBkColor(hDC, RGB(0, 255, 255));
    SetTextColor(hDC, RGB(0, 255, 255));
    return (LRESULT)TRUE;
  }

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif	// #ifndef _MAINDLG_H_
