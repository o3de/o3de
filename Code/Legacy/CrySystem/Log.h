/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ILog.h>
#include <MultiThread_Containers.h>
#include <list>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/IO/FileIO.h>

struct IConsole;
struct ICVar;
struct ISystem;

//////////////////////////////////////////////////////////////////////
#if defined(ANDROID) || defined(AZ_PLATFORM_MAC)
    #define MAX_TEMP_LENGTH_SIZE    4098
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(Log_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    #define MAX_TEMP_LENGTH_SIZE    8196
#endif
#define MAX_FILENAME_SIZE           256

#define KEEP_LOG_FILE_OPEN


//////////////////////////////////////////////////////////////////////
class CLog
    : public ILog
{
public:
    typedef std::list<ILogCallback*> Callbacks;
    typedef AZStd::fixed_string<MAX_TEMP_LENGTH_SIZE> LogStringType;

    // constructor
    CLog(ISystem* pSystem);
    // destructor
    ~CLog();


    // interface ILog, IMiniLog -------------------------------------------------

    virtual void Release() { delete this; };
    virtual bool SetFileName(const char* fileNameOrAbsolutePath, bool backupLogs);
    virtual const char* GetFileName();
    virtual const char* GetBackupFileName();
#if !defined(EXCLUDE_NORMAL_LOG)
    virtual void Log(const char* command, ...) PRINTF_PARAMS(2, 3);
    virtual void LogAlways(const char* command, ...) PRINTF_PARAMS(2, 3);
    virtual void LogWarning(const char* command, ...) PRINTF_PARAMS(2, 3);
    virtual void LogError(const char* command, ...) PRINTF_PARAMS(2, 3);
    // Append the log output with the previous logged line
    void LogAppendWithPrevLine(const char* command, ...) override PRINTF_PARAMS(2, 3);
    virtual void LogToFile  (const char* command, ...) PRINTF_PARAMS(2, 3);
    // Append the log output to the file with the previous logged line
    void LogToFileAppendWithPrevLine(const char* command, ...) override PRINTF_PARAMS(2, 3);
    virtual void LogToConsole(const char* command, ...) PRINTF_PARAMS(2, 3);
    // Append the log output to the console with the previous logged line
    void LogToConsoleAppendWithPrevLine(const char* command, ...) override PRINTF_PARAMS(2, 3);
#else
    virtual void Log(const char* command, ...) PRINTF_PARAMS(2, 3) {}
    virtual void LogAlways(const char* command, ...) PRINTF_PARAMS(2, 3) {}
    virtual void LogWarning(const char* command, ...) PRINTF_PARAMS(2, 3) {}
    virtual void LogError(const char* command, ...) PRINTF_PARAMS(2, 3) {}
    void LogAppendWithPrevLine(const char* command, ...) override PRINTF_PARAMS(2, 3) {}
    virtual void LogToFile  (const char* command, ...) PRINTF_PARAMS(2, 3) {}
    void LogToFileAppendWithPrevLine(const char* command, ...) override PRINTF_PARAMS(2, 3) {}
    virtual void LogToConsole(const char* command, ...) PRINTF_PARAMS(2, 3) {}
    void LogToConsoleAppendWithPrevLine(const char* command, ...) override PRINTF_PARAMS(2, 3) {}
#endif // !defined(EXCLUDE_NORMAL_LOG)
    virtual void UpdateLoadingScreen(const char* command, ...) PRINTF_PARAMS(2, 3);
    virtual void SetVerbosity(int verbosity);
    virtual int  GetVerbosityLevel();
    virtual void RegisterConsoleVariables();
    virtual void UnregisterConsoleVariables();
    virtual void AddCallback(ILogCallback* pCallback);
    virtual void RemoveCallback(ILogCallback* pCallback);
    virtual void LogV(ELogType ineType, int flags, const char* szFormat, va_list args);
    virtual void LogV(ELogType ineType, const char* szFormat, va_list args);
    void LogWithCallback(ELogType logType, const LogWriteCallback& messageCallback) override;
    virtual void Update();
    virtual const char* GetModuleFilter();
    virtual void FlushAndClose();
    virtual void Flush();

private: // -------------------------------------------------------------------
    struct SLogMsg
    {
        enum class Destination
        {
            Default, //LogString, sends OnWrite to anycallback registered with AddCallback
            Console,
            File
        };
        // Use a fixed_string buffer that can hold 512 characters + NUL terminating character
        // If a string is greater than the fixed string size, then the message is stored
        // in AZStd::string allocated from the heap
        using MessageString  = AZStd::variant<AZStd::fixed_string<512>, AZStd::string>;
        MessageString msg;
        ELogType logType;
        bool m_appendToPreviousLine;
        Destination destination;
    };

    void CheckAndPruneBackupLogs() const;

    bool IsError(ELogType logType) const { return logType == ELogType::eError || logType == ELogType::eErrorAlways || logType == ELogType::eWarning || logType == ELogType::eWarningAlways; }

    //helper function to pass calls to LogString... to the main thread, returns false if you are on the main thread already, in which case just process the work.
    bool LogToMainThread(AZStd::string_view szString, ELogType logType, bool m_appendToPreviousLine, SLogMsg::Destination destination);

    enum class MessageQueueState
    {
        NotQueued,
        Queued
    };

#if !defined(EXCLUDE_NORMAL_LOG)
    void LogString(AZStd::string_view szString, ELogType logType);
    void LogStringToFile(AZStd::string_view szString, ELogType logType, bool m_appendToPreviousLine, MessageQueueState queueState);
    void LogStringToConsole(AZStd::string_view szString, ELogType logType, bool m_appendToPreviousLine);
#else
    void LogString(AZStd::string_view szString, ELogType logType) {}
    void LogStringToFile(AZStd::string_view szString, ELogType logType, bool m_appendToPreviousLine, MessageQueueState queueState) {}
    void LogStringToConsole(AZStd::string_view szString, ELogType logType, bool m_appendToPreviousLine) {}
#endif // !defined(EXCLUDE_NORMAL_LOG)

    bool OpenLogFile(const char* filename, AZ::IO::OpenMode mode);
    void CloseLogFile();

    // will format the message into m_szTemp
    void FormatMessage(const char* szCommand, ...) PRINTF_PARAMS(2, 3);

#if defined(SUPPORT_LOG_IDENTER)
    void Indent(CLogIndenter* indenter);
    void Unindent(CLogIndenter* indenter);
    void BuildIndentString();
    virtual void PushAssetScopeName(const char* sAssetType, const char* sName);
    virtual void PopAssetScopeName();
    virtual const char* GetAssetScopeString();
#endif

    ISystem* m_pSystem;                                                       //
    float m_fLastLoadingUpdateTime;                           // for non-frequent streamingEngine update
    char m_szFilename[MAX_FILENAME_SIZE];            // can be with path
    mutable char m_sBackupFilename[MAX_FILENAME_SIZE];   // can be with path
    AZ::IO::FileIOStream m_logFileHandle;

    bool m_backupLogs;

#if defined(SUPPORT_LOG_IDENTER)
    uint8                   m_indentation;
    LogStringType           m_indentWithString;
    class CLogIndenter* m_topIndenter;

    struct SAssetScopeInfo
    {
        string sType;
        string sName;
    };

    std::vector<SAssetScopeInfo> m_assetScopeQueue;
    AZStd::mutex m_assetScopeQueueLock;
    string m_assetScopeString;
#endif

    IConsole*          m_pConsole;                                                      //

    struct SLogHistoryItem
    {
        char str[MAX_WARNING_LENGTH];
        const char* ptr;
        ELogType type;
        float time;
    };
    SLogHistoryItem m_history[16];
    int m_iLastHistoryItem;

    static bool CheckLogFormatter(const char* formatter);

#if defined(KEEP_LOG_FILE_OPEN)
    static void LogFlushFile(IConsoleCmdArgs* pArgs);
#endif

public: // -------------------------------------------------------------------
    // checks the verbosity of the message and returns NULL if the message must NOT be
    // logged, or the pointer to the part of the message that should be logged
    const char* CheckAgainstVerbosity(const char* pText, bool& logtofile, bool& logtoconsole, const uint8 DefaultVerbosity = 2);

    // create backup of log file, useful behavior - only on development platform
    void CreateBackupFile() const;

    ICVar*                 m_pLogVerbosity;                                             //
    ICVar*                 m_pLogWriteToFile;                                       //
    ICVar*                 m_pLogWriteToFileVerbosity;                      //
    ICVar*                 m_pLogVerbosityOverridesWriteToFile;     //
    ICVar*                 m_pLogSpamDelay;                       //
    ICVar*                 m_pLogModule;                                                    // Module filter for log
    Callbacks               m_callbacks;                                                    //

    threadID m_nMainThreadId;
    CryMT::queue<SLogMsg> m_threadSafeMsgQueue;
};
