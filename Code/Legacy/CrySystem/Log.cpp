/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Log related functions


#include "CrySystem_precompiled.h"
#include "Log.h"

//this should not be included here
#include <IConsole.h>
#include <ISystem.h>
#include "System.h"
#include "CryPath.h"                    // PathUtil::ReplaceExtension()

#include <AzFramework/IO/FileOperations.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Time/ITime.h>

#ifdef WIN32
#include <time.h>
#endif

#if defined(LINUX) || defined(APPLE)
#include <syslog.h>
#endif

#define LOG_BACKUP_PATH "@log@/LogBackups"

AZ_CVAR(int32_t, log_IncludeTime, 1, nullptr, AZ::ConsoleFunctorFlags::Null,
                "Toggles time stamping of log entries.\n"
                "Usage: log_IncludeTime [0/1/2/3/4/5]\n"
                "  0=off (default)\n"
                "  1=current time\n"
                "  2=relative time\n"
                "  3=current+relative time\n"
                "  4=absolute time in seconds since this mode was started\n"
                "  5=current time+server time"
                "  6=current date+current time");


//////////////////////////////////////////////////////////////////////
namespace LogCVars
{
    float s_log_tick = 0;
    int max_backup_directory_size_mb = 200; //200MB default
};

#if defined(SUPPORT_LOG_IDENTER)
static CLog::LogStringType indentString ("    ");
#endif

namespace
{
    // Definitions for Timestamp logging functions
    using TimeStringType = AZStd::fixed_string<128>;

    auto GetHourMinuteSeconds() -> TimeStringType
    {
        TimeStringType sTime;
        time_t ltime;
        time(&ltime);
#ifdef AZ_COMPILER_MSVC
        struct tm today;
        localtime_s(&today, &ltime);
        strftime(sTime.data(), sTime.capacity(), "<%H:%M:%S> ", &today);
        // Fix up the internal size member of the fixed string
        // Using the traits_type::length function to calculate the strlen of the c-string returned by strftime
        sTime.resize_no_construct(TimeStringType::traits_type::length(sTime.data()));
#else
        auto today = localtime(&ltime);
        strftime(sTime.data(), sTime.capacity(), "<%H:%M:%S> ", today);
        sTime.resize_no_construct(TimeStringType::traits_type::length(sTime.data()));
#endif
        return sTime;
    };

    auto GetDateAndHourMinuteSeconds() -> TimeStringType
    {
        TimeStringType sTime;
        time_t ltime;
        time(&ltime);
#ifdef AZ_COMPILER_MSVC
        struct tm today;
        localtime_s(&today, &ltime);
        strftime(sTime.data(), sTime.capacity(), "<%Y-%m-%d %H:%M:%S> ", &today);
        sTime.resize_no_construct(TimeStringType::traits_type::length(sTime.data()));
#else
        auto today = localtime(&ltime);
        strftime(sTime.data(), sTime.capacity(), "<%Y-%m-%d %H:%M:%S> ", today);
        sTime.resize_no_construct(TimeStringType::traits_type::length(sTime.data()));
#endif
        return sTime;
    };

    auto GetElapsedTimeInSeconds() -> TimeStringType
    {
        TimeStringType elapsedTime;
        static AZ::TimeMs lasttime = AZ::Time::ZeroTimeMs;
        const AZ::TimeMs currenttime = AZ::GetRealElapsedTimeMs();
        if (lasttime != AZ::Time::ZeroTimeMs)
        {
            uint32 dwMs = aznumeric_cast<uint32>(currenttime - lasttime);
            elapsedTime = TimeStringType::format("<%3d.%.3d>: ", dwMs / 1000, dwMs % 1000);
        }
        lasttime = currenttime;

        return elapsedTime;
    };

    auto GetElapsedTimeSinceStartInSeconds() -> TimeStringType
    {
        TimeStringType elapsedTime;

        static AZ::TimeMs lasttime = AZ::Time::ZeroTimeMs;
        const AZ::TimeMs currenttime = AZ::GetRealElapsedTimeMs();
        if (lasttime != AZ::Time::ZeroTimeMs)
        {
            uint32 dwMs = (uint32)(currenttime - lasttime);
            elapsedTime = TimeStringType::format("<%3d.%.3d>: ", dwMs / 1000, dwMs % 1000);
        }

        static bool bFirst = true;
        if (bFirst)
        {
            lasttime = currenttime;
            bFirst = false;
        }

        return elapsedTime;
    };
}

//////////////////////////////////////////////////////////////////////
CLog::CLog(ISystem* pSystem)
{
    memset(m_szFilename, 0, MAX_FILENAME_SIZE);
    memset(m_sBackupFilename, 0, MAX_FILENAME_SIZE);
    //memset(m_szTemp,0,MAX_TEMP_LENGTH_SIZE);
    m_pSystem = pSystem;
    m_pLogVerbosity = 0;
    m_pLogWriteToFile = 0;
    m_pLogWriteToFileVerbosity = 0;
    m_pLogVerbosityOverridesWriteToFile = 0;
    m_pLogSpamDelay = 0;
    m_pLogModule = 0;
    m_fLastLoadingUpdateTime = -1.f;    // for streaming engine update
    m_backupLogs = true;

#if defined(SUPPORT_LOG_IDENTER)
    m_indentation = 0;
    BuildIndentString();
    m_topIndenter = NULL;
#endif

    m_nMainThreadId = CryGetCurrentThreadId();

    m_iLastHistoryItem = 0;
    memset(m_history, 0, sizeof(m_history));

    CheckAndPruneBackupLogs();

}

