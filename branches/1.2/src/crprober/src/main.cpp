#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "CrashRptProbe.h"

typedef std::basic_string<TCHAR> tstring;

#define args_left() (argc-cur_arg)
#define arg_exists() (cur_arg<argc && argv[cur_arg]!=NULL)
#define get_arg() ( arg_exists() ? argv[cur_arg]:NULL )
#define skip_arg() cur_arg++
#define cmp_arg(val) (arg_exists() && (0==_tcscmp(argv[cur_arg], val)))

#define OUT_TEXT 0
#define OUT_HTML 1
#define OUT_XML  2

int process_command(TCHAR* szInput, TCHAR* szInputMD5, TCHAR* szOutput, 
  int nOutputFormat, std::vector<std::string> sym_search_list);

int output_text(CrpHandle handle, FILE* f);


// Prints usage
void print_usage()
{
  _tprintf(_T("Usage:\n"));
  _tprintf(_T("crprober /? Prints this usage help\n"));
  _tprintf(_T("crprober <param> [param ...]\n"));
  _tprintf(_T("  where param may be one of the following:\n"));
  _tprintf(_T("   /f <in_file_pattern>     Defines input ZIP file or search pattern. Required.\n"));
  _tprintf(_T("   /fmd5 <md5_file>         Defines name of .md5 file containing MD5 hash for ZIP archive.\n"));
  _tprintf(_T("   /o <out_file_or_dir>     Defines output file name or directory. If this parameter \
is ommitted, output is written to the terminal.\n"));
  _tprintf(_T("   /of <text | html | xml>  Defines output format (text, html or xml). If ommitted, text format is used.\n"));
  _tprintf(_T("   /sym <sym_search_dirs>   Symbol file search directory or list of directories \
separated with semicolon. If this parameter is ommitted, symbols files are searched in current directory.\n"));  
}

