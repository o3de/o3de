/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TraceMessageHook.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContextLogFormatter.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/PlatformIncl.h>

namespace AssetBuilder
{
    TraceMessageHook::TraceMessageHook()
        : m_stacks(nullptr)
        , m_inDebugMode(false)
        , m_skipErrorsCount(0)
        , m_skipWarningsCount(0)
        , m_skipPrintfsCount(0)
        , m_totalWarningCount(0)
        , m_totalErrorCount(0)
    {
        AssetBuilderSDK::AssetBuilderTraceBus::Handler::BusConnect();
        AZ::Debug::TraceMessageBus::Handler::BusConnect();
    }

    TraceMessageHook::~TraceMessageHook()
    {
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        AssetBuilderSDK::AssetBuilderTraceBus::Handler::BusDisconnect();
        delete m_stacks;
        m_stacks = nullptr;
    }

    void TraceMessageHook::EnableTraceContext(bool enable)
    {
        if (enable)
        {
            if (!m_stacks)
            {
                m_stacks = new AzToolsFramework::Debug::TraceContextMultiStackHandler();
            }
        }
        else
        {
            delete m_stacks;
            m_stacks = nullptr;
        }
    }

    void TraceMessageHook::EnableDebugMode(bool enable)
    {
        m_inDebugMode = enable;
    }

    bool TraceMessageHook::OnAssert(const char* message)
    {
        if (m_skipErrorsCount == 0)
        {
            CleanMessage(stdout, "E", message, true);
            AZ::Debug::Trace::Instance().PrintCallstack("", 3); // Skip all the Trace.cpp function calls
            std::fflush(stdout);
            ++m_totalErrorCount;
        }
        else
        {
            --m_skipErrorsCount;
        }

        return !m_inDebugMode;
    }

    bool TraceMessageHook::OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message)
    {
        if(m_skipErrorsCount == 0)
        {
            // Add the trace information and message type to context details to simplify the event log
            AZ_TraceContext("Trace", AZStd::string::format("%s(%d): '%s'", fileName, line, func));
            AZ_TraceContext("Type", "Trace::Error");

            CleanMessage(stdout, "E", AZStd::string::format("%s: %s", window, message).c_str(), true);

            ++m_totalErrorCount;
        }
        else
        {
            --m_skipErrorsCount;
        }

        return !m_inDebugMode;
    }

    bool TraceMessageHook::OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message)
    {
        if (m_skipWarningsCount == 0)
        {
            // Add the trace information and message type to context details to simplify the event log
            AZ_TraceContext("Trace", AZStd::string::format("%s(%d): '%s'", fileName, line, func));
            AZ_TraceContext("Type", "Trace::Warning");

            CleanMessage(stdout, "W", AZStd::string::format("%s: %s", window, message).c_str(), true);

            ++m_totalWarningCount;
        }
        else
        {
            --m_skipWarningsCount;
        }

        return !m_inDebugMode;
    }

    bool TraceMessageHook::OnException(const char* message)
    {
        m_isInException = true;
        CleanMessage(stdout, "E", message, true);
        ++m_totalErrorCount;
        AZ::Debug::Trace::HandleExceptions(false);
        AZ::Debug::Trace::Instance().PrintCallstack("", 3); // Skip all the Trace.cpp function calls
        // note that the above call ultimately results in a whole bunch of TracePrint/Outputs, which will end up in OnOutput below.

        std::fflush(stdout);

        // if we don't terminate here, the user may get a dialog box from the OS saying that the program crashed.
        // we don't want this, because in this case, the program is one of potentially many, many background worker processes
        // that are continuously starting/stopping and they'd get flooded by those message boxes.
        AZ::Debug::Trace::Terminate(1);

        return false;
    }

    bool TraceMessageHook::OnOutput(const char* /*window*/, const char* message)
    {
        if (m_isInException) // all messages that occur during an exception should be considered an error.
        {
            CleanMessage(stdout, "E", message, true);
            return true;
        }

        return false;
    }

    bool TraceMessageHook::OnPrintf(const char* window, const char* message)
    {
        if (m_skipPrintfsCount == 0)
        {
            CleanMessage(stdout, window, message, false);
        }
        else
        {
            --m_skipPrintfsCount;
        }

        return true;
    }

    void TraceMessageHook::IgnoreNextErrors(AZ::u32 count)
    {
        m_skipErrorsCount += count;
    }

    void TraceMessageHook::IgnoreNextWarning(AZ::u32 count)
    {
        m_skipWarningsCount += count;
    }

    void TraceMessageHook::IgnoreNextPrintf(AZ::u32 count)
    {
        m_skipPrintfsCount += count;
    }

    void TraceMessageHook::ResetWarningCount()
    {
        m_totalWarningCount = 0;
    }

    void TraceMessageHook::ResetErrorCount()
    {
        m_totalErrorCount = 0;
    }

    AZ::u32 TraceMessageHook::GetWarningCount()
    {
        return m_totalWarningCount;
    }

    AZ::u32 TraceMessageHook::GetErrorCount()
    {
        return m_totalErrorCount;
    }

    void TraceMessageHook::DumpTraceContext(FILE* stream) const
    {
        if (m_stacks)
        {
            AZStd::shared_ptr<const AzToolsFramework::Debug::TraceContextStack> stack = m_stacks->GetCurrentStack();
            if (stack)
            {
                AZStd::string line;
                size_t stackSize = stack->GetStackCount();
                for (size_t i = 0; i < stackSize; ++i)
                {
                    line.clear();
                    AzToolsFramework::Debug::TraceContextLogFormatter::PrintLine(line, *stack, i);
                    CleanMessage(stream, "C", line.c_str(), false, nullptr, false);
                }
            }
        }
    }

    void TraceMessageHook::CleanMessage(FILE* stream, const char* prefix, const char* message, bool forceFlush, const char* extraPrefix, bool includeTraceContext) const
    {
        if (message && message[0])
        {
            AZStd::vector<AZStd::string> lines;
            AzFramework::StringFunc::Tokenize(message, lines, '\n', true, true); // Make sure to keep empty lines because it could be intentional blank lines someone has added for formatting reasons

            // If the message ended with a newline, remove it, we're adding newlines to each line already
            if(lines.back().empty())
            {
                lines.pop_back();
            }

            for (const AZStd::string& line : lines)
            {
                if(includeTraceContext)
                {
                    DumpTraceContext(stream);
                }

                if (prefix && prefix[0])
                {
                    fprintf(stream, "%s: ", prefix);
                }

                if(extraPrefix && extraPrefix[0])
                {
                    fprintf(stream, "%s", extraPrefix);
                }

                fprintf(stream, "%s\n", line.c_str());
            }

            // Make sure the message ends with a newline
            if (message[AZStd::char_traits<char>::length(message) - 1] != '\n')
            {
                fprintf(stream, "\n");
            }

            if (forceFlush)
            {
                fflush(stream);
            }
        }
    }
} // namespace AssetBuilder
