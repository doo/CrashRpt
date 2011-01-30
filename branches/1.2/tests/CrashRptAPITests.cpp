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
#include "CrashRpt.h"
#include "Utility.h"
#include "strconv.h"

REGISTER_TEST_SUITE( CrashRptAPITests , "CrashRpt API function tests");

REGISTER_TEST(Test_InstallW);
void Test_InstallW()
{   
  // Test InstallW 
  
  // Should succeed
  LPVOID pState = InstallW(NULL, L"example@example.com", L"Error report");
  TEST_ASSERT(pState==NULL);

  // Call twice - should fail
  InstallW(NULL, L"example@example.com", L"Error report");
  
  __TEST_CLEANUP__;

  Uninstall(NULL);
}

REGISTER_TEST(Test_InstallA);
void Test_InstallA()
{   
  // Test InstallA 
  
  // Should succeed
  LPVOID pState = InstallA(NULL, "example@example.com", "Error report");
  TEST_ASSERT(pState==NULL);

  // Call twice - should fail
  InstallA(NULL, "example@example.com", "Error report");
  
  __TEST_CLEANUP__;

  Uninstall(NULL);
}

REGISTER_TEST(Test_AddFile);
void Test_AddFile()
{   
  LPVOID pState = InstallW(NULL, L"example@example.com", L"Error report");
  TEST_ASSERT(pState==NULL);

  AddFileW(NULL, L"abc.log", L"Log!");

  AddFileA(NULL, "abc.log", "Log!");
  
  __TEST_CLEANUP__;

  Uninstall(NULL);
}

REGISTER_TEST(Test_crInstall_null);
void Test_crInstall_null()
{   
  // Test crInstall with NULL info - should fail
  
  int nInstallResult = crInstallW(NULL);
  TEST_ASSERT(nInstallResult!=0);

  int nInstallResult2 = crInstallA(NULL);
  TEST_ASSERT(nInstallResult2!=0); 

  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crInstall_wrong_cb);
void Test_crInstall_wrong_cb()
{   
  // Test crInstall with wrong cb parameter - should fail
  
  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.cb = 1000;

  int nInstallResult = crInstall(&info);
  TEST_ASSERT(nInstallResult!=0);

  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crInstall_missing_app_ver);
void Test_crInstall_missing_app_ver()
{   
  // Test crInstall with with missing app version
  // As this console app has missing EXE product version - should fail
  
  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.cb = sizeof(CR_INSTALL_INFO);
  
  int nInstallResult = crInstall(&info);
  TEST_ASSERT(nInstallResult!=0);
  
  __TEST_CLEANUP__;

}

REGISTER_TEST(Test_crInstallW_zero_info);
void Test_crInstallW_zero_info()
{   
  // Test crInstallW with zero info
  
  CR_INSTALL_INFOW infoW;
  memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
  infoW.cb = sizeof(CR_INSTALL_INFOW);
  infoW.pszAppVersion = L"1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallW(&infoW);
  TEST_ASSERT(nInstallResult==0);

  __TEST_CLEANUP__

  crUninstall();  
}

REGISTER_TEST(Test_crInstallA_zero_info);
void Test_crInstallA_zero_info()
{   
  // Test crInstallA with zero info
  
  CR_INSTALL_INFOA infoA;
  memset(&infoA, 0, sizeof(CR_INSTALL_INFOA));
  infoA.cb = sizeof(CR_INSTALL_INFOA);
  infoA.pszAppVersion = "1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallA(&infoA);
  TEST_ASSERT(nInstallResult==0);

  __TEST_CLEANUP__

  crUninstall();  
}

REGISTER_TEST(Test_crInstallA_twice);
void Test_crInstallA_twice()
{   
  // Call crInstallA two times - the second one should fail
  
  CR_INSTALL_INFOA infoA;
  memset(&infoA, 0, sizeof(CR_INSTALL_INFOA));
  infoA.cb = sizeof(CR_INSTALL_INFOA);
  infoA.pszAppVersion = "1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallA(&infoA);
  TEST_ASSERT(nInstallResult==0);

  int nInstallResult2 = crInstallA(&infoA);
  TEST_ASSERT(nInstallResult2!=0);

  __TEST_CLEANUP__

  crUninstall();
  
}

