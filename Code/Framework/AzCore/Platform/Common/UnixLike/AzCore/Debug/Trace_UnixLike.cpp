/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string_view.h>

#include <signal.h>

namespace AZ::Debug
{
#if defined(AZ_ENABLE_DEBUG_TOOLS)
    void ExceptionHandler(int signal);

    constexpr int MaxMessageLength = 4096;
    constexpr int MaxStackLines = 100;
#endif

    namespace Platform
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        bool performDebuggerDetection()
        {
            AZ::IO::SystemFile processStatusFile;
            if (!processStatusFile.Open("/proc/self/status", AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
            {
                return false;
            }

            char buffer[MaxMessageLength];
            AZ::IO::SystemFile::SizeType numRead = processStatusFile.Read(sizeof(buffer), buffer);

            const AZStd::string_view processStatusView(buffer, buffer + numRead);
            constexpr AZStd::string_view tracerPidString = "TracerPid:";
            const size_t tracerPidOffset = processStatusView.find(tracerPidString);
            if (tracerPidOffset == AZStd::string_view::npos)
            {
                return false;
            }
            for (size_t i = tracerPidOffset + tracerPidString.length(); i < numRead; ++i)
            {
                if (processStatusView[i] != ' ')
                {
                    return processStatusView[i] != '0';
                }
            }
            return false;
        }

        bool IsDebuggerPresent()
        {
            static bool s_detectionPerformed = false;
            static bool s_debuggerDetected = false;
            if (!s_detectionPerformed)
            {
                s_debuggerDetected = performDebuggerDetection();
                s_detectionPerformed = true;
            }
            return s_debuggerDetected;
        }

        bool AttachDebugger()
        {
            // Not supported yet
            AZ_Assert(false, "AttachDebugger() is not supported for Unix platform yet");
            return false;
        }

        void HandleExceptions(bool isEnabled)
        {
            if (isEnabled)
            {
                signal(SIGSEGV, ExceptionHandler);
                signal(SIGTRAP, ExceptionHandler);
                signal(SIGILL, ExceptionHandler);
            }
            else
            {
                signal(SIGSEGV, SIG_DFL);
                signal(SIGTRAP, SIG_DFL);
                signal(SIGILL, SIG_DFL);
            }
        }

        void DebugBreak()
        {
            raise(SIGINT);
        }
#endif // AZ_ENABLE_DEBUG_TOOLS

        void Terminate(int exitCode)
        {
            _exit(exitCode);
        }
    } // namespace Platform

#if defined(AZ_ENABLE_DEBUG_TOOLS)
    void ExceptionHandler(int signal)
    {
        char message[MaxMessageLength];
        // Trace::RawOutput
        Debug::Trace::Instance().RawOutput(nullptr, "==================================================================\n");
        azsnprintf(message, MaxMessageLength, "Error: signal %s: \n", strsignal(signal));
        Debug::Trace::Instance().RawOutput(nullptr, message);

        StackFrame frames[MaxStackLines];
        SymbolStorage::StackLine stackLines[MaxStackLines];
        SymbolStorage decoder;
        const unsigned int numberOfFrames = StackRecorder::Record(frames, MaxStackLines);
        decoder.DecodeFrames(frames, numberOfFrames, stackLines);
        for (int i = 0; i < numberOfFrames; ++i)
        {
            azsnprintf(message, MaxMessageLength, "%s \n", stackLines[i]);
            Debug::Trace::Instance().RawOutput(nullptr, message);
        }
        Debug::Trace::Instance().RawOutput(nullptr, "==================================================================\n");
    }
#endif

} // namespace AZ::Debug
