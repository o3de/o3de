/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_CRYCOMMON_ILOG_H
#define CRYINCLUDE_CRYCOMMON_ILOG_H
#pragma once

#include <IMiniLog.h> // <> required for Interfuscator

// enable this define to support log scopes to provide more context information for log lines
// this code is disable by default due it's runtime cost
//#define SUPPORT_LOG_IDENTER

// Summary:
//   Callback interface to the ILog.
struct ILogCallback
{
    // <interfuscator:shuffle>
    virtual ~ILogCallback() {}
    //OnWrite will always be called even if verbosity settings cause OnWriteToConsole and OnWriteToFile to not be called.
    virtual void OnWrite(const char* sText, IMiniLog::ELogType type) = 0;
    virtual void OnWriteToConsole(const char* sText, bool bNewLine) = 0;
    virtual void OnWriteToFile(const char* sText, bool bNewLine) = 0;
    // </interfuscator:shuffle>
};

// Summary:
//   Interface for logging operations based on IMiniLog.
// Notes:
//   Logging in CryEngine should be done using the following global functions:
//      CryLog (eMessage)
//      CryLogAlways (eAlways)
//      CryError (eError)
//      CryWarning (eWarning)
//      CryComment (eComment)
//   ILog gives you more control on logging operations.
// See also:
//   IMiniLog, CryLog, CryLogAlways, CryError, CryWarning
struct ILog
    : public IMiniLog
{
    // <interfuscator:shuffle>
    virtual void Release() = 0;

    // Summary:
    //   Sets the file used to log to disk.
    //   if you don't specify the full path it will be assumed relative to the 'log folder'
    //   dobackups controls whether or not it will backup old logs when creating new ones.
    virtual bool SetFileName(const char* fileNameOrFullPath, bool doBackups) = 0;

    // Summary:
    //   Gets the filename used to log to disk.
    virtual const char* GetFileName() = 0;

    // Summary:
    //   Gets the filename where the current log backup was copied to on disk
    virtual const char* GetBackupFileName() = 0;

    //all the following functions will be removed are here just to be able to compile the project ---------------------------

    // Summary:
    //   Logs the text both to file and console.
    virtual void    Log(const char* szCommand, ...) PRINTF_PARAMS(2, 3) = 0;

    virtual void    LogAlways(const char* szCommand, ...) PRINTF_PARAMS(2, 3) = 0;

    virtual void    LogWarning(const char* szCommand, ...) PRINTF_PARAMS(2, 3) = 0;

    virtual void    LogError(const char* szCommand, ...) PRINTF_PARAMS(2, 3) = 0;

    // Summary:
    //   Logs the text both to the end of file and console.
    virtual void    LogPlus(const char* command, ...) PRINTF_PARAMS(2, 3) = 0;

    // Summary:
    //   Logs to the file specified in SetFileName.
    // See also:
    //   SetFileName
    virtual void    LogToFile(const char* command, ...) PRINTF_PARAMS(2, 3) = 0;

    //
    virtual void    LogToFilePlus(const char* command, ...) PRINTF_PARAMS(2, 3) = 0;

    // Summary:
    //   Logs to console only.
    virtual void    LogToConsole(const char* command, ...) PRINTF_PARAMS(2, 3) = 0;

    //
    virtual void    LogToConsolePlus(const char* command, ...) PRINTF_PARAMS(2, 3) = 0;

    //
    virtual void    UpdateLoadingScreen(const char* command, ...) PRINTF_PARAMS(2, 3) = 0;

    //
    virtual void RegisterConsoleVariables() {}

    //
    virtual void UnregisterConsoleVariables() {}

    // Notes:
    //   Full logging (to console and file) can be enabled with verbosity 4.
    //   In the console 'log_Verbosity 4' command can be used.
    virtual void    SetVerbosity(int verbosity) = 0;

    virtual int     GetVerbosityLevel() = 0;

    virtual void  AddCallback(ILogCallback* pCallback) = 0;
    virtual void  RemoveCallback(ILogCallback* pCallback) = 0;

    // Notes:
    //   The function called every frame by system.
    virtual void Update() = 0;

    virtual const char* GetModuleFilter() = 0;

    // Asset scope strings help to figure out asset dependencies in case of asset loading errors.
    // Should not be used directly, only by using define CRY_DEFINE_ASSET_SCOPE
    // @see CRY_DEFINE_ASSET_SCOPE
    virtual void PushAssetScopeName([[maybe_unused]] const char* sAssetType, [[maybe_unused]] const char* sName) {};
    virtual void PopAssetScopeName() {};
    virtual const char* GetAssetScopeString() { return ""; };
    // </interfuscator:shuffle>

#if defined(SUPPORT_LOG_IDENTER)
    virtual void Indent(class CLogIndenter* indenter) = 0;
    virtual void Unindent(class CLogIndenter* indenter) = 0;
#endif

    virtual void FlushAndClose() = 0;
};

