#pragma once
#include "stdafx.h"

typedef int (__cdecl *PFNTEST)();

extern std::map<std::string, PFNTEST> g_TestList;
extern std::vector<std::string> g_AssertionList;

#define BEGIN_TEST()\
std::string __func = __FUNCTION__;\
g_TestList[__func] = this;

#define END_TEST()\
return 0;

#define TEST_ASSERT(expr)\
if(!(expr)) { printf("ASSERTION FAILED IN FUNCTION:"__FUNCSIG__ " EXPRESSION:" #expr "\n"); \
std::string assertion = "ASSERTION FAILED IN FUNCTION:"__FUNCTION__ " EXPRESSION:" #expr"\n";\
g_AssertionList.push_back(assertion);\
return -1; }