void CLog::RegisterConsoleVariables()
{
    IConsole* console = m_pSystem->GetIConsole();

#ifdef  _RELEASE
    #if defined(RELEASE_LOGGING)
        #define DEFAULT_VERBOSITY 0
    #else
        #define DEFAULT_VERBOSITY -1
    #endif
#else
    #define DEFAULT_VERBOSITY 3
#endif

    if (console)
    {
        m_pLogVerbosity = REGISTER_INT("log_Verbosity", DEFAULT_VERBOSITY, VF_DUMPTODISK,
                "defines the verbosity level for log messages written to console\n"
                "-1=suppress all logs (including eAlways)\n"
                "0=suppress all logs(except eAlways)\n"
                "1=additional errors\n"
                "2=additional warnings\n"
                "3=additional messages\n"
                "4=additional comments");

        //writing to game.log during game play causes stalls on consoles
        m_pLogWriteToFile = REGISTER_INT("log_WriteToFile", 1, VF_DUMPTODISK, "toggle whether to write log to file (game.log)");

        m_pLogWriteToFileVerbosity = REGISTER_INT("log_WriteToFileVerbosity", DEFAULT_VERBOSITY, VF_DUMPTODISK,
                "defines the verbosity level for log messages written to files\n"
                "-1=suppress all logs (including eAlways)\n"
                "0=suppress all logs(except eAlways)\n"
                "1=additional errors\n"
                "2=additional warnings\n"
                "3=additional messages\n"
                "4=additional comments");
        m_pLogVerbosityOverridesWriteToFile = REGISTER_INT("log_VerbosityOverridesWriteToFile", 1, VF_DUMPTODISK, "when enabled, setting log_verbosity to 0 will stop all logging including writing to file");

        m_pLogSpamDelay = REGISTER_FLOAT("log_SpamDelay", 0.0f, 0, "Sets the minimum time interval between messages classified as spam");

        m_pLogModule = REGISTER_STRING("log_Module", "", VF_NULL, "Only show warnings from specified module");

        REGISTER_CVAR2("log_tick", &LogCVars::s_log_tick, LogCVars::s_log_tick, 0, "When not 0, writes tick log entry into the log file, every N seconds");

        REGISTER_CVAR2("max_log_backup_mb", &LogCVars::max_backup_directory_size_mb, LogCVars::max_backup_directory_size_mb, 0, "Maximum size of backup logs to keep on disk (in MB)");

#if defined(KEEP_LOG_FILE_OPEN)
        REGISTER_COMMAND("log_flush", &LogFlushFile, 0, "Flush the log file");
#endif
    }
    #undef DEFAULT_VERBOSITY
}

//////////////////////////////////////////////////////////////////////
CLog::~CLog()
{
#if defined(SUPPORT_LOG_IDENTER)
    while (m_topIndenter)
    {
        m_topIndenter->Enable(false);
    }

    assert (m_indentation == 0);
#endif

    CreateBackupFile();

    UnregisterConsoleVariables();

    CloseLogFile();
}

void CLog::UnregisterConsoleVariables()
{
    m_pLogVerbosity = 0;
    m_pLogWriteToFile = 0;
    m_pLogWriteToFileVerbosity = 0;
    m_pLogVerbosityOverridesWriteToFile = 0;
    m_pLogSpamDelay = 0;
}

//////////////////////////////////////////////////////////////////////////
void CLog::CloseLogFile()
{
    m_logFileHandle.Close();
}

//////////////////////////////////////////////////////////////////////////
bool CLog::OpenLogFile(const char* filename, AZ::IO::OpenMode mode)
{
    if (m_logFileHandle.IsOpen())
    {
        // Can only AZ_Assert if a file is open, otherwise the AZ_Assert
        // would eventually lead to OpenLogFile being opened up again
        AZ_Assert(false, "Attempt to open log file when one is already open.  This would lead to a handle leak.");
        return false;
    }

    if (filename == nullptr || filename[0] == '\0')
    {
        return false;
    }

    bool opened = m_logFileHandle.Open(filename, mode);

#if defined(LINUX) || defined(APPLE)
    if (!opened)
    {
        syslog(LOG_NOTICE, "Failed to open log file [%s], mode [%d]", filename, static_cast<int>(mode));
    }
#endif

    return opened;
}

//////////////////////////////////////////////////////////////////////////
void CLog::SetVerbosity(int verbosity)
{
    if (m_pLogVerbosity)
    {
        m_pLogVerbosity->Set(verbosity);
    }
}

bool CLog::CheckLogFormatter(const char* formatter)
{
    if (!formatter)
    {
        AZ_Assert(false, "code problem - A message was sent to the log system with nullptr as the formatting string.");
        return false;
    }

    if (formatter[0] == 0)
    {
        // if there's nothing to log, we don't log anything.  Blank lines at least contain a carriage return or something.
        return false;
    }
    return true;
}


//////////////////////////////////////////////////////////////////////////
#if !defined(EXCLUDE_NORMAL_LOG)
void CLog::LogWarning(const char* szFormat, ...)
{
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    va_list ArgList;
    char        szBuffer[MAX_WARNING_LENGTH];
    va_start(ArgList, szFormat);
    vsnprintf_s(szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, szFormat, ArgList);
    szBuffer[sizeof(szBuffer) - 1] = '\0';
    va_end(ArgList);

    va_start(ArgList, szFormat);
    LogV(eWarning, szFormat, ArgList);
    va_end(ArgList);
}

//////////////////////////////////////////////////////////////////////////
void CLog::LogError(const char* szFormat, ...)
{
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    va_list ArgList;
    char        szBuffer[MAX_WARNING_LENGTH];
    va_start(ArgList, szFormat);
    vsnprintf_s(szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, szFormat, ArgList);
    szBuffer[sizeof(szBuffer) - 1] = '\0';
    va_end(ArgList);

    va_start(ArgList, szFormat);
    LogV(eError, szFormat, ArgList);
    va_end(ArgList);
}

//////////////////////////////////////////////////////////////////////////
void CLog::Log(const char* szFormat, ...)
{
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    va_list arg;
    va_start(arg, szFormat);
    LogV (eMessage, szFormat, arg);
    va_end(arg);
}

//////////////////////////////////////////////////////////////////////////
void CLog::LogAlways(const char* szFormat, ...)
{
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    va_list arg;
    va_start(arg, szFormat);
    LogV (eAlways, szFormat, arg);
    va_end(arg);
}
#endif // !defined(EXCLUDE_NORMAL_LOG)

int MatchStrings(const char* str0, const char* str1)
{
    const char* str[] = { str0, str1 };
    int i, bSkipWord, bAlpha[2], bWS[2], bStop = 0, nDiffs = 0, nWordDiffs, len = 0;
    do
    {
        for (i = 0; i < 2; i++) // skip the spaces, stop at 0
        {
            while (*str[i] == ' ')
            {
                if (!*str[i]++)
                {
                    goto break2;
                }
            }
        }
        bWS[0] = bWS[1] = nWordDiffs = bSkipWord = 0;
        do
        {
            for (i = bAlpha[0] = bAlpha[1] = 0; i < 2; i++)
            {
                if (!bWS[i])
                {
                    do
                    {
                        int chr = *str[i]++;
                        bSkipWord |= iszero(chr - '\\') | iszero(chr - '/') | iszero(chr - '_'); // ignore different words with \,_,/
                        bAlpha[i] = inrange(chr, 'A' - 1, 'Z' + 1) | inrange(chr, 'a' - 1, 'z' + 1);
                        bWS[i] = iszero(chr - ' ');
                        bStop |= iszero(chr);
                    } while (!(bAlpha[i] | bWS[i] | bStop)); // wait for a letter or a space in each input string
                }
            }
            len += bAlpha[0] & bAlpha[1];
            nWordDiffs += 1 - iszero((int)(*str[0] - *str[1])) & - (bAlpha[0] & bAlpha[1]);   // count diffs in this word
        } while ((1 - bWS[0] | 1 - bWS[1]) & 1 - bStop); // wait for space (word end) in both strings
        nDiffs += nWordDiffs & ~-bSkipWord;
    } while (!bStop);
break2:
    return nDiffs * 10 < len;
}


