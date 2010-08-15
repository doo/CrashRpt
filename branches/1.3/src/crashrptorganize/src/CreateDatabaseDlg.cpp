#include "stdafx.h"
#include "CreateDatabaseDlg.h"

LRESULT CCreateDatabaseDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
  m_editDbName = GetDlgItem(IDC_DBNAME);
  m_editDbName.SetWindowText(_T("CrashReports"));

  m_chkNewFolder = GetDlgItem(IDC_NEWFOLDER);
  m_chkNewFolder.SetCheck(1);

  CenterWindow(GetParent());
	return TRUE;
}

LRESULT CCreateDatabaseDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  EndDialog(wID);
	return 0;  
}

LRESULT CCreateDatabaseDlg::OnBrowse(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  CFolderDialog dlg(m_hWnd);
  dlg.DoModal();

  return 0;
}