#include "stdafx.h"
#include "resource.h"
#include "VideoRecDlg.h"

LRESULT CVideoRecDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{  	
	// Set dialog caption.
    //SetWindowText(pSender->GetLangStr(_T("MainDlg"), _T("DlgCaption")));

    // Center the dialog on the screen.
    CenterWindow();
	   
    return TRUE;
}

LRESULT CVideoRecDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{   
	EndDialog(IDOK);
    return 0;
}

LRESULT CVideoRecDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{   
	EndDialog(IDCANCEL);
    return 0;
}

void CVideoRecDlg::CloseDialog(int nVal)
{
   
	// Destroy window
    DestroyWindow();
}

