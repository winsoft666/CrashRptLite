#pragma once

#include "resource.h"

class CrashSenderApp : public CWinApp {
 public:
  CrashSenderApp();

 protected:
  virtual BOOL InitInstance();

};

extern CrashSenderApp theApp;