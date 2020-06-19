#ifndef _CRASHRPT_H_
#define _CRASHRPT_H_

#include <windows.h>
#include <dbghelp.h>

#pragma comment(lib, "Version.lib")
#pragma comment(lib, "Rpcrt4.lib")
#pragma comment(lib, "dbghelp.lib")

// Define SAL macros to be empty if some old Visual Studio used
#ifndef __reserved
#define __reserved
#endif

#ifndef __in
#define __in
#endif

#ifndef __in_opt
#define __in_opt
#endif

#ifndef __out_ecount_z
#define __out_ecount_z(x)
#endif

#ifdef __cplusplus
#define CRASHRPT_EXTERNC extern "C"
#else
#define CRASHRPT_EXTERNC
#endif

#define CRASHRPTAPI(rettype) CRASHRPT_EXTERNC rettype WINAPI

// Exception types used in CR_EXCEPTION_INFO::exctype structure member.
#define CR_SEH_EXCEPTION 0           // SEH exception.
#define CR_CPP_TERMINATE_CALL 1      // C++ terminate() call.
#define CR_CPP_UNEXPECTED_CALL 2     // C++ unexpected() call.
#define CR_CPP_PURE_CALL 3           // C++ pure virtual function call (VS .NET and later).
#define CR_CPP_NEW_OPERATOR_ERROR 4  // C++ new operator fault (VS .NET and later).
#define CR_CPP_SECURITY_ERROR 5      // Buffer overrun error (VS .NET only).
#define CR_CPP_INVALID_PARAMETER 6   // Invalid parameter exception (VS 2005 and later).
#define CR_CPP_SIGABRT 7             // C++ SIGABRT signal (abort).
#define CR_CPP_SIGFPE 8              // C++ SIGFPE signal (floating-point exception).
#define CR_CPP_SIGILL 9              // C++ SIGILL signal (illegal instruction).
#define CR_CPP_SIGINT 10             // C++ SIGINT signal (CTRL+C).
#define CR_CPP_SIGSEGV 11            // C++ SIGSEGV signal (invalid storage access).
#define CR_CPP_SIGTERM 12            // C++ SIGTERM signal (termination request).

/* 
*  This structure contains information about the crash.
*  The information provided by this structure includes the exception type, exception code, 
*  exception pointers and so on. These are needed to generate crash mini-dump file and
*  provide the developer with other information about the error. 
*  This structure is used by the crGenerateErrorReport() function. 
*  Pointer to this structure is also passed to the crash callback function (see the PFNCRASHCALLBACK() function prototype).
*
*  Structure members details are provided below:
*
*  cb [in] 
*    This must contain the size of this structure in bytes.
*
*  pexcptrs [in, optional]
*    Should contain the exception pointers. If this parameter is NULL, 
*    the current CPU state is used to generate exception pointers.
*
*  exctype [in]
*    The type of exception. This parameter may be one of the following:
*     - CR_SEH_EXCEPTION             SEH (Structured Exception Handling) exception
*     - CR_CPP_TERMINATE_CALL        C++ terminate() function call
*     - CR_CPP_UNEXPECTED_CALL       C++ unexpected() function call
*     - CR_CPP_PURE_CALL             Pure virtual method call (Visual Studio .NET 2003 and later) 
*     - CR_CPP_NEW_OPERATOR_ERROR    C++ 'new' operator error (Visual Studio .NET 2003 and later)
*     - CR_CPP_SECURITY_ERROR        Buffer overrun (Visual Studio .NET 2003 only) 
*     - CR_CPP_INVALID_PARAMETER     Invalid parameter error (Visual Studio 2005 and later) 
*     - CR_CPP_SIGABRT               C++ SIGABRT signal 
*     - CR_CPP_SIGFPE                C++ floating point exception
*     - CR_CPP_SIGILL                C++ illegal instruction
*     - CR_CPP_SIGINT                C++ SIGINT signal
*     - CR_CPP_SIGSEGV               C++ invalid storage access
*     - CR_CPP_SIGTERM               C++ termination request
* 
*  code [in, optional]
*      Used if exctype is CR_SEH_EXCEPTION and represents the SEH exception code. 
*      If pexptrs is NULL, this value is used when generating exception information for initializing
*      pexptrs->ExceptionRecord->ExceptionCode member, otherwise it is ignored.
*
*  fpe_subcode [in, optional]
*      Used if exctype is equal to CR_CPP_SIGFPE. It defines the floating point
*      exception subcode (see signal() function ducumentation in MSDN).
* 
*  expression, function, file and line [in, optional]
*     These parameters are used when exctype is CR_CPP_INVALID_PARAMETER. 
*     These members are typically non-zero when using debug version of CRT.
*
*  bManual [in]
*     bManual parameter should be equal to TRUE if the report is generated manually. 
*     The value of bManual parameter affects the automatic application restart behavior. If the application
*     restart is requested by the CR_INST_APP_RESTART flag of CR_INSTALL_INFO::dwFlags structure member, 
*     and if bManual is FALSE, the application will be restarted after error report generation. 
*     If bManual is TRUE, the application won't be restarted.
*
*  hCrashReportProcess [out]
*     hCrashReportProcess parameter will contain the handle to the CrashReport.exe process when 
*     crGenerateErrorReport function returns. The caller may use this handle to wait until CrashReport.exe 
*     process exits and check the exit code. When the handle is not needed anymore, release it with the CloseHandle() function.
*/
typedef struct tagCR_EXCEPTION_INFO {
  WORD cb;  // Size of this structure in bytes; should be initialized before using.
  PEXCEPTION_POINTERS pexcptrs;  // Exception pointers.
  int exctype;                   // Exception type.
  DWORD code;                    // Code of SEH exception.
  unsigned int fpe_subcode;      // Floating point exception sub-code.
  const wchar_t* expression;     // Assertion expression.
  const wchar_t* function;       // Function in which assertion happened.
  const wchar_t* file;           // File in which assertion happened.
  unsigned int line;             // Line number.
  BOOL bManual;                  // Flag telling if the error report is generated manually or not.
  HANDLE hCrashReportProcess;    // Handle to the CrashReport.exe process.
} CR_EXCEPTION_INFO;
typedef CR_EXCEPTION_INFO* PCR_EXCEPTION_INFO;