#ifndef CRASHRPT_LIB
// Test the case when CrashRpt.dll and CrashSender.exe are located in
// a different folder (not the same where process executable is located).
// This test also checks that crInstall and crUninstall function names
// are undecorated.
REGISTER_TEST(Test_crInstall_in_different_folder);
void Test_crInstall_in_different_folder()
{ 
  CString sAppDataFolder;
  CString sExeFolder;
  CString sTmpFolder;
  HMODULE hCrashRpt = NULL;

  // Create a temporary folder  
  Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
  sTmpFolder = sAppDataFolder+_T("\\CrashRpt");
  BOOL bCreate = Utility::CreateFolder(sTmpFolder);
  TEST_ASSERT(bCreate);

  // Copy CrashRpt.dll and CrashSender.exe into that folder
  sExeFolder = Utility::GetModulePath(NULL);

#ifdef _DEBUG
  BOOL bCopy = CopyFile(sExeFolder+_T("\\CrashRptd.dll"), sTmpFolder+_T("\\CrashRptd.dll"), TRUE);
  TEST_ASSERT(bCopy);
  BOOL bCopy2 = CopyFile(sExeFolder+_T("\\CrashSenderd.exe"), sTmpFolder+_T("\\CrashSenderd.exe"), TRUE);
  TEST_ASSERT(bCopy2);
#else
  BOOL bCopy = CopyFile(sExeFolder+_T("\\CrashRpt.dll"), sTmpFolder+_T("\\CrashRpt.dll"), TRUE);
  TEST_ASSERT(bCopy);
  BOOL bCopy2 = CopyFile(sExeFolder+_T("\\CrashSender.exe"), sTmpFolder+_T("\\CrashSender.exe"), TRUE);
  TEST_ASSERT(bCopy2);
#endif

  BOOL bCopy3 = CopyFile(sExeFolder+_T("\\crashrpt_lang.ini"), sTmpFolder+_T("\\crashrpt_lang.ini"), TRUE);
  TEST_ASSERT(bCopy3);
  
  // Load CrashRpt.dll dynamically
#ifdef _DEBUG
  hCrashRpt = LoadLibrary(sTmpFolder+_T("\\CrashRptd.dll"));
  TEST_ASSERT(hCrashRpt!=NULL);
#else
  hCrashRpt = LoadLibrary(sTmpFolder+_T("\\CrashRpt.dll"));
  TEST_ASSERT(hCrashRpt!=NULL);
#endif
  

  // Install crash handler
  CR_INSTALL_INFO infoW;
  memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
  infoW.cb = sizeof(CR_INSTALL_INFOW);
  infoW.pszAppName = L"My& app Name & '"; // Use appname with restricted XML characters
  infoW.pszAppVersion = L"1.0.0"; // Specify app version, otherwise it will fail.  

  typedef int (WINAPI *PFNCRINSTALLW)(PCR_INSTALL_INFOW);
  PFNCRINSTALLW pfncrInstallW = (PFNCRINSTALLW)GetProcAddress(hCrashRpt, "crInstallW");
  TEST_ASSERT(pfncrInstallW!=NULL);

  typedef int (WINAPI *PFNCRUNINSTALL)();
  PFNCRUNINSTALL pfncrUninstall = (PFNCRUNINSTALL)GetProcAddress(hCrashRpt, "crUninstall");
  TEST_ASSERT(pfncrUninstall!=NULL);

  // Install should succeed
  int nInstallResult = pfncrInstallW(&infoW);
  TEST_ASSERT(nInstallResult==0);
  
  __TEST_CLEANUP__

  crUninstall();
    
  FreeLibrary(hCrashRpt);

  // Delete temporary folder
  Utility::RecycleFile(sTmpFolder, TRUE);
}
#endif //!CRASHRPT_LIB

REGISTER_TEST(Test_crUninstall);
void Test_crUninstall()
{   
  // Call crUninstall - should fail, because crInstall should be called first
    
  int nUninstallResult = crUninstall();
  TEST_ASSERT(nUninstallResult!=0); 

  // And another time... 
  int nUninstallResult2 = crUninstall();
  TEST_ASSERT(nUninstallResult2!=0); 

  __TEST_CLEANUP__;
}

