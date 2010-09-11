#include "stdafx.h"
#include "Tests.h"
#include "CrashRpt.h"

std::map<std::string, std::string>* g_pTestSuiteList = NULL;
std::map<std::string, PFNTEST>* g_pTestList = NULL;
std::vector<std::string>* g_pErrorList = NULL;

int main()
{
  printf("\n=== Automated tests for CrashRpt v.%d.%d.%d===\n\n",
    CRASHRPT_VER/1000,
    (CRASHRPT_VER%1000)/100,
    (CRASHRPT_VER%1000)%100);

  printf("The list of avaliable test suites:\n");

  // Print the list of test suites
  std::map<std::string, std::string>::iterator siter;  
  for(siter=g_pTestSuiteList->begin(); siter!=g_pTestSuiteList->end(); siter++)
  {
    printf(" - %s : %s\n", siter->first.c_str(), siter->second.c_str());    
  }

  printf("\nEnter which test suites to run (separate names by space) or enter '*' to run all test suites.\n");
  printf("Your choice > ");
  char szSuiteList[1024];
  scanf("%s", &szSuiteList);

  printf("\nRunning tests...\n");

  // Walk through all registered test and run each one
  std::map<std::string, PFNTEST>::iterator iter;
  int n = 1;
  for(iter=g_pTestList->begin(); iter!=g_pTestList->end(); iter++)
  {
    printf("Running test %d of %d : %s ...\n", n, g_pTestList->size(), iter->first.c_str());
    n++;
    iter->second();
  }

  printf("\n=== Summary ===\n\n");
  
  // Print all errors (if exist)
  if(g_pErrorList!=NULL)
  {
    size_t i;
    for(i=0; i<g_pErrorList->size(); i++)
    {
      printf("Error %d: %s\n", i, (*g_pErrorList)[i].c_str());
    }
  }

  printf("   Test count: %d\n", g_pTestList->size());
  size_t nErrorCount = g_pErrorList!=NULL?g_pErrorList->size():0;
  printf(" Tests passed: %d\n", g_pTestList->size()-nErrorCount);
  printf(" Tests failed: %d\n", nErrorCount);

  // Wait for key press
  _getch();

  // Clean up
  if(g_pTestSuiteList!=NULL)
    delete g_pTestSuiteList;

  if(g_pTestList!=NULL)
    delete g_pTestList;

  if(g_pErrorList)
    delete g_pErrorList;

  return 0;
}