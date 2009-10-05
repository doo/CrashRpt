#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "CrashRptProbe.h"

// Character set independent string type
typedef std::basic_string<TCHAR> tstring;

// The following macros are used for parsing the command line
#define args_left() (argc-cur_arg)
#define arg_exists() (cur_arg<argc && argv[cur_arg]!=NULL)
#define get_arg() ( arg_exists() ? argv[cur_arg]:NULL )
#define skip_arg() cur_arg++
#define cmp_arg(val) (arg_exists() && (0==_tcscmp(argv[cur_arg], val)))

// Function prototypes
int process_command(LPTSTR szInput, LPTSTR szInputMD5, LPTSTR szOutput, 
  LPTSTR szSymSearchPath);

int output_document(CrpHandle handle, FILE* f);

// COutputter
// This class is used for generating the content of the resulting file.
// Currently text format is supported.
class COutputter
{
public:

  void Init(FILE* f)
  {
    m_fOut = f;    
  }

  void BeginDocument(LPCTSTR pszTitle)
  {
    _ftprintf(m_fOut, _T("= %s = \n\n"), pszTitle);    
  }

  void EndDocument()
  {    
    _ftprintf(m_fOut, _T("\n== END ==\n"));
  }

  void BeginSection(LPCTSTR pszTitle)
  {
    _ftprintf(m_fOut, _T("== %s ==\n\n"), pszTitle);
  }

  void EndSection()
  {
    _ftprintf(m_fOut, _T("\n\n"));
  }

  void PutRecord(LPCTSTR pszName, LPCTSTR pszValue)
  {
    _ftprintf(m_fOut, _T("%s = %s\n"), pszName, pszValue);
  }

  void PutTableCell(LPCTSTR pszValue, int width, bool bLastInRow)
  {
    TCHAR szFormat[32];
    _stprintf_s(szFormat, 32, _T("%%-%ds%s"), width, bLastInRow?_T("\n"):_T(" "));
    _ftprintf(m_fOut, szFormat, pszValue);
  }
  
private:

  FILE* m_fOut;
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
  _tprintf(_T("   /sym <sym_search_dirs>   Symbol file search directory or list of directories \
separated with semicolon. If this parameter is ommitted, symbols files are searched in current directory.\n"));  
  _tprintf(_T("\nExample: \n"));
  _tprintf(_T("   crprober /f *.zip /o \"D:\\Error Reports\" /of text \n"));
}

// Program entry point
int _tmain(int argc, TCHAR** argv)
{
  int result = 1; // Return code
  int cur_arg = 1; // Current processed cmdline argument

  TCHAR* szInput = NULL;       
  TCHAR* szInputMD5 = NULL;
  TCHAR* szOutput = NULL;  
  TCHAR* szSymSearchPath = NULL;    

  if(args_left()==0)
    goto exit; // There are no arguments?

  // Parse command line arguments
  while(arg_exists())
  {
    if(cmp_arg(_T("/?"))) // help
    {
      // Print usage text
      goto exit;
    }
    else if(cmp_arg(_T("/f"))) // input file name or directory
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
    else if(cmp_arg(_T("/fmd5"))) // md5 file or directory
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
    else if(cmp_arg(_T("/o"))) // output file or directory
    {
      skip_arg();    
      szOutput = get_arg();
      skip_arg();          
    }    
    else if(cmp_arg(_T("/sym"))) // symbol search dir
    {
      skip_arg();    
      szSymSearchPath = get_arg();
      skip_arg();
    }
    else // unknown arg
    {
      _tprintf(_T("Unexpected parameter: %s\n"), get_arg());
      goto exit;
    }    
  }

  // Do the processing work
  result = process_command(szInput, szInputMD5, szOutput, szSymSearchPath); 

exit:

  if(result==1)
  {
    print_usage();
  }

  return result;
}


