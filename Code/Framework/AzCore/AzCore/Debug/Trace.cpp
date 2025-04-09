/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>

#include <AzCore/base.h>

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Settings/SettingsRegistry.h>

#include <cstdarg>

#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/Debug/TraceMessageBus.h>

// Used to keep a set of ignored asserts for CRC checking
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/chrono/chrono.h>

namespace AZ::Debug
{
    struct StackFrame;

    namespace Platform
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        bool AttachDebugger();
        bool IsDebuggerPresent();
        void HandleExceptions(bool isEnabled);
        void DebugBreak();
#endif
        void Terminate(int exitCode);
    }

    namespace DebugInternal
    {
        // other threads can trigger fatals and errors, but the same thread should not, to avoid stack overflow.
        // because its thread local, it does not need to be atomic.
        // its also important that this is inside the cpp file, so that its only in the trace.cpp module and not the header shared by everyone.
        static AZ_THREAD_LOCAL bool g_alreadyHandlingAssertOrFatal = false;
        AZ_THREAD_LOCAL bool g_suppressEBusCalls = false; // used when it would be dangerous to use ebus broadcasts.
    }

    //////////////////////////////////////////////////////////////////////////
    // Globals
    const int       g_maxMessageLength = 4096;
    static const char*    g_dbgSystemWnd = "System";
    void* g_exceptionInfo = nullptr;

    // Environment var needed to track ignored asserts across systems and disable native UI under certain conditions
    static const char* ignoredAssertUID = "IgnoredAssertSet";
    static const char* assertVerbosityUID = "assertVerbosityLevel";
    static const char* logVerbosityUID = "sys_LogLevel";
    static const int assertLevel_log = 1;
    static const int assertLevel_nativeUI = 2;
    static const int assertLevel_crash = 3;
    static const int logLevel_full = 2;
    static AZ::EnvironmentVariable<AZStd::unordered_set<size_t>> g_ignoredAsserts;
    static AZ::EnvironmentVariable<int> g_assertVerbosityLevel;
    static AZ::EnvironmentVariable<int> g_logVerbosityLevel;

    static AZ::EnvironmentVariable<bool> s_AssertsAutoBreak;
    AZ_CVAR(
        bool,
        bg_assertsAutoBreak,
        true,
        nullptr,
        ConsoleFunctorFlags::Null,
        "Automatically break on assert when the debugger is attached. 0=disabled, 1=enabled.");

    static void TraceLevelChanged(const int& newLevel)
    {
        Debug::ITrace::Instance().SetLogLevel(static_cast<LogLevel>(newLevel));
    }

    static void AlwaysShowCallstackChanged(const bool& enable)
    {
        Debug::ITrace::Instance().SetAlwaysPrintCallstack(enable);
    }

    AZ_CVAR_SCOPED(int, bg_traceLogLevel, static_cast<int>(LogLevel::Info), &TraceLevelChanged, ConsoleFunctorFlags::Null, "Enable trace message logging in release mode.  0=disabled, 1=errors, 2=warnings, 3=info, 4=debug, 5=trace.");
    AZ_CVAR_SCOPED(bool, bg_alwaysShowCallstack, false, &AlwaysShowCallstackChanged, ConsoleFunctorFlags::Null, "Force stack trace output without allowing ebus interception.");

    // Allow redirection of trace raw output writes to stdout, stderr or to /dev/null
    static constexpr const char* fileStreamIdentifier = "raw_c_stream";
    static AZ::EnvironmentVariable<FILE*> s_fileStream;
    void SetCFileStream(const RedirectCStream& redirectOption)
    {
        s_fileStream = AZ::Environment::FindVariable<FILE*>(fileStreamIdentifier);
        if (!s_fileStream)
        {
            return;
        }

        switch (redirectOption)
        {
        case RedirectCStream::Stdout:
            *s_fileStream = stdout;
            break;
        case RedirectCStream::Stderr:
            *s_fileStream = stderr;
            break;
        case RedirectCStream::None:
            *s_fileStream = nullptr;
            break;
        }
    }

    AZ_CVAR_SCOPED(RedirectCStream, bg_redirectrawoutput, RedirectCStream::Stdout, SetCFileStream, ConsoleFunctorFlags::Null,
        "Set to the value of the C stream FILE* object to write raw trace output."
        " Defaults to the stdout FILE stream."
        " Valid values are 0 = stdout, 1 = stderr, 2 = redirect to NUL");


    /**
     * If any listener returns true, store the result so we don't outputs detailed information.
     */
    struct TraceMessageResult
    {
        bool     m_value;
        TraceMessageResult()
            : m_value(false) {}
        void operator=(bool rhs)     { m_value = m_value || rhs; }
    };

    // definition of init to initialize assert tracking global
    void Trace::Init()
    {
        // if out ignored assert list exists, then another module has init the system
        g_ignoredAsserts = AZ::Environment::FindVariable<AZStd::unordered_set<size_t>>(ignoredAssertUID);
        if (!g_ignoredAsserts)
        {
            g_ignoredAsserts = AZ::Environment::CreateVariable<AZStd::unordered_set<size_t>>(ignoredAssertUID);
            g_assertVerbosityLevel = AZ::Environment::CreateVariable<int>(assertVerbosityUID);
            g_logVerbosityLevel = AZ::Environment::CreateVariable<int>(logVerbosityUID);
            s_fileStream = AZ::Environment::CreateVariable<FILE*>(fileStreamIdentifier, stdout);

            //default assert level is to log/print asserts this can be overriden with the sys_asserts CVAR
            g_assertVerbosityLevel.Set(assertLevel_log);
            g_logVerbosityLevel.Set(logLevel_full);
        }

        // Setup the raw C FILE* pointer to allow raw output to write to one of the std streams
        s_fileStream = AZ::Environment::FindVariable<FILE*>(fileStreamIdentifier);
        if (!s_fileStream)
        {
            FILE* redirectStream = stdout;
            if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
            {
                if (RedirectCStream redirectOption;
                    console->GetCvarValue("bg_redirectrawoutput", redirectOption) == AZ::GetValueResult::Success)
                {
                    switch (redirectOption)
                    {
                    case RedirectCStream::Stdout:
                        redirectStream = stdout;
                        break;
                    case RedirectCStream::Stderr:
                        redirectStream = stderr;
                        break;
                    case RedirectCStream::None:
                        redirectStream = nullptr;
                        break;
                    }
                }
            }

            AZ::Environment::CreateVariable<FILE*>(fileStreamIdentifier, redirectStream);
        }
    }

    // clean up the ignored assert container
    void Trace::Destroy()
    {
        g_ignoredAsserts = AZ::Environment::FindVariable<AZStd::unordered_set<size_t>>(ignoredAssertUID);
        if (g_ignoredAsserts)
        {
            if (g_ignoredAsserts.IsOwner())
            {
                g_ignoredAsserts.Reset();
            }
        }
    }

    //=========================================================================
    const char* Trace::GetDefaultSystemWindow()
    {
        return g_dbgSystemWnd;
    }

    //=========================================================================
    // IsDebuggerPresent
    // [8/3/2009]
    //=========================================================================
    bool
    Trace::IsDebuggerPresent()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        return Platform::IsDebuggerPresent();