// Stages of crash report generation (used by the crash callback function).
#define CR_CB_STAGE_PREPARE 10  // Stage after exception pointers've been retrieved.
#define CR_CB_STAGE_FINISH 20   // Stage after the launch of CrashSender.exe process.

/*
*  This structure contains information passed to crash callback function PFNCRASHCALLBACK().
*
*  The information contained in this structure may be used by the crash callback function
*  to determine what type of crash has occurred and perform some action. For example,
*  the client application may prefer to continue its execution on some type of crash, and
*  terminate itself on another type of crash.
*
*  Below, the structure fields are described:
*
*  cb [in] 
*    This contains the size of this structure in bytes. 
*
*  nStage [in]
*    This field specifies the crash report generation stage. The callback function
*    can be called once per each stage (depending on callback function's return value).
*    Currently, there are two stages:
*      - CR_CB_STAGE_PREPARE   Stage after exception pointers've been retrieved.
*      - CR_CB_STAGE_FINISH    Stage after the launch of CrashSender.exe process.
*
*  pszErrorReportFolder [in]
*    This field contains the absolute path to the directory containing uncompressed 
*    crash report files. 
*
*  pExceptionInfo [in]
*    This field contains a pointer to CR_EXCEPTION_INFO structure.
*
*  pUserParam [in, optional]
*    This is a pointer to user-specified data passed to the crSetCrashCallback() function as pUserParam argument.
*
*  bContinueExecution [in, out]
*    This field is set to FALSE by default. The crash callback function may set it
*    to true if it wants to continue its execution after crash report generation 
*    (otherwise the program will be terminated).
*/
typedef struct tagCR_CRASH_CALLBACK_INFOW {
  WORD cb;                            // Size of this structure in bytes.
  int nStage;                         // Stage.
  LPCWSTR pszErrorReportFolder;       // Directory where crash report files are located.
  CR_EXCEPTION_INFO* pExceptionInfo;  // Pointer to information about the crash.
  LPVOID pUserParam;                  // Pointer to user-defined data.
  BOOL bContinueExecution;            // Whether to terminate the process (the default) or to continue program execution.
} CR_CRASH_CALLBACK_INFOW;

typedef struct tagCR_CRASH_CALLBACK_INFOA {
  WORD cb;
  int nStage;
  LPCSTR pszErrorReportFolder;
  CR_EXCEPTION_INFO* pExceptionInfo;
  LPVOID pUserParam;
  BOOL bContinueExecution;
} CR_CRASH_CALLBACK_INFOA;





// Constants that may be returned by the crash callback function.
#define CR_CB_CANCEL 0             // Cancel crash report generation on the current stage.
#define CR_CB_DODEFAULT 1          // Proceed to the next stages of crash report generation without calling crash callback function.
#define CR_CB_NOTIFY_NEXT_STAGE 2  // Proceed and call the crash callback for the next stage.

/*
*  The crash callback function is called when a crash occurs. This way client application is notified about the crash.
*
*  Crash information is passed by CrashRpt to the callback function through the pInfo parameter as
*  a pointer to CR_CRASH_CALLBACK_INFO structure. See below for a code example. 
*
*  It is generally unsafe to do complex actions (e.g. memory allocation, heap operations) inside of this callback.
*  The application state may be unstable.
*
*  One reason the application may use this callback for is to close handles to open log files that the 
*  application plans to include into the error report. Files should be accessible for reading, otherwise
*  CrashRpt won't be able to include them into error report.
*
*  It is also possible (but not recommended) to add files (see crAddFile()), 
*  properties (see crAddProperty()), desktop screenshots (see crAddScreenshot())
*  and registry keys (see crAddRegKey()) inside of the crash callback function.
*
*  By default, CrashRpt terminates the client application after crash report generation and
*  launching the CrashReport.exe process. However, it is possible to continue program
*  execution after crash report generation by seting CR_CRASH_CALLBACK_INFO::bContinueExecution structure field to TRUE.
*
*  The crash report generation consists of several stages. First, exception pointers are retrieved
*  and the callback function is called for the first time. The callback function may check the
*  retrieved exception information and decide whether to proceed with crash report generation or to
*  continue client program execution. On the next stage, the CrashReport.exe process is launched and 
*  the crash callback function is (optionally) called for the second time.
*  Further crash report data collection and delivery work is performed in CrashReport.exe process. 
*  The crash callback may use the provided handle to CrashReport.exe process to wait until it exits.
*
*  The crash callback function should typically return CR_CB_DODEFAULT constant to proceed 
*  with error report generation without being called back on the next stage(s). Returning the
*  CR_CB_NOTIFY_NEXT_STAGE constant causes CrashRpt to call the crash callback function on the next stage, too. 
*  Returning CR_CB_CANCEL constant will prevent further stage(s) of crash report generation.
*
*  The following code example shows the simplest way of using the crash callback function:
*
*  // Define the crash callback
*  int CALLBACK CrashCallback(CR_CRASH_CALLBACK_INFO* pInfo)
*  {    
*     
*     // Do something...
*
*     // Proceed with crash report generation. 
*     // This return code also makes CrashRpt to not call this callback function for 
*     // the next crash report generation stage.
*     return CR_CB_DODEFAULT;
*  }
*
*  The following code example shows how to use the crash callback function to be notified
*  on every stage of crash report generation:
*
*  // Define the crash callback
*  int CALLBACK CrashCallback(CR_CRASH_CALLBACK_INFO* pInfo)
*  {    
*     
*     // We want to continue program execution after crash report generation
*     pInfo->bContinueExecution = TRUE;
*
*     switch(pInfo->nStage)
*     {
*         case CR_CB_STAGE_PREPARE:
*               // do something
*               break;
*         case CR_CB_STAGE_FINISH:
*               // do something
*               break;
*     }
*
*     // Proceed to the next stage. 
*     return CR_CB_NOTIFY_NEXT_STAGE;
*  }
*/
typedef int(CALLBACK* PFNCRASHCALLBACKW)(CR_CRASH_CALLBACK_INFOW* pInfo);
typedef int(CALLBACK* PFNCRASHCALLBACKA)(CR_CRASH_CALLBACK_INFOA* pInfo);

