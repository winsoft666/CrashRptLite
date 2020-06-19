#include "stdafx.h"
#include "CrashReportApp.h"
#include "resource.h"
#include "CrashReporter.h"
#include "strconv.h"
#include "Utility.h"


CrashReportApp::CrashReportApp() {
  m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}


CrashReportApp theApp;

BOOL CrashReportApp::InitInstance() {
  INITCOMMONCONTROLSEX InitCtrls;
  InitCtrls.dwSize = sizeof(InitCtrls);
  InitCtrls.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&InitCtrls);

  CWinApp::InitInstance();

  AfxEnableControlContainer();

  int nRet = 0;

  LPCWSTR szCommandLine = GetCommandLineW();

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(szCommandLine, &argc);
  if (argc != 2)
    return 1;

  if (_tcscmp(argv[1], _T("/terminate")) == 0) {
    return CrashReporter::TerminateAllCrashReportProcesses();
  }


  CString sFileMappingName = CString(argv[1]);

  CrashReporter* pReporter = CrashReporter::GetInstance();

  BOOL bInit = pReporter->Init(sFileMappingName.GetBuffer(0));
  if (!bInit) {
    delete pReporter;
    return 0;
  }

  pReporter->Run();
  pReporter->WaitForCompletion();
  nRet = pReporter->GetStatus();
  
  delete pReporter;

  return FALSE;
}