/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Trace.h>

#include <AzCore/base.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Debug/TraceMessagesDrillerBus.h>

#include <stdio.h>

namespace AZ
{
#if defined(AZ_ENABLE_DEBUG_TOOLS)
    LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo);
    LPTOP_LEVEL_EXCEPTION_FILTER g_previousExceptionHandler = nullptr;
#endif

    const int g_maxMessageLength = 4096;

    namespace Debug
    {
        namespace Platform
        {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
            bool IsDebuggerPresent()
            {
                return ::IsDebuggerPresent() ? true : false;
            }

            void HandleExceptions(bool isEnabled)
            {
                if (isEnabled)
                {
                    g_previousExceptionHandler = ::SetUnhandledExceptionFilter(&ExceptionHandler);
                }
                else
                {
                    ::SetUnhandledExceptionFilter(g_previousExceptionHandler);
                    g_previousExceptionHandler = NULL;
                }
            }

            void DebugBreak()
            {
                __debugbreak();
            }
#endif // AZ_ENABLE_DEBUG_TOOLS

            void Terminate(int exitCode)
            {
                TerminateProcess(GetCurrentProcess(), exitCode);
            }

            void OutputToDebugger(const char* window, const char* message)
            {
                AZ_UNUSED(window);
#ifdef _UNICODE
                wchar_t messageW[g_maxMessageLength];
                size_t numCharsConverted;
                if (mbstowcs_s(&numCharsConverted, messageW, message, g_maxMessageLength - 1) == 0)
                {
                    OutputDebugStringW(messageW);
                }
#else // !_UNICODE
                OutputDebugString(message);
#endif // !_UNICODE
            }
        }
    }

#if defined(AZ_ENABLE_DEBUG_TOOLS)

    const char* GetExeptionName(DWORD code)
    {
    #define RETNAME(exc)    case exc: \
    return (#exc);
        switch (code)
        {
            RETNAME(EXCEPTION_ACCESS_VIOLATION);
            RETNAME(EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
            RETNAME(EXCEPTION_BREAKPOINT);
            RETNAME(EXCEPTION_DATATYPE_MISALIGNMENT);
            RETNAME(EXCEPTION_FLT_DENORMAL_OPERAND);
            RETNAME(EXCEPTION_FLT_DIVIDE_BY_ZERO);
            RETNAME(EXCEPTION_FLT_INEXACT_RESULT);
            RETNAME(EXCEPTION_FLT_INVALID_OPERATION);
            RETNAME(EXCEPTION_FLT_OVERFLOW);
            RETNAME(EXCEPTION_FLT_STACK_CHECK);
            RETNAME(EXCEPTION_FLT_UNDERFLOW);
            RETNAME(EXCEPTION_ILLEGAL_INSTRUCTION);
            RETNAME(EXCEPTION_IN_PAGE_ERROR);
            RETNAME(EXCEPTION_INT_DIVIDE_BY_ZERO);
            RETNAME(EXCEPTION_INT_OVERFLOW);
            RETNAME(EXCEPTION_INVALID_DISPOSITION);
            RETNAME(EXCEPTION_NONCONTINUABLE_EXCEPTION);
            RETNAME(EXCEPTION_PRIV_INSTRUCTION);
            RETNAME(EXCEPTION_SINGLE_STEP);
            RETNAME(EXCEPTION_STACK_OVERFLOW);
            RETNAME(EXCEPTION_INVALID_HANDLE);
        default:
            return (char*)("Unknown exception");
        }
    #undef RETNAME
    }

    // Defined in Trace.cpp
    namespace DebugInternal
    {
        extern AZ_THREAD_LOCAL bool g_suppressEBusCalls; // used when it would be dangerous to use ebus broadcasts.
    }

    extern void* g_exceptionInfo;

    LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
    {
        static bool volatile isInExeption = false;
        if (isInExeption)
        {
            // prevent g_tracer from calling the tracebus:
            DebugInternal::g_suppressEBusCalls = true;
            Debug::Trace::Instance().Output(nullptr, "Exception handler loop!");
            Debug::Trace::Instance().PrintCallstack(nullptr, 0, ExceptionInfo->ContextRecord);
            DebugInternal::g_suppressEBusCalls = false;

            if (Debug::Trace::Instance().IsDebuggerPresent())
            {
                Debug::Trace::Instance().Break();
            }
            else
            {
               _exit(ExceptionInfo->ExceptionRecord->ExceptionCode); // do not proceed any further.  note that this is expected to terminate immediately.  exit without the underscore may still execute code.
            }
        }

        isInExeption = true;
        g_exceptionInfo = (void*)ExceptionInfo;

        char message[g_maxMessageLength];
        Debug::Trace::Instance().Output(nullptr, "==================================================================\n");
        azsnprintf(message, g_maxMessageLength, "Exception : 0x%X - '%s' [%p]\n", ExceptionInfo->ExceptionRecord->ExceptionCode, GetExeptionName(ExceptionInfo->ExceptionRecord->ExceptionCode), ExceptionInfo->ExceptionRecord->ExceptionAddress);
        Debug::Trace::Instance().Output(nullptr, message);

        EBUS_EVENT(Debug::TraceMessageDrillerBus, OnException, message);

        bool result = false;
        EBUS_EVENT_RESULT(result, Debug::TraceMessageBus, OnException, message);
        if (result)
        {
            Debug::Trace::Instance().Output(nullptr, "==================================================================\n");
            g_exceptionInfo = NULL;
            // if someone ever returns TRUE we assume that they somehow handled this exception and continue.
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        Debug::Trace::Instance().PrintCallstack(nullptr, 0, ExceptionInfo->ContextRecord);
        Debug::Trace::Instance().Output(nullptr, "==================================================================\n");

        // allowing continue of execution is not valid here.  This handler gets called for serious exceptions.
        // programs wanting things like a message box can implement them on a case-by-case basis, but we want no such 
        // default behavior - this code is used in automated build systems and UI applications alike.
        LONG lReturn = EXCEPTION_CONTINUE_SEARCH;
        isInExeption = false;
        g_exceptionInfo = NULL;
        return lReturn;
    }

#endif
}
