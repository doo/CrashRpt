#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "CrashRptProbe.h"

#define args_left() (argc-cur_arg)
#define arg_exists() (cur_arg<argc && argv[cur_arg]!=NULL)
#define get_arg() ( arg_exists() ? argv[cur_arg]:NULL )
#define skip_arg() cur_arg++
#define cmp_arg(val) (arg_exists() && (0==_tcscmp(argv[cur_arg], val)))

#define OUT_TEXT 0
#define OUT_HTML 1
#define OUT_XML  2

void print_usage()
{
  _tprintf(_T("Usage:\n"));
  _tprintf(_T("crprober /? Prints this usage help\n"));
  _tprintf(_T("crprober <param [param ...]>\n"));
  _tprintf(_T("  where param may be one of the following:\n"));
  _tprintf(_T("   -f <in_file_pattern>     Defines input ZIP file or search pattern. Required.\n"));
  _tprintf(_T("   -o <out_file_or_dir>     Defines output file name or directory. If this parameter \
is ommitted, output is written to the terminal.\n"));
  _tprintf(_T("   -of <text | html | xml>  Defines output format (text, html or xml). If ommitted, text format is used.\n"));
  _tprintf(_T("   -sym <sym_search_dirs>   Symbol file search directory or list of directories \
separated with semicolon. If this parameter is ommitted, symbols files are searched in current directory.\n"));  
}

int process()
{
  CrpHandle handle = -1;
  int res = crpOpenCrashReport(_T("0c9a73e8-f080-4f04-99c8-c9e07d317df4.zip"), 
    _T("8f3303040ca1607ddb2660550bd81dd0"), &handle);
  return 0;
}

int _tmain(int argc, TCHAR** argv)
{
  int result = 1;
  int cur_arg = 1;

  TCHAR* szInput = NULL;
  TCHAR* szOutput = NULL;
  TCHAR* szOutputFormat = NULL;
  TCHAR* szSymSearchPath = NULL;
 
  int out_format = OUT_TEXT;

  while(arg_exists())
  {
    if(cmp_arg(_T("/?")))
    {      
      goto exit;
    }
    if(cmp_arg(_T("/f")))
    {
      skip_arg();    
      szInput = get_arg();
      skip_arg();    
      if(szInput==NULL)
      {
        _tprintf(_T("Input file name is missing in /f parameter.\n"));
        goto exit;
      }
    }
    else if(cmp_arg(_T("/o")))
    {
      skip_arg();    
      szOutput = get_arg();
      skip_arg();          
    }
    else if(cmp_arg(_T("/of")))
    {
    }
    else if(cmp_arg(_T("/sym")))
    {
    }
    else
    {
      _tprintf(_T("Unexpected parameter: %s\n"), get_arg());
      goto exit;
    }    
  }

  result = 0;

exit:

  if(result==1)
  {
    print_usage();
  }

  return result;
}