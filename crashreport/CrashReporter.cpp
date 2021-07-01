#include "stdafx.h"
#include "CrashReporter.h"
#include "CrashRpt.h"
#include "Utility.h"
#include "zip.h"
#include "CrashInfoReader.h"
#include "strconv.h"
#include "ScreenCap.h"
#include <sys/stat.h>

CrashReporter* CrashReporter::m_pInstance = NULL;

CrashReporter::CrashReporter() {
  m_nStatus = 0;
  m_nCurReport = 0;
  m_SendAttempt = 0;
  m_bExport = FALSE;
  m_bErrors = FALSE;
}

CrashReporter::~CrashReporter() {
  Finalize();
}

CrashReporter* CrashReporter::GetInstance() {
  // Return singleton object
  if (m_pInstance == NULL)
    m_pInstance = new CrashReporter();
  return m_pInstance;
}

BOOL CrashReporter::Init(LPCTSTR szFileMappingName) {
  m_sErrorMsg = _T("Unspecified error.");

  // Read crash information from the file mapping object.
  int nInit = m_CrashInfo.Init(szFileMappingName);
  if (nInit != 0) {
    m_sErrorMsg.Format(_T("Error reading crash info: %s"), m_CrashInfo.GetErrorMsg().GetBuffer(0));
    return FALSE;
  }

  // Check if another instance of CrashSender.exe is running.
  ::CreateMutex(NULL, FALSE, _T("Local\\43773530-129a-4298-88f2-20eea3e4a59b"));
  if (::GetLastError() == ERROR_ALREADY_EXISTS) {
    m_sErrorMsg = _T("Another CrashReport.exe already running.");
    return FALSE;
  }

  if (m_CrashInfo.GetReportCount() == 0) {
    m_sErrorMsg = _T("There are no reports for us.");
    return FALSE;
  }

  // Done.
  m_sErrorMsg = _T("Success.");
  return TRUE;
}

BOOL CrashReporter::InitLog() {
  // Check if we have already created log
  if (!m_sCrashLogFile.IsEmpty())
    return TRUE;

  // Create directory where we will store recent crash logs
  CString sLogDir = m_CrashInfo.m_sUnsentCrashReportsFolder + _T("\\Logs");
  BOOL bCreateDir = Utility::CreateFolder(sLogDir);
  if (!bCreateDir) {
    return FALSE;
  }

  // Create new log file
  time_t cur_time;
  time(&cur_time);
  TCHAR szDateTime[64];

  struct tm timeinfo;
  localtime_s(&timeinfo, &cur_time);
  _tcsftime(szDateTime, 64, _T("%Y%m%d-%H%M%S"), &timeinfo);

  CString sLogFile;
  sLogFile.Format(_T("%s\\CrashRpt-Log-%s-{%s}.txt"), sLogDir, szDateTime, m_CrashInfo.GetReport(0)->GetCrashGUID());
  m_Assync.InitLogFile(sLogFile);

  m_sCrashLogFile = sLogFile;

  return TRUE;
}

CCrashInfoReader* CrashReporter::GetCrashInfo() {
  return &m_CrashInfo;
}

CString CrashReporter::GetErrorMsg() {
  return m_sErrorMsg;
}

void CrashReporter::SetNotificationWindow(HWND hWnd) {
  m_hWndNotify = hWnd;
}

BOOL CrashReporter::Run() {
  WaitForCompletion();

  SetExportFlag(FALSE, _T(""));

  thread_ = std::async(std::launch::async, &CrashReporter::DoWork, this);

  return TRUE;
}

int CrashReporter::GetStatus() {
  // Return global error report delivery status
  return m_nStatus;
}

int CrashReporter::GetCurReport() {
  // Returns the index of error report currently being sent
  return m_nCurReport;
}

void CrashReporter::UnblockParentProcess() {
  // Notify the parent process that we have finished with minidump,
  // so the parent process is able to unblock and terminate itself.

  // Open the event the parent process had created for us
  CString sEventName;
  sEventName.Format(_T("Local\\CrashRptEvent_%s"), GetCrashInfo()->GetReport(0)->GetCrashGUID());
  HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, sEventName);
  if (hEvent != NULL)
    SetEvent(hEvent);  // Signal event
}

BOOL CrashReporter::DoWork() {
  // Reset the completion event
  m_Assync.Reset();
  InitLog();

  // Add a message to log
  m_Assync.SetProgress(_T("Start collecting information about the crash..."), 0, false);

  // First take a screenshot of user's desktop (if needed).
  TakeDesktopScreenshot();

  if (m_Assync.IsCancelled())  // Check if user-cancelled
  {
    // Parent process can now terminate
    UnblockParentProcess();

    // Add a message to log
    m_Assync.SetProgress(_T("[exit_silently]"), 0, false);
    return FALSE;
  }

  // Create crash dump.
  CreateMiniDump();

  if (m_Assync.IsCancelled())  // Check if user-cancelled
  {
    // Parent process can now terminate
    UnblockParentProcess();

    // Add a message to log
    m_Assync.SetProgress(_T("[exit_silently]"), 0, false);
    return FALSE;
  }

  // Notify the parent process that we have finished with minidump,
  // so the parent process is able to unblock and terminate itself.
  UnblockParentProcess();

  // Copy user-provided files.
  CollectCrashFiles();

  if (m_Assync.IsCancelled())  // Check if user-cancelled
  {
    m_Assync.SetProgress(_T("[exit_silently]"), 0, false);
    return FALSE;
  }

  if (m_Assync.IsCancelled())  // Check if user-cancelled
  {
    m_Assync.SetProgress(_T("[exit_silently]"), 0, false);
    return FALSE;
  }

  // Create crash description XML
  CreateCrashDescriptionXML(*m_CrashInfo.GetReport(0));

  // Add a message to log
  m_Assync.SetProgress(_T("[confirm_send_report]"), 100, false);

  if (m_CrashInfo.m_bStoreZIPArchives) {
    BOOL bCompress = CompressReportFiles(m_CrashInfo.GetReport(m_nCurReport));
    if (!bCompress) {
      m_Assync.SetProgress(_T("[status_failed]"), 100, false);
    }
  }

  if (m_CrashInfo.m_bAppRestart) {
    RestartApp();
  }

  return TRUE;
}

// Returns the export flag (the flag is set if we are exporting error report as a ZIP archive)
void CrashReporter::SetExportFlag(BOOL bExport, CString sExportFile) {
  // This is used when we need to export error report files as a ZIP archive
  m_bExport = bExport;
  m_sExportFileName = sExportFile;
}

void CrashReporter::WaitForCompletion() {
  if (thread_.valid()) {
    thread_.wait();
  }
}

// Gets status of the local operation
void CrashReporter::GetCurOpStatus(int& nProgressPct, std::vector<CString>& msg_log) {
  m_Assync.GetProgress(nProgressPct, msg_log);
}

void CrashReporter::Cancel() {
  m_Assync.Cancel();
}

void CrashReporter::FeedbackReady(int code) {
  m_Assync.FeedbackReady(code);
}

BOOL CrashReporter::Finalize() {
  WaitForCompletion();

  return TRUE;
}