#ifdef UNICODE
typedef CR_CRASH_CALLBACK_INFOW CR_CRASH_CALLBACK_INFO;
typedef PFNCRASHCALLBACKW PFNCRASHCALLBACK;
#else
typedef PFNCRASHCALLBACKA PFNCRASHCALLBACK;
typedef CR_CRASH_CALLBACK_INFOA CR_CRASH_CALLBACK_INFO;
#endif  // UNICODE

/* Sets the crash callback function.
*  This function returns zero if succeeded. Use crGetLastErrorMsg() to retrieve the error message on fail.
*
*  [in] pfnCallbackFunc  Pointer to the crash callback function.
*  [in] lpParam          User defined parameter. Optional. 
*/
CRASHRPTAPI(int) crSetCrashCallbackW(PFNCRASHCALLBACKW pfnCallbackFunc, LPVOID lpParam);
CRASHRPTAPI(int) crSetCrashCallbackA(PFNCRASHCALLBACKA pfnCallbackFunc, LPVOID lpParam);




// Flags for CR_INSTALL_INFO::dwFlags
#define CR_INST_SEH_EXCEPTION_HANDLER 0x1  // Install SEH handler.
#define CR_INST_TERMINATE_HANDLER 0x2      // Install terminate handler.
#define CR_INST_UNEXPECTED_HANDLER 0x4     // Install unexpected handler.
#define CR_INST_PURE_CALL_HANDLER 0x8      // Install pure call handler (VS .NET and later).
#define CR_INST_NEW_OPERATOR_ERROR_HANDLER 0x10 // Install new operator error handler (VS .NET and later).
#define CR_INST_SECURITY_ERROR_HANDLER 0x20     // Install security error handler (VS .NET and later).
#define CR_INST_INVALID_PARAMETER_HANDLER 0x40  // Install invalid parameter handler (VS 2005 and later).
#define CR_INST_SIGABRT_HANDLER 0x80    // Install SIGABRT signal handler.
#define CR_INST_SIGFPE_HANDLER 0x100    // Install SIGFPE signal handler.
#define CR_INST_SIGILL_HANDLER 0x200    // Install SIGILL signal handler.
#define CR_INST_SIGINT_HANDLER 0x400    // Install SIGINT signal handler.
#define CR_INST_SIGSEGV_HANDLER 0x800   // Install SIGSEGV signal handler.
#define CR_INST_SIGTERM_HANDLER 0x1000  // Install SIGTERM signal handler.

#define CR_INST_ALL_POSSIBLE_HANDLERS 0x1FFF  // Install all possible exception handlers.
#define CR_INST_CRT_EXCEPTION_HANDLERS 0x1FFE // Install exception handlers for the linked CRT module.

#define CR_INST_NO_GUI 0x2000  //!< Do not show GUI, send report silently (use for non-GUI apps only).

#define CR_INST_APP_RESTART 0x10000  // Restart the application on crash.
#define CR_INST_NO_MINIDUMP 0x20000  // Do not include minidump file to crash report.
#define CR_INST_STORE_ZIP_ARCHIVES 0x80000     // CrashRpt should store both uncompressed error report files and ZIP archives.
#define CR_INST_AUTO_THREAD_HANDLERS 0x800000  // If this flag is set, installs exception handlers for newly created threads automatically.

