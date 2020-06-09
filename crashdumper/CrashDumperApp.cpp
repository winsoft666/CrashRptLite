#include "stdafx.h"
#include "CrashDumperApp.h"
#include "resource.h"
#include "CrashDumper.h"
#include "strconv.h"
#include "Utility.h"



CrashSenderApp::CrashSenderApp() {
  m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}


CrashSenderApp theApp;

BOOL CrashSenderApp::InitInstance() {
  system("pause");
  INITCOMMONCONTROLSEX InitCtrls;
  InitCtrls.dwSize = sizeof(InitCtrls);
  InitCtrls.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&InitCtrls);

  CWinApp::InitInstance();

  AfxEnableControlContainer();

  CShellManager* pShellManager = new CShellManager;

  CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

  int nRet = 0;

  LPCWSTR szCommandLine = GetCommandLineW();

  int argc = 0;
  LPWSTR* argv = CommandLineToArgvW(szCommandLine, &argc);
  if (argc != 2)
    return 1;

  if (_tcscmp(argv[1], _T("/terminate")) == 0) {
    return CErrorReportSender::TerminateAllCrashSenderProcesses();
  }


  CString sFileMappingName = CString(argv[1]);

  CErrorReportSender* pSender = CErrorReportSender::GetInstance();

  BOOL bInit = pSender->Init(sFileMappingName.GetBuffer(0));
  if (!bInit) {
    delete pSender;
    return 0;
  }

  pSender->Run();
  pSender->WaitForCompletion();
  nRet = pSender->GetStatus();
  
  delete pSender;

  if (pShellManager != nullptr) {
    delete pShellManager;
  }

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
  ControlBarCleanUp();
#endif

  return FALSE;
}