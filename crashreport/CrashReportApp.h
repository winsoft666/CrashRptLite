#pragma once

#include "resource.h"

class CrashReportApp : public CWinApp {
 public:
  CrashReportApp();

 protected:
  virtual BOOL InitInstance();

};

extern CrashReportApp theApp;