REGISTER_TEST(Test_crAddFileA);
void Test_crAddFileA()
{ 
  strconv_t strconv;
  CString sFileName;

  // Should fail, because crInstall() should be called first
  int nResult = crAddFileA("a.txt", "invalid file");
  TEST_ASSERT(nResult!=0);

  // Install crash handler
  CR_INSTALL_INFOA infoA;
  memset(&infoA, 0, sizeof(CR_INSTALL_INFOA));
  infoA.cb = sizeof(CR_INSTALL_INFOA);
  infoA.pszAppVersion = "1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallA(&infoA);
  TEST_ASSERT(nInstallResult==0);
  
  // Add not existing file, crAddFileA should fail
  int nResult2 = crAddFileA("a.txt", "invalid file");
  TEST_ASSERT(nResult2!=0);

  if(g_bRunningFromUNICODEFolder==FALSE)
  {
    // Add existing file, crAddFileA should succeed
    
    sFileName = Utility::GetModulePath(NULL)+_T("\\dummy.ini");
    LPCSTR szFileName = strconv.t2a(sFileName);
    int nResult3 = crAddFileA(szFileName, "Dummy INI File");
    TEST_ASSERT(nResult3==0);
  }

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();  
}

REGISTER_TEST(Test_crAddFileW);
void Test_crAddFileW()
{ 
  strconv_t strconv;
  CString sFileName;

  // Should fail, because crInstall() should be called first
  int nResult = crAddFileW(L"a.txt", L"invalid file");
  TEST_ASSERT(nResult!=0);

  // Install crash handler
  CR_INSTALL_INFOW infoW;
  memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
  infoW.cb = sizeof(CR_INSTALL_INFOW);
  infoW.pszAppVersion = L"1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallW(&infoW);
  TEST_ASSERT(nInstallResult==0);
  
  // Add not existing file, crAddFileA should fail
  int nResult2 = crAddFileW(L"a.txt", L"invalid file");
  TEST_ASSERT(nResult2!=0);

  // Add existing file, crAddFileA should succeed
  
  sFileName = Utility::GetModulePath(NULL)+_T("\\dummy.ini");
  LPCWSTR szFileName = strconv.t2w(sFileName);
  int nResult3 = crAddFileW(szFileName, L"Dummy INI File");
  TEST_ASSERT(nResult3==0);

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();  
}

REGISTER_TEST(Test_crAddPropertyA);
void Test_crAddPropertyA()
{   
  // Should fail, because crInstall() should be called first
  int nResult = crAddPropertyA("VideoAdapter", "nVidia GeForce GTS 250");
  TEST_ASSERT(nResult!=0);

  // Install crash handler
  CR_INSTALL_INFOA infoA;
  memset(&infoA, 0, sizeof(CR_INSTALL_INFOA));
  infoA.cb = sizeof(CR_INSTALL_INFOA);
  infoA.pszAppVersion = "1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallA(&infoA);
  TEST_ASSERT(nInstallResult==0);
  
  // Should fail, because property name is empty
  int nResult2 = crAddPropertyA("", "nVidia GeForce GTS 250");
  TEST_ASSERT(nResult2!=0);
  
  // Should succeed
  int nResult3 = crAddPropertyA("VideoAdapter", "nVidia GeForce GTS 250");
  TEST_ASSERT(nResult3==0);

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();
  
}

REGISTER_TEST(Test_crAddPropertyW);
void Test_crAddPropertyW()
{   
  // Should fail, because crInstall() should be called first
  int nResult = crAddPropertyW(L"VideoAdapter", L"nVidia GeForce GTS 250");
  TEST_ASSERT(nResult!=0);

  // Install crash handler
  CR_INSTALL_INFOW infoW;
  memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
  infoW.cb = sizeof(CR_INSTALL_INFOW);
  infoW.pszAppVersion = L"1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallW(&infoW);
  TEST_ASSERT(nInstallResult==0);
  
  // Should fail, because property name is empty
  int nResult2 = crAddPropertyW(L"", L"nVidia GeForce GTS 250");
  TEST_ASSERT(nResult2!=0);
  
  // Should succeed
  int nResult3 = crAddPropertyW(L"VideoAdapter", L"nVidia GeForce GTS 250");
  TEST_ASSERT(nResult3==0);

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();
  
}

