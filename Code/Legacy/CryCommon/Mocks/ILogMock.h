/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <ILog.h>

class LogMock
    : public ILog
{
public:
    MOCK_METHOD3(LogV,
        void(ELogType nType, const char* szFormat, va_list args));
    MOCK_METHOD4(LogV,
        void(ELogType nType, int flags, const char* szFormat, va_list args));
    MOCK_METHOD0(Release,
        void());
    MOCK_METHOD2(SetFileName,
        bool(const char* fileNameOrFullPath, bool doBackups));
    MOCK_METHOD0(GetFileName,
        const char*());
    MOCK_METHOD0(GetBackupFileName,
        const char*());

    virtual void Log([[maybe_unused]] const char* szCommand, ...) override {}
    virtual void LogWarning([[maybe_unused]] const char* szCommand, ...) override {}
    virtual void LogError([[maybe_unused]] const char* szCommand, ...) override {}
    virtual void LogAlways([[maybe_unused]] const char* szCommand, ...) override {}
    virtual void LogPlus([[maybe_unused]] const char* command, ...) override {}
    virtual void LogToFile([[maybe_unused]] const char* command, ...) override {}
    virtual void LogToFilePlus([[maybe_unused]] const char* command, ...) override {}
    virtual void LogToConsole([[maybe_unused]] const char* command, ...) override {}
    virtual void LogToConsolePlus([[maybe_unused]] const char* command, ...) override {}
    virtual void UpdateLoadingScreen([[maybe_unused]] const char* command, ...) override {}

    MOCK_METHOD1(SetVerbosity,
        void(int verbosity));
    MOCK_METHOD0(GetVerbosityLevel,
        int());
    MOCK_METHOD1(AddCallback,
        void(ILogCallback * pCallback));
    MOCK_METHOD1(RemoveCallback,
        void(ILogCallback * pCallback));
    MOCK_METHOD0(Update,
        void());
    MOCK_METHOD0(GetModuleFilter,
        const char*());
    MOCK_METHOD1(Indent,
        void(class CLogIndenter * indenter));
    MOCK_METHOD1(Unindent,
        void(class CLogIndenter * indenter));
    MOCK_METHOD0(FlushAndClose,
        void());
};
