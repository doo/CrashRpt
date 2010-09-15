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

  crAddScreenshot(CR_AS_VIRTUAL_SCREEN);
  crAddPropertyW(L"CustomProp", L"Property Value");

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

  // Open not existing file - should fail  
  int nOpenResult5 = crpOpenErrorReportW(L"NotExisting.zip", NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult5!=0);

  __TEST_CLEANUP__;
  
  crpCloseErrorReport(hReport);

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE);
}

REGISTER_TEST(Test_crpOpenErrorReportA);
void Test_crpOpenErrorReportA()
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
  int nOpenResult = crpOpenErrorReportA(NULL, NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult!=0);

  // Open report - should succeed
  LPCSTR szReportName = strconv.t2a(sErrorReportName);
  int nOpenResult2 = crpOpenErrorReportA(szReportName, NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult2==0 && hReport!=0);

  // Close report - should succeed
  int nCloseResult = crpCloseErrorReport(hReport);
  TEST_ASSERT(nCloseResult==0);
  hReport = 0;

  // Calc MD5 hash
  int nMD5 = ers.CalcFileMD5Hash(sErrorReportName, sMD5Hash);  
  TEST_ASSERT(nMD5==0);

  // Open report and check MD5 - should succeed
  LPCSTR szMD5Hash = strconv.t2a(sMD5Hash);
  int nOpenResult3 = crpOpenErrorReportA(szReportName, szMD5Hash, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult3==0 && hReport!=0);

  // Close report - should succeed
  int nCloseResult2 = crpCloseErrorReport(hReport);
  TEST_ASSERT(nCloseResult2==0);
  hReport = 0;

  // Open report with incorrect MD5 - should fail
  LPCSTR szInvalidMD5 = "1234567890123456";
  int nOpenResult4 = crpOpenErrorReportA(szReportName, szInvalidMD5, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult4!=0);

  // Open not existing file - should fail  
  int nOpenResult5 = crpOpenErrorReportW(L"NotExisting.zip", NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult5!=0);

  __TEST_CLEANUP__;
  
  crpCloseErrorReport(hReport);

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE);
}

REGISTER_TEST(Test_crpCloseErrorReport);
void Test_crpCloseErrorReport()
{ 
  CrpHandle hReport = 3;

  // Close invalid report - should fail
  int nCloseResult = crpCloseErrorReport(hReport);
  TEST_ASSERT(nCloseResult!=0);
  
  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crpExtractFileW);
void Test_crpExtractFileW()
{   
  CString sAppDataFolder;  
  CString sTmpFolder;
  CString sErrorReportName;
  strconv_t strconv;
  CrpHandle hReport = 0;
  CErrorReportSender ers;
  CString sMD5Hash;
  const int BUFF_SIZE = 1024;
  WCHAR szBuffer[BUFF_SIZE];

  // Create a temporary folder  
  Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
  sTmpFolder = sAppDataFolder+_T("\\CrashRpt");
  BOOL bCreate = Utility::CreateFolder(sTmpFolder);
  TEST_ASSERT(bCreate);

  // Create error report ZIP
  BOOL bCreateReport = CreateErrorReport(sTmpFolder, sErrorReportName);
  TEST_ASSERT(bCreateReport);
  
  // Open report - should succeed
  LPCWSTR szReportName = strconv.t2w(sErrorReportName);
  int nOpenResult = crpOpenErrorReportW(szReportName, NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult==0 && hReport!=0);

  // Enumerate files contained in error report and extract each one

  int nRowCount = crpGetPropertyW(hReport, CRP_TBL_XMLDESC_FILE_ITEMS, CRP_META_ROW_COUNT, 0, NULL, 0, NULL); 
  TEST_ASSERT(nRowCount>0);

  int i;
  for(i=0; i<nRowCount; i++)
  {
    // Get file name
    int nResult = crpGetPropertyW(hReport, CRP_TBL_XMLDESC_FILE_ITEMS, CRP_COL_FILE_ITEM_NAME, i, szBuffer, BUFF_SIZE, NULL);
    TEST_ASSERT(nResult==0);

    CString sDstFile = sTmpFolder + _T("\\") + CString(szBuffer);
    strconv_t strconv;
    LPCWSTR szDstFile = strconv.t2w(sDstFile);

    // Extract file - should succeed
    int nExtract = crpExtractFileW(hReport, szBuffer, szDstFile, FALSE);
    TEST_ASSERT(nExtract==0);

    // Check that file exists
    DWORD dwAttrs = GetFileAttributesW(szDstFile);
    TEST_ASSERT(dwAttrs!=INVALID_FILE_ATTRIBUTES);

    // Extract file the second time - should fail, because it already exists
    int nExtract2 = crpExtractFileW(hReport, szBuffer, szDstFile, FALSE);
    TEST_ASSERT(nExtract2!=0);

    // Extract file the second time and overwrite existing - should succeed
    int nExtract3 = crpExtractFileW(hReport, szBuffer, szDstFile, TRUE);
    TEST_ASSERT(nExtract3==0);

    // Check that file exists
    DWORD dwAttrs2 = GetFileAttributesW(szDstFile);
    TEST_ASSERT(dwAttrs2!=INVALID_FILE_ATTRIBUTES);

    // Extract file that doesnt exist - should fail
    int nExtract4 = crpExtractFileW(hReport, L"NotExisting.txt", szDstFile, TRUE);
    TEST_ASSERT(nExtract4!=0);
  }   

  __TEST_CLEANUP__;
  
  crpCloseErrorReport(hReport);

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE);  
}

