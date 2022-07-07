/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : This is the smallest possible interface to the Log -
//               it s independent and small  so that it can be easily moved
//               across the engine and test applications  to test engine
//               parts that need logging (e.g. Streaming Engine) separately


#ifndef CRYINCLUDE_CRYCOMMON_IMINILOG_H
#define CRYINCLUDE_CRYCOMMON_IMINILOG_H
#pragma once


struct IMiniLog
{
    enum ELogType
    {
        eMessage,
        eWarning,
        eError,
        eAlways,
        eWarningAlways,
        eErrorAlways,
        eInput,                     // e.g. "e_CaptureFolder ?" or all printouts from history or auto completion
        eInputResponse,             // e.g. "Set output folder for video capturing" in response to "e_CaptureFolder ?"
        eComment
    };

    // <interfuscator:shuffle>
    // Notes:
    //   You only have to implement this function.
    virtual void LogV(ELogType nType, const char* szFormat, va_list args) PRINTF_PARAMS(3, 0) = 0;

    virtual void LogV(ELogType nType, int flags, const char* szFormat, va_list args) PRINTF_PARAMS(4, 0) = 0;

    // Summary:
    //   Logs using type.
    inline void LogWithType(ELogType nType, const char* szFormat, ...) PRINTF_PARAMS(3, 4);

    inline void LogWithType(ELogType nType, int flags, const char* szFormat, ...) PRINTF_PARAMS(4, 5);

    // --------------------------------------------------------------------------

    // Summary:
    //   Destructor.
    virtual ~IMiniLog() {}

    // Description:
    //   This is the simplest log function for messages
    //   with the default implementation.
    virtual inline void Log(const char* szFormat, ...) PRINTF_PARAMS(2, 3);

    // Description:
    //   This is the simplest log function for warnings
    //   with the default implementation.
    virtual inline void LogWarning(const char* szFormat, ...) PRINTF_PARAMS(2, 3);

    // Description:
    //   This is the simplest log function for errors
    //   with the default implementation.
    virtual inline void LogError(const char* szFormat, ...) PRINTF_PARAMS(2, 3);
    // </interfuscator:shuffle>
};

inline void IMiniLog::LogWithType(ELogType nType, int flags, const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    LogV(nType, flags, szFormat, args);
    va_end(args);
}

inline void IMiniLog::LogWithType(ELogType nType, const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    LogV(nType, szFormat, args);
    va_end(args);
}

inline void IMiniLog::Log(const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    LogV (eMessage, szFormat, args);
    va_end(args);
}

inline void IMiniLog::LogWarning(const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    LogV (eWarning, szFormat, args);
    va_end(args);
}

inline void IMiniLog::LogError(const char* szFormat, ...)
{
    va_list args;
    va_start(args, szFormat);
    LogV (eError, szFormat, args);
    va_end(args);
}

// Notes:
//   By default, to make it possible not to implement the log at the beginning at all,
//   empty implementations of the two functions are given.
struct CNullMiniLog
    : public IMiniLog
{
    // Notes:
    //   The default implementation just won't do anything
    //##@{
    void LogV([[maybe_unused]] const char* szFormat, [[maybe_unused]] va_list args) {}
    void LogV([[maybe_unused]] ELogType nType, [[maybe_unused]] const char* szFormat, [[maybe_unused]] va_list args) override {}
    void LogV ([[maybe_unused]] ELogType nType, [[maybe_unused]] int flags, [[maybe_unused]] const char* szFormat, [[maybe_unused]] va_list args) override {}
    //##@}
};


#endif // CRYINCLUDE_CRYCOMMON_IMINILOG_H