#else
        return false;
#endif
    }

    bool
    Trace::AttachDebugger()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        return Platform::AttachDebugger();
#else
        return false;
#endif
    }

    bool
    Trace::WaitForDebugger([[maybe_unused]] float timeoutSeconds/*=-1.f*/)
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        using AZStd::chrono::steady_clock;
        using AZStd::chrono::time_point;
        using AZStd::chrono::milliseconds;

        milliseconds timeoutMs = milliseconds(aznumeric_cast<long long>(timeoutSeconds * 1000));
        steady_clock clock;
        time_point start = clock.now();
        auto hasTimedOut = [&clock, start, timeoutMs]()
        {
            return timeoutMs.count() >= 0 && (clock.now() - start) >= timeoutMs;
        };

        while (!Instance().IsDebuggerPresent() && !hasTimedOut())
        {
            AZStd::this_thread::sleep_for(milliseconds(1));
        }
        return Instance().IsDebuggerPresent();
#else
        return false;
#endif
    }

    //=========================================================================
    // HandleExceptions
    // [8/3/2009]
    //=========================================================================
    void
    Trace::HandleExceptions(bool isEnabled)
    {
        AZ_UNUSED(isEnabled);
        if (Instance().IsDebuggerPresent())
        {
            return;
        }

#if defined(AZ_ENABLE_DEBUG_TOOLS)
        Platform::HandleExceptions(isEnabled);
#endif
    }

    //=========================================================================
    // Break
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Break()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)

        if (!IsDebuggerPresent())
        {
            return; // Do not break when tests are running unless debugger is present
        }

        Platform::DebugBreak();

