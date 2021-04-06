#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <assert.h>
#include <process.h>
#include <strsafe.h>
#include "CrashRpt.h"

using namespace CrashReport;

int main(int argc, char* argv[]) {
  UNREFERENCED_PARAMETER(argc);
  UNREFERENCED_PARAMETER(argv);

  // Install crash reporting
  //
  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.cb = sizeof(CR_INSTALL_INFO);           // Size of the structure
  info.pszAppName = L"CrashRpt Console Test";  // App name
  info.pszAppVersion = L"1.0.0";               // App version
#ifdef _DEBUG
  WCHAR szCurDir[MAX_PATH] = {0};
  GetModuleFileNameW(NULL, szCurDir, _MAX_PATH);
  WCHAR* ptr = wcsrchr(szCurDir, L'\\');
  if (ptr != NULL)
    *(ptr) = 0;  // remove executable name
  WCHAR szCrashReportDebugPath[MAX_PATH];
  StringCchPrintfW(szCrashReportDebugPath, MAX_PATH, L"%s\\%s", szCurDir, L"CrashReportd.exe");
  info.pszCrashReportPath = szCrashReportDebugPath;
#endif
  info.dwFlags = CR_INST_ALL_POSSIBLE_HANDLERS | CR_INST_STORE_ZIP_ARCHIVES;

  // Install crash handlers
  int nInstResult = crInstall(&info);
  assert(nInstResult == 0);

  crAddFile(L"D:\\1989.mp4", nullptr, L"description", CR_AF_MAKE_FILE_COPY);

  crAddScreenshot(CR_AS_PROCESS_WINDOWS, 0);

  crAddRegKey(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\XXX", L"regkey.xml", 0);

  // Check result
  if (nInstResult != 0) {
    WCHAR buff[256];
    crGetLastErrorMsg(buff, 256);                // Get last error
    StringCchPrintfW(buff, 256, L"%s\n", buff);  // and output it to the screen
    return FALSE;
  }

  printf("Press Enter to simulate a null pointer exception or any other key to exit...\n");
  int n = _getch();
  if (n == 13) {
    int* p = 0;
    *p = 0;  // Access violation
  }

  // Uninstall exception handlers
  //
  int nUninstRes = crUninstall();
  assert(nUninstRes == 0);

  return 0;
}