REGISTER_TEST(Test_crpExtractFileA);
void Test_crpExtractFileA()
{ 
  CString sAppDataFolder;  
  CString sTmpFolder;
  CString sErrorReportName;
  strconv_t strconv;
  CrpHandle hReport = 0;
  CErrorReportSender ers;
  CString sMD5Hash;
  const int BUFF_SIZE = 1024;
  char szBuffer[BUFF_SIZE];

  // Create a temporary folder  
  Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
  sTmpFolder = sAppDataFolder+_T("\\CrashRpt");
  BOOL bCreate = Utility::CreateFolder(sTmpFolder);
  TEST_ASSERT(bCreate);

  // Create error report ZIP
  BOOL bCreateReport = CreateErrorReport(sTmpFolder, sErrorReportName);
  TEST_ASSERT(bCreateReport);
  
  // Open report - should succeed
  LPCSTR szReportName = strconv.t2a(sErrorReportName);
  int nOpenResult = crpOpenErrorReportA(szReportName, NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult==0 && hReport!=0);

  // Enumerate files contained in error report and extract each one

  int nRowCount = crpGetPropertyA(hReport, "XmlDescFileItems", "RowCount", 0, NULL, 0, NULL); 
  TEST_ASSERT(nRowCount>0);

  int i;
  for(i=0; i<nRowCount; i++)
  {
    // Get file name
    int nResult = crpGetPropertyA(hReport, "XmlDescFileItems", "FileItemName", i, szBuffer, BUFF_SIZE, NULL);
    TEST_ASSERT(nResult==0);

    CString sDstFile = sTmpFolder + _T("\\") + CString(szBuffer);
    strconv_t strconv;
    LPCSTR szDstFile = strconv.t2a(sDstFile);

    // Extract file - should succeed
    int nExtract = crpExtractFileA(hReport, szBuffer, szDstFile, FALSE);
    TEST_ASSERT(nExtract==0);

    // Check that file exists
    DWORD dwAttrs = GetFileAttributesA(szDstFile);
    TEST_ASSERT(dwAttrs!=INVALID_FILE_ATTRIBUTES);

    // Extract file the second time - should fail, because it already exists
    int nExtract2 = crpExtractFileA(hReport, szBuffer, szDstFile, FALSE);
    TEST_ASSERT(nExtract2!=0);

    // Extract file the second time and overwrite existing - should succeed
    int nExtract3 = crpExtractFileA(hReport, szBuffer, szDstFile, TRUE);
    TEST_ASSERT(nExtract3==0);

    // Check that file exists
    DWORD dwAttrs2 = GetFileAttributesA(szDstFile);
    TEST_ASSERT(dwAttrs2!=INVALID_FILE_ATTRIBUTES);

    // Extract file that doesnt exist - should fail
    int nExtract4 = crpExtractFileA(hReport, "NotExisting.txt", szDstFile, TRUE);
    TEST_ASSERT(nExtract4!=0);
  }   

  __TEST_CLEANUP__;
  
  crpCloseErrorReport(hReport);

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE); 
}