/*   
* This structure defines the general information used by crInstallW() function.
* Below, structure members are described in details. Required parameters must always be specified, while optional
* ones may be set with 0 (zero) or NULL. Most of parameters are optional.
*
* cb [in, required] 
*       This must contain the size of this structure in bytes. 
*
* pszAppName [in, optional] 
*       This is the friendly name of the client application. The application name is
*       displayed in the Error Report dialog. If this parameter is NULL, the name of EXE file 
*       that was used to start caller process becomes the application name.
*
* pszAppVersion [in, optional] 
*       Should be the application version. Example: "1.0.1". 
*       If this equals to NULL, product version is extracted from the executable file which started 
*       the caller process, and this product version is used as application version. If the executable file
*       doesn's have a version info resource, the crInstall() function will fail.
*
* pszCrashReportPath [in, optional] 
*       This is the absolute path to the directory where CrashReport.exe is located. 
*       The crash sender process is responsible for letting end user know about the crash and 
*       sending the error report. If this is NULL, it is assumed that CrashReport.exe is located in
*       the same directory as call process.
*
* dwFlags [in, optional]
*      dwFlags can be used to define behavior parameters. This can be a combination of the following values:
*
*      Use the combination of the following constants to specify what exception handlers to install:
*        CR_INST_ALL_POSSIBLE_HANDLERS          Install all available exception handlers.
*        CR_INST_SEH_EXCEPTION_HANDLER          Install SEH exception handler.
*        CR_INST_PURE_CALL_HANDLER              Install pure call handler (VS .NET and later).
*        CR_INST_NEW_OPERATOR_ERROR_HANDLER     Install new operator error handler (VS .NET and later).
*        CR_INST_SECURITY_ERROR_HANDLER         Install security error handler (VS .NET and later).
*        CR_INST_INVALID_PARAMETER_HANDLER      Install invalid parameter handler (VS 2005 and later).
*        CR_INST_SIGABRT_HANDLER                Install SIGABRT signal handler.
*        CR_INST_SIGINT_HANDLER                 Install SIGINT signal handler.  
*        CR_INST_SIGTERM_HANDLER                Install SIGTERM signal handler.  
*    Use the combination of the following constants to define behavior parameters:</i>
*        CR_INST_NO_GUI                Do not show GUI.
*        CR_INST_DONT_SEND_REPORT     
*            This parameter means do not send error report immediately on crash, just save it locally. 
*            Use this if you have direct access to the machine where crash happens and do not need to send report over the Internet. 
*            You can use this in couple with CR_INST_STORE_ZIP_ARCHIVES flag to store zipped error reports along with uncompressed error report files.
*        CR_INST_APP_RESTART     
*            This parameter allows to automatically restart the application on crash. 
*            The command line for the application is taken from pszRestartCmdLine parameter. 
*            To avoid cyclic restarts of an application which crashes on startup, the application is restarted only if at least 60 seconds elapsed since its start.
*        CR_INST_NO_MINIDUMP     
*            Specify this parameter if you want mini-dump file not to be included into crash report. 
*            The default behavior is to include the mini-dump file.
*
*        CR_INST_STORE_ZIP_ARCHIVES     
*            This parameter can be used in couple with CR_INST_DONT_SEND_REPORT flag to store not only uncompressed
*            error report files, but also ZIP archives. By default (if this flag omitted) CrashRpt stores all error report files
*            in uncompressed state.
*
*        CR_INST_AUTO_THREAD_HANDLERS     
*            Specifying this flag results in automatic installation of all available exception handlers to
*            all threads that will be created in the future. This flag only works if CrashRpt is compiled as a DLL, it does 
*            not work if you compile CrashRpt as static library.
*
* pszDebugHelpDLL [in, optional]  
*     This parameter defines the location of the dbghelp.dll to load. 
*     If this parameter is NULL, the dbghelp.dll is searched using the default search sequence.
*
* uMiniDumpType [in, optional] 
*     This parameter defines the minidump type. For the list of available minidump
*     types, see the documentation for the MiniDumpWriteDump() function in MSDN. 
*
*     It is recommended to set this parameter with zero (equivalent of MiniDumpNormal constant). 
*     Other values may increase the minidump size significantly. 
*
* pszErrorReportSaveDir [in, optional] 
*     This parameter defines the directory where to save the error reports. 
*     If this is NULL, the default directory is used (%%LOCAL_APP_DATA%\\CrashReports\\%%AppName%%_%%AppVersion%).
*
* pszRestartCmdLine [in, optional] 
*     This parameter defines the string that specifies the command-line arguments for the application when it is restarted (when using CR_INST_APP_RESTART flag). 
*     Do not include the name of the executable in the command line; it is added automatically. This parameter can be NULL.
* 
* pszCustomCrashReportIcon [in, optional] 
*     This parameter can be used to define a custom icon for Error Report dialog. 
*
*     The value of this parameter should be absolute path to the module containing the icon resource, followed 
*     by resource identifier separated by comma. You can set this parameter with NULL to use the default icon.  
*
*     The resource identifier is a zero-based index of the icon to retrieve. For example, if this value is 0, 
*     the first icon in the specified file is used. If the identifier is a negative number not equal to -1, 
*     the icon in the specified file whose resource identifier is equal to the absolute value of the resource 
*     identifier is used.
*     Example: "D:\MyApp\Resources.dll, -128". 
*
* nRestartTimeout [in, optional] 
*     This parameter defines the timeout (in seconds) for the application restart (when using CR_INST_APP_RESTART flag). 
*     It is recommended to set this with zero (in such a case, the default restart timeout of 60 seconds is applied). 
*/
typedef struct tagCR_INSTALL_INFOW {
  WORD cb;                  // Size of this structure in bytes; must be initialized before using!
  LPCWSTR pszAppName;       // Name of application.
  LPCWSTR pszAppVersion;    // Application version.
  LPCWSTR pszCrashReportPath;     // Directory name where CrashReport.exe is located.
  UINT uPriorities[5];            // Array of error sending transport priorities.
  DWORD dwFlags;                  // Flags.
  LPCWSTR pszDebugHelpDLL;        // File name or folder of Debug help DLL.
  MINIDUMP_TYPE uMiniDumpType;    // Mini-dump type.
  LPCWSTR pszErrorReportSaveDir;  // Directory where to save error reports.
  LPCWSTR pszRestartCmdLine;      // Command line for application restart (without executable name).
  LPCWSTR pszCustomCrashReportIcon;  // Custom icon used for Error Report dialog.
  int nRestartTimeout;               // Timeout for application restart.
} CR_INSTALL_INFOW;

