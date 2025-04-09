/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Console/IConsole.h>

#include <CryAssert.h>

namespace AZ::Debug
{
    AZ_CVAR_EXTERNED(int, bg_traceLogLevel);
}

/**
 * Hook Trace bus so we can funnel AZ asserts, warnings, etc to CryEngine.
 *
 * Note: This is currently owned by CrySystem, because CrySystem owns
 * the logging mechanism for which it is relevant.
 */
class AZCoreLogSink
    : public AZ::Debug::TraceMessageBus::Handler
{
public:
    ~AZCoreLogSink()
    {
        Disconnect();
    }

    inline static void Connect(bool suppressSystemOutput)
    {
        GetInstance().m_ignoredAsserts = new IgnoredAssertMap();
        GetInstance().m_suppressSystemOutput = suppressSystemOutput;
        GetInstance().BusConnect();
    }

    inline static void Disconnect()
    {
        GetInstance().BusDisconnect();
        delete GetInstance().m_ignoredAsserts;
        GetInstance().m_ignoredAsserts = nullptr;
    }

    static AZCoreLogSink& GetInstance()
    {
        static AZCoreLogSink s_sink;
        return s_sink;
    }

    static bool IsCryLogReady()
    {
        bool ready = gEnv && gEnv->pSystem && gEnv->pLog;

#ifdef _RELEASE
        static bool hasSetCVar = false;
        if(!hasSetCVar && ready)
        {
            // AZ logging only has a concept of 3 levels (error, warning, info) but cry logging has 4 levels (..., messaging).  If info level is set, we'll turn on messaging as well
            int logLevel = AZ::Debug::bg_traceLogLevel == static_cast<int>(AZ::Debug::LogLevel::Info) ? 4 : AZ::Debug::bg_traceLogLevel;

            gEnv->pConsole->GetCVar("log_WriteToFileVerbosity")->Set(logLevel);
            hasSetCVar = true;
        }
#endif

        return ready;
    }

    bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override
    {
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);
        AZ_UNUSED(message);
        return false; // allow AZCore to do its default behavior.   This usually results in an application shutdown.
    }

    bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override
    {
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);
        if (!IsCryLogReady())
        {
            return false; // allow AZCore to do its default behavior.
        }

        AZStd::string_view windowView = window;
        if (!windowView.empty())
        {
            gEnv->pLog->LogError("(%s) - %s", window, message);
        }
        else
        {
            gEnv->pLog->LogError("%s", message);
        }
        return m_suppressSystemOutput;
    }

    bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override
    {
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);

        if (!IsCryLogReady())
        {
            return false; // allow AZCore to do its default behavior.
        }

        AZStd::string_view windowView = window;
        if (!windowView.empty())
        {
            CryWarning(VALIDATOR_MODULE_UNKNOWN, VALIDATOR_WARNING, "(%s) - %s", window, message);
        }
        else
        {
            CryWarning(VALIDATOR_MODULE_UNKNOWN, VALIDATOR_WARNING, "%s", message);
        }
        return m_suppressSystemOutput;
    }

    bool OnOutput(const char* window, const char* message)  override
    {
        if (!IsCryLogReady())
        {
            return false; // allow AZCore to do its default behavior.
        }

        AZStd::string_view windowView = window;

        // Only print out the window if it is not equal to the NoWindow or the DefaultSystemWindow value
        if (windowView == AZ::Debug::Trace::GetNoWindow() || windowView == AZ::Debug::Trace::GetDefaultSystemWindow())
        {
            [[maybe_unused]] auto WriteToStream = [message = AZStd::string_view(message)]
            (AZ::IO::GenericStream& stream)
            {
                constexpr AZStd::string_view newline = "\n";
                stream.Write(message.size(), message.data());
                // performs does not format the output and no newline will automatically be added
                // Therefore an explicit invocation for a new line is supplied
                stream.Write(newline.size(), newline.data());
            };
            CryOutputToCallback(ILog::eAlways, WriteToStream);
        }
        else
        {
            [[maybe_unused]] auto WriteToStream = [window = AZStd::string_view(window), message = AZStd::string_view(message)]
            (AZ::IO::GenericStream& stream)
            {
                constexpr AZStd::string_view windowMessageSeparator = " - ";
                constexpr AZStd::string_view newline = "\n";
                constexpr AZStd::string_view leftParenthesis = "(";
                constexpr AZStd::string_view rightParenthesis = ")";
                stream.Write(leftParenthesis.size(), leftParenthesis.data());
                stream.Write(window.size(), window.data());
                stream.Write(rightParenthesis.size(), rightParenthesis.data());
                stream.Write(windowMessageSeparator.size(), windowMessageSeparator.data());
                stream.Write(message.size(), message.data());
                stream.Write(newline.size(), newline.data());
            };
            CryOutputToCallback(ILog::eMessage, WriteToStream);
        }

        return m_suppressSystemOutput;
    }

private:

    using IgnoredAssertMap = AZStd::unordered_map<AZ::Crc32, bool, AZStd::hash<AZ::Crc32>, AZStd::equal_to<AZ::Crc32>, AZ::OSStdAllocator>;
    IgnoredAssertMap* m_ignoredAsserts;
    bool m_suppressSystemOutput = true;
};
