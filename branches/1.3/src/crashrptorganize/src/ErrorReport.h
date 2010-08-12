#pragma once
#include "stdafx.h"

class CErrorReport
{
public:

  CErrorReport();
  ~CErrorReport();

  BOOL Open(CString sFileName, CString sMD5Name, CString sSymSearchDirs);
  BOOL Close();

private:

  CString m_sFileName;
  CString m_sMD5Name;
  CString m_sSymSearchDirs;
  CString m_sLastErrorMsg;
  CrpHandle m_hReport;
};