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
#include "Utility.h"
#include "CrashRpt.h"

REGISTER_TEST_SUITE( DeliveryTests , "Error report delivery tests");

REGISTER_TEST(Test_HttpDelivery_legacy_encoding);
void Test_HttpDelivery_legacy_encoding()
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
  info.dwFlags = CR_INST_NO_GUI;
  info.pszUrl = _T("localhost/crashrpt.php"); // Use HTTP address for delivery 
  info.uPriorities[CR_HTTP] = 0;
  info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;
  info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY;
  info.pszErrorReportSaveDir = sTmpFolder;
  int nInstResult = crInstall(&info);
  TEST_ASSERT(nInstResult==0);
    
  // Generate and send error report
  CR_EXCEPTION_INFO exc;
  memset(&exc, 0, sizeof(CR_EXCEPTION_INFO));
  exc.cb = sizeof(CR_EXCEPTION_INFO);
  int nResult2 = crGenerateErrorReport(&exc);
  TEST_ASSERT(nResult2==0);

  // Wait until CrashSender exits and check exit code
  WaitForSingleObject(exc.hSenderProcess, INFINITE);

  DWORD dwExitCode = 1;
  GetExitCodeProcess(exc.hSenderProcess, &dwExitCode);
  TEST_ASSERT(dwExitCode==0); // Exit code should be zero
  
  __TEST_CLEANUP__;  

  // Uninstall
  crUninstall();  

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE);
}

REGISTER_TEST(Test_HttpDelivery_binary_encoding);
void Test_HttpDelivery_binary_encoding()
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
  info.dwFlags = CR_INST_NO_GUI|CR_INST_HTTP_BINARY_ENCODING;
  info.pszUrl = _T("localhost/crashrpt.php"); // Use HTTP address for delivery 
  info.uPriorities[CR_HTTP] = 0;
  info.uPriorities[CR_SMTP] = CR_NEGATIVE_PRIORITY;
  info.uPriorities[CR_SMAPI] = CR_NEGATIVE_PRIORITY;  
  info.pszErrorReportSaveDir = sTmpFolder;
  int nInstResult = crInstall(&info);
  TEST_ASSERT(nInstResult==0);
    
  // Generate and send error report
  CR_EXCEPTION_INFO exc;
  memset(&exc, 0, sizeof(CR_EXCEPTION_INFO));
  exc.cb = sizeof(CR_EXCEPTION_INFO);
  int nResult2 = crGenerateErrorReport(&exc);
  TEST_ASSERT(nResult2==0);

  // Wait until CrashSender exits and check exit code
  WaitForSingleObject(exc.hSenderProcess, INFINITE);

  DWORD dwExitCode = 1;
  GetExitCodeProcess(exc.hSenderProcess, &dwExitCode);
  TEST_ASSERT(dwExitCode==0); // Exit code should be zero
  
  __TEST_CLEANUP__;  

  // Uninstall
  crUninstall();  

  // Delete tmp folder
  Utility::RecycleFile(sTmpFolder, TRUE);
}