REGISTER_TEST(Test_crAddScreenshot);
void Test_crAddScreenshot()
{   
  // Should fail, because crInstall() should be called first
  int nResult = crAddScreenshot(CR_AS_VIRTUAL_SCREEN);
  TEST_ASSERT(nResult!=0);

  // Install crash handler
  CR_INSTALL_INFOW infoW;
  memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
  infoW.cb = sizeof(CR_INSTALL_INFOW);
  infoW.pszAppVersion = L"1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallW(&infoW);
  TEST_ASSERT(nInstallResult==0);
  
  // Should succeed
  int nResult2 = crAddScreenshot(CR_AS_VIRTUAL_SCREEN);
  TEST_ASSERT(nResult2==0);

  // Call twice - should succeed
  int nResult3 = crAddScreenshot(CR_AS_MAIN_WINDOW);
  TEST_ASSERT(nResult3==0);

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();  
}

REGISTER_TEST(Test_crAddScreenshot2);
void Test_crAddScreenshot2()
{   
  // Should fail, because crInstall() should be called first
  int nResult = crAddScreenshot2(CR_AS_VIRTUAL_SCREEN, 95);
  TEST_ASSERT(nResult!=0);

  // Install crash handler
  CR_INSTALL_INFOW infoW;
  memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
  infoW.cb = sizeof(CR_INSTALL_INFOW);
  infoW.pszAppVersion = L"1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallW(&infoW);
  TEST_ASSERT(nInstallResult==0);
  
  // Should succeed
  int nResult2 = crAddScreenshot2(CR_AS_VIRTUAL_SCREEN, 50);
  TEST_ASSERT(nResult2==0);

  // Call twice - should succeed
  int nResult3 = crAddScreenshot2(CR_AS_MAIN_WINDOW, 60);
  TEST_ASSERT(nResult3==0);

  // Call with invalid JPEG quality - should fail
  int nResult4 = crAddScreenshot2(CR_AS_MAIN_WINDOW, -60);
  TEST_ASSERT(nResult4!=0);

  // Call with invalid JPEG quality - should fail
  int nResult5 = crAddScreenshot2(CR_AS_MAIN_WINDOW, 160);
  TEST_ASSERT(nResult5!=0);

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();  
}


REGISTER_TEST(Test_crAddRegKeyA);
void Test_crAddRegKeyA()
{   
  // Should fail, because crInstall() should be called first
  int nResult = crAddRegKeyA("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows", "regkey.xml", 0);
  TEST_ASSERT(nResult!=0);

  // Install crash handler
  CR_INSTALL_INFOA infoA;
  memset(&infoA, 0, sizeof(CR_INSTALL_INFOA));
  infoA.cb = sizeof(CR_INSTALL_INFOA);
  infoA.pszAppVersion = "1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallA(&infoA);
  TEST_ASSERT(nInstallResult==0);
  
  // Should fail, because registry key name is NULL
  int nResult2 = crAddRegKeyA(NULL, "regkey.xml", 0);
  TEST_ASSERT(nResult2!=0);
  
  // Should fail, because registry key name is empty
  int nResult3 = crAddRegKeyA("", "regkey.xml", 0);
  TEST_ASSERT(nResult3!=0);

  // Should succeed
  int nResult4 = crAddRegKeyA("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows", "regkey.xml", 0);
  TEST_ASSERT(nResult4==0);

  // Should fail, because registry key doesn't exist
  int nResult5 = crAddRegKeyA("HKEY_LOCAL_MACHINE\\Softweeere\\", "regkey.xml", 0);
  TEST_ASSERT(nResult5!=0);

  // Should fail, because registry key is a parent key
  int nResult6 = crAddRegKeyA("HKEY_LOCAL_MACHINE\\", "regkey.xml", 0);
  TEST_ASSERT(nResult6!=0);

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();  
}

