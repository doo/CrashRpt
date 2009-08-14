#pragma once
#include "stdafx.h"
#include <map>

class CCrashDescReader
{
public:

  CCrashDescReader();
  ~CCrashDescReader();

  int Load(CString sFileName);

private:

  bool m_bLoaded;

  DWORD m_dwGeneratorVersion;
  
  CString m_sCrashGUID;
  CString m_sAppName;
  CString m_sAppVersion;
  CString m_sImageName;
  CString m_sOperatingSystem;
  CString m_sSystemTimeUTC;
  
  CString m_sExceptionType;
  CString m_sExceptionCode;
  
  CString m_sFPESubcode;
  
  CString m_sInvParamExpression;
  CString m_sInvParamFunction;
  CString m_sInvParamFile;
  CString m_sInvParamLine;

  CString m_sUserEmail;
  CString m_sProblemDescription;

  std::map<CString, CString> m_sFileItems;
};