#endif // AZ_ENABLE_DEBUG_TOOLS
    }

    void Debug::Trace::Crash()
    {
        int* p = nullptr;
        *p = 1;
    }

    void Debug::Trace::Terminate(int exitCode)
    {
        AZ_TracePrintf("Exit", "Called Terminate() with exit code: 0x%x", exitCode);
        Instance().PrintCallstack("Exit");
        Platform::Terminate(exitCode);
    }

    //=========================================================================
    // Assert
    // [8/3/2009]
    //=========================================================================
    void Trace::Assert(const char* fileName, int line, const char* funcName, const char* format, ...)
    {
        using namespace DebugInternal;

        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        // Check our set to see if this assert has been previously ignored and early out if so
        size_t assertHash = line;
        AZStd::hash_combine(assertHash, AZ::Crc32(fileName));
        // Find our list of ignored asserts since we probably aren't the same system that initialized it
        g_ignoredAsserts = AZ::Environment::FindVariable<AZStd::unordered_set<size_t>>(ignoredAssertUID);

        if (g_ignoredAsserts)
        {
            auto ignoredAssertIt2 = g_ignoredAsserts->find(assertHash);
            if (ignoredAssertIt2 != g_ignoredAsserts->end())
            {
                return;
            }
        }

        if (g_alreadyHandlingAssertOrFatal)
        {
            return;
        }

        g_alreadyHandlingAssertOrFatal = true;

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength - 1, format, mark); // -1 to make room for the "/n" that will be appended below
        va_end(mark);

        TraceMessageResult result;
        TraceMessageBus::BroadcastResult(result, &TraceMessageBus::Events::OnPreAssert, fileName, line, funcName, message);

        if (GetAlwaysPrintCallstack())
        {
            // If we're always showing the callstack, print it now before there's any chance of an ebus handler interrupting
            PrintCallstack(g_dbgSystemWnd, 1);
        }

        if (result.m_value)
        {
            g_alreadyHandlingAssertOrFatal = false;
            return;
        }

        int currentLevel = GetAssertVerbosityLevel();
        if (currentLevel >= assertLevel_log)
        {
            Output(g_dbgSystemWnd, "\n==================================================================\n");
            azsnprintf(header, g_maxMessageLength, "Trace::Assert\n %s(%d): (%tu) '%s'\n", fileName, line, (uintptr_t)(AZStd::this_thread::get_id().m_id), funcName);
            Output(g_dbgSystemWnd, header);
            azstrcat(message, g_maxMessageLength, "\n");
            Output(g_dbgSystemWnd, message);

            TraceMessageBus::BroadcastResult(result, &TraceMessageBus::Events::OnAssert, message);
            if (result.m_value)
            {
                Output(g_dbgSystemWnd, "==================================================================\n");
                g_alreadyHandlingAssertOrFatal = false;
                return;
            }

            Output(g_dbgSystemWnd, "------------------------------------------------\n");
            if (!GetAlwaysPrintCallstack())
            {
                PrintCallstack(g_dbgSystemWnd, 1);
            }
            Output(g_dbgSystemWnd, "==================================================================\n");

            char dialogBoxText[g_maxMessageLength];
            azsnprintf(dialogBoxText, g_maxMessageLength, "Assert \n\n %s(%d) \n %s \n\n %s", fileName, line, funcName, message);

            // If we are logging only then ignore the assert after it logs once in order to prevent spam
            if (currentLevel == 1 && !IsDebuggerPresent())
            {
                if (g_ignoredAsserts)
                {
                    Output(g_dbgSystemWnd, "====Assert added to ignore list by spec and verbosity setting.====\n");
                    g_ignoredAsserts->insert(assertHash);
                }
            }

            bool assertsAutoBreak = false;
            if (auto* console = Interface<IConsole>::Get())
            {
                console->GetCvarValue("bg_assertsAutoBreak", assertsAutoBreak);
            }
            if (assertsAutoBreak && IsDebuggerPresent())
            {
                // You've encountered an assert! By default, the presence of a debugger will cause asserts
                // to DebugBreak (walk up a few stack frames to understand what happened).
                Instance().Break();
            }
#if AZ_ENABLE_TRACE_ASSERTS
            //display native UI dialogs at verbosity level 2
            else if (currentLevel == assertLevel_nativeUI)
            {
                AZ::NativeUI::AssertAction buttonResult;
                AZ::NativeUI::NativeUIRequestBus::BroadcastResult(
                    buttonResult, &AZ::NativeUI::NativeUIRequestBus::Events::DisplayAssertDialog, dialogBoxText);
                switch (buttonResult)
                {
                case AZ::NativeUI::AssertAction::BREAK:
                    Instance().Break();
                    break;
                case AZ::NativeUI::AssertAction::IGNORE_ALL_ASSERTS:
                    SetAssertVerbosityLevel(1);
                    g_alreadyHandlingAssertOrFatal = true;
                    return;
                case AZ::NativeUI::AssertAction::IGNORE_ASSERT:
                    //if ignoring this assert then add it to our ignore list so it doesn't interrupt again this run
                    if (g_ignoredAsserts)
                    {
                        g_ignoredAsserts->insert(assertHash);
                    }
                    break;
                default:
                    break;
                }
            }
            else
#endif //AZ_ENABLE_TRACE_ASSERTS
            // Crash the application directly at assert level 3
            if (currentLevel == assertLevel_crash)
            {
                AZ_Crash();
            }
        }
        g_alreadyHandlingAssertOrFatal = false;
    }

    //=========================================================================
    // Error
    // [8/3/2009]
    //=========================================================================
    void
    Trace::Error(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...)
    {
        if (!IsTraceLoggingEnabledForLevel(LogLevel::Errors))
        {
            return;
        }

        using namespace DebugInternal;
        if (window == nullptr)
        {
            window = NoWindow;
        }

        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        if (g_alreadyHandlingAssertOrFatal)
        {
            return;
        }
        g_alreadyHandlingAssertOrFatal = true;

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength - 1, format, mark); // -1 to make room for the "/n" that will be appended below
        va_end(mark);

        TraceMessageResult result;
        TraceMessageBus::BroadcastResult(result, &TraceMessageBus::Events::OnPreError, window, fileName, line, funcName, message);
        if (result.m_value)
        {
            g_alreadyHandlingAssertOrFatal = false;
            return;
        }

        Output(window, "\n==================================================================\n");
        azsnprintf(header, g_maxMessageLength, "Trace::Error\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(window, header);
        azstrcat(message, g_maxMessageLength, "\n");
        Output(window, message);

        TraceMessageBus::BroadcastResult(result, &TraceMessageBus::Events::OnError, window, message);
        Output(window, "==================================================================\n");
        if (result.m_value)
        {
            g_alreadyHandlingAssertOrFatal = false;
            return;
        }

        g_alreadyHandlingAssertOrFatal = false;
    }
    //=========================================================================
    // Warning
    // [8/3/2009]
    //=========================================================================
    void Trace::Warning(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...)
    {
        if (!IsTraceLoggingEnabledForLevel(LogLevel::Warnings))
        {
            return;
        }

        char message[g_maxMessageLength];
        char header[g_maxMessageLength];

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength - 1, format, mark); // -1 to make room for the "/n" that will be appended below
        va_end(mark);

        TraceMessageResult result;
        TraceMessageBus::BroadcastResult(result, &TraceMessageBus::Events::OnPreWarning, window, fileName, line, funcName, message);
        if (result.m_value)
        {
            return;
        }

        Output(window, "\n==================================================================\n");
        azsnprintf(header, g_maxMessageLength, "Trace::Warning\n %s(%d): '%s'\n", fileName, line, funcName);
        Output(window, header);
        azstrcat(message, g_maxMessageLength, "\n");
        Output(window, message);

        TraceMessageBus::BroadcastResult(result, &TraceMessageBus::Events::OnWarning, window, message);
        Output(window, "==================================================================\n");
    }

    //=========================================================================
    // Printf
    // [8/3/2009]
    //=========================================================================
    void Trace::Printf(const char* window, const char* format, ...)
    {
        if (window == nullptr)
        {
            window = NoWindow;
        }

        char message[g_maxMessageLength];

        va_list mark;
        va_start(mark, format);
        azvsnprintf(message, g_maxMessageLength, format, mark);
        va_end(mark);

        TraceMessageResult result;
        TraceMessageBus::BroadcastResult(result, &TraceMessageBus::Events::OnPrintf, window, message);
        if (result.m_value)
        {
            return;
        }

        Output(window, message);
    }

    //=========================================================================
    // Output
    // [8/3/2009]
    //=========================================================================
    void Trace::Output(const char* window, const char* message)
    {
        if (window == nullptr)
        {
            window = NoWindow;
        }

        if (!DebugInternal::g_suppressEBusCalls)
        {
            // only call into Ebusses if we are not in a recursive-exception situation as that
            // would likely just lead to even more exceptions.

            TraceMessageResult result;
            TraceMessageBus::BroadcastResult(result, &TraceMessageBus::Events::OnOutput, window, message);
            if (result.m_value)
            {
                return;
            }
        }

        OutputToRawAndDebugger(window, message);
    }

    void Trace::OutputToRawAndDebugger(const char* window, const char* message)
    {
        if (window == nullptr)
        {
            window = NoWindow;
        }

        Platform::OutputToDebugger(window, message);
        RawOutput(window, message);
    }

    void Trace::RawOutput(const char* window, const char* message)
    {
        if (window == nullptr)
        {
            window = NoWindow;
        }

        // printf on Windows platforms seem to have a buffer length limit of 4096 characters
        // Therefore fwrite is used directly to write the window and message to stdout or stderr

        // Wrapping the NoWindow constant in a string_view to allow use of string_view::operator== for string compare
        AZStd::string_view windowView{ window };
        AZStd::string_view messageView{ message };
        constexpr AZStd::string_view windowMessageSeparator{ ": " };

        // If the raw output stream environment variable is set to a non-nullptr FILE* stream
        // write to that stream, otherwise write stdout
        FILE* stdoutStream = stdout;
        if (FILE* rawOutputStream = s_fileStream ? *s_fileStream : stdoutStream; rawOutputStream != nullptr)
        {
            if (!windowView.empty())
            {
                fwrite(windowView.data(), 1, windowView.size(), rawOutputStream);
                fwrite(windowMessageSeparator.data(), 1, windowMessageSeparator.size(), rawOutputStream);
            }
            fwrite(messageView.data(), 1, messageView.size(), rawOutputStream);
        }
    }

    //=========================================================================
    // PrintCallstack
    // [8/3/2009]
    //=========================================================================
    void
    Trace::PrintCallstack(const char* window, unsigned int suppressCount, void* nativeContext)
    {
        StackFrame frames[25];

        SymbolStorage::StackLine lines[AZ_ARRAY_SIZE(frames)];
        unsigned int numFrames = 0;

        if (!nativeContext)
        {
            suppressCount += 1; /// If we don't provide a context we will capture in the RecordFunction, so skip us (Trace::PrintCallstack).
            numFrames = StackRecorder::Record(frames, AZ_ARRAY_SIZE(frames), suppressCount);
        }
        else
        {
            numFrames = StackConverter::FromNative(frames, AZ_ARRAY_SIZE(frames), nativeContext);
        }

        if (numFrames)
        {
            SymbolStorage::DecodeFrames(frames, numFrames, lines);
            for (unsigned int i = 0; i < numFrames; ++i)
            {
                if (lines[i][0] == 0)
                {
                    continue;
                }

                const size_t endOfStr = AZStd::min(strlen(lines[i]), AZ_ARRAY_SIZE(lines[i]) - 2);
                lines[i][endOfStr] = '\n';
                lines[i][endOfStr + 1] = '\0';

                // Use Output instead of AZ_Printf to be consistent with the exception output code and avoid
                // this accidentally being suppressed as a normal message

                if (GetAlwaysPrintCallstack())
                {
                    // Use Raw Output as this cannot be suppressed
                    RawOutput(window, lines[i]);
                }
                else
                {
                    Output(window, lines[i]);
                }
            }
        }
    }

    //=========================================================================
    // GetExceptionInfo
    // [4/2/2012]
    //=========================================================================
    void*
    Trace::GetNativeExceptionInfo()
    {
        return g_exceptionInfo;
    }

    // Gets/sets the current verbosity level from the global
    int Trace::GetAssertVerbosityLevel()
    {
        auto val = AZ::Environment::FindVariable<int>(assertVerbosityUID);
        if (val)
        {
            return val.Get();
        }
        else
        {
            return assertLevel_log;
        }
    }

    void Trace::SetAssertVerbosityLevel(int level)
    {
        auto val = AZ::Environment::FindVariable<int>(assertVerbosityUID);
        if (val)
        {
            val.Set(level);
        }
    }
} // namspace AZ::Debug
