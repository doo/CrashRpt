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

std::map<std::string, std::string>* g_pTestSuiteList = NULL;
std::string sCurTestSuite;
std::map<std::string, PFNTEST>* g_pTestList = NULL;
std::vector<std::string>* g_pErrorList = NULL;

// Helper function that removes spaces from the beginning and end of the string
void trim2(std::string& str, char* szTrim=" \t\n")
{
  std::string::size_type pos = str.find_last_not_of(szTrim);
  if(pos != std::string::npos) {
    str.erase(pos + 1);
    pos = str.find_first_not_of(szTrim);
    if(pos != std::string::npos) str.erase(0, pos);
  }
  else str.erase(str.begin(), str.end());
}

// Helper function that splits a string into list of tokens
std::vector<std::string> explode(std::string str, std::string separators = " \t")
{
  std::vector<std::string> aTokens;

  size_t pos = 0;
  for(;;)
  {
    pos = str.find_first_of(separators, 0);
    
    std::string sToken = str.substr(0, pos);
    if(pos!=std::string::npos)
      str = str.substr(pos+1);
    
    trim2(sToken);
    if(sToken.length()>0)
      aTokens.push_back(sToken);

    if(pos==std::string::npos)
      break;    
  }

  return aTokens;
}

int main()
{
  printf("\n=== Automated tests for CrashRpt v.%d.%d.%d ===\n\n",
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

  printf("\nEnter which test suites to run (separate names by space) or enter empty line to run all test suites.\n");
  printf("Your choice > ");
  char szSuiteList[1024]="";
#if _MSC_VER>=1400
  gets_s(szSuiteList, 1024);  
#else
  gets(szSuiteList);  
#endif

  // Create list of test suites to run
  std::string sSuiteList = szSuiteList;
  std::vector<std::string> aTokens = explode(sSuiteList);
  std::set<std::string> aTestSuitesToRun;
  size_t i;
  for(i=0; i<aTokens.size(); i++) 
    aTestSuitesToRun.insert(aTokens[i]);
  
  // Determine how many tests to run
  size_t nTestsToRun = 0;
  if(aTestSuitesToRun.size()==0)
  {
    nTestsToRun = g_pTestList->size();
  }
  else
  {    
    std::map<std::string, PFNTEST>::iterator iter;
    for(iter=g_pTestList->begin(); iter!=g_pTestList->end(); iter++)
    {
      std::string sName = iter->first;
      size_t pos = sName.find(':');
      std::string sTestSuite = sName.substr(0, pos);    
      std::set<std::string>::iterator sit = 
        aTestSuitesToRun.find(sTestSuite);
      if(sit!=aTestSuitesToRun.end())
      {
        nTestsToRun++;        
      }      
    }
  }
  
  if(nTestsToRun==0)
  {
    printf("\nNo tests selected, exiting.\n");
    return 0;
  }

  printf("\nRunning tests...\n");

  // Walk through all registered test and run each one
  std::map<std::string, PFNTEST>::iterator iter;
  int n = 1;
  for(iter=g_pTestList->begin(); iter!=g_pTestList->end(); iter++)
  {
    std::string sName = iter->first;
    size_t pos = sName.find(':');
    std::string sTestSuite = sName.substr(0, pos);    
    std::set<std::string>::iterator sit = 
      aTestSuitesToRun.find(sTestSuite);
    if(aTestSuitesToRun.size()==0 || sit!=aTestSuitesToRun.end())
    {
      printf("- %d/%d: %s ...\n", n, nTestsToRun, iter->first.c_str());
      n++;
      iter->second();
    }          
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

  printf("\n   Test count: %d\n", nTestsToRun);
  size_t nErrorCount = g_pErrorList!=NULL?g_pErrorList->size():0;
  printf(" Tests passed: %d\n", nTestsToRun-nErrorCount);
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