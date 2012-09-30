// File: VideoRecDlg.h
// Description: Video Recording Dialog.
// Authors: zexspectrum
// Date: Sep 2012

#pragma once
#include "stdafx.h"
#include "resource.h"

// class CVideoRecDlg
// Implements video recording notification dialog.
class CVideoRecDlg : 
    public CDialogImpl<CVideoRecDlg>    
{
public:
    enum { IDD = IDD_VIDEOREC };

    BEGIN_MSG_MAP(CVideoRecDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)            
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

    LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	CStatic m_statText;
	CButton m_btnAllow;
	CButton m_btnCancel;
};