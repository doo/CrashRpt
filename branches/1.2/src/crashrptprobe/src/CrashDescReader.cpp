#include "stdafx.h"
#include "CrashRpt.h"
#include "CrashDescReader.h"
#include "tinyxml.h"

CCrashDescReader::CCrashDescReader()
{
  m_bLoaded = false;
}

CCrashDescReader::~CCrashDescReader()
{
}

int CCrashDescReader::Load(CString sFileName)
{
  USES_CONVERSION;
  TiXmlDocument doc;
  FILE* f = NULL;

  // Check that the file exists
#if _MSC_VER<1400
  f = _tfopen(sFileName, _T("rt"));
#else
  _tfopen_s(&f, sFileName, _T("rt"));
#endif
  
  if(f==NULL)
    return -1; // File can't be opened

  fclose(f);

  // Open XML document
  LPCSTR szFileName = T2A(sFileName.GetBuffer(0));
  bool bLoaded = doc.LoadFile(szFileName);
  if(!bLoaded)
  {
    return -2; // XML is corrupted
  }
  
  TiXmlHandle hDoc(&doc);
  
  TiXmlHandle hRoot = hDoc.FirstChild("CrashRpt").ToElement();
  if(hRoot.ToElement()==NULL)
    return -3; // Invalid XML structure

  // Get CrashGUID
  TiXmlHandle hCrashGUID = hRoot.ToElement()->FirstChild("CrashGUID");
  if(hCrashGUID.ToElement())
  {
    const char* text = hCrashGUID.FirstChild().Text()->Value();     
    if(text)
      m_sCrashGUID = A2T(text);    
  }

  // Get AppName
  TiXmlHandle hAppName = hRoot.ToElement()->FirstChild("AppName");
  if(hAppName.ToElement())
  {
    const char* text = hAppName.FirstChild().Text()->Value();     
    if(text)
      m_sAppName = A2T(text);    
  }

  // Get AppVersion
  TiXmlHandle hAppVersion = hRoot.ToElement()->FirstChild("AppVersion");
  if(hAppVersion.ToElement())
  {
    const char* text = hAppVersion.FirstChild().Text()->Value();     
    if(text)
      m_sAppVersion = A2T(text);    
  }

  // Get ImageName
  TiXmlHandle hImageName = hRoot.ToElement()->FirstChild("ImageName");
  if(hImageName.ToElement())
  {
    const char* text = hImageName.FirstChild().Text()->Value();     
    if(text)
      m_sImageName = A2T(text);    
  }

  // Get OperatingSystem
  TiXmlHandle hOperatingSystem = hRoot.ToElement()->FirstChild("OperatingSystem");
  if(hOperatingSystem.ToElement())
  {
    const char* text = hOperatingSystem.FirstChild().Text()->Value();     
    if(text)
      m_sOperatingSystem = A2T(text);    
  }

  // Get SystemTimeUTC
  TiXmlHandle hSystemTimeUTC = hRoot.ToElement()->FirstChild("SystemTimeUTC");
  if(hSystemTimeUTC.ToElement())
  {
    const char* text = hSystemTimeUTC.FirstChild().Text()->Value();     
    if(text)
      m_sSystemTimeUTC = A2T(text);    
  }

  // Get ExceptionType
  TiXmlHandle hExceptionType = hRoot.ToElement()->FirstChild("ExceptionType");
  if(hExceptionType.ToElement())
  {
    const char* text = hExceptionType.FirstChild().Text()->Value();     
    if(text)
      m_sExceptionType = A2T(text);    
  }

  DWORD dwExceptionType = _ttol(m_sExceptionType);

  // Get UserEmail
  TiXmlHandle hUserEmail = hRoot.ToElement()->FirstChild("UserEmail");
  if(hUserEmail.ToElement())
  {
    const char* text = hUserEmail.FirstChild().Text()->Value();     
    if(text)
      m_sUserEmail = A2T(text);    
  }

  // Get ProblemDecription
  TiXmlHandle hProblemDescription = hRoot.ToElement()->FirstChild("ProblemDescription");
  if(hProblemDescription.ToElement())
  {
    const char* text = hProblemDescription.FirstChild().Text()->Value();     
    if(text)
      m_sProblemDescription = A2T(text);    
  }

  // Get ExceptionCode (for structured exceptions only)
  if(dwExceptionType==CR_WIN32_STRUCTURED_EXCEPTION)
  {    
    TiXmlHandle hExceptionCode = hRoot.ToElement()->FirstChild("ExceptionCode");
    if(hExceptionCode.ToElement())
    {
      const char* text = hExceptionCode.FirstChild().Text()->Value();     
      if(text)
        m_sExceptionCode = A2T(text);    
    }
  }

  // Get FPESubcode (for FPE exceptions only)
  if(dwExceptionType==CR_CPP_SIGFPE)
  {    
    TiXmlHandle hFPESubcode = hRoot.ToElement()->FirstChild("FPESubcode");
    if(hFPESubcode.ToElement())
    {
      const char* text = hFPESubcode.FirstChild().Text()->Value();     
      if(text)
        m_sFPESubcode = A2T(text);    
    }
  }

  // Get InvParamExpression, InvParamFunction, InvParamFile, InvParamLine 
  // (for invalid parameter exceptions only)
  if(dwExceptionType==CR_CPP_INVALID_PARAMETER)
  {    
    TiXmlHandle hInvParamExpression = hRoot.ToElement()->FirstChild("InvParamExpression");
    if(hInvParamExpression.ToElement())
    {
      const char* text = hInvParamExpression.FirstChild().Text()->Value();     
      if(text)
        m_sInvParamExpression = A2T(text);    
    }

    TiXmlHandle hInvParamFunction = hRoot.ToElement()->FirstChild("InvParamFunction");
    if(hInvParamFunction.ToElement())
    {
      const char* text = hInvParamFunction.FirstChild().Text()->Value();     
      if(text)
        m_sInvParamFunction = A2T(text);    
    }

    TiXmlHandle hInvParamFile = hRoot.ToElement()->FirstChild("InvParamFile");
    if(hInvParamFile.ToElement())
    {
      const char* text = hInvParamFile.FirstChild().Text()->Value();     
      if(text)
        m_sInvParamFile = A2T(text);    
    }

    TiXmlHandle hInvParamLine = hRoot.ToElement()->FirstChild("InvParamLine");
    if(hInvParamLine.ToElement())
    {
      const char* text = hInvParamLine.FirstChild().Text()->Value();     
      if(text)
        m_sInvParamLine = A2T(text);    
    }

  }

  // OK
  m_bLoaded = true;
  return 0;
}