REGISTER_TEST(Test_crAddRegKeyW);
void Test_crAddRegKeyW()
{   
  // Should fail, because crInstall() should be called first
  int nResult = crAddRegKeyW(L"HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows", L"regkey.xml", 0);
  TEST_ASSERT(nResult!=0);

  // Install crash handler
  CR_INSTALL_INFOW infoW;
  memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
  infoW.cb = sizeof(CR_INSTALL_INFOW);
  infoW.pszAppVersion = L"1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallW(&infoW);
  TEST_ASSERT(nInstallResult==0);
  
  // Should fail, because registry key name is NULL
  int nResult2 = crAddRegKeyW(NULL, L"regkey.xml", 0);
  TEST_ASSERT(nResult2!=0);
  
  // Should fail, because registry key name is empty
  int nResult3 = crAddRegKeyW(L"", L"regkey.xml", 0);
  TEST_ASSERT(nResult3!=0);

  // Should succeed
  int nResult4 = crAddRegKeyW(L"HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows", L"regkey.xml", 0);
  TEST_ASSERT(nResult4==0);

  // Should fail, because registry key doesn't exist
  int nResult5 = crAddRegKeyW(L"HKEY_LOCAL_MACHINE\\Softweeere\\", L"regkey.xml", 0);
  TEST_ASSERT(nResult5!=0);

  // Should fail, because registry key is a parent key
  int nResult6 = crAddRegKeyW(L"HKEY_LOCAL_MACHINE\\", L"regkey.xml", 0);
  TEST_ASSERT(nResult6!=0);

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();  
}

REGISTER_TEST(Test_crGetLastErrorMsgA);
void Test_crGetLastErrorMsgA()
{ 
  // Get error message before Install
  char szErrMsg[256] = "";
  int nResult = crGetLastErrorMsgA(szErrMsg, 256);
  TEST_ASSERT(nResult>0);

  // Install crash handler
  CR_INSTALL_INFOA infoA;
  memset(&infoA, 0, sizeof(CR_INSTALL_INFOA));
  infoA.cb = sizeof(CR_INSTALL_INFOA);
  infoA.pszAppVersion = "1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallA(&infoA);
  TEST_ASSERT(nInstallResult==0);
  
  // Get error message
  char szErrMsg2[256] = "";
  int nResult2 = crGetLastErrorMsgA(szErrMsg2, 256);
  TEST_ASSERT(nResult2>0);

  // Get error message to NULL buffer - must fail  
  int nResult3 = crGetLastErrorMsgA(NULL, 256);
  TEST_ASSERT(nResult3<0);

  // Get error message to a buffer, but zero length - must fail  
  char szErrMsg3[256] = "";
  int nResult4 = crGetLastErrorMsgA(szErrMsg3, 0);
  TEST_ASSERT(nResult4<0);

  // Get error message to a single-char buffer, must trunkate message and succeed
  char szErrMsg5[1] = "";
  int nResult5 = crGetLastErrorMsgA(szErrMsg5, 1);
  TEST_ASSERT(nResult5==0);

  // Get error message to a small buffer, must trunkate message and succeed
  char szErrMsg6[2] = "";
  int nResult6 = crGetLastErrorMsgA(szErrMsg6, 2);
  TEST_ASSERT(nResult6>0);

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();  
}

REGISTER_TEST(Test_crGetLastErrorMsgW);
void Test_crGetLastErrorMsgW()
{ 
  // Get error message before Install
  WCHAR szErrMsg[256] = L"";
  int nResult = crGetLastErrorMsgW(szErrMsg, 256);
  TEST_ASSERT(nResult>0);

  // Install crash handler
  CR_INSTALL_INFOW infoW;
  memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
  infoW.cb = sizeof(CR_INSTALL_INFOW);
  infoW.pszAppVersion = L"1.0.0"; // Specify app version, otherwise it will fail.

  int nInstallResult = crInstallW(&infoW);
  TEST_ASSERT(nInstallResult==0);
  
  // Get error message
  WCHAR szErrMsg2[256] = L"";
  int nResult2 = crGetLastErrorMsgW(szErrMsg2, 256);
  TEST_ASSERT(nResult2>0);

  // Get error message to NULL buffer - must fail  
  int nResult3 = crGetLastErrorMsgW(NULL, 256);
  TEST_ASSERT(nResult3<0);

  // Get error message to a buffer, but zero length - must fail  
  WCHAR szErrMsg3[256] = L"";
  int nResult4 = crGetLastErrorMsgW(szErrMsg3, 0);
  TEST_ASSERT(nResult4<0);

  // Get error message to a single-char buffer, must trunkate message and succeed
  WCHAR szErrMsg5[1] = L"";
  int nResult5 = crGetLastErrorMsgW(szErrMsg5, 1);
  TEST_ASSERT(nResult5==0);

  // Get error message to a small buffer, must trunkate message and succeed
  WCHAR szErrMsg6[2] = L"";
  int nResult6 = crGetLastErrorMsgW(szErrMsg6, 2);
  TEST_ASSERT(nResult6>0);

  __TEST_CLEANUP__;

  // Uninstall
  crUninstall();
  
}