// This method takes the desktop screenshot (screenshot of entire virtual screen
// or screenshot of the main window).
BOOL CrashReporter::TakeDesktopScreenshot() {
  CScreenCapture sc;   // Screen capture object
  ScreenshotInfo ssi;  // Screenshot params

  // Add a message to log
  m_Assync.SetProgress(_T("[taking_screenshot]"), 0);

  // Check if screenshot capture is allowed
  if (!m_CrashInfo.m_bAddScreenshot) {
    // Add a message to log
    m_Assync.SetProgress(_T("Desktop screenshot generation disabled; skipping."), 0);
    // Exit, nothing to do here
    return TRUE;
  }

  // Add a message to log
  m_Assync.SetProgress(_T("Taking desktop screenshot"), 0);

  // Get screenshot flags passed by the parent process
  DWORD dwFlags = m_CrashInfo.m_dwScreenshotFlags;

  BOOL bAllowDelete = false;

  // Determine what image format to use (JPG or PNG)
  SCREENSHOT_IMAGE_FORMAT fmt = SCREENSHOT_FORMAT_PNG;  // PNG by default

  if ((dwFlags & CR_AS_USE_JPEG_FORMAT) != 0)
    fmt = SCREENSHOT_FORMAT_JPG;  // Use JPEG format

  // Determine what to use - color or grayscale image
  BOOL bGrayscale = (dwFlags & CR_AS_GRAYSCALE_IMAGE) != 0;

  SCREENSHOT_TYPE type = SCREENSHOT_TYPE_VIRTUAL_SCREEN;
  if ((dwFlags & CR_AS_MAIN_WINDOW) != 0)  // We need to capture the main window
    type = SCREENSHOT_TYPE_MAIN_WINDOW;
  else if ((dwFlags & CR_AS_PROCESS_WINDOWS) != 0)  // Capture all process windows
    type = SCREENSHOT_TYPE_ALL_PROCESS_WINDOWS;
  else  // (dwFlags&CR_AS_VIRTUAL_SCREEN)!=0 // Capture the virtual screen
    type = SCREENSHOT_TYPE_VIRTUAL_SCREEN;

  // Take the screen shot
  BOOL bTakeScreenshot =
      sc.TakeDesktopScreenshot(m_CrashInfo.GetReport(m_nCurReport)->GetErrorReportDirName(), ssi, type, m_CrashInfo.m_dwProcessId, fmt, m_CrashInfo.m_nJpegQuality, bGrayscale);
  if (bTakeScreenshot == FALSE) {
    return FALSE;
  }

  // Save screenshot info
  m_CrashInfo.GetReport(0)->SetScreenshotInfo(ssi);

  // Prepare the list of screenshot files we will add to the error report
  std::vector<ERIFileItem> FilesToAdd;
  size_t i;
  for (i = 0; i < ssi.m_aMonitors.size(); i++) {
    CString sFileName = ssi.m_aMonitors[i].m_sFileName;
    CString sDestFile;
    int nSlashPos = sFileName.ReverseFind('\\');
    sDestFile = sFileName.Mid(nSlashPos + 1);
    ERIFileItem fi;
    fi.m_sSrcFile = sFileName;
    fi.m_sDestFile = sDestFile;
    fi.m_sDesc = TEXT("Desktop Screenshot");
    fi.m_bAllowDelete = bAllowDelete;
    m_CrashInfo.GetReport(0)->AddFileItem(&fi);
  }

  // Done
  return TRUE;
}

// This callback function is called by MinidumpWriteDump
BOOL CALLBACK CrashReporter::MiniDumpCallback(PVOID CallbackParam, PMINIDUMP_CALLBACK_INPUT CallbackInput, PMINIDUMP_CALLBACK_OUTPUT CallbackOutput) {
  // Delegate back to the CErrorReportSender
  CrashReporter* pErrorReportSender = (CrashReporter*)CallbackParam;
  return pErrorReportSender->OnMinidumpProgress(CallbackInput, CallbackOutput);
}

// This method is called when MinidumpWriteDump notifies us about
// currently performed action
BOOL CrashReporter::OnMinidumpProgress(const PMINIDUMP_CALLBACK_INPUT CallbackInput, PMINIDUMP_CALLBACK_OUTPUT CallbackOutput) {
  switch (CallbackInput->CallbackType) {
    case CancelCallback: {
      // This callback allows to cancel minidump generation
      if (m_Assync.IsCancelled()) {
        CallbackOutput->Cancel = TRUE;
        m_Assync.SetProgress(_T("Dump generation cancelled by user"), 0, true);
      }
    } break;

    case ModuleCallback: {
      // We are currently dumping some module
      strconv_t strconv;
      CString sMsg;
      sMsg.Format(_T("Dumping info for module %s"), strconv.w2t(CallbackInput->Module.FullPath));

      // Here we want to collect module information
      CErrorReportInfo* eri = m_CrashInfo.GetReport(0);
      if (eri->GetExceptionAddress() != 0) {
        // Check if this is the module where exception has happened
        ULONG64 dwExcAddr = eri->GetExceptionAddress();
        if (dwExcAddr >= CallbackInput->Module.BaseOfImage && dwExcAddr <= CallbackInput->Module.BaseOfImage + CallbackInput->Module.SizeOfImage) {
          // Save module information to the report
          eri->SetExceptionModule(CallbackInput->Module.FullPath);
          eri->SetExceptionModuleBase(CallbackInput->Module.BaseOfImage);

          // Save module version info
          VS_FIXEDFILEINFO* fi = &CallbackInput->Module.VersionInfo;
          if (fi) {
            WORD dwVerMajor = HIWORD(fi->dwProductVersionMS);
            WORD dwVerMinor = LOWORD(fi->dwProductVersionMS);
            WORD dwPatchLevel = HIWORD(fi->dwProductVersionLS);
            WORD dwVerBuild = LOWORD(fi->dwProductVersionLS);

            CString sVer;
            sVer.Format(_T("%u.%u.%u.%u"), dwVerMajor, dwVerMinor, dwPatchLevel, dwVerBuild);
            eri->SetExceptionModuleVersion(sVer);
          }
        }
      }

      // Update progress
      m_Assync.SetProgress(sMsg, 0, true);
    } break;
    case ThreadCallback: {
      // We are currently dumping some thread
      CString sMsg;
      sMsg.Format(_T("Dumping info for thread 0x%X"), CallbackInput->Thread.ThreadId);
      m_Assync.SetProgress(sMsg, 0, true);
    } break;
  }

  return TRUE;
}

