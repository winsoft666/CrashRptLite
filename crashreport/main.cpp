#include "stdafx.h"
#include "CrashReportApp.h"
#include "resource.h"
#include "CrashReporter.h"
#include "strconv.h"
#include "Utility.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hInstance);
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);

  system("pause");

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
    return 1;
  }

  pReporter->Run();
  pReporter->WaitForCompletion();
  int nRet = pReporter->GetStatus();

  delete pReporter;

  return nRet;
}