// Processes crash report file or group of files
int process_command(
  LPTSTR szInput, 
  LPTSTR szInputMD5,
  LPTSTR szOutput,   
  LPTSTR szSymSearchPath)
{
  int result = 2;       // Status
  CrpHandle hReport = 0; // Handle to the error report
  WIN32_FIND_DATA fd;   // Used to enumerate files in directory
  HANDLE hFind = INVALID_HANDLE_VALUE;   
  BOOL bNext = TRUE;    
  tstring sDirName;
  tstring sFileName;
  BOOL bOutputToDir = FALSE; // Do we save resulting files to directory or save single resulting file?  

  // Validate input parameters

  if(szInput==NULL)
  {
    _tprintf(_T("Input file name or pattern is missing.\n"));
    goto exit;
  }
  
  // Determine if user wants us to save resulting files in directory using their respective 
  // file names or if he specifies the file name for the single saved file
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
   
  sDirName = szInput;
  int pos = sDirName.rfind('\\');
  if(pos<0) // There is no back slash in path
    sDirName = _T(""); 
  else if(pos!=(int)sDirName.length()-1) // Append the back slash to dir name
    sDirName = sDirName.substr(0, pos+1);
  
  // Enumerate files in the search directory
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
        
    // Open the error report file
    sFileName = sDirName + fd.cFileName;
    int res = crpOpenErrorReport(sFileName.c_str(), szMD5Hash, szSymSearchPath, 0, &hReport);
    if(res!=0)
    {
      TCHAR buf[1024];
      crpGetLastErrorMsg(buf, 1024);
      _tprintf(_T("Error '%s' while processing file '%s'\n"), buf , fd.cFileName);      
    }
    else
    {
      // Output results
      if(szOutput!=NULL)
      {        
        if(bOutputToDir)
        {
          // Write output to directory
          sOutFileName = tstring(szOutput);
          if( sOutFileName[sOutFileName.length()-1]!='\\' )
            sOutFileName += _T("\\"); 
          sOutFileName += tstring(fd.cFileName) + _T(".txt");
        }
        else
        {
          // Write output to single file
          sOutFileName = tstring(fd.cFileName) + _T(".txt");
        }              

        // Open resulting file
        _tfopen_s(&f, sOutFileName.c_str(), _T("wt, ccs=UTF-8"));
        if(f==NULL)
        {
          _tprintf(_T("Error: couldn't open output file '%s' while processing file '%s'\n"), 
            sOutFileName.c_str(), fd.cFileName);      
          goto exit;
        }
      }
      else
      {
        f=stdout; // Write output to terminal
      }

      if(f==NULL)
      {
        _tprintf(_T("Error: couldn't open output file (unexpected error).\n")); 
        goto exit;
      }

      // Write error report properties to the resulting file
      output_document(hReport, f);
      
      if(f!=stdout)
        fclose(f);
    }

    // Clean up
    if(hReport!=NULL)
    {
      crpCloseErrorReport(hReport);
    }
    
    // Go to the next file
    bNext = FindNextFile(hFind, &fd);
  }
  
  result = 0;

exit:

  if(hFind!=INVALID_HANDLE_VALUE)
    FindClose(hFind);

  if(hReport!=0)
    crpCloseErrorReport(hReport);

  return result;
}

// Helper function thatr etrieves an error report property
int get_prop(CrpHandle handle, CRP_ErrorReportProperty propid, tstring& str, int index=0)
{
  const int BUFF_SIZE = 1024;
  TCHAR buffer[BUFF_SIZE];  
  int result = crpGetProperty(handle, propid, index, buffer, BUFF_SIZE, NULL);
  str = buffer;
  return result;
}

