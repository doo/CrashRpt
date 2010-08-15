#include "stdafx.h"
#include "ErrorReport.h"

CErrorReport::CErrorReport()
{
  m_hReport = 0;
}

CErrorReport::~CErrorReport()
{
  Close();
}

BOOL CErrorReport::Open(CString sFileName, CString sMD5Name, CString sSymSearchDirs)
{
  CrpHandle hReport = 0;
  TCHAR szErrorMsg[512] = _T("");

  int nResult = crpOpenErrorReport(sFileName, NULL, m_sSymSearchDirs, 0, &hReport);
  if(nResult!=0)
  {
    crpGetLastErrorMsg(szErrorMsg, 512);
    return FALSE;
  }

  nResult = crpCloseErrorReport(hReport);
  if(nResult!=0)
  {
    crpGetLastErrorMsg(szErrorMsg, 512);
    return FALSE;
  }

  return TRUE;
}

BOOL CErrorReport::Close()
{
  TCHAR szErrorMsg[512] = _T("");
  if(m_hReport!=0)
  {
    int nResult = crpCloseErrorReport(m_hReport);
    if(nResult!=0)
    {
      crpGetLastErrorMsg(szErrorMsg, 512);
      return FALSE;
    }
  }

  return TRUE;
}