//will log the text both to file and console
//////////////////////////////////////////////////////////////////////
void CLog::LogV(const ELogType type, const char* szFormat, va_list args)
{
    // this is here in case someone called LogV directly, with an invalid formatter.
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    LogV(type, 0, szFormat, args);
}

void CLog::LogV(const ELogType type, [[maybe_unused]] int flags, const char* szFormat, va_list args)
{
    // this is here in case someone called LogV directly, with an invalid formatter.
    if (!CheckLogFormatter(szFormat))
    {
        return;

    }
    if (!szFormat)
    {
        return;
    }

    if (m_pLogVerbosityOverridesWriteToFile && m_pLogVerbosityOverridesWriteToFile->GetIVal())
    {
        if (m_pLogVerbosity && m_pLogVerbosity->GetIVal() < 0)
        {
            return;
        }
    }


    bool bfile = false, bconsole = false;
    const char* szCommand = szFormat;

    uint8 DefaultVerbosity = 0;   // 0 == Always log (except for special -1 verbosity overrides)

    switch (type)
    {
    case eAlways:
        DefaultVerbosity = 0;
        break;
    case eWarningAlways:
        DefaultVerbosity = 0;
        break;
    case eErrorAlways:
        DefaultVerbosity = 0;
        break;
    case eInput:
        DefaultVerbosity = 0;
        break;
    case eInputResponse:
        DefaultVerbosity = 0;
        break;
    case eError:
        DefaultVerbosity = 1;
        break;
    case eWarning:
        DefaultVerbosity = 2;
        break;
    case eMessage:
        DefaultVerbosity = 3;
        break;
    case eComment:
        DefaultVerbosity = 4;
        break;

    default:
        assert(0);
    }

    szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole, DefaultVerbosity);
    if (!bfile && !bconsole)
    {
        return;
    }

    char szBuffer[MAX_WARNING_LENGTH + 32];
    char* szString = szBuffer;
    char* szAfterColour = szString;
    size_t prefixSize = 0;
    switch (type)
    {
    case eWarning:
    case eWarningAlways:
        azstrcpy(szString, MAX_WARNING_LENGTH, "$6[Warning] ");
        szString += 12;     // strlen("$6[Warning] ");
        szAfterColour += 2;
        prefixSize = 12;
        break;

    case eError:
    case eErrorAlways:
        azstrcpy(szString, MAX_WARNING_LENGTH, "$4[Error] ");
        szString += 10;     // strlen("$4[Error] ");
        szAfterColour += 2;
        prefixSize = 10;
        break;

    default:
        break;
    }

    int bufferlen = static_cast<int>(sizeof(szBuffer) - prefixSize);
    if (bufferlen > 0)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(Log_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        int count = vsnprintf_s(szString, bufferlen, bufferlen - 1, szCommand, args);
#endif
        if (count == -1 || count >= bufferlen)
        {
            szBuffer[sizeof(szBuffer) - 1] = '\0';
        }
    }

    if (type == eWarningAlways || type == eWarning || type == eError || type == eErrorAlways)
    {
        const char* sAssetScope = GetAssetScopeString();
        if (*sAssetScope)
        {
            stack_string s = szBuffer;
            s += "\t<Scope> ";
            s += sAssetScope;
            azstrcpy(szBuffer, AZ_ARRAY_SIZE(szBuffer), s.c_str());
        }
    }

    float dt;
    const char* szSpamCheck = *szFormat == '%' ? szString : szFormat;
    if (m_pLogSpamDelay && (dt = m_pLogSpamDelay->GetFVal()) > 0.0f && type != eAlways && type != eInputResponse)
    {
        const int sz = sizeof(m_history) / sizeof(m_history[0]);
        int i, j;
        const AZ::TimeMs realTimeMs = AZ::GetRealElapsedTimeMs();
        const float time = AZ::TimeMsToSeconds(realTimeMs);
        for (i = m_iLastHistoryItem, j = 0; m_history[i].time > time - dt && j < sz; j++, i = i - 1 & sz - 1)
        {
            if (m_history[i].type != type)
            {
                continue;
            }
            if (m_history[i].ptr == szSpamCheck && *(int*)m_history[i].str == *(int*)szFormat || MatchStrings(m_history[i].str, szSpamCheck))
            {
                return;
            }
        }
        i = m_iLastHistoryItem = m_iLastHistoryItem + 1 & sz - 1;
        azstrcpy(m_history[i].str, AZ_ARRAY_SIZE(m_history[i].str), m_history[i].ptr = szSpamCheck);
        m_history[i].type = type;
        m_history[i].time = time;
    }

    LogString(szAfterColour, type);
    if (bfile)
    {
        LogStringToFile(szAfterColour, type, false, MessageQueueState::NotQueued);
    }
    if (bconsole)
    {
        LogStringToConsole(szBuffer, ELogType::eAlways, false);
    }

    switch (type)
    {
    case eAlways:
    case eInput:
    case eInputResponse:
    case eComment:
    case eMessage:
        GetISystem()->GetIRemoteConsole()->AddLogMessage(szString);
        break;
    case eWarning:
    case eWarningAlways:
        GetISystem()->GetIRemoteConsole()->AddLogWarning(szString);
        break;
    case eError:
    case eErrorAlways:
        GetISystem()->GetIRemoteConsole()->AddLogError(szString);
        break;
    }
}

