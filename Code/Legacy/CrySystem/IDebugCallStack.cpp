/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : A multiplatform base class for handling errors and collecting call stacks


#include "CrySystem_precompiled.h"
#include "IDebugCallStack.h"
#include "System.h"
#include <AzFramework/IO/FileOperations.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>
//#if !defined(LINUX)

#include <ISystem.h>

const char* const IDebugCallStack::s_szFatalErrorCode = "FATAL_ERROR";

IDebugCallStack::IDebugCallStack()
    : m_bIsFatalError(false)
    , m_postBackupProcess(0)
    , m_memAllocFileHandle(AZ::IO::InvalidHandle)
{
}

IDebugCallStack::~IDebugCallStack()
{
    StopMemLog();
}

#if AZ_LEGACY_CRYSYSTEM_TRAIT_DEBUGCALLSTACK_SINGLETON
IDebugCallStack* IDebugCallStack::instance()
{
    static IDebugCallStack sInstance;
    return &sInstance;
}
#endif

void IDebugCallStack::FileCreationCallback(void (* postBackupProcess)())
{
    m_postBackupProcess = postBackupProcess;
}
//////////////////////////////////////////////////////////////////////////
void IDebugCallStack::LogCallstack()
{
    AZ::Debug::Trace::PrintCallstack("", 2);
}

const char* IDebugCallStack::TranslateExceptionCode(DWORD dwExcept)
{
    switch (dwExcept)
    {
#if AZ_LEGACY_CRYSYSTEM_TRAIT_DEBUGCALLSTACK_TRANSLATE
    case EXCEPTION_ACCESS_VIOLATION:
        return "EXCEPTION_ACCESS_VIOLATION";
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        return "EXCEPTION_DATATYPE_MISALIGNMENT";
        break;
    case EXCEPTION_BREAKPOINT:
        return "EXCEPTION_BREAKPOINT";
        break;
    case EXCEPTION_SINGLE_STEP:
        return "EXCEPTION_SINGLE_STEP";
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        return "EXCEPTION_FLT_DENORMAL_OPERAND";
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        return "EXCEPTION_FLT_INEXACT_RESULT";
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        return "EXCEPTION_FLT_INVALID_OPERATION";
        break;
    case EXCEPTION_FLT_OVERFLOW:
        return "EXCEPTION_FLT_OVERFLOW";
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        return "EXCEPTION_FLT_STACK_CHECK";
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        return "EXCEPTION_FLT_UNDERFLOW";
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        break;
    case EXCEPTION_INT_OVERFLOW:
        return "EXCEPTION_INT_OVERFLOW";
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        return "EXCEPTION_PRIV_INSTRUCTION";
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        return "EXCEPTION_IN_PAGE_ERROR";
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        return "EXCEPTION_ILLEGAL_INSTRUCTION";
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        break;
    case EXCEPTION_STACK_OVERFLOW:
        return "EXCEPTION_STACK_OVERFLOW";
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        return "EXCEPTION_INVALID_DISPOSITION";
        break;
    case EXCEPTION_GUARD_PAGE:
        return "EXCEPTION_GUARD_PAGE";
        break;
    case EXCEPTION_INVALID_HANDLE:
        return "EXCEPTION_INVALID_HANDLE";
        break;
    //case EXCEPTION_POSSIBLE_DEADLOCK: return "EXCEPTION_POSSIBLE_DEADLOCK";   break ;

    case STATUS_FLOAT_MULTIPLE_FAULTS:
        return "STATUS_FLOAT_MULTIPLE_FAULTS";
        break;
    case STATUS_FLOAT_MULTIPLE_TRAPS:
        return "STATUS_FLOAT_MULTIPLE_TRAPS";
        break;


#endif
    default:
        return "Unknown";
        break;
    }
}

