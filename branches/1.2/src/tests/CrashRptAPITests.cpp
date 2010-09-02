#include "stdafx.h"
#include "Tests.h"
#include "CrashRpt.h"

REGISTER_TEST(Test_crInstall_null);
void Test_crInstall_null()
{   
  // Test crInstall with NULL info - should fail
  
  int nInstallResult = crInstallW(NULL);
  TEST_ASSERT(nInstallResult!=0);

  int nInstallResult2 = crInstallA(NULL);
  TEST_ASSERT(nInstallResult2!=0); 
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

  int nUninstallResult = crUninstall();
  TEST_ASSERT(nUninstallResult==0); 
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

  int nUninstallResult = crUninstall();
  TEST_ASSERT(nUninstallResult==0); 
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

  int nUninstallResult = crUninstall();
  TEST_ASSERT(nUninstallResult==0); 
}

REGISTER_TEST(Test_crUninstall);
void Test_crUninstall()
{   
  // Call crUninstall - should fail, because crInstall should be called first
    
  int nUninstallResult = crUninstall();
  TEST_ASSERT(nUninstallResult!=0); 

  // And another time... 
  int nUninstallResult2 = crUninstall();
  TEST_ASSERT(nUninstallResult2!=0); 
}