void CLog::LogWithCallback(ELogType type, const LogWriteCallback& messageCallback)
{
    if (!messageCallback)
    {
        return;
    }

    if (m_pLogVerbosityOverridesWriteToFile && m_pLogVerbosityOverridesWriteToFile->GetIVal())
    {
        if (m_pLogVerbosity && m_pLogVerbosity->GetIVal() < 0)
        {
            return;
        }
    }

    bool bfile = false;
    bool bconsole = false;

    uint8_t DefaultVerbosity = 0;   // 0 == Always log (except for special -1 verbosity overrides)

    switch (type)
    {
    case eAlways:
    case eWarningAlways:
    case eErrorAlways:
    case eInput:
    case eInputResponse:
        DefaultVerbosity = 0;
        break;
    case eError:
        DefaultVerbosity = 1;
        break;
    case eWarning:
        DefaultVerbosity = 2;
        break;
    case eMessage:
        DefaultVerbosity = 3;
        break;
    case eComment:
        DefaultVerbosity = 4;
        break;

    default:
        break;
    }

    CheckAgainstVerbosity("", bfile, bconsole, DefaultVerbosity);
    if (!bfile && !bconsole)
    {
        return;
    }

    AZStd::fixed_string<8> colorString;
    AZStd::fixed_string<32> logCategoryString;
    switch (type)
    {
    case eWarning:
    case eWarningAlways:
        colorString = "$6";
        logCategoryString = "[Warning] ";
        break;

    case eError:
    case eErrorAlways:
        colorString = "$4";
        logCategoryString = "[Error] ";
        break;

    default:
        break;
    }

    auto LogStringWithCallback = [this](ELogType logType,
        const LogWriteCallback& messageCallback, AZStd::string_view logCategoryString)
    {
        AZStd::string message;
        AZ::IO::ByteContainerStream outputStream(&message);
        messageCallback(outputStream);
        if (message.empty())
        {
            return;
        }

        AZStd::string logString(logCategoryString);
        logString += message;

        constexpr bool appendToPrevLine = false;
        if (LogToMainThread(logString, logType, appendToPrevLine, SLogMsg::Destination::Default))
        {
            return;
        }

        for (auto callback : m_callbacks)
        {
            callback->OnWrite(logString, logType);
        }
    };
    LogStringWithCallback(type, messageCallback, logCategoryString);
    if (bfile)
    {
        auto LogStringToFileWithCallback = [this](ELogType logType,
            const LogWriteCallback& messageCallback, AZStd::string_view logCategoryString)
        {
#if defined(_RELEASE) && defined(EXCLUDE_NORMAL_LOG) // no file logging in release
            return;
#endif
            if (!m_pSystem || !AZ::IO::FileIOBase::GetInstance())
            {
                return;
            }

            AZStd::string message;
            AZ::IO::ByteContainerStream outputStream(&message);
            messageCallback(outputStream);
            if (message.empty())
            {
                return;
            }

            AZStd::string logString(logCategoryString);
            logString += message;

            constexpr bool appendToPrevLine = false;
            const bool bIsMainThread = LogToMainThread(logString, logType, appendToPrevLine, SLogMsg::Destination::File) == false;

#if defined(_RELEASE)
            if (!bIsMainThread)
            {
                return;
            }
#endif

            // this is a temp timeStr, it is reused in many branches(moved here to reduce stack usage)
            LogStringType timeStr;
            auto console = AZ::Interface<AZ::IConsole>::Get();
            if (uint32_t dwCVarState;
                console != nullptr
                && console->GetCvarValue("log_IncludeTime", dwCVarState) == AZ::GetValueResult::Success)
            {
                // See the log_IncludeTime CVar description as to what
                // values correspond to what time strings
                switch (dwCVarState)
                {
                case 1:
                case 5:
                    timeStr = GetHourMinuteSeconds();
                    break;
                case 2:
                    timeStr = GetElapsedTimeInSeconds();
                    break;
                case 3:
                    timeStr = GetHourMinuteSeconds();
                    timeStr += GetElapsedTimeInSeconds();
                    break;
                case 4:
                    timeStr = GetElapsedTimeSinceStartInSeconds();
                    break;
                case 6:
                    timeStr = GetDateAndHourMinuteSeconds();
                    break;
                default:
                    break;
                }
            }

            // do not OutputDebugString in release.
#if !defined(_RELEASE)
            if (!timeStr.empty())
            {
                AZ::Debug::Platform::OutputToDebugger({}, timeStr);
            }
            AZ::Debug::Platform::OutputToDebugger({}, logString);

            if (!bIsMainThread)
            {
                return;
            }
#endif // !defined(_RELEASE)


            //////////////////////////////////////////////////////////////////////////
            // Call callback function.
            for (auto callback : m_callbacks)
            {
                callback->OnWriteToFile(logString, !appendToPrevLine);
            }
            ////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Write to file.
            //////////////////////////////////////////////////////////////////////////
            if (int logToFile = m_pLogWriteToFile ? m_pLogWriteToFile->GetIVal() : 1; logToFile)
            {
                if (!m_logFileHandle.IsOpen())
                {
                    OpenLogFile(m_szFilename, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
                }

                if (m_logFileHandle.IsOpen())
                {
                    if (appendToPrevLine)
                    {
                        // if adding to a prior line erase the \n at the end.
                        m_logFileHandle.Seek(-2, AZ::IO::GenericStream::SeekMode::ST_SEEK_END);
                    }

                    if (!timeStr.empty())
                    {
                        m_logFileHandle.Write(timeStr.size(), timeStr.data());
                    }
                    m_logFileHandle.Write(logString.size(), logString.data());

#if !defined(KEEP_LOG_FILE_OPEN)
                    CloseLogFile();
#endif
                    // do not use FLUSH on log files.  Doing so will slow the engine down greatly when logging.
                    // (the log is flushed automatically when an unhandled exception occurs)
                }
            }
        };
        LogStringToFileWithCallback(type, messageCallback, logCategoryString);
    }
    if (bconsole)
    {
        auto LogStringToConsoleWithCallback = [this](const LogWriteCallback& messageCallback,
            AZStd::string_view colorString, AZStd::string_view logCategoryString)
        {
#if defined(_RELEASE) && defined(EXCLUDE_NORMAL_LOG) // no console logging in release
            return;
#endif
            const ELogType logType = ELogType::eAlways;

            AZStd::string message;
            AZ::IO::ByteContainerStream outputStream(&message);
            messageCallback(outputStream);
            if (message.empty())
            {
                return;
            }

            AZStd::string logString(colorString);
            logString += AZStd::string_view(logCategoryString);
            logString += message;

            constexpr bool appendToPrevLine = false;
            if (LogToMainThread(logString, logType, appendToPrevLine, SLogMsg::Destination::Console))
            {
                return;
            }

            if (!m_pSystem)
            {
                return;
            }
            IConsole* console = m_pSystem->GetIConsole();
            if (!console)
            {
                return;
            }

            console->PrintLine(logString);

            // Call callback function.
            for (auto callback : m_callbacks)
            {
                callback->OnWriteToConsole(logString, !appendToPrevLine);
            }
        };
        LogStringToConsoleWithCallback(messageCallback, colorString, logCategoryString);
    }

    if (m_pSystem != nullptr)
    {
        auto LogStringToRemoteConsoleWithCallback = [](ELogType logType, const LogWriteCallback& messageCallback)
        {
            AZStd::string message;
            AZ::IO::ByteContainerStream outputStream(&message);
            messageCallback(outputStream);
            if (message.empty())
            {
                return;
            }

            switch (logType)
            {
            case eAlways:
            case eInput:
            case eInputResponse:
            case eComment:
            case eMessage:
                GetISystem()->GetIRemoteConsole()->AddLogMessage(message);
                break;
            case eWarning:
            case eWarningAlways:
                GetISystem()->GetIRemoteConsole()->AddLogWarning(message);
                break;
            case eError:
            case eErrorAlways:
                GetISystem()->GetIRemoteConsole()->AddLogError(message);
                break;
            }
        };

        LogStringToRemoteConsoleWithCallback(type, messageCallback);
    }
}

//will log the text both to the end of file and console
//////////////////////////////////////////////////////////////////////
#if !defined(EXCLUDE_NORMAL_LOG)
void CLog::LogAppendWithPrevLine(const char* szFormat, ...)
{
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    if (m_pLogVerbosity && m_pLogVerbosity->GetIVal() < 0)
    {
        return;
    }

    if (m_pLogSpamDelay && m_pLogSpamDelay->GetFVal())
    { // Vlad: SpamDelay does not work correctly with LogAppendWithPrevLine
        return;
    }

    if (!szFormat)
    {
        return;
    }

    bool bfile = false, bconsole = false;
    const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
    if (!bfile && !bconsole)
    {
        return;
    }

    va_list     arglist;

    char szTemp[MAX_TEMP_LENGTH_SIZE];
    va_start(arglist, szFormat);
    vsnprintf_s(szTemp, sizeof(szTemp), sizeof(szTemp) - 1, szCommand, arglist);
    szTemp[sizeof(szTemp) - 1] = 0;
    va_end(arglist);

    if (bfile)
    {
        LogToFileAppendWithPrevLine("%s", szTemp);
    }
    if (bconsole)
    {
        LogToConsoleAppendWithPrevLine("%s", szTemp);
    }
}

//log to console only
//////////////////////////////////////////////////////////////////////
void CLog::LogStringToConsole(AZStd::string_view message, ELogType logType, bool appendToPrevLine)
{
    #if defined(_RELEASE) && defined(EXCLUDE_NORMAL_LOG) // no console logging in release
    return;
    #endif

    if (message.empty())
    {
        return;
    }

    if (LogToMainThread(message, logType, appendToPrevLine, SLogMsg::Destination::Console))
    {
        return;
    }

    if (!m_pSystem)
    {
        return;
    }
    IConsole* console = m_pSystem->GetIConsole();
    if (!console)
    {
        return;
    }

    if (appendToPrevLine)
    {
        console->PrintLineAppendWithPrevLine(message);
    }
    else
    {
        console->PrintLine(message);
    }

    // Call callback function.
    for (auto callback : m_callbacks)
    {
        callback->OnWriteToConsole(message, !appendToPrevLine);
    }
}

//log to console only
//////////////////////////////////////////////////////////////////////
void CLog::LogToConsole(const char* szFormat, ...)
{
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    if (m_pLogVerbosity && m_pLogVerbosity->GetIVal() < 0)
    {
        return;
    }

    if (!szFormat)
    {
        return;
    }

    bool bfile = false, bconsole = false;
    const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
    if (!bconsole)
    {
        return;
    }

    va_list     arglist;

    char szBuffer[MAX_WARNING_LENGTH];
    va_start(arglist, szFormat);
    vsnprintf_s(szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, szCommand, arglist);
    szBuffer[sizeof(szBuffer) - 1] = 0;
    va_end(arglist);

    LogStringToConsole(szBuffer, ELogType::eAlways, false);
}

//////////////////////////////////////////////////////////////////////
void CLog::LogToConsoleAppendWithPrevLine(const char* szFormat, ...)
{
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    if (m_pLogVerbosity && m_pLogVerbosity->GetIVal() < 0)
    {
        return;
    }

    if (!szFormat)
    {
        return;
    }

    bool bfile = false, bconsole = false;
    const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
    if (!bconsole)
    {
        return;
    }

    va_list     arglist;

    char szTemp[MAX_TEMP_LENGTH_SIZE];
    va_start(arglist, szFormat);
    vsnprintf_s(szTemp, sizeof(szTemp), sizeof(szTemp) - 1, szCommand, arglist);
    szTemp[sizeof(szTemp) - 1] = 0;
    va_end(arglist);

    if (!m_pSystem)
    {
        return;
    }

    LogStringToConsole(szTemp, ELogType::eAlways, true);
}
#endif // !defined(EXCLUDE_NORMAL_LOG)



//////////////////////////////////////////////////////////////////////
[[maybe_unused]] static AZStd::string_view RemoveColorCode(AZStd::string_view rStr)
{
    if (rStr.size() >= 2 && rStr.starts_with('$') && rStr[1] >= '0' && rStr[1] <= '9')
    {
        rStr.remove_prefix(2);
    }

    return rStr;
}

#if defined(SUPPORT_LOG_IDENTER)
//////////////////////////////////////////////////////////////////////
void CLog::BuildIndentString()
{
    m_indentWithString = "";

    for (uint8 i = 0; i < m_indentation; ++i)
    {
        m_indentWithString = indentString + m_indentWithString;
    }
}

//////////////////////////////////////////////////////////////////////
void CLog::Indent(CLogIndenter* indenter)
{
    indenter->SetNextIndenter(m_topIndenter);
    m_topIndenter = indenter;
    ++m_indentation;
    BuildIndentString();
}

//////////////////////////////////////////////////////////////////////
void CLog::Unindent(CLogIndenter* indenter)
{
    assert (indenter == m_topIndenter);
    assert (m_indentation);
    m_topIndenter = m_topIndenter->GetNextIndenter();
    --m_indentation;
    BuildIndentString();
}

//////////////////////////////////////////////////////////////////////////
void CLog::PushAssetScopeName(const char* sAssetType, const char* sName)
{
    assert(sAssetType);
    assert(sName);

    SAssetScopeInfo as;
    as.sType = sAssetType;
    as.sName = sName;
    AZStd::scoped_lock scope_lock(m_assetScopeQueueLock);
    m_assetScopeQueue.push_back(as);
}

void CLog::PopAssetScopeName()
{
    AZStd::scoped_lock scope_lock(m_assetScopeQueueLock);
    assert(!m_assetScopeQueue.empty());
    if (!m_assetScopeQueue.empty())
    {
        m_assetScopeQueue.pop_back();
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CLog::GetAssetScopeString()
{
    AZStd::scoped_lock scope_lock(m_assetScopeQueueLock);

    m_assetScopeString.clear();
    for (size_t i = 0; i < m_assetScopeQueue.size(); i++)
    {
        m_assetScopeString += "[";
        m_assetScopeString += m_assetScopeQueue[i].sType;
        m_assetScopeString += "]";
        m_assetScopeString += m_assetScopeQueue[i].sName;
        if (i <  m_assetScopeQueue.size() - 1)
        {
            m_assetScopeString += " > ";
        }
    }
    return m_assetScopeString.c_str();
};
#endif

bool CLog::LogToMainThread(AZStd::string_view szString, ELogType logType, bool appendToPrevLine, SLogMsg::Destination destination)
{
    if (CryGetCurrentThreadId() != m_nMainThreadId)
    {
        // When logging from other thread then main, push all log strings to queue.
        constexpr size_t fixedBufferMaxSize = AZStd::variant_alternative_t<0, SLogMsg::MessageString>{}.max_size();
        SLogMsg msg;
        if (szString.size() <= fixedBufferMaxSize)
        {
            // Store string in fixed buffer if less than the fixed_string::max_size
            msg.msg.emplace<0>(szString);
        }
        else
        {
            msg.msg.emplace<AZStd::string>(szString);
        }
        msg.m_appendToPreviousLine = appendToPrevLine;
        msg.destination = destination;
        msg.logType = logType;
        m_threadSafeMsgQueue.push(msg);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
#if !defined(EXCLUDE_NORMAL_LOG)
void CLog::LogStringToFile(AZStd::string_view message, ELogType logType, bool appendToPrevLine, [[maybe_unused]] MessageQueueState queueState)
{
#if defined(_RELEASE) && defined(EXCLUDE_NORMAL_LOG) // no file logging in release
    return;
#endif

    if (message.empty())
    {
        return;
    }

    if (!m_pSystem || !AZ::IO::FileIOBase::GetInstance())
    {
        return;
    }

    // this is a temp timeStr, it is reused in many branches(moved here to reduce stack usage)
    LogStringType timeStr;

    message = RemoveColorCode(message);

    bool bIsMainThread = LogToMainThread(message, logType, appendToPrevLine, SLogMsg::Destination::File) == false;

#if defined(_RELEASE)
    if (!bIsMainThread)
    {
        return;
    }
#endif

#if defined(SUPPORT_LOG_IDENTER)
    if (bIsMainThread)
    {
        if (m_topIndenter)
        {
            m_topIndenter->DisplaySectionText();
        }
        tempString = m_indentWithString + tempString;
    }
#endif

    auto console = AZ::Interface<AZ::IConsole>::Get();
    if (uint32_t dwCVarState;
        console != nullptr
        && console->GetCvarValue("log_IncludeTime", dwCVarState) == AZ::GetValueResult::Success)
    {
        // See the log_IncludeTime CVar description as to what
        // values correspond to what time strings
        switch (dwCVarState)
        {
        case 1:
        case 5:
            timeStr = GetHourMinuteSeconds();
            break;
        case 2:
            timeStr = GetElapsedTimeInSeconds();
            break;
        case 3:
            timeStr = GetHourMinuteSeconds();
            timeStr += GetElapsedTimeInSeconds();
            break;
        case 4:
            timeStr = GetElapsedTimeSinceStartInSeconds();
            break;
        case 6:
            timeStr = GetDateAndHourMinuteSeconds();
            break;
        default:
            break;
        }
    }

    // do not output in release.
#if !defined(_RELEASE)
    if (queueState == MessageQueueState::NotQueued)
    {
        if (!timeStr.empty())
        {
            AZ::Debug::Trace::Instance().OutputToRawAndDebugger(nullptr, timeStr.data());
        }
        AZ::Debug::Trace::Instance().OutputToRawAndDebugger(nullptr, message.data());
        if (!message.ends_with('\n'))
        {
           AZ::Debug::Trace::Instance().OutputToRawAndDebugger(nullptr, "\n");
        }
    }

    if (!bIsMainThread)
    {
        return;
    }
#endif // !defined(_RELEASE)


    //////////////////////////////////////////////////////////////////////////
    // Call callback function.
    for (auto callback : m_callbacks)
    {
        callback->OnWriteToFile(message, !appendToPrevLine);
    }
    ////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Write to file.
    //////////////////////////////////////////////////////////////////////////
    int logToFile = m_pLogWriteToFile ? m_pLogWriteToFile->GetIVal() : 1;

    if (logToFile)
    {
        if (!m_logFileHandle.IsOpen())
        {
            OpenLogFile(m_szFilename, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath);
        }

        if (m_logFileHandle.IsOpen())
        {
            if (appendToPrevLine)
            {
                // if adding to a prior line erase the \n at the end.
                m_logFileHandle.Seek(-2, AZ::IO::GenericStream::SeekMode::ST_SEEK_END);
            }

            if (!timeStr.empty())
            {
                m_logFileHandle.Write(timeStr.size(), timeStr.data());
            }
            m_logFileHandle.Write(message.size(), message.data());
            if (!message.ends_with('\n'))
            {
                constexpr AZStd::string_view newline("\n");
                m_logFileHandle.Write(newline.size(), newline.data());
            }

#if !defined(KEEP_LOG_FILE_OPEN)
            CloseLogFile();
#endif
            // do not use FLUSH on log files.  Doing so will slow the engine down greatly when logging.
            // (the log is flushed automatically when an unhandled exception occurs)
        }
    }
}

void CLog::LogString(AZStd::string_view szString, ELogType logType)
{
    if (szString.empty())
    {
        return;
    }
    if (LogToMainThread(szString, logType, false, SLogMsg::Destination::Default))
    {
        return;
    }

    for (auto callback : m_callbacks)
    {
        callback->OnWrite(szString, logType);
    }
}

//same as above but to a file
//////////////////////////////////////////////////////////////////////
void CLog::LogToFileAppendWithPrevLine(const char* szFormat, ...)
{
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    if (m_pLogVerbosity && m_pLogVerbosity->GetIVal() < 0)
    {
        return;
    }

    if (!m_szFilename[0] || !szFormat)
    {
        return;
    }

    bool bfile = false, bconsole = false;
    const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
    if (!bfile)
    {
        return;
    }

    va_list     arglist;
    va_start(arglist, szFormat);
    auto szTemp = AZStd::fixed_string<MAX_TEMP_LENGTH_SIZE>::format_arg(szCommand, arglist);
    va_end(arglist);

    LogStringToFile(szTemp, ELogType::eAlways, true, MessageQueueState::NotQueued);
}

//log to the file specified in setfilename
//////////////////////////////////////////////////////////////////////
void CLog::LogToFile(const char* szFormat, ...)
{
    if (!CheckLogFormatter(szFormat))
    {
        return;
    }

    if (m_pLogVerbosity && m_pLogVerbosity->GetIVal() < 0)
    {
        return;
    }

    if (!m_szFilename[0] || !szFormat)
    {
        return;
    }

    bool bfile = false, bconsole = false;
    const char* szCommand = CheckAgainstVerbosity(szFormat, bfile, bconsole);
    if (!bfile)
    {
        return;
    }

    va_list     arglist;
    va_start(arglist, szFormat);
    auto szTemp = AZStd::fixed_string<MAX_TEMP_LENGTH_SIZE>::format_arg(szCommand, arglist);
    va_end(arglist);

    LogStringToFile(szTemp, ELogType::eAlways, false, MessageQueueState::NotQueued);
}
#endif // !defined(EXCLUDE_NORMAL_LOG)

//////////////////////////////////////////////////////////////////////
void CLog::CreateBackupFile() const
{
    if (!m_backupLogs)
    {
        return;
    }
#if AZ_LEGACY_CRYSYSTEM_TRAIT_ALLOW_CREATE_BACKUP_LOG_FILE
    // simple:
    //      string bakpath = PathUtil::ReplaceExtension(m_szFilename,"bak");
    //      CopyFile(m_szFilename,bakpath.c_str(),false);

    // boswej: only create a backup if logging to the engine root, otherwise the
    // log output has been overridden and the user is responsible
    AZStd::string logDir = PathUtil::RemoveSlash(PathUtil::ToUnixPath(PathUtil::GetParentDirectory(m_szFilename)));

    AZStd::string sExt = PathUtil::GetExt(m_szFilename);
    AZStd::string sFileWithoutExt = PathUtil::GetFileName(m_szFilename);

    {
        assert(::strstr(sFileWithoutExt.c_str(), ":") == 0);
        assert(::strstr(sFileWithoutExt.c_str(), "\\") == 0);
    }

    PathUtil::RemoveExtension(sFileWithoutExt);

    AZ::IO::FileIOBase* fileSystem = AZ::IO::FileIOBase::GetDirectInstance();
    AZ::IO::HandleType inFileHandle = AZ::IO::InvalidHandle;
    fileSystem->Open(m_szFilename, AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary, inFileHandle);

    AZStd::string sBackupNameAttachment;

    // parse backup name attachment
    // e.g. BackupNameAttachment="attachment name"
    if (inFileHandle != AZ::IO::InvalidHandle)
    {
        bool bKeyFound = false;
        AZStd::string sName;

        while (!fileSystem->Eof(inFileHandle))
        {
            uint8 c = static_cast<uint8>(AZ::IO::GetC(inFileHandle));

            if (c == '\"')
            {
                if (!bKeyFound)
                {
                    bKeyFound = true;

                    if (sName.find("BackupNameAttachment=") == AZStd::string::npos)
                    {
                        AZ::Debug::Platform::OutputToDebugger("CrySystem Log", "Log::CreateBackupFile ERROR '");
                        AZ::Debug::Platform::OutputToDebugger({}, sName.c_str());
                        AZ::Debug::Platform::OutputToDebugger({}, "' not recognized \n");
                        assert(0);      // broken log file? - first line should include this name - written by LogVersion()
                        return;
                    }
                    sName.clear();
                }
                else
                {
                    sBackupNameAttachment = sName;
                    break;
                }
                continue;
            }
            if (c >= ' ')
            {
                sName += c;
            }
            else
            {
                break;
            }
        }
        fileSystem->Close(inFileHandle);
    }

    AZStd::string bakdest = PathUtil::Make(LOG_BACKUP_PATH, sFileWithoutExt + sBackupNameAttachment + "." + sExt);
    fileSystem->CreatePath(LOG_BACKUP_PATH);
    azstrcpy(m_sBackupFilename, AZ_ARRAY_SIZE(m_sBackupFilename), bakdest.c_str());
    // Remove any existing backup file with the same name first since the copy will fail otherwise.
    fileSystem->Remove(m_sBackupFilename);
    fileSystem->Copy(m_szFilename, bakdest.c_str());
#endif // AZ_LEGACY_CRYSYSTEM_TRAIT_ALLOW_CREATE_BACKUP_LOG_FILE
}

void CLog::CheckAndPruneBackupLogs() const
{
    AZ::IO::FileIOBase* fileSystem = AZ::IO::FileIOBase::GetDirectInstance();

    AZ::u64 totalBackupDirectorySize = 0;

    struct fileInfo
    {
        time_t modTime;
        AZStd::string filename;
        AZ::u64 filesize;

        fileInfo(time_t _modTime, const char* _name, AZ::u64 _size)
        {
            modTime = _modTime;
            filename = _name;
            filesize = _size;
        }
    };

    AZStd::list<fileInfo> fileInfoList;

    // Now that we've copied the new log over, lets check the size of the backup folder and trim it as necessary to keep it within appropriate limits
    fileSystem->FindFiles(LOG_BACKUP_PATH, "*",
        [&totalBackupDirectorySize, &fileSystem, &fileInfoList](const char* fileName)
    {
        AZ::u64 size;
        fileSystem->Size(fileName, size);
        AZ::u64 modTime = fileSystem->ModificationTime(fileName);
        fileInfoList.push_back(fileInfo(modTime, fileName, size));

        totalBackupDirectorySize += size;
        return true;
    });

    AZ::u64 max_size = LogCVars::max_backup_directory_size_mb;
    max_size = max_size << 20;  // Convert from MB to bytes

    if (totalBackupDirectorySize > max_size)
    {
        // Sort the list from lowest to highest modtime (oldest to newest logs)
        fileInfoList.sort([](const fileInfo &a, const fileInfo&b) { return a.modTime < b.modTime; });

        while (totalBackupDirectorySize > max_size && fileInfoList.size() > 0)
        {
            fileSystem->Remove(fileInfoList.front().filename.c_str());
            totalBackupDirectorySize -= fileInfoList.front().filesize;
            fileInfoList.pop_front();
        }
    }
}

//set the file used to log to disk
//////////////////////////////////////////////////////////////////////
bool CLog::SetFileName(const char* fileNameOrAbsolutePath, bool backupLogs)
{
    m_backupLogs = backupLogs;
    if (!fileNameOrAbsolutePath)
    {
        return false;
    }

    AZStd::string previousFilename = m_szFilename;
    azstrncpy(m_szFilename, AZ_ARRAY_SIZE(m_szFilename), fileNameOrAbsolutePath, sizeof(m_szFilename));

    CreateBackupFile();

    if (m_logFileHandle.IsOpen() && m_szFilename != previousFilename)
    {
        CloseLogFile();
        if (!OpenLogFile(m_szFilename, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath))
        {
            // Failed to open/create the new file. Go back to the previous
            // state of the log appending to the previous file.
            azstrncpy(m_szFilename, AZ_ARRAY_SIZE(m_szFilename), previousFilename.c_str(), sizeof(m_szFilename));
            OpenLogFile(m_szFilename, AZ::IO::OpenMode::ModeAppend);
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
const char* CLog::GetFileName()
{
    return m_szFilename;
}

const char* CLog::GetBackupFileName()
{
    return m_sBackupFilename;
}

//////////////////////////////////////////////////////////////////////
void CLog::UpdateLoadingScreen(const char* szFormat, ...)
{
#if !defined(EXCLUDE_NORMAL_LOG)
    if (szFormat)
    {
        // This function is OK to call with nullptr, but then it does not log anything.
        va_list args;
        va_start(args, szFormat);

        LogV(ILog::eMessage, szFormat, args);

        va_end(args);
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
int CLog::GetVerbosityLevel()
{
    if (m_pLogVerbosity)
    {
        return (m_pLogVerbosity->GetIVal());
    }

    return (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Checks the verbosity of the message and returns NULL if the message must NOT be
// logged, or the pointer to the part of the message that should be logged
// NOTE:
//    Normally, this is either the pText pointer itself, or the pText+1, meaning
//    the first verbosity character may be cut off)
//    This is done in order to avoid modification of const char*, which may cause GPF
//    sometimes, or kill the verbosity qualifier in the text that's gonna be passed next time.
const char* CLog::CheckAgainstVerbosity(const char* pText, bool& logtofile, bool& logtoconsole, const uint8 DefaultVerbosity)
{
    // the max verbosity (most detailed level)
#if defined(RELEASE)
    const unsigned char nMaxVerbosity = 0;
#else // #if defined(RELEASE)
    const unsigned char nMaxVerbosity = 8;
#endif // #if defined(RELEASE)

    // the current verbosity of the log
    int nLogVerbosityConsole = m_pLogVerbosity ? m_pLogVerbosity->GetIVal() : nMaxVerbosity;
    int nLogVerbosityFile = m_pLogWriteToFileVerbosity ? m_pLogWriteToFileVerbosity->GetIVal() : nMaxVerbosity;

    logtoconsole = (nLogVerbosityConsole >= DefaultVerbosity);

    //to preserve logging to TTY, logWriteToFile logic has been moved to inside logStringToFile
    //int logToFileCVar = m_pLogWriteToFile ? m_pLogWriteToFile->GetIVal() : 1;

    logtofile = (nLogVerbosityFile >= DefaultVerbosity);

    return pText;
}


//////////////////////////////////////////////////////////////////////////
void CLog::AddCallback(ILogCallback* pCallback)
{
    stl::push_back_unique(m_callbacks, pCallback);
}

//////////////////////////////////////////////////////////////////////////
void CLog::RemoveCallback(ILogCallback* pCallback)
{
    m_callbacks.remove(pCallback);
}

//////////////////////////////////////////////////////////////////////////
void CLog::Update()
{
    if (CryGetCurrentThreadId() == m_nMainThreadId)
    {
        if (!m_threadSafeMsgQueue.empty())
        {
            AZStd::scoped_lock lock(m_threadSafeMsgQueue.get_lock());   // Get the lock and hold onto it until we clear the entire queue (prevents other threads adding more things in while we clear it)
            // Must be called from main thread
            constexpr auto GetMessageView = [](auto&& messageView) constexpr -> AZStd::string_view
            {
                return messageView;
            };
            SLogMsg msg;
            while (m_threadSafeMsgQueue.try_pop(msg))
            {
                AZStd::string_view messageView = AZStd::visit(GetMessageView, msg.msg);
                if (msg.destination == SLogMsg::Destination::Console)
                {
                    LogStringToConsole(messageView, msg.logType, msg.m_appendToPreviousLine);
                }
                else if (msg.destination == SLogMsg::Destination::File)
                {
                    LogStringToFile(messageView, msg.logType, msg.m_appendToPreviousLine, MessageQueueState::Queued);
                }
                else
                {
                    LogString(messageView, msg.logType);
                }
            }
            stl::free_container(m_threadSafeMsgQueue);
        }

        if (LogCVars::s_log_tick != 0)
        {
            static AZ::TimeUs t0 = AZ::GetElapsedTimeUs();
            const AZ::TimeUs t1 = AZ::GetElapsedTimeUs();
            const float tSec = AZ::TimeUsToSeconds(t1 - t0);
            if (tSec > LogCVars::s_log_tick)
            {
                t0 = t1;

                char sTime[128];
                time_t ltime;
                time(&ltime);
#ifdef AZ_COMPILER_MSVC
                struct tm today;
                localtime_s(&today, &ltime);
                strftime(sTime, sizeof(sTime) - 1, "<%H:%M:%S> ", &today);
#else
                auto today = localtime(&ltime);
                strftime(sTime, sizeof(sTime) - 1, "<%H:%M:%S> ", today);
#endif
                LogAlways("<tick> %s", sTime);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CLog::GetModuleFilter()
{
    if (m_pLogModule)
    {
        return m_pLogModule->GetString();
    }
    return "";
}

void CLog::FlushAndClose()
{
#if defined(KEEP_LOG_FILE_OPEN)
    CloseLogFile();
#endif
}

void CLog::Flush()
{
#if defined(KEEP_LOG_FILE_OPEN)
    m_logFileHandle.Flush();
#endif
}

#if defined(KEEP_LOG_FILE_OPEN)
void CLog::LogFlushFile([[maybe_unused]] IConsoleCmdArgs* pArgs)
{
    if ((gEnv) && (gEnv->pLog))
    {
        gEnv->pLog->Flush();
    }
}
#endif
