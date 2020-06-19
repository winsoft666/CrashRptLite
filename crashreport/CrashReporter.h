#pragma once
#include "AssyncNotification.h"
#include "tinyxml.h"
#include "CrashInfoReader.h"
#include <future>

// Action type
enum ActionType {
  COLLECT_CRASH_INFO = 0x01,  // Crash info should be collected.
  COMPRESS_REPORT = 0x02,     // Error report files should be packed into ZIP archive.
  RESTART_APP = 0x04,         // Crashed app should be restarted.
};

class CrashReporter {
 public:
  // Constructor.
  CrashReporter();

  // Destructor.
  virtual ~CrashReporter();

  // Returns singleton of this class.
  static CrashReporter* GetInstance();

  // Performs initialization.
  BOOL Init(LPCTSTR szFileMappingName);

  // Cleans up all temp files and does other finalizing work.
  BOOL Finalize();

  // Returns pointer to object containing crash information.
  CCrashInfoReader* GetCrashInfo();

  // Returns last error message.
  CString GetErrorMsg();

  // Set the window that will receive notifications from this object.
  void SetNotificationWindow(HWND hWnd);

  // Compresses and sends the report(s).
  BOOL Run();

  // Blocks until an assync operation finishes.
  void WaitForCompletion();

  // Cancels the assync operation.
  void Cancel();

  // Returns error report sending status.
  int GetStatus();

  // Gets current operation status.
  void GetCurOpStatus(int& nProgressPct, std::vector<CString>& msg_log);

  // Unblocks waiting worker thread.
  void FeedbackReady(int code);

  // Returns current error report's index.
  int GetCurReport();

  // Returns path to log file.
  CString GetLogFilePath();

  // Allows to specify file name for exporting error report.
  void SetExportFlag(BOOL bExport, CString sExportFile);

  // Returns TRUE if there were errors.
  BOOL HasErrors();

  // This method finds and terminates all instances of CrashSender.exe process.
  static int TerminateAllCrashReportProcesses();

 private:
  // Creates log file
  BOOL InitLog();

  // This method performs an action or several actions.
  BOOL DoWork(int Action);

  // Collects crash report files.
  BOOL CollectCrashFiles();

  // Includes a single file to crash report
  BOOL CollectSingleFile(ERIFileItem* pfi);

  // Includes all files matching search pattern to crash report
  BOOL CollectFilesBySearchTemplate(ERIFileItem* pfi, std::vector<ERIFileItem>& file_list);

  // Takes desktop screenshot.
  BOOL TakeDesktopScreenshot();

  // Creates crash dump file.
  BOOL CreateMiniDump();

  // This method is used to have the current process be able to call MiniDumpWriteDump.
  BOOL SetDumpPrivileges();

  // Creates crash description XML file.
  BOOL CreateCrashDescriptionXML(CErrorReportInfo& eri);

  // Adds an element to XML file.
  void AddElemToXML(CString sName, CString sValue, TiXmlNode* root);

  // Minidump callback.
  static BOOL CALLBACK MiniDumpCallback(PVOID CallbackParam,
                                        PMINIDUMP_CALLBACK_INPUT CallbackInput,
                                        PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);

  // Minidump callback.
  BOOL OnMinidumpProgress(const PMINIDUMP_CALLBACK_INPUT CallbackInput,
                          PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);

  // Restarts the application.
  BOOL RestartApp();

  // Dumps registry key to the XML file.
  int DumpRegKey(CString sRegKey, CString sDestFile, CString& sErrorMsg);

  // Used internally for dumping a registry key.
  int DumpRegKey(HKEY hKeyParent, CString sSubKey, TiXmlElement* elem);

  // Packs error report files to ZIP archive.
  BOOL CompressReportFiles(CErrorReportInfo* eri);

  // Unblocks parent process.
  void UnblockParentProcess();



  // Internal variables
  static CrashReporter* m_pInstance;       // Singleton
  CCrashInfoReader m_CrashInfo;            // Contains crash information.
  CString m_sErrorMsg;                     // Last error message.
  HWND m_hWndNotify;                       // Notification window.
  int m_nStatus;                           // Error report sending status.
  int m_nCurReport;                        // Index of current error report.

  int m_SendAttempt;                       // Number of current sending attempt.
  AssyncNotification m_Assync;             // Used for communication with the main thread.
  
  CString m_sZipName;                      // Name of the ZIP archive to send.
  int m_Action;                            // Current assynchronous action.
  BOOL m_bExport;                          // If TRUE than export should be performed.
  CString m_sExportFileName;               // File name for exporting.
  BOOL m_bErrors;                          // TRUE if there were errors.
  CString m_sCrashLogFile;                 // Log file.

  std::future<BOOL> thread_;
};
