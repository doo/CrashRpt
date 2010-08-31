#include "stdafx.h"
#include "Tests.h"
#include "CrashRpt.h"

int Test_crInstall()
{
  BEGIN_TEST();

  // Test crInstallW with zero info
  {
    CR_INSTALL_INFOW infoW;
    memset(&infoW, 0, sizeof(CR_INSTALL_INFOW));
    infoW.cb = sizeof(CR_INSTALL_INFOW);

    int nResult = crInstallW(&infoW);
  }

  END_TEST();
}