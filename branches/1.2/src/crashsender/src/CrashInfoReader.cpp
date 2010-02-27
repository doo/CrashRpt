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
#include "tinyxml.h"

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
  else
    m_MinidumpType = MiniDumpNormal;

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
  else
    m_uPriorities[CR_HTTP] = 0;

  TiXmlHandle hSmtpPriority = hRoot.FirstChild("SmtpPriority");
  const char* szSmtpPriority = hSmtpPriority.FirstChild().ToText()->Value();
  if(szSmtpPriority!=NULL)
    m_uPriorities[CR_SMTP] = atoi(szSmtpPriority);
  else
    m_uPriorities[CR_SMTP] = 0;

  TiXmlHandle hMapiPriority = hRoot.FirstChild("MapiPriority");
  const char* szMapiPriority = hMapiPriority.FirstChild().ToText()->Value();
  if(szMapiPriority!=NULL)
    m_uPriorities[CR_SMAPI] = atoi(szMapiPriority);
  else
    m_uPriorities[CR_SMAPI] = 0;

  TiXmlHandle hProcessId = hRoot.FirstChild("ProcessId");
  const char* szProcessId = hProcessId.FirstChild().ToText()->Value();
  if(szProcessId!=NULL)
    m_dwProcessId = strtoul(szProcessId, NULL, 10);
  else
    m_dwProcessId = 0;

  TiXmlHandle hThreadId = hRoot.FirstChild("ThreadId");
  const char* szThreadId = hThreadId.FirstChild().ToText()->Value();
  if(szThreadId!=NULL)
    m_dwThreadId = strtoul(szThreadId, NULL, 10);
  else 
    m_dwThreadId = 0;

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
  else
  {
    m_pExInfo = NULL;
  }

  TiXmlHandle hAddScreenshot = hRoot.FirstChild("AddScreenshot");
  const char* szAddScreenshot = hAddScreenshot.FirstChild().ToText()->Value();
  if(szAddScreenshot!=NULL)
    m_bAddScreenshot = strtol(szAddScreenshot, NULL, 10);
  else
    m_bAddScreenshot = FALSE;

  TiXmlHandle hScreenshotFlags = hRoot.FirstChild("ScreenshotFlags");
  const char* szScreenshotFlags = hScreenshotFlags.FirstChild().ToText()->Value();
  if(szScreenshotFlags!=NULL)
    m_dwScreenshotFlags = strtoul(szScreenshotFlags, NULL, 10);
  else 
    m_dwScreenshotFlags = 0;

  m_rcAppWnd.SetRectEmpty();
  TiXmlHandle hAppWndRect = hRoot.FirstChild("AppWndRect");
  if(hAppWndRect.ToElement()!=NULL)
  {
    const char* szLeft = hScreenshotFlags.ToElement()->Attribute("left");
    const char* szTop = hScreenshotFlags.ToElement()->Attribute("top");
    const char* szRight = hScreenshotFlags.ToElement()->Attribute("right");
    const char* szBottom = hScreenshotFlags.ToElement()->Attribute("bottom");

    if(szLeft && szTop && szRight && szBottom)
    {
      m_rcAppWnd.left = atoi(szLeft);
      m_rcAppWnd.top = atoi(szTop);
      m_rcAppWnd.right = atoi(szRight);
      m_rcAppWnd.bottom = atoi(szBottom);
    }
  }

  TiXmlHandle hMultiPartHttpUploads = hRoot.FirstChild("MultiPartHttpUploads");
  const char* szMultiPartHttpUploads = hMultiPartHttpUploads.FirstChild().ToText()->Value();
  if(szMultiPartHttpUploads!=NULL)
    m_bMultiPartHttpUploads = atoi(szMultiPartHttpUploads);
  else
    m_bMultiPartHttpUploads = FALSE;    

  TiXmlHandle hSilentMode = hRoot.FirstChild("SilentMode");
  const char* szSilentMode = hSilentMode.FirstChild().ToText()->Value();
  if(szSilentMode!=NULL)
    m_bSilentMode = atoi(szSilentMode);
  else
    m_bSilentMode = FALSE;    

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

BOOL CCrashInfoReader::AddUserInfoToCrashDescriptionXML(CString sEmail, CString sDesc)
{ 
  strconv_t strconv;

  TiXmlDocument doc;
  
  CString sFileName = g_CrashInfo.m_sErrorReportDirName + _T("\\crashrpt.xml");
  bool bLoad = doc.LoadFile(strconv.t2a(sFileName.GetBuffer(0)));
  if(!bLoad)
    return FALSE;

  TiXmlNode* root = doc.FirstChild("CrashRpt");
  if(!root)
    return FALSE;

  // Write user e-mail

  TiXmlElement* email = new TiXmlElement("UserEmail");
  root->LinkEndChild(email);

  LPCSTR lpszEmail = strconv.t2a(sEmail.GetBuffer(0));
  TiXmlText* email_text = new TiXmlText(lpszEmail);
  email->LinkEndChild(email_text);              

  // Write problem description

  TiXmlElement* desc = new TiXmlElement("ProblemDescription");
  root->LinkEndChild(desc);

  LPCSTR lpszDesc = strconv.t2a(sDesc.GetBuffer(0));
  TiXmlText* desc_text = new TiXmlText(lpszDesc);
  desc->LinkEndChild(desc_text);              

  bool bSave = doc.SaveFile(); 
  if(!bSave)
    return FALSE;
  return TRUE;
}

BOOL CCrashInfoReader::AddFilesToCrashDescriptionXML(std::vector<FileItem> FilesToAdd)
{
  strconv_t strconv;

  TiXmlDocument doc;
  
  CString sFileName = g_CrashInfo.m_sErrorReportDirName + _T("\\crashrpt.xml");
  bool bLoad = doc.LoadFile(strconv.t2a(sFileName.GetBuffer(0)));
  if(!bLoad)
    return FALSE;

  TiXmlNode* root = doc.FirstChild("CrashRpt");
  if(!root)
    return FALSE;
  
  TiXmlHandle hFileItems = root->FirstChild("FileItems");
  if(hFileItems.ToElement()==NULL)
  {
    hFileItems = new TiXmlElement("FileItems");
    root->LinkEndChild(hFileItems.ToNode());
  }
  
  unsigned i;
  for(i=0; i<FilesToAdd.size(); i++)
  {
    LPCSTR lpszName = strconv.t2a(FilesToAdd[i].m_sDestFile);
    LPCSTR lpszDesc = strconv.t2a(FilesToAdd[i].m_sDesc);
    
    TiXmlHandle hFileItem = new TiXmlElement("FileItem");
    hFileItem.ToElement()->SetAttribute("name", lpszName);
    hFileItem.ToElement()->SetAttribute("descrition", lpszDesc);
    hFileItems.ToElement()->LinkEndChild(hFileItem.ToNode());              

    m_FileItems[FilesToAdd[i].m_sDestFile] = FilesToAdd[i];
  }
  
  bool bSave = doc.SaveFile(); 
  if(!bSave)
    return FALSE;
  return TRUE;
}