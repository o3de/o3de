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
            int logLevel = AZ::Debug::bg_traceLogLevel == AZ::Debug::LogLevel::Info ? 4 : AZ::Debug::bg_traceLogLevel;

            gEnv->pConsole->GetCVar("log_WriteToFileVerbosity")->Set(logLevel);
            hasSetCVar = true;
        }
#endif

        return ready;
    }

    bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override
    {
#if defined(USE_CRY_ASSERT) && AZ_LEGACY_CRYSYSTEM_TRAIT_DO_PREASSERT
        AZ::Crc32 crc;
        crc.Add(&line, sizeof(line));
        if (fileName)
        {
            crc.Add(fileName, strlen(fileName));
        }

        bool* ignore = nullptr;
        auto foundIter = m_ignoredAsserts->find(crc);
        if (foundIter == m_ignoredAsserts->end())
        {
            ignore = &((*m_ignoredAsserts)[crc]);
            *ignore = false;
        }
        else
        {
            ignore = &((*m_ignoredAsserts)[crc]);
        }

        if (!(*ignore))
        {
            using namespace AZ::Debug;

            Trace::Output(nullptr, "\n==================================================================\n");
            AZ::OSString outputMsg = AZ::OSString::format("Trace::Assert\n %s(%d): '%s'\n%s\n", fileName, line, func, message);
            Trace::Output(nullptr, outputMsg.c_str());

            // Suppress 3 in stack depth - this function, the bus broadcast that got us here, and Trace::Assert
            Trace::Output(nullptr, "------------------------------------------------\n");
            Trace::PrintCallstack(nullptr, 3);
            Trace::Output(nullptr, "\n==================================================================\n");

            AZ::EnvironmentVariable<bool> inEditorBatchMode = AZ::Environment::FindVariable<bool>("InEditorBatchMode");
            if (!inEditorBatchMode.IsConstructed() || !inEditorBatchMode.Get())
            {
                // Note - CryAssertTrace doesn't actually print any info to logging
                // it just stores the message internally for the message box in CryAssert to use
                CryAssertTrace("%s", message);
                if (CryAssert("Assertion failed", fileName, line, ignore) || Trace::IsDebuggerPresent())
                {
                    Trace::Break();
                }
            }
        }
        else
        {
            CryLogAlways("%s", message);
        }

        return m_suppressSystemOutput;
#else
        AZ_UNUSED(fileName);
        AZ_UNUSED(line);
        AZ_UNUSED(func);
        AZ_UNUSED(message);
        return false; // allow AZCore to do its default behavior.   This usually results in an application shutdown.
#endif
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
        gEnv->pLog->LogError("(%s) - %s", window, message);
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

        CryWarning(VALIDATOR_MODULE_UNKNOWN, VALIDATOR_WARNING, "(%s) - %s", window, message);
        return m_suppressSystemOutput;
    }

    bool OnOutput(const char* window, const char* message)  override
    {
        if (!IsCryLogReady())
        {
            return false; // allow AZCore to do its default behavior.
        }

        if (window == AZ::Debug::Trace::GetDefaultSystemWindow())
        {
            CryLogAlways("%s", message);
        }
        else
        {
            CryLog("(%s) - %s", window, message);
        }
        
        return m_suppressSystemOutput;
    }

private:

    using IgnoredAssertMap = AZStd::unordered_map<AZ::Crc32, bool, AZStd::hash<AZ::Crc32>, AZStd::equal_to<AZ::Crc32>, AZ::OSStdAllocator>;
    IgnoredAssertMap* m_ignoredAsserts;
    bool m_suppressSystemOutput = true;
};
