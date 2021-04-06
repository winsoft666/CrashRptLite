#include "StdAfx.h"
#include "CritSec.h"

namespace CrashReport {
CCritSec::CCritSec() {
  InitializeCriticalSection(&m_CritSec);
};

CCritSec::~CCritSec() {
  DeleteCriticalSection(&m_CritSec);
}

void CCritSec::Lock() {
  EnterCriticalSection(&m_CritSec);
};

void CCritSec::Unlock() {
  LeaveCriticalSection(&m_CritSec);
};

CAutoLock::CAutoLock(__in CCritSec* plock) {
  m_pLock = plock;
  m_pLock->Lock();
};

CAutoLock::~CAutoLock() {
  m_pLock->Unlock();
};
}  // namespace CrashReport