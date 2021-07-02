/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the Core headers
#include <AzCore/PlatformIncl.h>
#include "LogManager.h"

#include <iostream>

namespace MCore
{
    // static mutex
    Mutex LogManager::mGlobalMutex;

    //--------------------------------------------------------------------------------------------

    // constructor
    LogCallback::LogCallback()
    {
        mLogLevels = LOGLEVEL_DEFAULT;
    }


    // set the log levels this callback will accept and pass through
    void LogCallback::SetLogLevels(ELogLevel logLevels)
    {
        mLogLevels = logLevels;
        GetLogManager().InitLogLevels();
    }

    //-------------------------------------------------------------------------------------------

    void AzLogCallback::Log([[maybe_unused]] const char* text, ELogLevel logLevel)
    {
        switch (logLevel)
        {
            case LogCallback::LOGLEVEL_FATAL:
            case LogCallback::LOGLEVEL_ERROR:
                AZ_Error("EMotionFX", false, "%s\n", text);
                break;
            case LogCallback::LOGLEVEL_WARNING:
                AZ_Warning("EMotionFX", false, "%s\n", text);
                break;
            case LogCallback::LOGLEVEL_INFO:
            case LogCallback::LOGLEVEL_DETAILEDINFO:
            case LogCallback::LOGLEVEL_DEBUG:
            default:
                AZ_TracePrintf("EMotionFX", "%s\n", text);
                break;
        }
    }

    //-------------------------------------------------------------------------------------------

    // constructor
    LogManager::LogManager()
    {
        mLogCallbacks.SetMemoryCategory(MCORE_MEMCATEGORY_LOGMANAGER);

        // initialize the enabled log levels
        InitLogLevels();
    }


    // destructor
    LogManager::~LogManager()
    {
        // get rid of the callbacks
        ClearLogCallbacks();
    }


    // add a log file callback instance
    void LogManager::AddLogCallback(LogCallback* callback)
    {
        MCORE_ASSERT(callback);
        LockGuard lock(mMutex);

        // add the callback to the stack
        mLogCallbacks.Add(callback);

        // collect the enabled log levels
        InitLogLevels();
    }


    // remove a specific log callback from the stack
    void LogManager::RemoveLogCallback(uint32 index)
    {
        MCORE_ASSERT(mLogCallbacks.GetIsValidIndex(index));
        LockGuard lock(mMutex);

        // delete it from memory
        delete mLogCallbacks[index];

        // remove the callback from the stack
        mLogCallbacks.Remove(index);

        // collect the enabled log levels
        InitLogLevels();
    }

    // remove all given log callbacks by type
    void LogManager::RemoveAllByType(uint32 type)
    {
        LockGuard lock(mMutex);

        // iterate through all log callbacks
        for (uint32 i = 0; i < mLogCallbacks.GetLength(); )
        {
            LogCallback* callback = mLogCallbacks[i];

            // check if we are dealing with a log file
            if (callback->GetType() == type)
            {
                // get rid of the callback instance
                delete callback;

                // remove the callback from the stack
                mLogCallbacks.Remove(i);
            }
            else
            {
                i++;
            }
        }

        // collect the enabled log levels
        InitLogLevels();
    }


    // remove all log callbacks from the stack
    void LogManager::ClearLogCallbacks()
    {
        LockGuard lock(mMutex);

        // get rid of the callbacks
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            delete mLogCallbacks[i];
        }

        mLogCallbacks.Clear(true);

        // collect the enabled log levels
        InitLogLevels();
    }


    // retrieve a pointer to the given log callback
    LogCallback* LogManager::GetLogCallback(uint32 index)
    {
        return mLogCallbacks[index];
    }

    // return number of log callbacks in the stack
    uint32 LogManager::GetNumLogCallbacks() const
    {
        return mLogCallbacks.GetLength();
    }

    // collect all enabled log levels
    void LogManager::InitLogLevels()
    {
        // reset the loglevels
        int32 logLevels = LogCallback::LOGLEVEL_NONE;

        // enable all log levels that are enabled by any of the callbacks
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            logLevels |= (int32)mLogCallbacks[i]->GetLogLevels();
        }

        mLogLevels = (LogCallback::ELogLevel)logLevels;
    }


    // force set the given log levels to all callbacks
    void LogManager::SetLogLevels(LogCallback::ELogLevel logLevels)
    {
        // iterate through all log callbacks and set it to the given log levels
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            mLogCallbacks[i]->SetLogLevels(logLevels);
        }

        // force set the log manager's log levels to the given one as well
        mLogLevels = logLevels;
    }


    // the main logging method
    void LogManager::LogMessage(const char* message, LogCallback::ELogLevel logLevel)
    {
        LockGuard lock(mMutex);

        // iterate through all callbacks
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            if (mLogCallbacks[i]->GetLogLevels() & logLevel)
            {
                mLogCallbacks[i]->Log(message, logLevel);
            }
        }
    }


    // find the index of a given callback
    uint32 LogManager::FindLogCallback(LogCallback* callback) const
    {
        // iterate through all callbacks
        const uint32 num = mLogCallbacks.GetLength();
        for (uint32 i = 0; i < num; ++i)
        {
            if (mLogCallbacks[i] == callback)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    void LogFatalError(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_FATAL)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            azvsnprintf(textBuf, 4096, what, args);
            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_FATAL);
        }
    }


    void LogError(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_ERROR)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            azvsnprintf(textBuf, 4096, what, args);
            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_ERROR);
        }
    }


    void LogWarning(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_WARNING)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            azvsnprintf(textBuf, 4096, what, args);
            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_WARNING);
        }
    }


    void LogInfo(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_INFO)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            azvsnprintf(textBuf, 4096, what, args);
            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_INFO);
        }
    }


    void LogDetailedInfo(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_DETAILEDINFO)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            azvsnprintf(textBuf, 4096, what, args);
            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_DETAILEDINFO);
        }
    }


    void LogDebug(const char* what, ...)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_DEBUG)
        {
            // construct the final log line
            char textBuf[4096];
            va_list args;
            va_start(args, what);
            azvsnprintf(textBuf, 4096, what, args);
            va_end(args);

            // log the message
            GetLogManager().LogMessage(textBuf, LogCallback::LOGLEVEL_DEBUG);
        }
    }

    void LogDebugMsg(const char* msg)
    {
        LockGuard lock(LogManager::mGlobalMutex);

        // skip the va list construction in case that the message won't be logged by any of the callbacks
        if (GetLogManager().GetLogLevels() & LogCallback::LOGLEVEL_DEBUG)
        {
            // log the message
            GetLogManager().LogMessage(msg, LogCallback::LOGLEVEL_DEBUG);
        }
    }


    // print a debug line to the visual studio output, or console output, etc
    void Print([[maybe_unused]] const char* message)
    {
        AZ_TracePrintf("EMotionFX", "%s\n", message);
    }


    // format an std string
    AZStd::string FormatStdString(const char* fmt, ...)
    {
        int size = ((int)strlen(fmt)) * 2 + 128; // guess an initial size
        AZStd::string result;
        va_list ap;
        for (;; )
        {
            result.resize(size);
            va_start(ap, fmt);
            const int n = azvsnprintf((char*)result.data(), size, fmt, ap);
            va_end(ap);

            if (n > -1 && n < size)
            {
                result.resize(n);
                return result;
            }

            if (n > -1)     // needed size returned
            {
                size = n + 1; // +1 for null char
            }
            else
            {
                size *= 2;  // guess at a larger size (OS specific)
            }
        }
    }
}   // namespace MCore
