#include "stdafx.h"
#include "CrashRpt.h"
#include "CrashDescReader.h"
#include "tinyxml.h"
#include "Utility.h"

CCrashDescReader::CCrashDescReader()
{
  m_bLoaded = false;
}

CCrashDescReader::~CCrashDescReader()
{
}

int CCrashDescReader::Load(CString sFileName)
{
  TiXmlDocument doc;
  FILE* f = NULL;
  strconv_t strconv;

  if(m_bLoaded)
    return 1; // already loaded

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
  LPCSTR szFileName = strconv.t2a(sFileName.GetBuffer(0));
  bool bLoaded = doc.LoadFile(szFileName);
  if(!bLoaded)
  {
    return -2; // XML is corrupted
  }
  
  TiXmlHandle hDoc(&doc);
  
  TiXmlHandle hRoot = hDoc.FirstChild("CrashRpt").ToElement();
  if(hRoot.ToElement()==NULL)
  {
    if(LoadXmlv10(hDoc)==0)
    {
      return 0;
    }  

    return -3; // Invalid XML structure
  }

  // Get generator version

  const char* szCrashRptVersion = hRoot.ToElement()->Attribute("version");
  if(szCrashRptVersion!=NULL)
  {
    m_dwGeneratorVersion = atoi(szCrashRptVersion);
  }

  // Get CrashGUID
  TiXmlHandle hCrashGUID = hRoot.ToElement()->FirstChild("CrashGUID");
  if(hCrashGUID.ToElement())
  {
    const char* text = hCrashGUID.FirstChild().Text()->Value();     
    if(text)
      m_sCrashGUID = strconv.a2t(text);    
  }

  // Get AppName
  TiXmlHandle hAppName = hRoot.ToElement()->FirstChild("AppName");
  if(hAppName.ToElement())
  {
    const char* text = hAppName.FirstChild().Text()->Value();     
    if(text)
      m_sAppName = strconv.a2t(text);    
  }

  // Get AppVersion
  TiXmlHandle hAppVersion = hRoot.ToElement()->FirstChild("AppVersion");
  if(hAppVersion.ToElement())
  {
    const char* text = hAppVersion.FirstChild().Text()->Value();     
    if(text)
      m_sAppVersion = strconv.a2t(text);    
  }

  // Get ImageName
  TiXmlHandle hImageName = hRoot.ToElement()->FirstChild("ImageName");
  if(hImageName.ToElement())
  {
    const char* text = hImageName.FirstChild().Text()->Value();     
    if(text)
      m_sImageName = strconv.a2t(text);    
  }

  // Get OperatingSystem
  TiXmlHandle hOperatingSystem = hRoot.ToElement()->FirstChild("OperatingSystem");
  if(hOperatingSystem.ToElement())
  {
    const char* text = hOperatingSystem.FirstChild().Text()->Value();     
    if(text)
      m_sOperatingSystem = strconv.a2t(text);    
  }

  // Get SystemTimeUTC
  TiXmlHandle hSystemTimeUTC = hRoot.ToElement()->FirstChild("SystemTimeUTC");
  if(hSystemTimeUTC.ToElement())
  {
    const char* text = hSystemTimeUTC.FirstChild().Text()->Value();     
    if(text)
      m_sSystemTimeUTC = strconv.a2t(text);    
  }

  // Get ExceptionType
  TiXmlHandle hExceptionType = hRoot.ToElement()->FirstChild("ExceptionType");
  if(hExceptionType.ToElement())
  {
    const char* text = hExceptionType.FirstChild().Text()->Value();     
    if(text)
      m_dwExceptionType = atoi(text);    
  }

  // Get UserEmail
  TiXmlHandle hUserEmail = hRoot.ToElement()->FirstChild("UserEmail");
  if(hUserEmail.ToElement())
  {
    const char* text = hUserEmail.FirstChild().Text()->Value();     
    if(text)
      m_sUserEmail = strconv.a2t(text);    
  }

  // Get ProblemDecription
  TiXmlHandle hProblemDescription = hRoot.ToElement()->FirstChild("ProblemDescription");
  if(hProblemDescription.ToElement())
  {
    const char* text = hProblemDescription.FirstChild().Text()->Value();     
    if(text)
      m_sProblemDescription = strconv.a2t(text);    
  }

  // Get ExceptionCode (for structured exceptions only)
  if(m_dwExceptionType==CR_WIN32_STRUCTURED_EXCEPTION)
  {    
    TiXmlHandle hExceptionCode = hRoot.ToElement()->FirstChild("ExceptionCode");
    if(hExceptionCode.ToElement())
    {
      const char* text = hExceptionCode.FirstChild().Text()->Value();     
      if(text)
        m_dwExceptionCode = atoi(text);    
    }
  }

  // Get FPESubcode (for FPE exceptions only)
  if(m_dwExceptionType==CR_CPP_SIGFPE)
  {    
    TiXmlHandle hFPESubcode = hRoot.ToElement()->FirstChild("FPESubcode");
    if(hFPESubcode.ToElement())
    {
      const char* text = hFPESubcode.FirstChild().Text()->Value();     
      if(text)
        m_dwFPESubcode = atoi(text);    
    }
  }

  // Get InvParamExpression, InvParamFunction, InvParamFile, InvParamLine 
  // (for invalid parameter exceptions only)
  if(m_dwExceptionType==CR_CPP_INVALID_PARAMETER)
  {    
    TiXmlHandle hInvParamExpression = hRoot.ToElement()->FirstChild("InvParamExpression");
    if(hInvParamExpression.ToElement())
    {
      const char* text = hInvParamExpression.FirstChild().Text()->Value();     
      if(text)
        m_sInvParamExpression = strconv.a2t(text);    
    }

    TiXmlHandle hInvParamFunction = hRoot.ToElement()->FirstChild("InvParamFunction");
    if(hInvParamFunction.ToElement())
    {
      const char* text = hInvParamFunction.FirstChild().Text()->Value();     
      if(text)
        m_sInvParamFunction = strconv.a2t(text);    
    }

    TiXmlHandle hInvParamFile = hRoot.ToElement()->FirstChild("InvParamFile");
    if(hInvParamFile.ToElement())
    {
      const char* text = hInvParamFile.FirstChild().Text()->Value();     
      if(text)
        m_sInvParamFile = strconv.a2t(text);    
    }

    TiXmlHandle hInvParamLine = hRoot.ToElement()->FirstChild("InvParamLine");
    if(hInvParamLine.ToElement())
    {
      const char* text = hInvParamLine.FirstChild().Text()->Value();     
      if(text)
        m_dwInvParamLine = atoi(text);    
    }

  }

  // Get file items list
  TiXmlHandle hFileList = hRoot.ToElement()->FirstChild("FileList");
  if(hFileList.ToElement())
  {
    TiXmlHandle hFileItem = hFileList.ToElement()->FirstChild("FileItem");
    while(hFileItem.ToElement())
    {
      const char* szFileName = hFileItem.ToElement()->Attribute("name");
      const char* szFileDescription = hFileItem.ToElement()->Attribute("description");
      
      CString sFileName, sFileDescription;
      if(szFileName!=NULL)
        sFileName = strconv.a2t(szFileName);    
      if(szFileName!=NULL)
        sFileDescription = strconv.a2t(szFileDescription);    
        
      m_aFileItems[sFileName]=sFileDescription;

      hFileItem = hFileItem.ToElement()->NextSibling();
    }
  }


  // OK
  m_bLoaded = true;
  return 0;
}

int CCrashDescReader::LoadXmlv10(TiXmlHandle hDoc)
{
  TiXmlHandle hRoot = hDoc.FirstChild("Exception").ToElement();
  if(hRoot.ToElement()==NULL)
  {
    return -3; // Invalid XML structure
  }

  // Set CrashRpt version to 1000

  m_dwGeneratorVersion = 1000;

  // Get ExceptionRecord element

  TiXmlHandle hExceptionRecord = hRoot.FirstChild("ExceptionRecord").ToElement();

  if(hExceptionRecord.ToElement()!=NULL)
  {
    const char* szImageName = hRoot.ToElement()->Attribute("ModuleName");
    if(szImageName!=NULL)
    {
      m_sImageName = szImageName;
    }
  }  

  // OK
  m_bLoaded = true;
  return 0;
}