void IDebugCallStack::PutVersion(char* str, size_t length)
{
AZ_PUSH_DISABLE_WARNING(4996, "-Wunknown-warning-option")

    if (!gEnv || !gEnv->pSystem)
    {
        return;
    }

    char sFileVersion[128];
    gEnv->pSystem->GetFileVersion().ToString(sFileVersion, sizeof(sFileVersion));

    char sProductVersion[128];
    gEnv->pSystem->GetProductVersion().ToString(sProductVersion, sizeof(sFileVersion));


    //! Get time.
    time_t ltime;
    time(&ltime);
    tm* today = localtime(&ltime);

    char s[1024];
    //! Use strftime to build a customized time string.
    strftime(s, 128, "Logged at %#c\n", today);
    azstrcat(str, length, s);
    sprintf_s(s, "FileVersion: %s\n", sFileVersion);
    azstrcat(str, length, s);
    sprintf_s(s, "ProductVersion: %s\n", sProductVersion);
    azstrcat(str, length, s);

    if (gEnv->pLog)
    {
        const char* logfile = gEnv->pLog->GetFileName();
        if (logfile)
        {
            sprintf (s, "LogFile: %s\n", logfile);
            azstrcat(str, length, s);
        }
    }

    AZ::IO::FixedMaxPathString projectPath = AZ::Utils::GetProjectPath();
    azstrcat(str, length, "ProjectDir: ");
    azstrcat(str, length, projectPath.c_str());
    azstrcat(str, length, "\n");

#if AZ_LEGACY_CRYSYSTEM_TRAIT_DEBUGCALLSTACK_APPEND_MODULENAME
    AZ::Utils::GetExecutablePath(s, sizeof(s));
    
    // Log EXE filename only if possible (not full EXE path which could contain sensitive info)
    AZStd::string exeName;
    if (AZ::StringFunc::Path::GetFullFileName(s, exeName))
    {
        azstrcat(str, length, "Executable: ");
        azstrcat(str, length, exeName.c_str());

#   ifdef AZ_DEBUG_BUILD
        azstrcat(str, length, " (debug: yes");
#   else
        azstrcat(str, length, " (debug: no");
#   endif
    }
#endif
AZ_POP_DISABLE_WARNING
}


//Crash the application, in this way the debug callstack routine will be called and it will create all the necessary files (error.log, dump, and eventually screenshot)
void IDebugCallStack::FatalError(const char* description)
{
    m_bIsFatalError = true;
    WriteLineToLog(description);

#ifndef _RELEASE
    bool bShowDebugScreen = g_cvars.sys_no_crash_dialog == 0;
    // showing the debug screen is not safe when not called from mainthread
    // it normally leads to a infinity recursion followed by a stack overflow, preventing
    // useful call stacks, thus they are disabled
    bShowDebugScreen = bShowDebugScreen && gEnv->mMainThreadId == CryGetCurrentThreadId();
    if (bShowDebugScreen)
    {
        EBUS_EVENT(AZ::NativeUI::NativeUIRequestBus, DisplayOkDialog, "Open 3D Engine Fatal Error", description, false);
    }
#endif

#if defined(WIN32) || !defined(_RELEASE)
    int* p = 0x0;
    *p = 1; // we're intentionally crashing here
#endif
}

void IDebugCallStack::WriteLineToLog(const char* format, ...)
{
    va_list ArgList;
    char        szBuffer[MAX_WARNING_LENGTH];
    va_start(ArgList, format);
    vsnprintf_s(szBuffer, sizeof(szBuffer), sizeof(szBuffer) - 1, format, ArgList);
    azstrcat(szBuffer, MAX_WARNING_LENGTH, "\n");
    szBuffer[sizeof(szBuffer) - 1] = '\0';
    va_end(ArgList);

    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
    AZ::IO::FileIOBase::GetDirectInstance()->Open("@log@\\error.log", AZ::IO::GetOpenModeFromStringMode("a+t"), fileHandle);
    if (fileHandle != AZ::IO::InvalidHandle)
    {
        AZ::IO::FileIOBase::GetDirectInstance()->Write(fileHandle, szBuffer, strlen(szBuffer));
        AZ::IO::FileIOBase::GetDirectInstance()->Flush(fileHandle);
        AZ::IO::FileIOBase::GetDirectInstance()->Close(fileHandle);
    }
}

//////////////////////////////////////////////////////////////////////////
void IDebugCallStack::StartMemLog()
{
    AZ::IO::FileIOBase::GetDirectInstance()->Open("@log@\\memallocfile.log", AZ::IO::OpenMode::ModeWrite, m_memAllocFileHandle);

    assert(m_memAllocFileHandle != AZ::IO::InvalidHandle);
}

//////////////////////////////////////////////////////////////////////////
void IDebugCallStack::StopMemLog()
{
    if (m_memAllocFileHandle != AZ::IO::InvalidHandle)
    {
        AZ::IO::FileIOBase::GetDirectInstance()->Close(m_memAllocFileHandle);
        m_memAllocFileHandle = AZ::IO::InvalidHandle;
    }
}
//#endif //!defined(LINUX)