typedef struct tagCR_INSTALL_INFOA {
  WORD cb;
  LPCSTR pszAppName;
  LPCSTR pszAppVersion;
  LPCSTR pszCrashSenderPath;
  UINT uPriorities[5];
  DWORD dwFlags;
  LPCSTR pszDebugHelpDLL;
  MINIDUMP_TYPE uMiniDumpType;
  LPCSTR pszErrorReportSaveDir;
  LPCSTR pszRestartCmdLine;
  LPCSTR pszCustomSenderIcon;
  int nRestartTimeout;
} CR_INSTALL_INFOA;

typedef CR_INSTALL_INFOW* PCR_INSTALL_INFOW;
typedef CR_INSTALL_INFOA* PCR_INSTALL_INFOA;


/*
*  Installs exception handlers for the caller process.
*  This function returns zero if succeeded.
*
*  [in] pInfo General congiration information.
*
*  remarks:
*    This function installs unhandled exception filter for the caller process.
*    It also installs various CRT exception/error handlers that function for all threads of the caller process.
*    For more information, see exception_handling
*
*    Below is the list of installed handlers:
*     - Top-level SEH exception filter [ SetUnhandledExceptionFilter() ]
*     - C++ pure virtual call handler (Visual Studio .NET 2003 and later) [ _set_purecall_handler() ]
*     - C++ invalid parameter handler (Visual Studio .NET 2005 and later) [ _set_invalid_parameter_handler() ]
*     - C++ new operator error handler (Visual Studio .NET 2003 and later) [ _set_new_handler() ]
*     - C++ buffer overrun handler (Visual Studio .NET 2003 only) [ _set_security_error_handler() ]
*     - C++ abort handler [ signal(SIGABRT) ]
*     - C++ illegal instruction handler [ signal(SIGINT) ]
*     - C++ termination request [ signal(SIGTERM) ]
*
*    In a multithreaded program, additionally use crInstallToCurrentThread() function for each execution
*    thread, except the main one.
* 
*    The pInfo parameter contains all required information needed to install CrashRpt.
*
*    This function fails when pInfo->pszCrashReportPath doesn't contain valid path to CrashReport.exe
*    or when pInfo->pszCrashReportPath is equal to NULL, but CrashReport.exe is not located in the
*    directory where caller process located.
*
*    On crash, the crash mini-dump file is created, which contains CPU information and stack trace information. 
*    Also XML file is created that contains additional information that may be helpful for crash analysis. 
*    These files along with several additional files added with crAddFile() are packed to a single ZIP file.
*
*    When crash information is collected, another process, CrashReport.exe, is launched and the process where crash had occurred is terminated. 
*    The CrashReport process is responsible for letting the user know about the crash and send the error report.
*
*    If this function fails, use crGetLastErrorMsg() to retrieve the error message.
*/
CRASHRPTAPI(int) crInstallW(__in PCR_INSTALL_INFOW pInfo);
CRASHRPTAPI(int) crInstallA(__in PCR_INSTALL_INFOA pInfo);


/*
* Uninitializes the CrashRpt library and unsinstalls exception handlers previously installed with crInstall().
* This function returns zero if succeeded.
*
*  remarks:
*    Call this function on application exit to uninitialize the library and uninstall exception
*    handlers previously installed with crInstall(). After function call, the exception handlers
*    are restored to states they had before calling crInstall().
*
*    This function fails if crInstall() wasn't previously called in context of the caller process.
*    When this function fails, use the crGetLastErrorMsg() function to retrieve the error message.
*/
CRASHRPTAPI(int) crUninstall();


/*
* Installs exception handlers to the caller thread. This function returns zero if succeeded.
*
* [in] dwFlags Flags.
*
* remarks:
*
*  The function sets exception handlers for the caller thread. 
*  If you have several execution threads, you ought to call the function for each thread, except the main one.
*   
*  dwFlags defines what exception handlers to install. Use zero value to install all possible exception handlers. 
*  Or use a combination of the following constants:
*      - CR_INST_TERMINATE_HANDLER              Install terminate handler
*      - CR_INST_UNEXPECTED_HANDLER             Install unexpected handler
*      - CR_INST_SIGFPE_HANDLER                 Install SIGFPE signal handler   
*      - CR_INST_SIGILL_HANDLER                 Install SIGILL signal handler  
*      - CR_INST_SIGSEGV_HANDLER                Install SIGSEGV signal handler 
* 
*  Example:
*
*   DWORD WINAPI ThreadProc(LPVOID lpParam)
*   {
*     // Install exception handlers
*     crInstallToCurrentThread(0);
*
*     // Your code...
*
*     // Uninstall exception handlers
*     crUninstallFromCurrentThread();
*    
*     return 0;
*   }
*/
CRASHRPTAPI(int) crInstallToCurrentThread(DWORD dwFlags);


/*
* Uninstalls C++ exception handlers from the current thread. This function returns zero if succeeded.
*  
* remarks:
*    This function unsets exception handlers from the caller thread.
*    If you have several execution threads, you ought to call the function for each thread.
*    After calling this function, the exception handlers for current thread are
*    replaced with the handlers that were before call of crInstallToCurrentThread().
*
*    This function fails if crInstallToCurrentThread() wasn't called for current thread.
*
*    When this function fails, use crGetLastErrorMsg() to retrieve the error message.
*
*    No need to call this function for the main execution thread. The crUninstall()
*    will automatically uninstall C++ exception handlers for the main thread.
*/
CRASHRPTAPI(int) crUninstallFromCurrentThread();