#if !defined(SUPPORT_LOG_IDENTER)
#define INDENT_LOG_DURING_SCOPE(...) (void)(0)
#define CRY_DEFINE_ASSET_SCOPE(sAssetType, sAssetName) (void)(0)
#else
class CLogIndenter
{
public:
    CLogIndenter(ILog* log)
        : m_log(log)
        , m_enabled(false)
        , m_nextIndenter(NULL)
        , m_needToPrintSectionText(false)
    {
    }

    void Enable(bool enable = true, const char* sectionTextFormat = NULL, ...)
    {
        va_list args;
        va_start (args, sectionTextFormat);

        enable &= (m_log != NULL);

        if (enable != m_enabled)
        {
            if (sectionTextFormat && enable)
            {
                char buffer[1024];

                vsnprintf(buffer, sizeof(buffer), sectionTextFormat, args);
                buffer[sizeof(buffer) - 1] = '\0';

                m_sectionText = buffer;
                m_needToPrintSectionText = true;
            }
            else
            {
                m_sectionText = "";
                m_needToPrintSectionText = m_nextIndenter ? m_nextIndenter->m_needToPrintSectionText : false;
            }

            assert(m_log);

            if (enable)
            {
                m_log->Indent(this);
            }
            else
            {
                m_log->Unindent(this);
            }
            m_enabled = enable;
        }

        va_end (args);
    }

    CLogIndenter* GetNextIndenter()
    {
        return m_nextIndenter;
    }

    void SetNextIndenter(CLogIndenter* indenter)
    {
        m_nextIndenter = indenter;
    }

    void DisplaySectionText()
    {
        if (m_needToPrintSectionText)
        {
            m_needToPrintSectionText = false;
            string sectionText = m_sectionText;
            Enable(false);

            if (m_nextIndenter)
            {
                m_nextIndenter->DisplaySectionText();
            }

            if (!sectionText.empty())
            {
                assert (m_log);
                m_log->Log ("%s", sectionText.c_str());
            }
            Enable(true);
        }
    }

    ~CLogIndenter()
    {
        Enable(false);
    }

private:
    bool m_enabled;
    bool m_needToPrintSectionText;
    ILog* m_log;
    CLogIndenter* m_nextIndenter;
    string m_sectionText;
};

class CLogAssetScopeName
{
    ILog* m_pLog;
public:
    CLogAssetScopeName(ILog* pLog, const char* sAssetType, const char* sAssetName)
        : m_pLog(pLog) { pLog->PushAssetScopeName(sAssetType, sAssetName); }
    ~CLogAssetScopeName() { m_pLog->PopAssetScopeName(); }
};

#define ILOG_CONCAT_IMPL(x, y) x##y
#define ILOG_CONCAT_MACRO(x, y) ILOG_CONCAT_IMPL(x, y)
#define INDENT_LOG_DURING_SCOPE(...) CLogIndenter ILOG_CONCAT_MACRO(indentMe, __LINE__) ((CryGetCurrentThreadId() == gEnv->mMainThreadId) ? gEnv->pLog : NULL); ILOG_CONCAT_MACRO(indentMe, __LINE__).Enable(__VA_ARGS__)

#define CRY_DEFINE_ASSET_SCOPE(sAssetType, sAssetName) CLogAssetScopeName __asset_scope_name(gEnv->pLog, sAssetType, sAssetName);
#endif


#endif // CRYINCLUDE_CRYCOMMON_ILOG_H



