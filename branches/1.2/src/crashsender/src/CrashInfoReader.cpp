/************************************************************************************* 
  This file is a part of CrashRpt library.

  CrashRpt is Copyright (c) 2003, Michael Carruth
  All rights reserved.
 
  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:
 
   * Redistributions of source code must retain the above copyright notice, this 
     list of conditions and the following disclaimer.
 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.
 
   * Neither the name of the author nor the names of its contributors 
     may be used to endorse or promote products derived from this software without 
     specific prior written permission.
 

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

// File: CrashInfoReader.cpp
// Description: Retrieves crash information passed from CrashRpt.dll in form of XML files.
// Authors: zexspectrum
// Date: 2010

#include "stdafx.h"
#include "CrashRpt.h"
#include "CrashInfoReader.h"
#include "strconv.h"

// Define global CCrashInfoReader object
CCrashInfoReader g_CrashInfo;

int CCrashInfoReader::Init(CString sCrashInfoFileName)
{ 
  strconv_t strconv;

  TiXmlDocument doc;
  bool bOpen = doc.LoadFile(strconv.t2a(sCrashInfoFileName));
  if(!bOpen)
    return 1;

  TiXmlHandle hRoot = doc.FirstChild("CrashRptInternal");
  if(hRoot.ToElement()==NULL)
    return 1;

  TiXmlHandle hReportFolder = hRoot.FirstChild("ReportFolder");
  const char* szReportFolder = hReportFolder.FirstChild().ToText()->Value();
  if(szReportFolder!=NULL)
    m_sErrorReportDirName = szReportFolder;
  
  TiXmlHandle hCrashGUID = hRoot.FirstChild("CrashGUID");
  const char* szCrashGUID = hCrashGUID.FirstChild().ToText()->Value();
  if(szCrashGUID!=NULL)
    m_sCrashGUID = szCrashGUID;

  TiXmlHandle hDbgHelpPath = hRoot.FirstChild("DbgHelpPath");
  const char* szDbgHelpPath = hDbgHelpPath.FirstChild().ToText()->Value();
  if(szDbgHelpPath!=NULL)
    m_sDbgHelpPath = szDbgHelpPath;

  TiXmlHandle hMinidumpType = hRoot.FirstChild("MinidumpType");
  const char* szMinidumpType = hMinidumpType.FirstChild().ToText()->Value();
  if(szMinidumpType!=NULL)
    m_MinidumpType = (MINIDUMP_TYPE)atol(szMinidumpType);

  TiXmlHandle hUrl = hRoot.FirstChild("Url");
  const char* szUrl = hUrl.FirstChild().ToText()->Value();
  if(szUrl!=NULL)
    m_sUrl = szUrl;

  TiXmlHandle hEmailTo = hRoot.FirstChild("EmailTo");
  const char* szEmailTo = hEmailTo.FirstChild().ToText()->Value();
  if(szEmailTo!=NULL)
    m_sEmailTo = szEmailTo;

  TiXmlHandle hEmailSubject = hRoot.FirstChild("EmailSubject");
  const char* szEmailSubject = hEmailSubject.FirstChild().ToText()->Value();
  if(szEmailSubject!=NULL)
    m_sEmailSubject = szEmailSubject;

  TiXmlHandle hPrivacyPolicyUrl = hRoot.FirstChild("PrivacyPolicyUrl");
  const char* szPrivacyPolicyUrl = hPrivacyPolicyUrl.FirstChild().ToText()->Value();
  if(szPrivacyPolicyUrl!=NULL)
    m_sPrivacyPolicyURL = szPrivacyPolicyUrl;

  TiXmlHandle hHttpPriority = hRoot.FirstChild("HttpPriority");
  const char* szHttpPriority = hHttpPriority.FirstChild().ToText()->Value();
  if(szHttpPriority!=NULL)
    m_uPriorities[CR_HTTP] = atoi(szHttpPriority);

  TiXmlHandle hSmtpPriority = hRoot.FirstChild("SmtpPriority");
  const char* szSmtpPriority = hSmtpPriority.FirstChild().ToText()->Value();
  if(szSmtpPriority!=NULL)
    m_uPriorities[CR_SMTP] = atoi(szSmtpPriority);

  TiXmlHandle hMapiPriority = hRoot.FirstChild("MapiPriority");
  const char* szMapiPriority = hMapiPriority.FirstChild().ToText()->Value();
  if(szMapiPriority!=NULL)
    m_uPriorities[CR_SMAPI] = atoi(szMapiPriority);

  TiXmlHandle hProcessId = hRoot.FirstChild("ProcessId");
  const char* szProcessId = hProcessId.FirstChild().ToText()->Value();
  if(szProcessId!=NULL)
    m_dwProcessId = strtoul(szProcessId, NULL, 10);

  TiXmlHandle hThreadId = hRoot.FirstChild("ThreadId");
  const char* szThreadId = hThreadId.FirstChild().ToText()->Value();
  if(szThreadId!=NULL)
    m_dwThreadId = strtoul(szThreadId, NULL, 10);

  TiXmlHandle hExceptionPointersAddress = hRoot.FirstChild("ExceptionPointersAddress");
  const char* szExceptionPointersAddress = hExceptionPointersAddress.FirstChild().ToText()->Value();
  if(szExceptionPointersAddress!=NULL)
  {
#ifdef _WIN64
    m_pExInfo = _tcstoul(szExceptionPointersAddress, NULL, 10);
#else
    m_pExInfo = (PEXCEPTION_POINTERS)strtoul(szExceptionPointersAddress, NULL, 10);
#endif 
  }

  TiXmlHandle hAddScreenshot = hRoot.FirstChild("AddScreenshot");
  const char* szAddScreenshot = hAddScreenshot.FirstChild().ToText()->Value();
  if(szAddScreenshot!=NULL)
    m_bAddScreenshot = strtol(szAddScreenshot, NULL, 10);

  TiXmlHandle hScreenshotFlags = hRoot.FirstChild("ScreenshotFlags");
  const char* szScreenshotFlags = hScreenshotFlags.FirstChild().ToText()->Value();
  if(szScreenshotFlags!=NULL)
    m_dwScreenshotFlags = strtoul(szScreenshotFlags, NULL, 10);

  ParseCrashDescriptor(m_sErrorReportDirName + _T("\\crashrpt.xml"));

  return ParseFileList(hRoot);
}

int CCrashInfoReader::ParseFileList(TiXmlHandle& hRoot)
{
  strconv_t strconv;
   
  TiXmlHandle fl = hRoot.FirstChild("FileItems");
  if(fl.ToElement()==0)
  {    
    return 1;
  }

  TiXmlHandle fi = fl.FirstChild("FileItem");
  while(fi.ToElement()!=0)
  {
    const char* pszDestFile = fi.ToElement()->Attribute("destfile");
    const char* pszSrcFile = fi.ToElement()->Attribute("srcfile");
    const char* pszDesc = fi.ToElement()->Attribute("description");
    const char* pszMakeCopy = fi.ToElement()->Attribute("makecopy");

    if(pszDestFile!=NULL)
    {
	    CString sDestFile = pszDestFile;      
      FileItem item;
      item.m_sDestFile = sDestFile;
      if(pszSrcFile)
        item.m_sSrcFile = pszSrcFile;
      if(pszDesc)
        item.m_sDesc = pszDesc;

      if(pszMakeCopy)
      {
        if(strcmp(pszMakeCopy, "1")==0)
          item.m_bMakeCopy = TRUE;
        else
          item.m_bMakeCopy = FALSE;
      }
      else
        item.m_bMakeCopy = FALSE;
      
      m_FileItems[sDestFile] = item;
    }

    fi = fi.ToElement()->NextSibling("FileItem");
  }

  return 0;
}

int CCrashInfoReader::ParseCrashDescriptor(CString sFileName)
{
  strconv_t strconv;

  TiXmlDocument doc;
  bool bOpen = doc.LoadFile(strconv.t2a(sFileName));
  if(!bOpen)
    return 1;

  TiXmlHandle hRoot = doc.FirstChild("CrashRpt");
  if(hRoot.ToElement()==NULL)
    return 1;

  TiXmlHandle hAppName = hRoot.FirstChild("AppName");
  const char* szAppName = hAppName.FirstChild().ToText()->Value();
  if(szAppName!=NULL)
    m_sAppName = szAppName;

  TiXmlHandle hAppVersion = hRoot.FirstChild("AppVersion");
  const char* szAppVersion = hAppVersion.FirstChild().ToText()->Value();
  if(szAppVersion!=NULL)
    m_sAppVersion = szAppVersion;

  TiXmlHandle hImageName = hRoot.FirstChild("ImageName");
  const char* szImageName = hAppName.FirstChild().ToText()->Value();
  if(szImageName!=NULL)
    m_sImageName = szImageName;

  return 0;
}