// Flags for crAddFile() function.
#define CR_AF_TAKE_ORIGINAL_FILE 0  // Take the original file (do not copy it to the error report folder).
#define CR_AF_FILE_MUST_EXIST 0     // Function will fail if file doesn't exist at the moment of function call.
#define CR_AF_MAKE_FILE_COPY 1  // Copy the file to the error report folder.
#define CR_AF_MISSING_FILE_OK 2  // Do not fail if file is missing (assume it will be created later).

/*
* Adds a file to crash report. This function returns zero if succeeded.
*
*  [in] pszFile     Absolute path to the file (or file search pattern) to add to crash report, required.
*  [in] pszDestFile Destination file name, optional.
*  [in] pszDesc     File description (used in Error Report Details dialog), optional.
*  [in] dwFlags     Flags, optional.
*
*    This function can be called anytime after crInstall() to add one or more files to the generated crash report. 
*  
*    When this function is called, the file is marked to be added to the error report, then the function returns control to the caller.
*    When a crash occurs, all marked files are added to the report by the CrashReport.exe process. 
*    If a file is locked by someone for exclusive access, the file won't be included. 
*    Inside of PFNCRASHCALLBACK() crash callback, close open file handles and ensure files to be included are accessible for reading.
*
*    pszFile should be either a valid absolute path to the file or a file search pattern (e.g. "*.log") to be added to crash report. 
*
*    pszDestFile should be the name of destination file. This parameter can be used
*    to specify different file name for the file in ZIP archive. If this parameter is NULL, the pszFile
*    file name is used as destination file name. If pszFile is a search pattern, this argument is ignored.
*
*    pszDesc is a short description of the file. It can be NULL.
*
*    dwFlags parameter defines the behavior of the function. This can be a combination of the following flags:
*       - CR_AF_TAKE_ORIGINAL_FILE  On crash, the CrashReport.exe process will try to locate the file from its original location. This behavior is the default one.
*       - CR_AF_MAKE_FILE_COPY      On crash, the CrashReport.exe process will make a copy of the file and save it to the error report folder.  
*       - CR_AF_FILE_MUST_EXIST     The function will fail if file doesn't exist at the moment of function call (the default behavior). 
*       - CR_AF_MISSING_FILE_OK     The function will not fail if file is missing (assume it will be created later).
*
*    This function fails if pszFile doesn't exist at the moment of function call, unless you specify CR_AF_MISSING_FILE_OK flag. 
*/

CRASHRPTAPI(int) crAddFileW(LPCWSTR pszFile, LPCWSTR pszDestFile, LPCWSTR pszDesc, DWORD dwFlags);
CRASHRPTAPI(int) crAddFileA(LPCSTR pszFile, LPCSTR pszDestFile, LPCSTR pszDesc, DWORD dwFlags);



// Flags for crAddScreenshot function.
#define CR_AS_VIRTUAL_SCREEN 0   // Take a screenshot of the virtual screen.
#define CR_AS_MAIN_WINDOW 1      // Take a screenshot of application's main window.
#define CR_AS_PROCESS_WINDOWS 2  // Take a screenshot of all visible process windows.
#define CR_AS_GRAYSCALE_IMAGE 4  // Make a grayscale image instead of a full-color one.
#define CR_AS_USE_JPEG_FORMAT 8  // Store screenshots as JPG files.


/*
* Adds a screenshot to the crash report.
* This function returns zero if succeeded. Use crGetLastErrorMsg() to retrieve the error message on fail.
*
*  [in] dwFlags Flags, optional.
*  [in] nJpegQuality Defines the JPEG image quality, optional.
*  
*  remarks:
* 
*  This function can be used to take a screenshot at the moment of crash and add it to the error report. 
*  Screenshot information may help the developer to better understand state of the application at the moment of crash and reproduce the error.
*
*  When this function is called, screenshot flags are saved, then the function returns control to the caller.
*  When crash occurs, screenshot is made by the CrashReport.exe process and added to the report. 
* 
*  dwFlags 
*    Use one of the following constants to specify what part of virtual screen to capture:
*    - CR_AS_VIRTUAL_SCREEN  Use this to take a screenshot of the whole desktop (virtual screen).
*    - CR_AS_MAIN_WINDOW     Use this to take a screenshot of the main application main window.
*    - CR_AS_PROCESS_WINDOWS Use this to take a screenshot of all visible windows that belong to the process.
* 
*  The main application window is a window that has a caption (WS_CAPTION), system menu (WS_SYSMENU) and
*  the WS_EX_APPWINDOW extended style. If CrashRpt doesn't find such window, it considers the first found process window as the main window.
*
*  Screenshots are added in form of PNG files by default. 
*  You can specify the CR_AS_USE_JPEG_FORMAT flag to save screenshots as JPEG files instead. 
*
*  If you use JPEG format, you can use the nJpegQuality parameter to define the JPEG image quality. 
*  This should be the number between 0 and 100, inclusively. The bigger the number, the better the quality and the bigger the JPEG file size. 
*  If you use PNG file format, this parameter is ignored.
*
*  In addition, you can specify the CR_AS_GRAYSCALE_IMAGE flag to make a grayscale screenshot (by default color image is made). 
*  Grayscale image gives smaller file size.
*
*  When capturing entire desktop consisting of several monitors, one screenshot file is added per each monitor. 
*
*  You should be careful when using this feature, because screenshots may contain user-identifying 
*  or private information. Always specify purposes you will use collected information for in your Privacy Policy. 
*/
CRASHRPTAPI(int) crAddScreenshot(DWORD dwFlags, int nJpegQuality);