BOOL CrashReporter::CreateMiniDump() {
  if (m_CrashInfo.m_bGenerateMinidump == FALSE) {
    m_Assync.SetProgress(_T("Crash dump generation disabled; skipping."), 0, false);
    return TRUE;
  }

  BOOL bStatus = FALSE;
  HMODULE hDbgHelp = NULL;
  HANDLE hFile = NULL;
  MINIDUMP_EXCEPTION_INFORMATION mei;
  MINIDUMP_CALLBACK_INFORMATION mci;
  CString sMinidumpFile = m_CrashInfo.GetReport(m_nCurReport)->GetErrorReportDirName() + _T("\\crashdump.dmp");
  std::vector<ERIFileItem> files_to_add;
  ERIFileItem fi;
  CString sErrorMsg;

  // Update progress
  m_Assync.SetProgress(_T("Creating crash dump file..."), 0, false);
  m_Assync.SetProgress(_T("[creating_dump]"), 0, false);

  // Load dbghelp.dll
  hDbgHelp = LoadLibrary(m_CrashInfo.m_sDbgHelpPath);
  if (hDbgHelp == NULL) {
    // Try again ... fallback to dbghelp.dll in path
    const CString sDebugHelpDLL_name = TEXT("dbghelp.dll");
    hDbgHelp = LoadLibrary(sDebugHelpDLL_name);
  }

  if (hDbgHelp == NULL) {
    sErrorMsg = _T("dbghelp.dll couldn't be loaded");
    m_Assync.SetProgress(_T("dbghelp.dll couldn't be loaded."), 0, false);
    goto cleanup;
  }

  // Try to adjust process privilegies to be able to generate minidumps.
  SetDumpPrivileges();

  // Create the minidump file
  hFile = CreateFile(sMinidumpFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  // Check if file has been created
  if (hFile == INVALID_HANDLE_VALUE) {
    DWORD dwError = GetLastError();
    CString sMsg;
    sMsg.Format(_T("Couldn't create minidump file: %s"), Utility::FormatErrorMsg(dwError));
    m_Assync.SetProgress(sMsg, 0, false);
    sErrorMsg = sMsg;
    return FALSE;
  }

  // Set valid dbghelp API version
  typedef LPAPI_VERSION(WINAPI * LPIMAGEHLPAPIVERSIONEX)(LPAPI_VERSION AppVersion);
  LPIMAGEHLPAPIVERSIONEX lpImagehlpApiVersionEx = (LPIMAGEHLPAPIVERSIONEX)GetProcAddress(hDbgHelp, "ImagehlpApiVersionEx");
  ATLASSERT(lpImagehlpApiVersionEx != NULL);
  if (lpImagehlpApiVersionEx != NULL) {
    API_VERSION CompiledApiVer;
    CompiledApiVer.MajorVersion = 10;
    CompiledApiVer.MinorVersion = 0;
    CompiledApiVer.Revision = 12;
    CompiledApiVer.Reserved = 0;
    LPAPI_VERSION pActualApiVer = lpImagehlpApiVersionEx(&CompiledApiVer);
    pActualApiVer;
    ATLASSERT(CompiledApiVer.MajorVersion == pActualApiVer->MajorVersion);
    ATLASSERT(CompiledApiVer.MinorVersion == pActualApiVer->MinorVersion);
    ATLASSERT(CompiledApiVer.Revision == pActualApiVer->Revision);
  }

  // Write minidump to the file
  mei.ThreadId = m_CrashInfo.m_dwThreadId;
  mei.ExceptionPointers = m_CrashInfo.m_pExInfo;
  mei.ClientPointers = TRUE;

  mci.CallbackRoutine = MiniDumpCallback;
  mci.CallbackParam = this;

  typedef BOOL(WINAPI * LPMINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
                                             CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

  // Get address of MiniDumpWirteDump function
  LPMINIDUMPWRITEDUMP pfnMiniDumpWriteDump = (LPMINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
  if (!pfnMiniDumpWriteDump) {
    m_Assync.SetProgress(_T("Bad MiniDumpWriteDump function."), 0, false);
    sErrorMsg = _T("Bad MiniDumpWriteDump function");
    return FALSE;
  }

  // Open client process
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_CrashInfo.m_dwProcessId);

  // Now actually write the minidump
  BOOL bWriteDump = pfnMiniDumpWriteDump(hProcess, m_CrashInfo.m_dwProcessId, hFile, m_CrashInfo.m_MinidumpType, &mei, NULL, &mci);

  // Check result
  if (!bWriteDump) {
    CString sMsg = Utility::FormatErrorMsg(GetLastError());
    m_Assync.SetProgress(_T("Error writing dump."), 0, false);
    m_Assync.SetProgress(sMsg, 0, false);
    sErrorMsg = sMsg;
    goto cleanup;
  }

  // Update progress
  bStatus = TRUE;
  m_Assync.SetProgress(_T("Finished creating dump."), 100, false);

cleanup:

  // Close file
  if (hFile)
    CloseHandle(hFile);

  // Unload dbghelp.dll
  if (hDbgHelp)
    FreeLibrary(hDbgHelp);

  // Add the minidump file to error report
  fi.m_bMakeCopy = false;
  fi.m_sDesc = TEXT("Crash Minidump");
  fi.m_sDestFile = _T("crashdump.dmp");
  fi.m_sSrcFile = sMinidumpFile;
  fi.m_sErrorStatus = sErrorMsg;
  files_to_add.push_back(fi);

  // Add file to the list
  m_CrashInfo.GetReport(0)->AddFileItem(&fi);

  return bStatus;
}

BOOL CrashReporter::SetDumpPrivileges() {
  // This method is used to have the current process be able to call MiniDumpWriteDump
  // This code was taken from:
  // http://social.msdn.microsoft.com/Forums/en-US/vcgeneral/thread/f54658a4-65d2-4196-8543-7e71f3ece4b6/

  BOOL fSuccess = FALSE;
  HANDLE TokenHandle = NULL;
  TOKEN_PRIVILEGES TokenPrivileges;

  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle)) {
    m_Assync.SetProgress(_T("SetDumpPrivileges: Could not get the process token"), 0);
    goto Cleanup;
  }

  TokenPrivileges.PrivilegeCount = 1;

  if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &TokenPrivileges.Privileges[0].Luid)) {
    m_Assync.SetProgress(_T("SetDumpPrivileges: Couldn't lookup SeDebugPrivilege name"), 0);
    goto Cleanup;
  }

  TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  //Add privileges here.
  if (!AdjustTokenPrivileges(TokenHandle, FALSE, &TokenPrivileges, sizeof(TokenPrivileges), NULL, NULL)) {
    m_Assync.SetProgress(_T("SetDumpPrivileges: Could not revoke the debug privilege"), 0);
    goto Cleanup;
  }

  fSuccess = TRUE;

Cleanup:

  if (TokenHandle) {
    CloseHandle(TokenHandle);
  }

  return fSuccess;
}

// This method adds an element to XML file
void CrashReporter::AddElemToXML(CString sName, CString sValue, TiXmlNode* root) {
  strconv_t strconv;
  TiXmlHandle hElem = new TiXmlElement(strconv.t2utf8(sName));
  root->LinkEndChild(hElem.ToNode());
  TiXmlText* text = new TiXmlText(strconv.t2utf8(sValue));
  hElem.ToElement()->LinkEndChild(text);
}

