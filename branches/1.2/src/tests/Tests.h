#pragma once
#include "stdafx.h"

typedef void (__cdecl *PFNTEST)();

extern std::map<std::string, PFNTEST>* g_pTestList;
extern std::vector<std::string>* g_pErrorList;

class CTestRegistrator
{
public:
  CTestRegistrator(LPCSTR szTestName, PFNTEST pfnTest)
  {
    if(g_pTestList==NULL)
    {
      g_pTestList = new std::map<std::string, PFNTEST>;
    }
    std::string sName = std::string(szTestName);
    (*g_pTestList)[sName] = pfnTest;
  }
};

#define REGISTER_TEST(pfnTest)\
  void pfnTest();\
  CTestRegistrator __test##pfnTest ( #pfnTest , pfnTest );


#define TEST_ASSERT(expr)\
if(!(expr)) { printf("!!!Error in test: "__FUNCTION__ " Expr: " #expr "\n"); \
std::string assertion = "Error in test: "__FUNCTION__ " Expr: " #expr;\
  if(g_pErrorList==NULL) g_pErrorList = new std::vector<std::string>;\
g_pErrorList->push_back(assertion);\
goto test_cleanup; }

#define __TEST_CLEANUP__ test_cleanup: