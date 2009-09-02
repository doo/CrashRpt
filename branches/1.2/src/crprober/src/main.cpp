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

int process_command(LPTSTR szInput, LPTSTR szInputMD5, LPTSTR szOutput, 
  int nOutputFormat, LPTSTR szSymSearchPath);

int output_document(CrpHandle handle, FILE* f, int nOutputFormat);

class COutputter
{
public:

  void Init(FILE* f, int out_format)
  {
    m_fOut = f;
    m_nOutFormat = out_format;
  }

  void BeginDocument(LPCTSTR pszRootName, LPCTSTR pszTitle)
  {
    if(m_nOutFormat==OUT_TEXT)
    {
      _ftprintf(m_fOut, _T("= %s = \n\n"), pszTitle);
    }
    else if(m_nOutFormat==OUT_HTML)
    {
      _ftprintf(m_fOut, _T("<html>\n"));
      _ftprintf(m_fOut, _T("<head><title>%s</title></head>\n"), pszTitle);
      _ftprintf(m_fOut, _T("<body>\n"));
    }
    else if(m_nOutFormat==OUT_XML)
    {
      _ftprintf(m_fOut, _T("<?xml version=\"1.0\" encoding=\"utf-8\">\n"));
      _ftprintf(m_fOut, _T("<%s>\n"), pszRootName);
    }
  }

  void EndDocument(LPCTSTR pszRootName)
  {
    if(m_nOutFormat==OUT_TEXT)
    {
      
    }
    else if(m_nOutFormat==OUT_HTML)
    {
      _ftprintf(m_fOut, _T("</body>\n"));
      _ftprintf(m_fOut, _T("</html>\n"));
    }
    else if(m_nOutFormat==OUT_XML)
    {
      _ftprintf(m_fOut, _T("</%s>\n"), pszRootName);
    }
  }

  void BeginTable(LPCTSTR pszName, LPCTSTR pszFriendlyName)
  {
    if(m_nOutFormat==OUT_TEXT)
    {
      _ftprintf(m_fOut, pszFriendlyName);
    }
    else if(m_nOutFormat==OUT_HTML)
    {
      _ftprintf(m_fOut, _T("<h3>%s</h3>\n"), pszFriendlyName);
      _ftprintf(m_fOut, _T("<table id= \"%s\" border=\"1\" cellpadding=\"2\" cellspacing=\"2\">\n"));
    }
    else if(m_nOutFormat==OUT_XML)
    {
      _ftprintf(m_fOut, _T("<%s>"), pszName);
    }
  }

  void EndTable(LPCTSTR pszName)
  {
    if(m_nOutFormat==OUT_TEXT)
    {
      _ftprintf(m_fOut, _T("\n\n"));
    }
    else if(m_nOutFormat==OUT_HTML)
    {
      _ftprintf(m_fOut, _T("</table>\n"));
    }
    else if(m_nOutFormat==OUT_XML)
    {
      _ftprintf(m_fOut, _T("<%s>\n"), pszName);
    }
  }

  void BeginRow(/*LPCTSTR pszName, LPCTSTR pszHeading*/)
  {
    if(m_nOutFormat==OUT_TEXT)
    {
      
    }
    else if(m_nOutFormat==OUT_HTML)
    {
      _ftprintf(m_fOut, _T("<tr>\n"));
      /*if(pszHeading)
      {
        _ftprintf(m_fOut, _T("<td><b>%s</b></td>\n"), pszHeading);
      }*/
    }
    else if(m_nOutFormat==OUT_XML)
    {
      
    }
  }

  void EndRow()
  {
    if(m_nOutFormat==OUT_TEXT)
    {
      _ftprintf(m_fOut, _T("\n"));
    }
    else if(m_nOutFormat==OUT_HTML)
    {
      _ftprintf(m_fOut, _T("</tr>\n"));
    }
    else if(m_nOutFormat==OUT_XML)
    {
      
    }
  }

  void PutRecord(/*LPCTSTR pszName, */LPCTSTR pszValue)
  {
    if(m_nOutFormat==OUT_TEXT)
    {
      _ftprintf(m_fOut, _T("\n\n"));
    }
    else if(m_nOutFormat==OUT_HTML)
    {        
      _ftprintf(m_fOut, _T("<td>%s</td>\n"), pszValue);
    }
    else if(m_nOutFormat==OUT_XML)
    {
      
    }  
  }
  
private:

  FILE* m_fOut;
  int m_nOutFormat;
};