/*
* Adds a string property to the crash report. 
* This function returns zero if succeeded. Use crGetLastErrorMsg() to retrieve the error message on fail.
*
*  [in] pszPropName   Name of the property, required.
*  [in] pszPropValue  Value of the property, required.
*  
*  remarks:
*
*  Use this function to add a string property to the crash description XML file.
*  User-added properties are listed under \<CustomProps\> tag of the XML file.
*  In the XML file properties are ordered by names in alphabetic order.
*/
CRASHRPTAPI(int) crAddPropertyW(LPCWSTR pszPropName, LPCWSTR pszPropValue);
CRASHRPTAPI(int) crAddPropertyA(LPCSTR pszPropName, LPCSTR pszPropValue);



/*
* Adds a registry key dump to the crash report.
* This function returns zero if succeeded. Use crGetLastErrorMsg() to retrieve the error message on fail.
*
*  [in] pszRegKey        Registry key to dump, required.
*  [in] pszDstFileName   Name of the destination file, required. 
*  [in] dwFlags          Flags, reserved.
*  
*  remarks:
*
*  Use this function to add a dump of a Windows registry key into the crash report. 
*
*  The pszRegKey parameter must be the name of the registry key. The key name should begin with "HKEY_CURRENT_USER"
*  or "HKEY_LOCAL_MACHINE". Other root keys are not supported.
*
*  The content of the key specified by the pszRegKey parameter will be stored in a human-readable XML
*  format and included into the error report as pszDstFileName destination file. You can dump multiple registry keys
*  to the same destination file.
*
*  The dwFlags parameter can be set to zero.
*/
CRASHRPTAPI(int) crAddRegKeyW(LPCWSTR pszRegKey, LPCWSTR pszDstFileName, DWORD dwFlags);
CRASHRPTAPI(int) crAddRegKeyA(LPCSTR pszRegKey, LPCSTR pszDstFileName, DWORD dwFlags);



/*
* Manually generates an error report.
* This function returns zero if succeeded. When failed, it returns a non-zero value. Use crGetLastErrorMsg() function to retrieve the error message.
*  
*  [in] pExceptionInfo Exception information. 
*
*  remarks:
*    Call this function to manually generate a crash report. When crash information is collected, control is returned to the caller. 
*    The crGenerateErrorReport() doesn't terminate the caller process.
*
*    The crash report may contain the crash minidump file, crash description file in XML format and
*    additional custom files added with a function like crAddFile().
*
*    The exception information should be passed using CR_EXCEPTION_INFO structure. 
*
*    The following example shows how to use crGenerateErrorReport() function.
*
*    CR_EXCEPTION_INFO ei;
*    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
*    ei.cb = sizeof(CR_EXCEPTION_INFO);
*    ei.exctype = CR_SEH_EXCEPTION;
*    ei.code = 1234;
*    ei.pexcptrs = NULL;
*
*    int result = crGenerateErrorReport(&ei);
*
*    if(result!=0)
*    {
*      // If goes here, crGenerateErrorReport() has failed, get the last error message
*      TCHAR szErrorMsg[256];
*      crGetLastErrorMsg(szErrorMsg, 256);
*    }
*   
*    // Manually terminate program
*    ExitProcess(0);
*/
CRASHRPTAPI(int) crGenerateErrorReport(__in_opt CR_EXCEPTION_INFO* pExceptionInfo);




/*
* Can be used as a SEH exception filter.
* This function returns EXCEPTION_EXECUTE_HANDLER if succeeds; otherwise EXCEPTION_CONTINUE_SEARCH.
*
*  [in] code Exception code.
*  [in] ep   Exception pointers.
*
*  remarks:
*     This function can be called instead of a SEH exception filter
*     inside of __try{}__except(Expression){} construction. The function generates an error report
*     and returns control to the exception handler block.
*
*     The exception code is usually retrieved with GetExceptionCode() intrinsic function
*     and the exception pointers are retrieved with GetExceptionInformation() intrinsic function.
*
*     If an error occurs, this function returns EXCEPTION_CONTINUE_SEARCH. Use crGetLastErrorMsg() to retrieve the error message on fail.
*
*     The following example shows how to use crExceptionFilter().
*    
*     int* p = NULL;   // pointer to NULL
*     __try
*     {
*        *p = 13; // causes an access violation exception;
*     }
*     __except(crExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
*     {   
*       // Terminate program
*       ExitProcess(1);
*     }
*/
CRASHRPTAPI(int) crExceptionFilter(unsigned int code, __in_opt struct _EXCEPTION_POINTERS* ep);




// Flags used by crEmulateCrash() function
#define CR_NONCONTINUABLE_EXCEPTION 32  // Non continuable software exception.
#define CR_THROW 33                     // Throw C++ typed exception.
#define CR_STACK_OVERFLOW 34            // Stack overflow.