REGISTER_TEST(Test_CrAutoInstallHelper);
void Test_CrAutoInstallHelper()
{   
  // Install crash handler

  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.cb = sizeof(CR_INSTALL_INFO);
  info.pszAppVersion = _T("1.0.0"); // Specify app version, otherwise it will fail.

  CrAutoInstallHelper cr_install_helper(&info);
  TEST_ASSERT(cr_install_helper.m_nInstallStatus==0);
    
  __TEST_CLEANUP__;  
}

REGISTER_TEST(Test_CrThreadAutoInstallHelper);

DWORD WINAPI ThreadProc1(LPVOID lpParam)
{
  // Install thread exception handlers
  CrThreadAutoInstallHelper cr_thread_install(0);
 
  int* pnResult = (int*)lpParam;
  *pnResult = cr_thread_install.m_nInstallStatus;
  
  return 0;
}

void Test_CrThreadAutoInstallHelper()
{   
  // Install crash handler for the main thread

  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.cb = sizeof(CR_INSTALL_INFO);
  info.pszAppVersion = _T("1.0.0"); // Specify app version, otherwise it will fail.

  CrAutoInstallHelper cr_install_helper(&info);
  TEST_ASSERT(cr_install_helper.m_nInstallStatus==0);
  
  // Run a worker thread
  int nResult = -1;
  HANDLE hThread = CreateThread(NULL, 0, ThreadProc1, &nResult, 0, NULL);

  // Wait until thread exits
  WaitForSingleObject(hThread, INFINITE);

  TEST_ASSERT(nResult==0);

  __TEST_CLEANUP__;  
}

REGISTER_TEST(Test_crEmulateCrash);
void Test_crEmulateCrash()
{ 
  CString sAppDataFolder;
  CString sExeFolder;
  CString sTmpFolder;
  
  // Test it with invalid argument - should fail
  int nResult = crEmulateCrash((UINT)-1);
  TEST_ASSERT(nResult!=0);
  
  // Test it with invalid argument - should fail
  int nResult2 = crEmulateCrash(CR_THROW+1);
  TEST_ASSERT(nResult2!=0);

  __TEST_CLEANUP__;    
  
  crUninstall();

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE);
}

REGISTER_TEST(Test_crInstallToCurrentThread);

DWORD WINAPI ThreadProc2(LPVOID /*lpParam*/)
{  
  // Uninstall before install - should fail
  int nUnResult = crUninstallFromCurrentThread();
  TEST_ASSERT(nUnResult!=0);

  // Install thread exception handlers - should succeed
  int nResult = crInstallToCurrentThread();
  TEST_ASSERT(nResult==0);
  
  // Install thread exception handlers the second time - should fail
  int nResult2 = crInstallToCurrentThread();
  TEST_ASSERT(nResult2!=0);

  __TEST_CLEANUP__;

  // Uninstall - should succeed
  crUninstallFromCurrentThread();
  

  return 0;
}

void Test_crInstallToCurrentThread()
{ 
  // Call before install - must fail
  int nResult = crInstallToCurrentThread();
  TEST_ASSERT(nResult!=0);

  // Call before install - must fail
  int nResult2 = crInstallToCurrentThread2(0);
  TEST_ASSERT(nResult2!=0);

  // Install crash handler for the main thread

  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.cb = sizeof(CR_INSTALL_INFO);
  info.pszAppVersion = _T("1.0.0"); // Specify app version, otherwise it will fail.

  int nInstResult = crInstall(&info);
  TEST_ASSERT(nInstResult==0);
  
  // Call in the main thread - must fail
  int nResult3 = crInstallToCurrentThread2(0);
  TEST_ASSERT(nResult3!=0);

  // Run a worker thread
  HANDLE hThread = CreateThread(NULL, 0, ThreadProc2, NULL, 0, NULL);

  // Wait until thread exits
  WaitForSingleObject(hThread, INFINITE);

  __TEST_CLEANUP__;  

  // Uninstall should succeed
  crUninstall();  
}