// Prints usage
void print_usage()
{
  _tprintf(_T("Usage:\n"));
  _tprintf(_T("crprober /? Prints this usage help\n"));
  _tprintf(_T("crprober <option> [option ...]\n"));
  _tprintf(_T("  where the option may be any of the following:\n"));
  _tprintf(_T("   /f <in_file_pattern>     Input ZIP file or search pattern. Required.\n"));
  _tprintf(_T("   /fmd5 <md5_file>         Name of .md5 file containing MD5 hash for ZIP archive.\n"));
  _tprintf(_T("   /o <out_file_or_dir>     Output file name or directory. If this parameter \
is ommitted, output is written to the terminal.\n"));
  _tprintf(_T("   /of <text | html | xml>  Output format (text, html or xml). If ommitted, text format is used.\n"));
  _tprintf(_T("   /sym <sym_search_dirs>   Symbol file search directory or list of directories \
separated with semicolon. If this parameter is ommitted, symbols files are searched in current directory.\n"));  
  _tprintf(_T("\nExample: \n"));
  _tprintf(_T("   crprober /f *.zip /o \"D:\\Error Reports\" /of text \n"));
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
      skip_arg();    
      szSymSearchPath = get_arg();
      skip_arg();
    }
    else
    {
      _tprintf(_T("Unexpected parameter: %s\n"), get_arg());
      goto exit;
    }    
  }

  result = process_command(szInput, szInputMD5, szOutput, 
    out_format, szSymSearchPath); 

exit:

  if(result==1)
  {
    print_usage();
  }

  return result;
}