// This method generates an XML file describing the crash
BOOL CrashReporter::CreateCrashDescriptionXML(CErrorReportInfo& eri) {
  BOOL bStatus = FALSE;
  ERIFileItem fi;
  CString sFileName = eri.GetErrorReportDirName() + _T("\\crashrpt.xml");
  CString sErrorMsg;
  strconv_t strconv;
  TiXmlDocument doc;
  FILE* f = NULL;
  CString sNum;
  CString sOSIs64Bit;
  CString sExceptionType;

  fi.m_bMakeCopy = false;
  fi.m_sDesc = TEXT("Crash Description XML");
  fi.m_sDestFile = _T("crashrpt.xml");
  fi.m_sSrcFile = sFileName;
  fi.m_sErrorStatus = sErrorMsg;
  // Add this file to the list
  eri.AddFileItem(&fi);

  TiXmlNode* root = root = new TiXmlElement("CrashRpt");
  doc.LinkEndChild(root);

  TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", "");
  doc.InsertBeforeChild(root, *decl);

  AddElemToXML(_T("CrashGUID"), eri.GetCrashGUID(), root);
  AddElemToXML(_T("AppName"), eri.GetAppName(), root);
  AddElemToXML(_T("AppVersion"), eri.GetAppVersion(), root);
  AddElemToXML(_T("ImageName"), eri.GetImageName(), root);
  AddElemToXML(_T("OperatingSystem"), eri.GetOSName(), root);

  sOSIs64Bit.Format(_T("%d"), eri.IsOS64Bit());
  AddElemToXML(_T("OSIs64Bit"), sOSIs64Bit, root);

  AddElemToXML(_T("GeoLocation"), eri.GetGeoLocation(), root);
  AddElemToXML(_T("SystemTimeUTC"), eri.GetSystemTimeUTC(), root);

  if (eri.GetExceptionAddress() != 0) {
    sNum.Format(_T("0x%I64x"), eri.GetExceptionAddress());
    AddElemToXML(_T("ExceptionAddress"), sNum, root);

    AddElemToXML(_T("ExceptionModule"), eri.GetExceptionModule(), root);

    sNum.Format(_T("0x%I64x"), eri.GetExceptionModuleBase());
    AddElemToXML(_T("ExceptionModuleBase"), sNum, root);

    AddElemToXML(_T("ExceptionModuleVersion"), eri.GetExceptionModuleVersion(), root);
  }

  sExceptionType.Format(_T("%d"), m_CrashInfo.m_nExceptionType);
  AddElemToXML(_T("ExceptionType"), sExceptionType, root);
  if (m_CrashInfo.m_nExceptionType == CR_SEH_EXCEPTION) {
    CString sExceptionCode;
    sExceptionCode.Format(_T("%d"), m_CrashInfo.m_dwExceptionCode);
    AddElemToXML(_T("ExceptionCode"), sExceptionCode, root);
  }
  else if (m_CrashInfo.m_nExceptionType == CR_CPP_SIGFPE) {
    CString sFPESubcode;
    sFPESubcode.Format(_T("%d"), m_CrashInfo.m_uFPESubcode);
    AddElemToXML(_T("FPESubcode"), sFPESubcode, root);
  }
  else if (m_CrashInfo.m_nExceptionType == CR_CPP_INVALID_PARAMETER) {
    AddElemToXML(_T("InvParamExpression"), m_CrashInfo.m_sInvParamExpr, root);
    AddElemToXML(_T("InvParamFunction"), m_CrashInfo.m_sInvParamFunction, root);
    AddElemToXML(_T("InvParamFile"), m_CrashInfo.m_sInvParamFile, root);

    CString sInvParamLine;
    sInvParamLine.Format(_T("%d"), m_CrashInfo.m_uInvParamLine);
    AddElemToXML(_T("InvParamLine"), sInvParamLine, root);
  }

  CString sGuiResources;
  sGuiResources.Format(_T("%d"), eri.GetGuiResourceCount());
  AddElemToXML(_T("GUIResourceCount"), sGuiResources, root);

  CString sProcessHandleCount;
  sProcessHandleCount.Format(_T("%d"), eri.GetProcessHandleCount());
  AddElemToXML(_T("OpenHandleCount"), sProcessHandleCount, root);

  AddElemToXML(_T("MemoryUsageKbytes"), eri.GetMemUsage(), root);

  if (eri.GetScreenshotInfo().m_bValid) {
    TiXmlHandle hScreenshotInfo = new TiXmlElement("ScreenshotInfo");
    root->LinkEndChild(hScreenshotInfo.ToNode());

    TiXmlHandle hVirtualScreen = new TiXmlElement("VirtualScreen");

    sNum.Format(_T("%d"), eri.GetScreenshotInfo().m_rcVirtualScreen.left);
    hVirtualScreen.ToElement()->SetAttribute("left", strconv.t2utf8(sNum));

    sNum.Format(_T("%d"), eri.GetScreenshotInfo().m_rcVirtualScreen.top);
    hVirtualScreen.ToElement()->SetAttribute("top", strconv.t2utf8(sNum));

    sNum.Format(_T("%d"), eri.GetScreenshotInfo().m_rcVirtualScreen.Width());
    hVirtualScreen.ToElement()->SetAttribute("width", strconv.t2utf8(sNum));

    sNum.Format(_T("%d"), eri.GetScreenshotInfo().m_rcVirtualScreen.Height());
    hVirtualScreen.ToElement()->SetAttribute("height", strconv.t2utf8(sNum));

    hScreenshotInfo.ToNode()->LinkEndChild(hVirtualScreen.ToNode());

    TiXmlHandle hMonitors = new TiXmlElement("Monitors");
    hScreenshotInfo.ToElement()->LinkEndChild(hMonitors.ToNode());

    size_t i;
    for (i = 0; i < eri.GetScreenshotInfo().m_aMonitors.size(); i++) {
      MonitorInfo& mi = eri.GetScreenshotInfo().m_aMonitors[i];
      TiXmlHandle hMonitor = new TiXmlElement("Monitor");

      sNum.Format(_T("%d"), mi.m_rcMonitor.left);
      hMonitor.ToElement()->SetAttribute("left", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), mi.m_rcMonitor.top);
      hMonitor.ToElement()->SetAttribute("top", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), mi.m_rcMonitor.Width());
      hMonitor.ToElement()->SetAttribute("width", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), mi.m_rcMonitor.Height());
      hMonitor.ToElement()->SetAttribute("height", strconv.t2utf8(sNum));

      hMonitor.ToElement()->SetAttribute("file", strconv.t2utf8(Utility::GetFileName(mi.m_sFileName)));

      hMonitors.ToElement()->LinkEndChild(hMonitor.ToNode());
    }

    TiXmlHandle hWindows = new TiXmlElement("Windows");
    hScreenshotInfo.ToElement()->LinkEndChild(hWindows.ToNode());

    for (i = 0; i < eri.GetScreenshotInfo().m_aWindows.size(); i++) {
      WindowInfo& wi = eri.GetScreenshotInfo().m_aWindows[i];
      TiXmlHandle hWindow = new TiXmlElement("Window");

      sNum.Format(_T("%d"), wi.m_rcWnd.left);
      hWindow.ToElement()->SetAttribute("left", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), wi.m_rcWnd.top);
      hWindow.ToElement()->SetAttribute("top", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), wi.m_rcWnd.Width());
      hWindow.ToElement()->SetAttribute("width", strconv.t2utf8(sNum));

      sNum.Format(_T("%d"), wi.m_rcWnd.Height());
      hWindow.ToElement()->SetAttribute("height", strconv.t2utf8(sNum));

      hWindow.ToElement()->SetAttribute("title", strconv.t2utf8(wi.m_sTitle));

      hWindows.ToElement()->LinkEndChild(hWindow.ToNode());
    }
  }

  TiXmlHandle hCustomProps = new TiXmlElement("CustomProps");
  root->LinkEndChild(hCustomProps.ToNode());

  int i;
  for (i = 0; i < eri.GetPropCount(); i++) {
    CString sName;
    CString sVal;
    eri.GetPropByIndex(i, sName, sVal);

    TiXmlHandle hProp = new TiXmlElement("Prop");

    hProp.ToElement()->SetAttribute("name", strconv.t2utf8(sName));
    hProp.ToElement()->SetAttribute("value", strconv.t2utf8(sVal));

    hCustomProps.ToElement()->LinkEndChild(hProp.ToNode());
  }

  TiXmlHandle hFileItems = new TiXmlElement("FileList");
  root->LinkEndChild(hFileItems.ToNode());

  for (i = 0; i < eri.GetFileItemCount(); i++) {
    ERIFileItem* rfi = eri.GetFileItemByIndex(i);
    TiXmlHandle hFileItem = new TiXmlElement("FileItem");

    hFileItem.ToElement()->SetAttribute("name", strconv.t2utf8(rfi->m_sDestFile));
    hFileItem.ToElement()->SetAttribute("description", strconv.t2utf8(rfi->m_sDesc));
    if (rfi->m_bAllowDelete)
      hFileItem.ToElement()->SetAttribute("optional", "1");
    if (!rfi->m_sErrorStatus.IsEmpty())
      hFileItem.ToElement()->SetAttribute("error", strconv.t2utf8(rfi->m_sErrorStatus));

    hFileItems.ToElement()->LinkEndChild(hFileItem.ToNode());
  }