/*
* Emulates a predefined crash situation.
* This function doesn't return if succeeded. If failed, returns non-zero value. 
* Call crGetLastErrorMsg() to get the last error message.
*
*  [in] ExceptionType Type of crash.
*
*  remarks:
*    This function uses some a priori incorrect or vulnerable code or raises a C++ signal or raises an non-continuable software exception to cause crash.
*    This function can be used to test if CrashRpt handles a crash situation correctly.
*    
*    CrashRpt will intercept an error or exception if crInstall() and/or crInstallToCurrentThread() were previously called. 
*    crInstall() installs exception handlers that function on per-process basis.
*    crInstallToCurrentThread() installs exception handlers that function on per-thread basis.
*    
*  ExceptionType can be one of the following constants:
*    - CR_SEH_EXCEPTION  This will generate a null pointer exception.
*    - CR_CPP_TERMINATE_CALL This results in call of terminate() C++ function.
*    - CR_CPP_UNEXPECTED_CALL This results in call of unexpected() C++ function.
*    - CR_CPP_PURE_CALL This emulates a call of pure virtual method call of a C++ class instance (Visual Studio .NET 2003 and later).
*    - CR_CPP_NEW_OPERATOR_ERROR This emulates C++ new operator failure (Visual Studio .NET 2003 and later).
*    - CR_CPP_SECURITY_ERROR This emulates copy of large amount of data to a small buffer (Visual Studio .NET 2003 only).
*    - CR_CPP_INVALID_PARAMETER This emulates an invalid parameter C++ exception (Visual Studio 2005 and later). 
*    - CR_CPP_SIGABRT This raises SIGABRT signal (abnormal program termination).
*    - CR_CPP_SIGFPE This causes floating point exception.
*    - CR_CPP_SIGILL This raises SIGILL signal (illegal instruction signal).
*    - CR_CPP_SIGINT This raises SIGINT signal.
*    - CR_CPP_SIGSEGV This raises SIGSEGV signal.
*    - CR_CPP_SIGTERM This raises SIGTERM signal (program termination request).
*    - CR_NONCONTINUABLE_EXCEPTION This raises a non-continuable software exception (expected result is the same as in CR_SEH_EXCEPTION).
*    - CR_THROW This throws a C++ typed exception (expected result is the same as in CR_CPP_TERMINATE_CALL).
*    - CR_STACK_OVERFLOW This causes stack overflow.
*
*  The CR_SEH_EXCEPTION uses null pointer write operation to cause the access violation.
*
*  The CR_NONCONTINUABLE_EXCEPTION has the same effect as CR_SEH_EXCEPTION, but it uses RaiseException() WinAPI function to raise non-continuable software exception.
*
*  The following example shows how to use crEmulateCrash() function.
*
*  // emulate null pointer exception (access violation)
*  crEmulateCrash(CR_SEH_EXCEPTION);
*/

CRASHRPTAPI(int) crEmulateCrash(unsigned ExceptionType) throw(...);



/*
* Gets the last CrashRpt error message.
* This function returns length of error message in characters. If output buffer is invalid, returns a negative number.
*
*  [out] pszBuffer Pointer to the buffer.
*  [in]  uBuffSize Size of buffer in characters.
*
*  remarks:
*    This function gets the last CrashRpt error message. 
*    You can use this function to retrieve the text status of the last called CrashRpt function.
*
*    If buffer is too small for the error message, the message is truncated.
*/
CRASHRPTAPI(int) crGetLastErrorMsgW(__out_ecount_z(uBuffSize) LPWSTR pszBuffer, UINT uBuffSize);
CRASHRPTAPI(int) crGetLastErrorMsgA(__out_ecount_z(uBuffSize) LPSTR pszBuffer, UINT uBuffSize);




// Helper wrapper classes
#ifndef _CRASHRPT_NO_WRAPPERS
class CrAutoInstallHelper {
 public:
  //! Installs exception handlers to the caller process
  CrAutoInstallHelper(__in PCR_INSTALL_INFOA pInfo) { m_nInstallStatus = crInstallA(pInfo); }

  //! Installs exception handlers to the caller process
  CrAutoInstallHelper(__in PCR_INSTALL_INFOW pInfo) { m_nInstallStatus = crInstallW(pInfo); }

  //! Uninstalls exception handlers from the caller process
  ~CrAutoInstallHelper() {
    if (m_nInstallStatus == 0)
      crUninstall();
  }

  //! Install status
  int m_nInstallStatus;
};

class CrThreadAutoInstallHelper {
 public:
  //! Installs exception handlers to the caller thread
  CrThreadAutoInstallHelper(DWORD dwFlags = 0) {
    m_nInstallStatus = crInstallToCurrentThread(dwFlags);
  }

  //! Uninstalls exception handlers from the caller thread
  ~CrThreadAutoInstallHelper() {
    if (m_nInstallStatus == 0)
      crUninstallFromCurrentThread();
  }

  //! Install status
  int m_nInstallStatus;
};

#endif  //!_CRASHRPT_NO_WRAPPERS


#ifdef UNICODE
typedef CR_INSTALL_INFOW CR_INSTALL_INFO;
typedef PCR_INSTALL_INFOW PCR_INSTALL_INFO;
#define crInstall crInstallW
#define crAddFile crAddFileW
#define crAddProperty crAddPropertyW
#define crAddRegKey crAddRegKeyW
#define crGetLastErrorMsg crGetLastErrorMsgW
#define crSetCrashCallback crSetCrashCallbackW
#else
typedef CR_INSTALL_INFOA CR_INSTALL_INFO;
typedef PCR_INSTALL_INFOA PCR_INSTALL_INFO;
#define crInstall crInstallA
#define crAddFile crAddFileA
#define crAddProperty crAddPropertyA
#define crAddRegKey crAddRegKeyA
#define crGetLastErrorMsg crGetLastErrorMsgA
#define crSetCrashCallback crSetCrashCallbackA
#endif

#endif  //_CRASHRPT_H_
