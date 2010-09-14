/************************************************************************************* 
  This file is a part of CrashRpt library.

  Copyright (c) 2003, Michael Carruth
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

#include "stdafx.h"
#include "Tests.h"
#include "CrashRptProbe.h"
#include "Utility.h"
#include "strconv.h"
#include "CrashHandler.h"
#include "ErrorReportSender.h"
#include "CrashInfoReader.h"

REGISTER_TEST_SUITE( CrashRptProbeAPITests , "CrashRptProbe API function tests");


BOOL CreateErrorReport(CString sTmpFolder, CString& sErrorReportName)
{
  BOOL bStatus = FALSE;
  CErrorReportSender ers;  
  ErrorReportInfo eri;
  CString sReportFolder;
  
  CR_INSTALL_INFOW infoW;
  memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
  infoW.cb = sizeof(CR_INSTALL_INFOW);
  infoW.pszAppName = L"Test";
  infoW.pszAppVersion = L"1.0.0"; 
  infoW.pszErrorReportSaveDir = sTmpFolder;
  infoW.dwFlags = CR_INST_NO_GUI|CR_INST_DONT_SEND_REPORT;
  
  int nInstallResult = crInstallW(&infoW);
  if(nInstallResult!=0)
    goto cleanup;

  CR_EXCEPTION_INFO ei;
  memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
  ei.cb = sizeof(ei);
  ei.exctype = CR_SEH_EXCEPTION;
  ei.code = 0x123;

  int nGenResult = crGenerateErrorReport(&ei);
  if(nGenResult!=0)
    goto cleanup;

  CCrashHandler* pCrashHandler = 
    CCrashHandler::GetCurrentProcessCrashHandler();
  if(pCrashHandler==NULL)
    goto cleanup;

  sReportFolder = pCrashHandler->m_sReportFolderName;

  int nResult = g_CrashInfo.ParseCrashDescription(sReportFolder+_T("\\crashrpt.xml"), TRUE, eri);
  if(nResult!=0)
    goto cleanup;
  
  eri.m_sErrorReportDirName = sReportFolder;

  BOOL bCompress = ers.CompressReportFiles(eri);
  if(bCompress!=TRUE)
    goto cleanup;

  sErrorReportName = sReportFolder + _T(".zip");

  bStatus = TRUE;

cleanup:

  crUninstall();

  return bStatus;
}

REGISTER_TEST(Test_crpOpenErrorReportW);
void Test_crpOpenErrorReportW()
{ 
  CString sAppDataFolder;
  CString sExeFolder;
  CString sTmpFolder;
  CString sErrorReportName;
  strconv_t strconv;
  CrpHandle hReport = 0;
  CErrorReportSender ers;
  CString sMD5Hash;

  // Create a temporary folder  
  Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
  sTmpFolder = sAppDataFolder+_T("\\CrashRpt");
  BOOL bCreate = Utility::CreateFolder(sTmpFolder);
  TEST_ASSERT(bCreate);

  // Create error report ZIP
  BOOL bCreateReport = CreateErrorReport(sTmpFolder, sErrorReportName);
  TEST_ASSERT(bCreateReport);

  // Open NULL report - should fail
  int nOpenResult = crpOpenErrorReportW(NULL, NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult!=0);

  // Open report - should succeed
  LPCWSTR szReportName = strconv.t2w(sErrorReportName);
  int nOpenResult2 = crpOpenErrorReportW(szReportName, NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult2==0 && hReport!=0);

  // Close report - should succeed
  int nCloseResult = crpCloseErrorReport(hReport);
  TEST_ASSERT(nCloseResult==0);
  hReport = 0;

  // Calc MD5 hash
  int nMD5 = ers.CalcFileMD5Hash(sErrorReportName, sMD5Hash);  
  TEST_ASSERT(nMD5==0);

  // Open report and check MD5 - should succeed
  LPCWSTR szMD5Hash = strconv.t2w(sMD5Hash);
  int nOpenResult3 = crpOpenErrorReportW(szReportName, szMD5Hash, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult3==0 && hReport!=0);

  // Close report - should succeed
  int nCloseResult2 = crpCloseErrorReport(hReport);
  TEST_ASSERT(nCloseResult2==0);
  hReport = 0;

  // Open report with incorrect MD5 - should fail
  LPCWSTR szInvalidMD5 = L"1234567890123456";
  int nOpenResult4 = crpOpenErrorReportW(szReportName, szInvalidMD5, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult4!=0);

  __TEST_CLEANUP__;
  
  crpCloseErrorReport(hReport);

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE);
}

REGISTER_TEST(Test_crpOpenErrorReportA);
void Test_crpOpenErrorReportA()
{   
  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crpCloseErrorReport);
void Test_crpCloseErrorReport()
{   
  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crpExtractFileW);
void Test_crpExtractFileW()
{   
  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crpExtractFileA);
void Test_crpExtractFileA()
{   
  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crpGetLastErrorW);
void Test_crpGetLastErrorW()
{   
  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crpGetLastErrorA);
void Test_crpGetLastErrorA()
{   
  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crpGetPropertyW);
void Test_crpGetPropertyW()
{   
  __TEST_CLEANUP__;
}


