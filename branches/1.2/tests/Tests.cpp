#include "stdafx.h"
#include "Tests.h"

std::map<std::string, PFNTEST>* g_pTestList = NULL;
std::vector<std::string>* g_pErrorList = NULL;

int main()
{
  printf("Running tests...\n");

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

  return 0;
}