// Writes all error report properties to the file
int output_document(CrpHandle hReport, FILE* f)
{  
  int result = -1;
  COutputter doc;

  doc.Init(f);
  doc.BeginDocument(_T("Error Report"));

  doc.BeginSection(_T("Summary"));
  
  // Print CrashRpt version
  tstring sCrashRptVer;
  result = get_prop(hReport, CRP_PROP_CRASHRPT_VERSION, sCrashRptVer);
  if(result==0)
    doc.PutRecord(_T("Generator version"), sCrashRptVer.c_str());
  
  // Print CrashGUID  
  tstring sCrashGUID;
  result = get_prop(hReport, CRP_PROP_CRASH_GUID, sCrashGUID);
  if(result==0)
    doc.PutRecord(_T("Crash GUID"), sCrashGUID.c_str());
    
  // Print SystemTimeUTC
  tstring sSystemTimeUTC;
  result = get_prop(hReport, CRP_PROP_SYSTEM_TIME_UTC, sSystemTimeUTC);
  if(result==0)
    doc.PutRecord(_T("Date created (UTC)"), sSystemTimeUTC.c_str());

  // Print AppName
  tstring sAppName;
  result = get_prop(hReport, CRP_PROP_APP_NAME, sAppName);
  if(result==0)
    doc.PutRecord(_T("Application name"), sAppName.c_str());
   
  // Print AppVersion
  tstring sAppVersion;
  result = get_prop(hReport, CRP_PROP_APP_VERSION, sAppVersion);
  if(result==0)
    doc.PutRecord(_T("Application version"), sAppVersion.c_str());

  // Print ImageName
  tstring sImageName;
  result = get_prop(hReport, CRP_PROP_IMAGE_NAME, sImageName);
  if(result==0)
    doc.PutRecord(_T("Executable image"), sImageName.c_str());

  // Print OperatingSystem
  tstring sOperatingSystem;
  result = get_prop(hReport, CRP_PROP_OPERATING_SYSTEM, sOperatingSystem);
  if(result==0)
    doc.PutRecord(_T("OS name (from user's registry)"), sOperatingSystem.c_str());
    
  tstring sOsVerMajor;
  result = get_prop(hReport, CRP_PROP_OS_VER_MAJOR, sOsVerMajor);
  tstring sOsVerMinor;
  result = get_prop(hReport, CRP_PROP_OS_VER_MINOR, sOsVerMinor);
  tstring sOsVerBuild;
  result = get_prop(hReport, CRP_PROP_OS_VER_BUILD, sOsVerBuild);
  tstring sOsVerCSD;
  result = get_prop(hReport, CRP_PROP_OS_VER_CSD, sOsVerCSD);

  tstring sOsVer;
  sOsVer += sOsVerMajor;
  sOsVer += _T(".");
  sOsVer += sOsVerMinor;
  sOsVer += _T(".");
  sOsVer += sOsVerBuild;
  sOsVer += _T(" ");
  sOsVer += sOsVerCSD; 
  
  doc.PutRecord(_T("OS version (from minidump)"), sOsVer.c_str());
  
  // Print SystemType
  tstring sSystemType;
  result = get_prop(hReport, CRP_PROP_SYSTEM_TYPE, sSystemType);
  if(result==0)
    doc.PutRecord(_T("Product type"), sSystemType.c_str());
  
  // Print UserEmail
  tstring sUserEmail;
  result = get_prop(hReport, CRP_PROP_USER_EMAIL, sUserEmail);
  if(result==0)
    doc.PutRecord(_T("User email"), sUserEmail.c_str());

  // Print ProblemDescription
  tstring sProblemDescription;
  result = get_prop(hReport, CRP_PROP_PROBLEM_DESCRIPTION, sProblemDescription);
  if(result==0)
    doc.PutRecord(_T("Problem description"), sProblemDescription.c_str());
    
  // Print ProcessorArchitecture
  tstring sProcessorArchitecture;
  result = get_prop(hReport, CRP_PROP_CPU_ARCHITECTURE, sProcessorArchitecture);
  if(result==0)
    doc.PutRecord(_T("CPU architecture"), sProcessorArchitecture.c_str());

  // Print NumberOfProcessors
  tstring sCPUCount;
  result = get_prop(hReport, CRP_PROP_CPU_COUNT, sCPUCount);
  if(result==0)
    doc.PutRecord(_T("CPU count"), sCPUCount.c_str());

  doc.EndSection();
  
  doc.BeginSection(_T("File list"));
  
  // Print file list  
  doc.PutTableCell(_T("#"), 2, false);
  doc.PutTableCell(_T("Name"), 16, false);
  doc.PutTableCell(_T("Description"), 32, true);

  tstring sFileCount;
  result = get_prop(hReport, CRP_PROP_FILE_COUNT, sFileCount);
  if(result==0)
  {
    int nItemCount = _ttoi(sFileCount.c_str());
    int i;
    for(i=0; i<nItemCount; i++)
    { 
      TCHAR szBuffer[10];
      _stprintf_s(szBuffer, 10, _T("%d"), i+1);
      doc.PutTableCell(szBuffer, 2, false);
      tstring sFileName;
      get_prop(hReport, CRP_PROP_FILE_ITEM_NAME, sFileName, i);
      doc.PutTableCell(sFileName.c_str(), 16, false);
      tstring sDesc;
      get_prop(hReport, CRP_PROP_FILE_ITEM_DESCRIPTION, sDesc, i);
      doc.PutTableCell(sDesc.c_str(), 32, true);      
    }
  }
  
  doc.EndSection();

  doc.BeginSection(_T("Stack Trace"));

  doc.PutTableCell(_T("#"), 2, false);
  doc.PutTableCell(_T("Frame"), 32, true);
  
  tstring sStackFrameCount;
  result = get_prop(hReport, CRP_PROP_STACK_FRAME_COUNT, sStackFrameCount);
  if(result==0)
  {    
    int nItemCount = _ttoi(sStackFrameCount.c_str());
    int i;
    for(i=0; i<nItemCount; i++)
    { 
      TCHAR szBuffer[10];
      _stprintf_s(szBuffer, 10, _T("%d"), i+1);
      doc.PutTableCell(szBuffer, 2, false);

      tstring str;

      tstring sModuleName;
      result = get_prop(hReport, CRP_PROP_STACK_MODULE_NAME, sModuleName, i);
      str += sModuleName + _T("!");
            
      tstring sSymName;
      result = get_prop(hReport, CRP_PROP_STACK_SYMBOL_NAME, sSymName, i);
      str += sSymName+ _T("+");
              
      tstring sOffsInSym;
      result = get_prop(hReport, CRP_PROP_STACK_OFFSET_IN_SYMBOL, sOffsInSym, i);
      str += sOffsInSym + _T(" ");
       
      tstring sSrcFile;
      result = get_prop(hReport, CRP_PROP_STACK_SOURCE_FILE, sSrcFile, i);
      
      tstring sSrcLine;
      result = get_prop(hReport, CRP_PROP_STACK_SOURCE_LINE, sSrcLine, i);
      
      doc.PutTableCell(str.c_str(), 32, true);      
    }
  }

  doc.EndSection();

  // Print module list
  doc.BeginSection(_T("Module List"));

  doc.PutTableCell(_T("#"), 2, false);
  doc.PutTableCell(_T("Name"), 32, true);
  
  tstring sModuleCount;
  result = get_prop(hReport, CRP_PROP_MODULE_COUNT, sModuleCount);  
  if(result==0)
  {
    int nItemCount = _ttoi(sModuleCount.c_str());
    int i;
    for(i=0; i<nItemCount; i++)
    {
      TCHAR szBuffer[10];
      _stprintf_s(szBuffer, 10, _T("%d"), i+1);
      doc.PutTableCell(szBuffer, 2, false);

      tstring sModuleName;
      result = get_prop(hReport, CRP_PROP_MODULE_NAME, sModuleName);  
      doc.PutTableCell(sModuleName.c_str(), 32, true);      
    }
  }

  doc.EndDocument();
  
  return 0;
}