// Program entry point
int _tmain(int argc, TCHAR** argv)
{
  int result = 1;
  int cur_arg = 1;

  TCHAR* szInput = NULL;
  TCHAR* szInputMD5 = NULL;
  TCHAR* szOutput = NULL;
  TCHAR* szOutputFormat = NULL;
  TCHAR* szSymSearchPath = NULL;
  std::vector<std::string> sym_search_list; 
  int out_format = OUT_TEXT;

  if(args_left()==0)
    goto exit;

  while(arg_exists())
  {
    if(cmp_arg(_T("/?")))
    {      
      goto exit;
    }
    else if(cmp_arg(_T("/f")))
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
    else if(cmp_arg(_T("/fmd5")))
    {
      skip_arg();    
      szInputMD5 = get_arg();
      skip_arg();    
      if(szInputMD5==NULL)
      {
        _tprintf(_T("Input MD5 file name is missing in /fmd5 parameter.\n"));
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
      skip_arg();    
      szOutputFormat = get_arg();
      skip_arg();   
      if(szOutputFormat!=NULL)
      {
        if(_tcscmp(szOutputFormat, _T("text"))==0)
          out_format = OUT_TEXT;
        else if(_tcscmp(szOutputFormat, _T("html"))==0)
          out_format = OUT_HTML;
        else if(_tcscmp(szOutputFormat, _T("xml"))==0)
          out_format = OUT_XML;
        else
        {
          _tprintf(_T("Invalid output format in /of parameter: %s\n"), szOutputFormat);
          goto exit;
        }        
      }
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

  result = process_command(szInput, szInputMD5, szOutput, 
    out_format, sym_search_list); 

exit:

  if(result==1)
  {
    print_usage();
  }

  return result;
}


// Processes file processing command
int process_command(
  TCHAR* szInput, 
  TCHAR* szInputMD5,
  TCHAR* szOutput, 
  int nOutputFormat, 
  std::vector<std::string> sym_search_list)
{
  int result = 2;
  CrpHandle handle = 0;
  WIN32_FIND_DATA fd;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  BOOL bNext = TRUE;

  // Validate input parameters

  if(szInput==NULL)
  {
    _tprintf(_T("Input file name or pattern is missing.\n"));
    goto exit;
  }

  if(nOutputFormat!=OUT_TEXT &&
    nOutputFormat!=OUT_HTML &&
    nOutputFormat!=OUT_XML)
  {
    _tprintf(_T("Invalid output format specified.\n"));
    goto exit;
  }

  // Search input files 
  hFind = FindFirstFile(szInput, &fd);
  if(hFind==INVALID_HANDLE_VALUE)
  {
    _tprintf(_T("No files found matching the search pattern: %s\n"), szInput);
    goto exit;
  }

  while(bNext)
  {    
    tstring str = fd.cFileName;
    str += _T(".md5");
    TCHAR szMD5Buffer[64];
    TCHAR* szMD5Hash = NULL;
    FILE* f = NULL;
   
    _tprintf(_T("Processing file: %s\n"), fd.cFileName);
     
    _tfopen_s(&f, str.c_str(), _T("rt"));
    if(f!=NULL)
    {
      szMD5Hash = _fgetts(szMD5Buffer, 64, f);      
      _tprintf(_T("Found MD5 file %s; MD5=%s\n"), fd.cFileName, szMD5Hash);
    }    
    else
    {
      _tprintf(_T("The MD5 file is not detected; no integrity check will be performed.\n"));
    }
    
    int res = crpOpenErrorReport(fd.cFileName, szMD5Hash, 0, &handle);
    if(res!=0)
    {
      TCHAR buf[1024];
      crpGetLastErrorMsg(buf, 1024);
      _tprintf(_T("Error '%s' while processing file '%s'\n"), buf , fd.cFileName);
    }

    // Output
    if(nOutputFormat==OUT_TEXT)
    {
      output_text(handle, stdout);
    }

    // Clean up
    if(handle!=NULL)
    {
      crpCloseErrorReport(handle);
    }
    
    bNext = FindNextFile(hFind, &fd);
  }
  
  result = 0;

exit:

  if(hFind!=INVALID_HANDLE_VALUE)
    FindClose(hFind);

  if(handle!=0)
    crpCloseErrorReport(handle);

  return result;
}

int output_text(CrpHandle handle, FILE* f)
{
  _ftprintf(f, _T("= Error Report =\n"));

  const int BUFF_SIZE = 1024;
  TCHAR buffer[BUFF_SIZE];
  ULONG uBuffSize = 0;  
  int result = -1;

  // Print CrashRpt version
  result = crpGetProperty(handle, CRP_PROP_CRASHRPT_VERSION, 0, buffer, BUFF_SIZE, NULL);
  if(result==0)
  {
    DWORD dwVersion = _ttoi(buffer);
    int nMajor = dwVersion/1000;
    int nMinor = (dwVersion%1000)/100;
    int nBuild = ((dwVersion%1000)%100)%100;
    _ftprintf(f, _T("Generated in CrashRpt %d.%d.%d\n"), nMajor, nMinor, nBuild);
  }

  // Print CrashGUID  
  result = crpGetProperty(handle, CRP_PROP_CRASH_GUID, 0, buffer, BUFF_SIZE, NULL);
  if(result==0)
  {    
    _ftprintf(f, _T("CrashGUID = %s\n"), buffer);
  }

  // Print file list  
  result = crpGetProperty(handle, CRP_PROP_FILE_COUNT, 0, buffer, BUFF_SIZE, NULL);
  if(result==0)
  {
    int nItemCount = _ttoi(buffer);
    int i;
    for(i=0; i<nItemCount; i++)
    {       
      int result2 = crpGetProperty(handle, CRP_PROP_FILE_ITEM_NAME, i, buffer, BUFF_SIZE, NULL);
      if(result2==0)
      {
        _ftprintf(f, _T("%d. %16s"), i+1, buffer);
        
        int result3 = crpGetProperty(handle, CRP_PROP_FILE_ITEM_DESCRIPTION, i, buffer, BUFF_SIZE, NULL);
        if(result3==0)
          _ftprintf(f, _T("  %s\n"), buffer);
        else
          _ftprintf(f, _T("\n"));
      }
      else
      {
        _ftprintf(f, _T("Failed to retieve file item #%d.\n"), i+1);
      }
    }
  }
  else
  {
    _ftprintf(f, _T("Failed to retieve file item count.\n"));
  }

  result = crpGetProperty(handle, CRP_PROP_STACK_FRAME_COUNT, 0, buffer, BUFF_SIZE, NULL);
  if(result==0)
  {
    int nItemCount = _ttoi(buffer);
    int i;
    for(i=0; i<nItemCount; i++)
    {       
      result = crpGetProperty(handle, CRP_PROP_STACK_MODULE_NAME, i, buffer, BUFF_SIZE, NULL);
      if(result==0)
        _ftprintf(f, _T("%s!"), buffer);
      
      result = crpGetProperty(handle, CRP_PROP_STACK_SYMBOL_NAME, i, buffer, BUFF_SIZE, NULL);
      if(result==0)
        _ftprintf(f, _T("%s"), buffer);

      result = crpGetProperty(handle, CRP_PROP_STACK_OFFSET_IN_SYMBOL, i, buffer, BUFF_SIZE, NULL);
      if(result==0)
        _ftprintf(f, _T("+%s"), buffer);
        
      result = crpGetProperty(handle, CRP_PROP_STACK_SOURCE_FILE, i, buffer, BUFF_SIZE, NULL);
      if(result==0)
        _ftprintf(f, _T(" %s"), buffer);

      result = crpGetProperty(handle, CRP_PROP_STACK_SOURCE_LINE, i, buffer, BUFF_SIZE, NULL);
      if(result==0)
        _ftprintf(f, _T(":%s"), buffer);

      _ftprintf(f, _T("\n"), buffer);    
    }
  }

  return 0;
}

int output_html()
{
  return 0;
}