#if _MSC_VER < 1400
  f = _tfopen(sFileName, _T("w"));
#else
  _tfopen_s(&f, sFileName, _T("w"));
#endif

  if (f == NULL) {
    sErrorMsg = _T("Error opening file for writing");
    goto cleanup;
  }

  doc.useMicrosoftBOM = true;
  bool bSave = doc.SaveFile(f);
  if (!bSave) {
    sErrorMsg = doc.ErrorDesc();
    goto cleanup;
  }

  fclose(f);
  f = NULL;

  bStatus = TRUE;

cleanup:

  if (f)
    fclose(f);

  if (!bStatus) {
    eri.GetFileItemByName(fi.m_sDestFile)->m_sErrorStatus = sErrorMsg;
  }

  return bStatus;
}

// This method collects user-specified files
BOOL CrashReporter::CollectCrashFiles() {
  BOOL bStatus = FALSE;
  CString str;
  CString sErrorReportDir = m_CrashInfo.GetReport(m_nCurReport)->GetErrorReportDirName();
  CString sSrcFile;
  CString sDestFile;
  std::vector<ERIFileItem> file_list;

  // Copy application-defined files that should be copied on crash
  m_Assync.SetProgress(_T("[copying_files]"), 0, false);

  // Walk through error report files
  int i;
  for (i = 0; i < m_CrashInfo.GetReport(m_nCurReport)->GetFileItemCount(); i++) {
    ERIFileItem* pfi = m_CrashInfo.GetReport(m_nCurReport)->GetFileItemByIndex(i);

    // Check if operation has been cancelled by user
    if (m_Assync.IsCancelled())
      goto cleanup;

    // Check if the file name is a search template.
    BOOL bSearchPattern = Utility::IsFileSearchPattern(pfi->m_sSrcFile);
    if (bSearchPattern)
      CollectFilesBySearchTemplate(pfi, file_list);
    else
      CollectSingleFile(pfi);
  }

  // Add newly collected files to the list of file items
  for (i = 0; i < (int)file_list.size(); i++) {
    m_CrashInfo.GetReport(0)->AddFileItem(&file_list[i]);
  }

  // Remove file items that are search patterns
  BOOL bFound = FALSE;
  do {
    bFound = FALSE;
    for (i = 0; i < m_CrashInfo.GetReport(m_nCurReport)->GetFileItemCount(); i++) {
      ERIFileItem* pfi = m_CrashInfo.GetReport(m_nCurReport)->GetFileItemByIndex(i);

      // Check if the file name is a search template.
      BOOL bSearchPattern = Utility::IsFileSearchPattern(pfi->m_sSrcFile);
      if (bSearchPattern) {
        // Delete this item
        m_CrashInfo.GetReport(m_nCurReport)->DeleteFileItemByIndex(i);
        bFound = TRUE;
        break;
      }
    }
  } while (bFound);

  // Create dump of registry keys

  CErrorReportInfo* eri = m_CrashInfo.GetReport(m_nCurReport);
  if (eri->GetRegKeyCount() != 0) {
    m_Assync.SetProgress(_T("Dumping registry keys..."), 0, false);
  }

  // Walk through our registry key list
  for (i = 0; i < eri->GetRegKeyCount(); i++) {
    CString sKeyName;
    ERIRegKey rki;
    eri->GetRegKeyByIndex(i, sKeyName, rki);

    if (m_Assync.IsCancelled())
      goto cleanup;

    CString sFilePath = eri->GetErrorReportDirName() + _T("\\") + rki.m_sDstFileName;

    str.Format(_T("Dumping registry key '%s' to file '%s' "), sKeyName, sFilePath);
    m_Assync.SetProgress(str, 0, false);

    // Create registry key dump
    CString sErrorMsg;
    DumpRegKey(sKeyName, sFilePath, sErrorMsg);
    ERIFileItem fi;
    fi.m_sSrcFile = sFilePath;
    fi.m_sDestFile = rki.m_sDstFileName;
    fi.m_sDesc = TEXT("Registry Key Dump");
    fi.m_bMakeCopy = FALSE;
    fi.m_bAllowDelete = rki.m_bAllowDelete;
    fi.m_sErrorStatus = sErrorMsg;
    std::vector<ERIFileItem> file_list;
    file_list.push_back(fi);
    // Add file to the list of file items
    m_CrashInfo.GetReport(0)->AddFileItem(&fi);
  }

  // Success
  bStatus = TRUE;

cleanup:

  // Clean up
  m_Assync.SetProgress(_T("Finished copying files."), 100, false);

  return 0;
}