REGISTER_TEST(Test_crpGetLastErrorW);
void Test_crpGetLastErrorW()
{   
  // Get error message
  WCHAR szErrMsg[256] = L"";
  int nResult = crpGetLastErrorMsgW(szErrMsg, 256);
  TEST_ASSERT(nResult>0);

  // Get error message to NULL buffer - must fail  
  int nResult2 = crpGetLastErrorMsgW(NULL, 256);
  TEST_ASSERT(nResult2<0);

  // Get error message to a buffer, but zero length - must fail  
  WCHAR szErrMsg2[256] = L"";
  int nResult3 = crpGetLastErrorMsgW(szErrMsg2, 0);
  TEST_ASSERT(nResult3<0);

  // Get error message to a single-char buffer, must trunkate message and succeed
  WCHAR szErrMsg3[1] = L"";
  int nResult4 = crpGetLastErrorMsgW(szErrMsg3, 1);
  TEST_ASSERT(nResult4>0);

  // Get error message to a small buffer, must trunkate message and succeed
  WCHAR szErrMsg6[2] = L"";
  int nResult6 = crpGetLastErrorMsgW(szErrMsg6, 2);
  TEST_ASSERT(nResult6>0);

  __TEST_CLEANUP__;
  
}

REGISTER_TEST(Test_crpGetLastErrorA);
void Test_crpGetLastErrorA()
{   
  // Get error message
  char szErrMsg[256] = "";
  int nResult = crpGetLastErrorMsgA(szErrMsg, 256);
  TEST_ASSERT(nResult>0);

  // Get error message to NULL buffer - must fail  
  int nResult2 = crpGetLastErrorMsgA(NULL, 256);
  TEST_ASSERT(nResult2<0);

  // Get error message to a buffer, but zero length - must fail  
  char szErrMsg2[256] = "";
  int nResult3 = crpGetLastErrorMsgA(szErrMsg2, 0);
  TEST_ASSERT(nResult3<0);

  // Get error message to a single-char buffer, must trunkate message and succeed
  char szErrMsg3[1] = "";
  int nResult4 = crpGetLastErrorMsgA(szErrMsg3, 1);
  TEST_ASSERT(nResult4>0);

  // Get error message to a small buffer, must trunkate message and succeed
  char szErrMsg6[2] = "";
  int nResult6 = crpGetLastErrorMsgA(szErrMsg6, 2);
  TEST_ASSERT(nResult6>0);

  __TEST_CLEANUP__;
  
}

REGISTER_TEST(Test_crpGetPropertyW);
void Test_crpGetPropertyW()
{   
  CString sAppDataFolder;  
  CString sTmpFolder;
  CString sErrorReportName;
  strconv_t strconv;
  CrpHandle hReport = 0;
  CErrorReportSender ers;
  CString sMD5Hash;
  const int BUFF_SIZE = 1024;
  WCHAR szBuffer[BUFF_SIZE];
  ULONG uCount = 0;

  // Create a temporary folder  
  Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
  sTmpFolder = sAppDataFolder+_T("\\CrashRpt");
  BOOL bCreate = Utility::CreateFolder(sTmpFolder);
  TEST_ASSERT(bCreate);

  // Create error report ZIP
  BOOL bCreateReport = CreateErrorReport(sTmpFolder, sErrorReportName);
  TEST_ASSERT(bCreateReport);
  
  // Get property from unopened report - should fail
  int nResult = crpGetPropertyW(hReport, CRP_TBL_XMLDESC_MISC, CRP_COL_APP_NAME, 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult!=0 && uCount==0);

  // Open report - should succeed
  LPCWSTR szReportName = strconv.t2w(sErrorReportName);
  int nOpenResult = crpOpenErrorReportW(szReportName, NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult==0 && hReport!=0);

  // Get property from opened report - should succeed
  int nResult2 = crpGetPropertyW(hReport, CRP_TBL_XMLDESC_MISC, CRP_COL_APP_NAME, 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult2==0 && uCount>0);

  __TEST_CLEANUP__;
  
  crpCloseErrorReport(hReport);

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE); 
}

