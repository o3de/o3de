/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_LOGFILE_H
#define CRYINCLUDE_EDITOR_LOGFILE_H

#pragma once

#include "ILog.h"
#include <IConsole.h>
#include <stdarg.h>

#define MAX_LOGBUFFER_SIZE 16384

class QTextEdit;
class QListWidget;

//struct IConsole;
//struct ICVar;

//////////////////////////////////////////////////////////////////////////
// Global log functions.
//////////////////////////////////////////////////////////////////////////

// the 'v' versions are for when you've already done your unpack, so that we are not forced to truncate the buffer

//! Displays error message.
SANDBOX_API void Error(const char* format, ...);
SANDBOX_API void ErrorV(const char* format, va_list argList);
//! Log to console and file.
SANDBOX_API void Log(const char* format, ...);
SANDBOX_API void LogV(const char* format, va_list argList);
//! Display Warning dialog.
SANDBOX_API void Warning(const char* format, ...);
SANDBOX_API void WarningV(const char* format, va_list argList);

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
/*!
 *  CLogFile implements ILog interface.
 */
class SANDBOX_API CLogFile
    : public ILogCallback
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    static const char* GetLogFileName();
    static void AttachListBox(QListWidget* hWndListBox);
    static void AttachEditBox(QTextEdit* hWndEditBox);

    //! Write to log snapshot of current process memory usage.
    static QString GetMemUsage();

    static void WriteString(const char* pszString);
    static void WriteString(const QString& string) { WriteString(string.toUtf8().data()); }
    static void WriteLine(const char* pszLine);
    static void WriteLine(const QString& string) { WriteLine(string.toUtf8().data()); }
    static void FormatLine(const char* pszMessage, ...);
    static void FormatLineV(const char* pszMessage, va_list argList);

    //////////////////////////////////////////////////////////////////////////
    // ILogCallback
    //////////////////////////////////////////////////////////////////////////
    virtual void OnWrite([[maybe_unused]] AZStd::string_view sText, [[maybe_unused]] IMiniLog::ELogType type) override {};
    virtual void OnWriteToConsole(AZStd::string_view sText, bool bNewLine) override;
    virtual void OnWriteToFile(AZStd::string_view sText, bool bNewLine) override;
    //////////////////////////////////////////////////////////////////////////

    // logs some useful information
    // should be called after CryLog() is available
    static void AboutSystem();

private:
    static void OpenFile();

    // Attached control(s)
    static QListWidget* m_hWndListBox;
    static QTextEdit* m_hWndEditBox;
    static bool m_bShowMemUsage;
    static bool m_bIsQuitting;
};

#endif // CRYINCLUDE_EDITOR_LOGFILE_H
