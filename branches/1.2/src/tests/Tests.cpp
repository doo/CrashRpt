#include "stdafx.h"
#include "Tests.h"

std::map<std::string, PFNTEST>* g_pTestList = NULL;
std::vector<std::string>* g_pErrorList = NULL;

int main()
{
  printf("Running tests...\n");

  std::map<std::string, PFNTEST>::iterator iter;
  for(iter=g_pTestList->begin(); iter!=g_pTestList->end(); iter++)
  {
    printf("Running test %s of %d...\n", iter->first.c_str(), g_pTestList->size());
    iter->second();
  }

  printf("\n=== Summary ===\n\n");
  size_t i;
  for(i=0; i<g_pErrorList->size(); i++)
  {
    printf("Error %d: %s\n", i, (*g_pErrorList)[i].c_str());
  }
  printf("Test count: %d\n", g_pTestList->size());
  printf("Tests passed: %d\n", g_pTestList->size()-g_pErrorList->size());
  printf("Tests failed: %d\n", g_pErrorList->size());


  _getch();

  return 0;
}