REGISTER_TEST(Test_crpGetPropertyA);
void Test_crpGetPropertyA()
{   
  CString sAppDataFolder;  
  CString sTmpFolder;
  CString sErrorReportName;
  strconv_t strconv;
  CrpHandle hReport = 0;
  CErrorReportSender ers;
  CString sMD5Hash;
  const int BUFF_SIZE = 1024;
  char szBuffer[BUFF_SIZE];
  ULONG uCount = 0;

  // Create a temporary folder  
  Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
  sTmpFolder = sAppDataFolder+_T("\\CrashRpt");
  BOOL bCreate = Utility::CreateFolder(sTmpFolder);
  TEST_ASSERT(bCreate);

  // Create error report ZIP
  BOOL bCreateReport = CreateErrorReport(sTmpFolder, sErrorReportName);
  TEST_ASSERT(bCreateReport);
  
  // Get property from unopened report - should fail
  int nResult = crpGetPropertyA(hReport, "XmlDescMisc", "AppName", 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult!=0 && uCount==0);

  // Open report - should succeed
  LPCSTR szReportName = strconv.t2a(sErrorReportName);
  int nOpenResult = crpOpenErrorReportA(szReportName, NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult==0 && hReport!=0);

  // Get property from opened report - should succeed
  int nResult2 = crpGetPropertyA(hReport, "XmlDescMisc", "AppName", 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult2==0 && uCount>0);

  __TEST_CLEANUP__;
  
  crpCloseErrorReport(hReport);

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE); 
}

REGISTER_TEST(Test_crpGetProperty);
void Test_crpGetProperty()
{   
  CString sAppDataFolder;  
  CString sTmpFolder;
  CString sErrorReportName;
  strconv_t strconv;
  CrpHandle hReport = 0;  
  const int BUFF_SIZE = 1024;
  TCHAR szBuffer[BUFF_SIZE];
  ULONG uCount = 0;

  // Create a temporary folder  
  Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
  sTmpFolder = sAppDataFolder+_T("\\CrashRpt");
  BOOL bCreate = Utility::CreateFolder(sTmpFolder);
  TEST_ASSERT(bCreate);

  // Create error report ZIP
  BOOL bCreateReport = CreateErrorReport(sTmpFolder, sErrorReportName);
  TEST_ASSERT(bCreateReport);
    
  // Open report - should succeed
  LPCSTR szReportName = strconv.t2a(sErrorReportName);
  int nOpenResult = crpOpenErrorReportA(szReportName, NULL, NULL, 0, &hReport);
  TEST_ASSERT(nOpenResult==0 && hReport!=0);
  
  // Get row count in CRP_TBL_XMLDESC_MISC table - should return 1 (this table always has single row)
  int nResult = crpGetProperty(hReport, CRP_TBL_XMLDESC_MISC, CRP_META_ROW_COUNT, 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult==1 && uCount==0);

  // Get row count in CRP_TBL_XMLDESC_FILE_ITEMS table - should return >0 
  int nResult2 = crpGetProperty(hReport, CRP_TBL_XMLDESC_FILE_ITEMS, CRP_META_ROW_COUNT, 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult2>1 && uCount==0);

  // Get row count in CRP_TBL_XMLDESC_FILE_ITEMS table - should return >0 
  int nResult3 = crpGetProperty(hReport, CRP_TBL_XMLDESC_FILE_ITEMS, CRP_META_ROW_COUNT, 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult3>1 && uCount==0);

  // Get row count in CRP_TBL_XMLDESC_CUSTOM_PROPS table - should return 1 (added one custom property) 
  int nResult4 = crpGetProperty(hReport, CRP_TBL_XMLDESC_CUSTOM_PROPS, CRP_META_ROW_COUNT, 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult4==1 && uCount==0);

  // Get row count in CRP_TBL_MDMP_MISC table - should return 1 (always has one row) 
  int nResult5 = crpGetProperty(hReport, CRP_TBL_MDMP_MISC, CRP_META_ROW_COUNT, 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult5==1 && uCount==0);
  
  // Get row count in CRP_TBL_MDMP_MODULES table - should return >0 
  int nResult6 = crpGetProperty(hReport, CRP_TBL_MDMP_MODULES, CRP_META_ROW_COUNT, 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult6>0 && uCount==0);

  // Get row count in CRP_TBL_MDMP_THREADS table - should return >0 
  int nResult7 = crpGetProperty(hReport, CRP_TBL_MDMP_THREADS, CRP_META_ROW_COUNT, 
    0, szBuffer, BUFF_SIZE, &uCount);
  TEST_ASSERT(nResult7>0 && uCount==0);

  __TEST_CLEANUP__;
  
  crpCloseErrorReport(hReport);

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE); 
}