// Processes file processing command
int process_command(
  LPTSTR szInput, 
  LPTSTR szInputMD5,
  LPTSTR szOutput, 
  int nOutputFormat, 
  LPTSTR szSymSearchPath)
{
  int result = 2;
  CrpHandle handle = 0;
  WIN32_FIND_DATA fd;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  BOOL bNext = TRUE;
  tstring sDirName = szInput;
  tstring sFileName;
  BOOL bOutputToDir = FALSE;
  FILE* f = NULL;

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

  // 

  DWORD dwFileAttrs = GetFileAttributes(szOutput);
  if(dwFileAttrs!=INVALID_FILE_ATTRIBUTES && 
     (dwFileAttrs&FILE_ATTRIBUTE_DIRECTORY))
    bOutputToDir = TRUE;  

  // Search input files 

  hFind = FindFirstFile(szInput, &fd);
  if(hFind==INVALID_HANDLE_VALUE)
  {
    _tprintf(_T("No files found matching the search pattern: %s\n"), szInput);
    goto exit;
  }
    
  int pos = sDirName.rfind('\\');
  if(pos>=0)
    sDirName = sDirName.substr(0, pos+1);
  else
    sDirName += _T("\\");

  while(bNext)
  {    
    tstring str = fd.cFileName;
    str += _T(".md5");
    TCHAR szMD5Buffer[64];
    TCHAR* szMD5Hash = NULL;
    FILE* f = NULL;
    tstring sOutFileName;
   
    _tprintf(_T("Processing file: %s\n"), fd.cFileName);
     
    _tfopen_s(&f, str.c_str(), _T("rt"));
    if(f!=NULL)
    {
      szMD5Hash = _fgetts(szMD5Buffer, 64, f);   
      fclose(f);
      _tprintf(_T("Found MD5 file %s; MD5=%s\n"), fd.cFileName, szMD5Hash);
    }    
    else
    {
      _tprintf(_T("The MD5 file is not detected; no integrity check will be performed.\n"));
    }
        
    sFileName = sDirName + fd.cAlternateFileName;
    int res = crpOpenErrorReport(sFileName.c_str(), szMD5Hash, szSymSearchPath, 0, &handle);
    if(res!=0)
    {
      TCHAR buf[1024];
      crpGetLastErrorMsg(buf, 1024);
      _tprintf(_T("Error '%s' while processing file '%s'\n"), buf , fd.cFileName);      
    }
    else
    {
      // Output

      if(szOutput!=NULL)
      {        
        if(bOutputToDir)
        {
          sOutFileName = tstring(szOutput);
          if( sOutFileName[sOutFileName.length()-1]!='\\' )
            sOutFileName += _T("\\"); 

          if(nOutputFormat==OUT_TEXT)
            sOutFileName += tstring(fd.cFileName) + _T(".txt");
          else if(nOutputFormat==OUT_HTML)
            sOutFileName += tstring(fd.cFileName) + _T(".html");
          else if(nOutputFormat==OUT_XML)
            sOutFileName += tstring(fd.cFileName) + _T(".xml");
        }
        else
        {
          if(nOutputFormat==OUT_TEXT)
            sOutFileName = tstring(fd.cFileName) + _T(".txt");
        }              
      }
      else
      {
        f=stdout; // Write output to terminal
      }

      _tfopen_s(&f, sOutFileName.c_str(), _T("wt, ccs=UTF-8"));

      if(f==NULL)
      {
        _tprintf(_T("Error: couldn't open output file '%s' while processing file '%s'\n"), 
          sOutFileName.c_str(), fd.cFileName);      
        goto exit;
      }

      output_document(handle, f, nOutputFormat);
      
      fclose(f);
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

int get_prop(CrpHandle handle, int propid, tstring& str)
{
  const int BUFF_SIZE = 1024;
  TCHAR buffer[BUFF_SIZE];  
  int result = crpGetProperty(handle, propid, 0, buffer, BUFF_SIZE, NULL);
  str = buffer;
  return result;
}

int output_document(CrpHandle handle, FILE* f, int out_format)
{  
  int result = -1;
  COutputter doc;

  doc.Init(f, out_format);
  doc.BeginDocument(_T("ErrorReport"), _T("Error Report"));

  doc.BeginTable(_T("GeneralInfo"), _T("General Information"));
  
  // Print CrashRpt version
  tstring sCrashRptVer;
  result = get_prop(handle, CRP_PROP_CRASHRPT_VERSION, sCrashRptVer);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("Generator version"));
    doc.PutRecord(sCrashRptVer.c_str());
    doc.EndRow();
  }
  
  // Print CrashGUID  
  tstring sCrashGUID;
  result = get_prop(handle, CRP_PROP_CRASH_GUID, sCrashGUID);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("Crash GUID"));
    doc.PutRecord(sCrashGUID.c_str());
    doc.EndRow();
  }

  // Print SystemTimeUTC
  tstring sSystemTimeUTC;
  result = get_prop(handle, CRP_PROP_SYSTEM_TIME_UTC, sSystemTimeUTC);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("Date created (UTC)"));
    doc.PutRecord(sSystemTimeUTC.c_str());
    doc.EndRow();
  }

  // Print AppName
  tstring sAppName;
  result = get_prop(handle, CRP_PROP_APP_NAME, sAppName);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("Application name"));
    doc.PutRecord(sAppName.c_str());
    doc.EndRow();
  }

  // Print AppVersion
  tstring sAppVersion;
  result = get_prop(handle, CRP_PROP_APP_VERSION, sAppVersion);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("Application version"));
    doc.PutRecord(sAppVersion.c_str());
    doc.EndRow();
  }

  // Print ImageName
  tstring sImageName;
  result = get_prop(handle, CRP_PROP_IMAGE_NAME, sImageName);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("Executable image"));
    doc.PutRecord(sImageName.c_str());
    doc.EndRow();
  }

  // Print OperatingSystem
  tstring sOperatingSystem;
  result = get_prop(handle, CRP_PROP_OPERATING_SYSTEM, sOperatingSystem);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("OS name (user's registry)"));
    doc.PutRecord(sOperatingSystem.c_str());
    doc.EndRow();
  }

  tstring sOsVerMajor;
  result = get_prop(handle, CRP_PROP_OS_VER_MAJOR, sOsVerMajor);
  tstring sOsVerMinor;
  result = get_prop(handle, CRP_PROP_OS_VER_MINOR, sOsVerMinor);
  tstring sOsVerBuild;
  result = get_prop(handle, CRP_PROP_OS_VER_BUILD, sOsVerBuild);
  tstring sOsVerCSD;
  result = get_prop(handle, CRP_PROP_OS_VER_CSD, sOsVerCSD);

  tstring sOsVer;
  sOsVer += sOsVerMajor;
  sOsVer += _T(".");
  sOsVer += sOsVerMinor;
  sOsVer += _T(".");
  sOsVer += sOsVerBuild;
  sOsVer += _T(" ");
  sOsVer += sOsVerCSD; 

  doc.BeginRow();
  doc.PutRecord(_T("OS version (minidump)"));
  doc.PutRecord(sOsVer.c_str());
  doc.EndRow();

  // Print SystemType
  tstring sSystemType;
  result = get_prop(handle, CRP_PROP_SYSTEM_TYPE, sSystemType);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("Product type"));
    doc.PutRecord(sSystemType.c_str());
    doc.EndRow();
  }

  // Print UserEmail
  tstring sUserEmail;
  result = get_prop(handle, CRP_PROP_USER_EMAIL, sUserEmail);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("User email"));
    doc.PutRecord(sUserEmail.c_str());
    doc.EndRow();
  }

  // Print ProblemDescription
  tstring sProblemDescription;
  result = get_prop(handle, CRP_PROP_PROBLEM_DESCRIPTION, sProblemDescription);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("Problem description"));
    doc.PutRecord(sProblemDescription.c_str());
    doc.EndRow();
  }

  // Print ProcessorArchitecture
  tstring sProcessorArchitecture;
  result = get_prop(handle, CRP_PROP_CPU_ARCHITECTURE, sProcessorArchitecture);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("CPU architecture"));
    doc.PutRecord(sProcessorArchitecture.c_str());
    doc.EndRow();
  }

  // Print NumberOfProcessors
  tstring sCPUCount;
  result = get_prop(handle, CRP_PROP_CPU_COUNT, sCPUCount);
  if(result==0)
  {    
    doc.BeginRow();
    doc.PutRecord(_T("CPU count"));
    doc.PutRecord(sCPUCount.c_str());
    doc.EndRow();
  }

  doc.EndTable(_T("GeneralInfo"));
  
  

  //// Print file list  
  //result = crpGetProperty(handle, CRP_PROP_FILE_COUNT, 0, buffer, BUFF_SIZE, NULL);
  //if(result==0)
  //{
  //  int nItemCount = _ttoi(buffer);
  //  int i;
  //  for(i=0; i<nItemCount; i++)
  //  {       
  //    int result2 = crpGetProperty(handle, CRP_PROP_FILE_ITEM_NAME, i, buffer, BUFF_SIZE, NULL);
  //    if(result2==0)
  //    {
  //      _ftprintf(f, _T("%d. %16s"), i+1, buffer);
  //      
  //      int result3 = crpGetProperty(handle, CRP_PROP_FILE_ITEM_DESCRIPTION, i, buffer, BUFF_SIZE, NULL);
  //      if(result3==0)
  //        _ftprintf(f, _T("  %s\n"), buffer);
  //      else
  //        _ftprintf(f, _T("\n"));
  //    }
  //    else
  //    {
  //      _ftprintf(f, _T("Failed to retieve file item #%d.\n"), i+1);
  //    }
  //  }
  //}
  //else
  //{
  //  _ftprintf(f, _T("Failed to retieve file item count.\n"));
  //}

  //result = crpGetProperty(handle, CRP_PROP_STACK_FRAME_COUNT, 0, buffer, BUFF_SIZE, NULL);
  //if(result==0)
  //{
  //  int nItemCount = _ttoi(buffer);
  //  int i;
  //  for(i=0; i<nItemCount; i++)
  //  {       
  //    result = crpGetProperty(handle, CRP_PROP_STACK_MODULE_NAME, i, buffer, BUFF_SIZE, NULL);
  //    if(result==0)
  //      _ftprintf(f, _T("%s!"), buffer);
  //    
  //    result = crpGetProperty(handle, CRP_PROP_STACK_SYMBOL_NAME, i, buffer, BUFF_SIZE, NULL);
  //    if(result==0)
  //      _ftprintf(f, _T("%s"), buffer);

  //    result = crpGetProperty(handle, CRP_PROP_STACK_OFFSET_IN_SYMBOL, i, buffer, BUFF_SIZE, NULL);
  //    if(result==0)
  //      _ftprintf(f, _T("+%s"), buffer);
  //      
  //    result = crpGetProperty(handle, CRP_PROP_STACK_SOURCE_FILE, i, buffer, BUFF_SIZE, NULL);
  //    if(result==0)
  //      _ftprintf(f, _T(" %s"), buffer);

  //    result = crpGetProperty(handle, CRP_PROP_STACK_SOURCE_LINE, i, buffer, BUFF_SIZE, NULL);
  //    if(result==0)
  //      _ftprintf(f, _T(":%s"), buffer);

  //    _ftprintf(f, _T("\n"), buffer);    
  //  }
  //}

  //result = crpGetProperty(handle, CRP_PROP_MODULE_COUNT, 0, buffer, BUFF_SIZE, NULL);
  //if(result==0)
  //{
  //  int nItemCount = _ttoi(buffer);
  //  int i;
  //  for(i=0; i<nItemCount; i++)
  //  {
  //    result = crpGetProperty(handle, CRP_PROP_MODULE_NAME, i, buffer, BUFF_SIZE, NULL);
  //    if(result!=0)
  //      continue;

  //    _ftprintf(f, _T("%s"), buffer);

  //    result = crpGetProperty(handle, CRP_PROP_MODULE_SYMBOLS_LOADED, i, buffer, BUFF_SIZE, NULL);
  //    if(result==0)
  //      _ftprintf(f, _T(" %s\n"), buffer);
  //  }
  //}

  doc.EndDocument(_T("ErrorReport"));
  
  return 0;
}

int output_html()
{
  return 0;
}