BOOL CrashReporter::CollectSingleFile(ERIFileItem* pfi) {
  BOOL bStatus = false;
  CString str;
  HANDLE hSrcFile = INVALID_HANDLE_VALUE;
  HANDLE hDestFile = INVALID_HANDLE_VALUE;
  BOOL bGetSize = FALSE;
  LARGE_INTEGER lFileSize;
  LARGE_INTEGER lTotalWritten;
  CString sDestFile;
  BOOL bRead = FALSE;
  BOOL bWrite = FALSE;
  LPBYTE buffer[1024];
  DWORD dwBytesRead = 0;
  DWORD dwBytesWritten = 0;

  CString sErrorReportDir = m_CrashInfo.GetReport(m_nCurReport)->GetErrorReportDirName();

  // Open source file with read/write sharing permissions.
  hSrcFile = CreateFile(pfi->m_sSrcFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
  if (hSrcFile == INVALID_HANDLE_VALUE) {
    pfi->m_sErrorStatus = Utility::FormatErrorMsg(GetLastError());
    str.Format(_T("Error opening file %s."), pfi->m_sSrcFile);
    m_Assync.SetProgress(str, 0, false);
    goto cleanup;
  }

  // If we should make a copy of the file
  if (pfi->m_bMakeCopy) {
    str.Format(_T("Copying file %s."), pfi->m_sSrcFile);
    m_Assync.SetProgress(str, 0, false);

    bGetSize = GetFileSizeEx(hSrcFile, &lFileSize);
    if (!bGetSize) {
      pfi->m_sErrorStatus = Utility::FormatErrorMsg(GetLastError());
      str.Format(_T("Couldn't get file size of %s"), pfi->m_sSrcFile);
      m_Assync.SetProgress(str, 0, false);
      CloseHandle(hSrcFile);
      hSrcFile = INVALID_HANDLE_VALUE;
      goto cleanup;
    }

    sDestFile = sErrorReportDir + _T("\\") + pfi->m_sDestFile;

    hDestFile = CreateFile(sDestFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
    if (hDestFile == INVALID_HANDLE_VALUE) {
      pfi->m_sErrorStatus = Utility::FormatErrorMsg(GetLastError());
      str.Format(_T("Error creating file %s."), sDestFile);
      m_Assync.SetProgress(str, 0, false);
      CloseHandle(hSrcFile);
      hSrcFile = INVALID_HANDLE_VALUE;
      goto cleanup;
    }

    lTotalWritten.QuadPart = 0;

    for (;;) {
      if (m_Assync.IsCancelled())
        goto cleanup;

      bRead = ReadFile(hSrcFile, buffer, 1024, &dwBytesRead, NULL);
      if (!bRead || dwBytesRead == 0)
        break;

      bWrite = WriteFile(hDestFile, buffer, dwBytesRead, &dwBytesWritten, NULL);
      if (!bWrite || dwBytesRead != dwBytesWritten)
        break;

      lTotalWritten.QuadPart += dwBytesWritten;

      int nProgress = (int)(100.0f * lTotalWritten.QuadPart / lFileSize.QuadPart);

      m_Assync.SetProgress(nProgress, false);
    }

    CloseHandle(hSrcFile);
    hSrcFile = INVALID_HANDLE_VALUE;
    CloseHandle(hDestFile);
    hDestFile = INVALID_HANDLE_VALUE;

    // Use the copy for display and zipping.
    pfi->m_sSrcFile = sDestFile;
  }

  bStatus = true;

cleanup:

  if (hSrcFile != INVALID_HANDLE_VALUE)
    CloseHandle(hSrcFile);

  if (hDestFile != INVALID_HANDLE_VALUE)
    CloseHandle(hDestFile);

  return bStatus;
}

BOOL CrashReporter::CollectFilesBySearchTemplate(ERIFileItem* pfi, std::vector<ERIFileItem>& file_list) {
  CString sMsg;
  sMsg.Format(_T("Looking for files using search template: %s"), pfi->m_sSrcFile);
  m_Assync.SetProgress(sMsg, 0);

  // Look for files matching search pattern
  WIN32_FIND_DATA ffd;
  HANDLE hFind = FindFirstFile(pfi->m_sSrcFile, &ffd);
  if (hFind == INVALID_HANDLE_VALUE) {
    // Nothing found
    m_Assync.SetProgress(_T("Could not find any files matching the search template."), 0);
    return FALSE;
  }
  else {
    // Get owning directory name
    int nPos = pfi->m_sSrcFile.ReverseFind(_T('\\'));
    CString sDir = pfi->m_sSrcFile.Mid(0, nPos);

    // Enumerate matching files
    BOOL bFound = TRUE;
    int nFileCount = 0;
    while (bFound) {
      if (m_Assync.IsCancelled())
        break;

      if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        CString sFile = sDir + _T("\\");
        sFile += ffd.cFileName;

        // Add file to file list.
        ERIFileItem fi;
        fi.m_sSrcFile = sFile;
        fi.m_sDestFile = ffd.cFileName;
        fi.m_sDesc = pfi->m_sDesc;
        fi.m_bMakeCopy = pfi->m_bMakeCopy;
        fi.m_bAllowDelete = pfi->m_bAllowDelete;
        file_list.push_back(fi);

        CollectSingleFile(&fi);

        nFileCount++;
      }

      // Go to next file
      bFound = FindNextFile(hFind, &ffd);
    }

    // Clean up
    FindClose(hFind);
  }

  // Done
  return TRUE;
}

// This method dumps a registry key contents to an XML file
int CrashReporter::DumpRegKey(CString sRegKey, CString sDestFile, CString& sErrorMsg) {
  strconv_t strconv;
  TiXmlDocument document;

  // Load document if file already exists
  // otherwise create new document.

  FILE* f = NULL;
#if _MSC_VER < 1400
  f = _tfopen(sDestFile, _T("rb"));
#else
  _tfopen_s(&f, sDestFile, _T("rb"));
#endif
  if (f != NULL) {
    document.LoadFile(f);
    fclose(f);
    f = NULL;
  }

  TiXmlHandle hdoc(&document);

  TiXmlElement* registry = document.RootElement();

  if (registry == NULL) {
    registry = new TiXmlElement("registry");
    document.LinkEndChild(registry);
  }

  TiXmlNode* declaration = hdoc.Child(0).ToNode();
  if (declaration == NULL || declaration->Type() != TiXmlNode::TINYXML_DECLARATION) {
    TiXmlDeclaration* decl = new TiXmlDeclaration("1.0", "UTF-8", "");
    document.InsertBeforeChild(registry, *decl);
  }

  DumpRegKey(NULL, sRegKey, registry);

#if _MSC_VER < 1400
  f = _tfopen(sDestFile, _T("wb"));
#else
  _tfopen_s(&f, sDestFile, _T("wb"));
#endif
  if (f == NULL) {
    sErrorMsg = _T("Error opening file for writing.");
    return 1;
  }

  bool bSave = document.SaveFile(f);

  fclose(f);

  if (!bSave) {
    sErrorMsg = _T("Error saving XML document to file: ");
    sErrorMsg += document.ErrorDesc();
  }

  return (bSave == true) ? 0 : 1;
}

int CrashReporter::DumpRegKey(HKEY hParentKey, CString sSubKey, TiXmlElement* elem) {
  strconv_t strconv;
  HKEY hKey = NULL;

  if (hParentKey == NULL) {
    int nSkip = 0;
    if (sSubKey.Left(19).Compare(_T("HKEY_LOCAL_MACHINE\\")) == 0) {
      hKey = HKEY_LOCAL_MACHINE;
      nSkip = 18;
    }
    else if (sSubKey.Left(18).Compare(_T("HKEY_CURRENT_USER\\")) == 0) {
      hKey = HKEY_CURRENT_USER;
      nSkip = 17;
    }
    else {
      return 1;  // Invalid key.
    }

    CString sKey = sSubKey.Mid(0, nSkip);
    LPCSTR szKey = strconv.t2utf8(sKey);
    sSubKey = sSubKey.Mid(nSkip + 1);

    TiXmlHandle key_node = elem->FirstChild(szKey);
    if (key_node.ToElement() == NULL) {
      key_node = new TiXmlElement("k");
      elem->LinkEndChild(key_node.ToNode());
      key_node.ToElement()->SetAttribute("name", szKey);
    }

    DumpRegKey(hKey, sSubKey, key_node.ToElement());
  }
  else {
    int pos = sSubKey.Find('\\');
    CString sKey = sSubKey;
    if (pos > 0)
      sKey = sSubKey.Mid(0, pos);
    LPCSTR szKey = strconv.t2utf8(sKey);

    TiXmlHandle key_node = elem->FirstChild(szKey);
    if (key_node.ToElement() == NULL) {
      key_node = new TiXmlElement("k");
      elem->LinkEndChild(key_node.ToNode());
      key_node.ToElement()->SetAttribute("name", szKey);
    }

    if (ERROR_SUCCESS == RegOpenKeyEx(hParentKey, sKey, 0, GENERIC_READ, &hKey)) {
      if (pos > 0) {
        sSubKey = sSubKey.Mid(pos + 1);
        DumpRegKey(hKey, sSubKey, key_node.ToElement());
      }
      else {
        DWORD dwSubKeys = 0;
        DWORD dwMaxSubKey = 0;
        DWORD dwValues = 0;
        DWORD dwMaxValueNameLen = 0;
        DWORD dwMaxValueLen = 0;
        LONG lResult = RegQueryInfoKey(hKey, NULL, 0, 0, &dwSubKeys, &dwMaxSubKey, 0, &dwValues, &dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL);
        if (lResult == ERROR_SUCCESS) {
          // Enumerate and dump subkeys
          int i;
          for (i = 0; i < (int)dwSubKeys; i++) {
            LPWSTR szName = new WCHAR[dwMaxSubKey + 1];
            DWORD dwLen = dwMaxSubKey + 1;
            lResult = RegEnumKeyEx(hKey, i, szName, &dwLen, 0, NULL, 0, NULL);
            if (lResult == ERROR_SUCCESS) {
              DumpRegKey(hKey, CString(szName), key_node.ToElement());
            }

            delete[] szName;
          }

          // Dump key values
          for (i = 0; i < (int)dwValues; i++) {
            LPWSTR szName = new WCHAR[dwMaxValueNameLen + 1];
            memset(szName, 0, sizeof(WCHAR) * (dwMaxValueNameLen + 1));
            LPBYTE pData = new BYTE[dwMaxValueLen];
            memset(pData, 0, sizeof(BYTE) * (dwMaxValueLen));
            DWORD dwNameLen = dwMaxValueNameLen + 1;
            DWORD dwValueLen = dwMaxValueLen;
            DWORD dwType = 0;

            lResult = RegEnumValue(hKey, i, szName, &dwNameLen, 0, &dwType, pData, &dwValueLen);
            if (lResult == ERROR_SUCCESS) {
              TiXmlHandle val_node = key_node.ToElement()->FirstChild(strconv.w2utf8(szName));
              if (val_node.ToElement() == NULL) {
                val_node = new TiXmlElement("v");
                key_node.ToElement()->LinkEndChild(val_node.ToNode());
              }

              val_node.ToElement()->SetAttribute("name", strconv.w2utf8(szName));

              char str[128] = "";
              LPSTR szType = NULL;
              if (dwType == REG_BINARY)
                szType = "REG_BINARY";
              else if (dwType == REG_DWORD)
                szType = "REG_DWORD";
              else if (dwType == REG_EXPAND_SZ)
                szType = "REG_EXPAND_SZ";
              else if (dwType == REG_MULTI_SZ)
                szType = "REG_MULTI_SZ";
              else if (dwType == REG_QWORD)
                szType = "REG_QWORD";
              else if (dwType == REG_SZ)
                szType = "REG_SZ";
              else {
#if _MSC_VER < 1400
                sprintf(str, "Unknown type (0x%08x)", dwType);
#else
                sprintf_s(str, 128, "Unknown type (0x%08x)", dwType);
#endif
                szType = str;
              }

              val_node.ToElement()->SetAttribute("type", szType);

              if (dwType == REG_BINARY) {
                std::string str2;
                int j;
                for (j = 0; j < (int)dwValueLen; j++) {
                  char num[10];
#if _MSC_VER < 1400
                  sprintf(num, "%02X", pData[j]);
#else
                  sprintf_s(num, 10, "%02X", pData[j]);
#endif
                  str2 += num;
                  if (j < (int)dwValueLen)
                    str2 += " ";
                }

                val_node.ToElement()->SetAttribute("value", str2.c_str());
              }
              else if (dwType == REG_DWORD) {
                LPDWORD pdwValue = (LPDWORD)pData;
                char str3[64] = "";
#if _MSC_VER < 1400
                sprintf(str3, "0x%08x (%lu)", *pdwValue, *pdwValue);
#else
                sprintf_s(str3, 64, "0x%08x (%lu)", *pdwValue, *pdwValue);
#endif
                val_node.ToElement()->SetAttribute("value", str3);
              }
              else if (dwType == REG_SZ || dwType == REG_EXPAND_SZ) {
                LPCSTR szValue = strconv.t2utf8((LPCTSTR)pData);
                val_node.ToElement()->SetAttribute("value", szValue);
              }
              else if (dwType == REG_MULTI_SZ) {
                LPCTSTR szValues = (LPCTSTR)pData;
                int prev = 0;
                int pos2 = 0;
                for (;;) {
                  if (szValues[pos2] == 0) {
                    CString sValue = CString(szValues + prev, pos2 - prev);
                    LPCSTR szValue = strconv.t2utf8(sValue);

                    TiXmlHandle str_node = new TiXmlElement("str");
                    val_node.ToElement()->LinkEndChild(str_node.ToNode());
                    str_node.ToElement()->SetAttribute("value", szValue);

                    prev = pos2 + 1;
                  }

                  if (szValues[pos2] == 0 && szValues[pos2 + 1] == 0)
                    break;  // Double-null

                  pos2++;
                }
              }
            }

            delete[] szName;
            delete[] pData;
          }
        }
      }

      RegCloseKey(hKey);
    }
    else {
      CString sErrMsg = Utility::FormatErrorMsg(GetLastError());
      LPCSTR szErrMsg = strconv.t2utf8(sErrMsg);
      key_node.ToElement()->SetAttribute("error", szErrMsg);
    }
  }

  return 0;
}

// This method restarts the client application
BOOL CrashReporter::RestartApp() {
  // Check our config - if we should restart the client app or not?
  if (m_CrashInfo.m_bAppRestart == FALSE)
    return FALSE;  // No need to restart

  // Reset restart flag to avoid restarting the app twice
  // (if app has been restarted already).
  m_CrashInfo.m_bAppRestart = FALSE;

  // Add a message to log and reset progress
  m_Assync.SetProgress(_T("Restarting the application..."), 0, false);

  // Set up process start up info
  STARTUPINFO si;
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);

  // Set up process information
  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));

  // Format command line
  CString sCmdLine;
  if (m_CrashInfo.m_sRestartCmdLine.IsEmpty()) {
    if (m_sZipName.IsEmpty())
      sCmdLine.Format(_T("\"%s\""), m_CrashInfo.GetReport(m_nCurReport)->GetImageName());
    else
      sCmdLine.Format(_T("\"%s\" -reportzip:\"%s\""), m_CrashInfo.GetReport(m_nCurReport)->GetImageName(), m_sZipName.GetBuffer(0));
  }
  else {
    if (m_sZipName.IsEmpty())
      sCmdLine.Format(_T("\"%s\" %s"), m_CrashInfo.GetReport(m_nCurReport)->GetImageName(), m_CrashInfo.m_sRestartCmdLine.GetBuffer(0));
    else
      sCmdLine.Format(_T("\"%s\" %s -reportzip:\"%s\""), m_CrashInfo.GetReport(m_nCurReport)->GetImageName(), m_CrashInfo.m_sRestartCmdLine.GetBuffer(0), m_sZipName.GetBuffer(0));
  }

  // Create process using the command line prepared earlier
  BOOL bCreateProcess = CreateProcess(m_CrashInfo.GetReport(m_nCurReport)->GetImageName(), sCmdLine.GetBuffer(0), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

  // The following is to avoid a handle leak
  if (pi.hProcess) {
    CloseHandle(pi.hProcess);
    pi.hProcess = NULL;
  }

  // The following is to avoid a handle leak
  if (pi.hThread) {
    CloseHandle(pi.hThread);
    pi.hThread = NULL;
  }

  // Check if process was created
  if (!bCreateProcess) {
    // Add error message
    m_Assync.SetProgress(_T("Error restarting the application!"), 0, false);
    return FALSE;
  }

  // Success
  m_Assync.SetProgress(_T("Application restarted OK."), 0, false);
  return TRUE;
}