// This test runs several threads and installs/uninstalls exception handlers in
// them concurrently. 
REGISTER_TEST(Test_crInstallToCurrentThread_concurrent);

DWORD WINAPI ThreadProc3(LPVOID /*lpParam*/)
{ 
  int i;
  for(i=0; i<100; i++)
  {
    // Install thread exception handlers - should succeed
    int nResult = crInstallToCurrentThread();
    TEST_ASSERT(nResult==0);
      
    Sleep(10);

    // Uninstall - should succeed
    int nUnResult2 = crUninstallFromCurrentThread();
    TEST_ASSERT(nUnResult2==0);
  }

  __TEST_CLEANUP__;

  crUninstallFromCurrentThread();
  return 0;
}

void Test_crInstallToCurrentThread_concurrent()
{ 
  // Install crash handler for the main thread

  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.cb = sizeof(CR_INSTALL_INFO);
  info.pszAppVersion = _T("1.0.0"); // Specify app version, otherwise it will fail.

  int nInstResult = crInstall(&info);
  TEST_ASSERT(nInstResult==0);
    
  // Run a worker thread
  HANDLE hThread = CreateThread(NULL, 0, ThreadProc3, NULL, 0, NULL);

  // Run another worker thread
  HANDLE hThread2 = CreateThread(NULL, 0, ThreadProc3, NULL, 0, NULL);

  // Run the third worker thread
  HANDLE hThread3 = CreateThread(NULL, 0, ThreadProc3, NULL, 0, NULL);

  // Wait until threads exit
  WaitForSingleObject(hThread, INFINITE);
  WaitForSingleObject(hThread2, INFINITE);
  WaitForSingleObject(hThread3, INFINITE);

  __TEST_CLEANUP__;  

  // Uninstall
  crUninstall();  
}

REGISTER_TEST(Test_crGenerateErrorReport);
void Test_crGenerateErrorReport()
{ 
  CString sAppDataFolder;
  CString sExeFolder;
  CString sTmpFolder;

  // Create a temporary folder  
  Utility::GetSpecialFolder(CSIDL_APPDATA, sAppDataFolder);
  sTmpFolder = sAppDataFolder+_T("\\CrashRpt");
  BOOL bCreate = Utility::CreateFolder(sTmpFolder);
  TEST_ASSERT(bCreate);

  // Install crash handler for the main thread

  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.cb = sizeof(CR_INSTALL_INFO);
  info.pszAppVersion = _T("1.0.0"); // Specify app version, otherwise it will fail.
  info.dwFlags = CR_INST_NO_GUI|CR_INST_DONT_SEND_REPORT;
  info.pszErrorReportSaveDir = sTmpFolder;
  int nInstResult = crInstall(&info);
  TEST_ASSERT(nInstResult==0);
  
  // Call with NULL parameter - should fail
  int nResult = crGenerateErrorReport(NULL);
  TEST_ASSERT(nResult!=0);
  
  // Call with valid parameter - should succeed
  CR_EXCEPTION_INFO exc;
  memset(&exc, 0, sizeof(CR_EXCEPTION_INFO));
  exc.cb = sizeof(CR_EXCEPTION_INFO);
  int nResult2 = crGenerateErrorReport(&exc);
  TEST_ASSERT(nResult2==0);

  // Check that a folder with crash report files exists
  WIN32_FIND_DATA fd;
  HANDLE hFind = FindFirstFile(sTmpFolder+_T("\\*"), &fd);
  FindClose(hFind);
  TEST_ASSERT(hFind!=INVALID_HANDLE_VALUE && hFind!=NULL);

  __TEST_CLEANUP__;  

  // Uninstall
  crUninstall();  

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE);
}