// This method compresses the files contained in the report and produces a ZIP archive.
BOOL CrashReporter::CompressReportFiles(CErrorReportInfo* eri) {
  BOOL bStatus = FALSE;
  strconv_t strconv;
  zipFile hZip = NULL;
  CString sMsg;
  LONG64 lTotalSize = 0;
  LONG64 lTotalCompressed = 0;
  BYTE buff[1024];
  DWORD dwBytesRead = 0;
  HANDLE hFile = INVALID_HANDLE_VALUE;
  std::map<CString, ERIFileItem>::iterator it;
  FILE* f = NULL;
  CString sMD5Hash;

  // Add a different log message depending on the current mode.
  if (m_bExport)
    m_Assync.SetProgress(_T("[exporting_report]"), 0, false);
  else
    m_Assync.SetProgress(_T("[compressing_files]"), 0, false);

  // Calculate the total size of error report files
  lTotalSize = eri->GetTotalSize();

  // Add a message to log
  sMsg.Format(_T("Total file size for compression is %I64d bytes"), lTotalSize);
  m_Assync.SetProgress(sMsg, 0, false);

  // Determine what name to use for the output ZIP archive file.
  if (m_bExport)
    m_sZipName = m_sExportFileName;
  else
    m_sZipName = eri->GetErrorReportDirName() + _T(".zip");

  // Update progress
  sMsg.Format(_T("Creating ZIP archive file %s"), m_sZipName);
  m_Assync.SetProgress(sMsg, 1, false);

  // Create ZIP archive
  hZip = zipOpen((const char*)m_sZipName.GetBuffer(0), APPEND_STATUS_CREATE);
  if (hZip == NULL) {
    m_Assync.SetProgress(_T("Failed to create ZIP file."), 100, true);
    goto cleanup;
  }

  // Enumerate files contained in the report
  int i;
  for (i = 0; i < eri->GetFileItemCount(); i++) {
    ERIFileItem* pfi = eri->GetFileItemByIndex(i);

    // Check if the operation was cancelled by user
    if (m_Assync.IsCancelled())
      goto cleanup;

    // Define destination file name in ZIP archive
    CString sDstFileName = pfi->m_sDestFile.GetBuffer(0);
    // Define source file name
    CString sFileName = pfi->m_sSrcFile.GetBuffer(0);
    // Define file description
    CString sDesc = pfi->m_sDesc;

    // Update progress
    sMsg.Format(_T("Compressing file %s"), sDstFileName);
    m_Assync.SetProgress(sMsg, 0, false);

    // Open file for reading
    hFile = CreateFile(sFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      sMsg.Format(_T("Couldn't open file %s"), sFileName);
      m_Assync.SetProgress(sMsg, 0, false);
      continue;
    }

    // Get file information.
    BY_HANDLE_FILE_INFORMATION fi;
    GetFileInformationByHandle(hFile, &fi);

    // Convert file creation time to system file time.
    SYSTEMTIME st;
    FileTimeToSystemTime(&fi.ftLastWriteTime, &st);

    // Fill in the ZIP file info
    zip_fileinfo info;
    info.dosDate = 0;
    info.tmz_date.tm_year = st.wYear;
    info.tmz_date.tm_mon = st.wMonth - 1;
    info.tmz_date.tm_mday = st.wDay;
    info.tmz_date.tm_hour = st.wHour;
    info.tmz_date.tm_min = st.wMinute;
    info.tmz_date.tm_sec = st.wSecond;
    info.external_fa = FILE_ATTRIBUTE_NORMAL;
    info.internal_fa = FILE_ATTRIBUTE_NORMAL;

    // Create new file inside of our ZIP archive
    int n = zipOpenNewFileInZip(hZip, (const char*)strconv.t2a(sDstFileName.GetBuffer(0)), &info, NULL, 0, NULL, 0, strconv.t2a(sDesc), Z_DEFLATED, Z_DEFAULT_COMPRESSION);
    if (n != 0) {
      sMsg.Format(_T("Couldn't compress file %s"), sDstFileName);
      m_Assync.SetProgress(sMsg, 0, false);
      continue;
    }

    // Read source file contents and write it to ZIP archive
    for (;;) {
      // Check if operation was cancelled by user
      if (m_Assync.IsCancelled())
        goto cleanup;

      // Read a portion of source file
      BOOL bRead = ReadFile(hFile, buff, 1024, &dwBytesRead, NULL);
      if (!bRead || dwBytesRead == 0)
        break;

      // Write a portion into destination file
      int res = zipWriteInFileInZip(hZip, buff, dwBytesRead);
      if (res != 0) {
        zipCloseFileInZip(hZip);
        sMsg.Format(_T("Couldn't write to compressed file %s"), sDstFileName);
        m_Assync.SetProgress(sMsg, 0, false);
        break;
      }

      // Update totals
      lTotalCompressed += dwBytesRead;

      // Update progress
      float fProgress = 100.0f * lTotalCompressed / lTotalSize;
      m_Assync.SetProgress((int)fProgress, false);
    }

    // Close file
    zipCloseFileInZip(hZip);
    CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
  }

  // Close ZIP archive
  if (hZip != NULL) {
    zipClose(hZip, NULL);
    hZip = NULL;
  }

  // Check if totals match
  if (lTotalSize == lTotalCompressed)
    bStatus = TRUE;

cleanup:

  // Clean up

  if (hZip != NULL)
    zipClose(hZip, NULL);

  if (hFile != INVALID_HANDLE_VALUE)
    CloseHandle(hFile);

  if (f != NULL)
    fclose(f);

  if (bStatus)
    m_Assync.SetProgress(_T("Finished compressing files...OK"), 100, true);
  else
    m_Assync.SetProgress(_T("File compression failed."), 100, true);

  if (m_bExport) {
    if (bStatus)
      m_Assync.SetProgress(_T("[end_exporting_report_ok]"), 100, false);
    else
      m_Assync.SetProgress(_T("[end_exporting_report_failed]"), 100, false);
  }
  else {
    m_Assync.SetProgress(_T("[end_compressing_files]"), 100, false);
  }

  return bStatus;
}

BOOL CrashReporter::HasErrors() {
  return m_bErrors;
}

CString CrashReporter::GetLogFilePath() {
  return m_Assync.GetLogFilePath();
}

int CrashReporter::TerminateAllCrashReportProcesses() {
  // This method looks for all runing CrashReport.exe processes
  // and terminates each one. This may be needed when an application's installer
  // wants to shutdown all crash sender processes running in background
  // to replace the locked files.

  CString sProcessName;
  sProcessName.Format(_T("CrashReport.exe"));

  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);

  // Create the list of all processes in the system
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

  if (Process32First(snapshot, &entry) == TRUE) {
    // Walk through processes
    while (Process32Next(snapshot, &entry) == TRUE) {
      // Compare process name
      if (_tcsicmp(entry.szExeFile, sProcessName) == 0 && entry.th32ProcessID != GetCurrentProcessId()) {
        // Open process handle
        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);

        // Terminate process.
        TerminateProcess(hProcess, 1);

        CloseHandle(hProcess);
      }
    }
  }

  // Clean up
  CloseHandle(snapshot);

  return 0;
}