#ifndef CRASHRPT_LIB
// Test that API function names are undecorated
REGISTER_TEST(Test_undecorated_func_names);
void Test_undecorated_func_names()
{
  HMODULE hCrashRpt = NULL;
    
    // Load CrashRpt.dll dynamically
#ifdef _DEBUG
  hCrashRpt = LoadLibrary(_T("CrashRptd.dll"));
#else
  hCrashRpt = LoadLibrary(_T("CrashRpt.dll"));
#endif
  TEST_ASSERT(hCrashRpt!=NULL);

  typedef int (WINAPI *PFNCRINSTALLA)(PCR_INSTALL_INFOA);
  PFNCRINSTALLA pfncrInstallA = (PFNCRINSTALLA)GetProcAddress(hCrashRpt, "crInstallA");
  TEST_ASSERT(pfncrInstallA!=NULL);

  typedef int (WINAPI *PFNCRINSTALLW)(PCR_INSTALL_INFOW);
  PFNCRINSTALLW pfncrInstallW = (PFNCRINSTALLW)GetProcAddress(hCrashRpt, "crInstallW");
  TEST_ASSERT(pfncrInstallW!=NULL);

  typedef int (WINAPI *PFNCRUNINSTALL)();
  PFNCRUNINSTALL pfncrUninstall = (PFNCRUNINSTALL)GetProcAddress(hCrashRpt, "crUninstall");
  TEST_ASSERT(pfncrUninstall!=NULL);

  typedef int (WINAPI *PFNCRINSTALLTOCURRENTTHREAD)();
  PFNCRINSTALLTOCURRENTTHREAD pfncrInstallToCurrentThread = 
    (PFNCRINSTALLTOCURRENTTHREAD)GetProcAddress(hCrashRpt, "crInstallToCurrentThread");
  TEST_ASSERT(pfncrInstallToCurrentThread!=NULL);

  typedef int (WINAPI *PFNCRINSTALLTOCURRENTTHREAD2)();
  PFNCRINSTALLTOCURRENTTHREAD2 pfncrInstallToCurrentThread2 = 
    (PFNCRINSTALLTOCURRENTTHREAD2)GetProcAddress(hCrashRpt, "crInstallToCurrentThread2");
  TEST_ASSERT(pfncrInstallToCurrentThread2!=NULL);

  typedef int (WINAPI *PFNCRUNINSTALLFROMCURRENTTHREAD)();
  PFNCRUNINSTALLFROMCURRENTTHREAD pfncrUninstallFromCurrentThread = 
    (PFNCRINSTALLTOCURRENTTHREAD)GetProcAddress(hCrashRpt, "crUninstallFromCurrentThread");
  TEST_ASSERT(pfncrUninstallFromCurrentThread!=NULL);

  typedef int (WINAPI *PFNCRADDFILEW)(LPCWSTR, LPCWSTR);
  PFNCRADDFILEW pfncrAddFileW = 
    (PFNCRADDFILEW)GetProcAddress(hCrashRpt, "crAddFileW");
  TEST_ASSERT(pfncrAddFileW!=NULL);

  typedef int (WINAPI *PFNCRADDFILEA)(LPCSTR, LPCSTR);
  PFNCRADDFILEA pfncrAddFileA = 
    (PFNCRADDFILEA)GetProcAddress(hCrashRpt, "crAddFileA");
  TEST_ASSERT(pfncrAddFileA!=NULL);
  
  typedef int (WINAPI *PFNCRADDFILE2W)(LPCWSTR, LPCWSTR);
  PFNCRADDFILE2W pfncrAddFile2W = 
    (PFNCRADDFILE2W)GetProcAddress(hCrashRpt, "crAddFile2W");
  TEST_ASSERT(pfncrAddFile2W!=NULL);

  typedef int (WINAPI *PFNCRADDFILE2A)(LPCSTR, LPCSTR);
  PFNCRADDFILE2A pfncrAddFile2A = 
    (PFNCRADDFILE2A)GetProcAddress(hCrashRpt, "crAddFile2A");
  TEST_ASSERT(pfncrAddFile2A!=NULL);

  __TEST_CLEANUP__
    
  FreeLibrary(hCrashRpt);
}
#endif //!CRASHRPT_LIB