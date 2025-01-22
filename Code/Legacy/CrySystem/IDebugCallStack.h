/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : A multiplatform base class for handling errors and collecting call stacks

#ifndef CRYINCLUDE_CRYSYSTEM_IDEBUGCALLSTACK_H
#define CRYINCLUDE_CRYSYSTEM_IDEBUGCALLSTACK_H
#pragma once

#include "System.h"

#include <AzCore/std/string/string.h>
#include <AzCore/IO/FileIO.h>

#if AZ_LEGACY_CRYSYSTEM_TRAIT_FORWARD_EXCEPTION_POINTERS
struct EXCEPTION_POINTERS;
#endif
//! Limits the maximal number of functions in call stack.
enum
{
    MAX_DEBUG_STACK_ENTRIES = 80
};

class IDebugCallStack
{
public:
    // Returns single instance of DebugStack
    static IDebugCallStack* instance();

    virtual int handleException([[maybe_unused]] EXCEPTION_POINTERS* exception_pointer){return 0; }

    // returns the module name of a given address
    virtual AZStd::string GetModuleNameForAddr([[maybe_unused]] void* addr) { return "[unknown]"; }

    // returns the function name of a given address together with source file and line number (if available) of a given address
    virtual void GetProcNameForAddr(void* addr, AZStd::string& procName, void*& baseAddr, AZStd::string& filename, int& line)
    {
        filename = "[unknown]";
        line = 0;
        baseAddr = addr;
        procName = AZStd::string::format("[%p]", addr);
    }

    // returns current filename
    virtual AZStd::string GetCurrentFilename()  { return "[unknown]"; }

    //! Dumps Current Call Stack to log.
    virtual void LogCallstack();
    //triggers a fatal error, so the DebugCallstack can create the error.log and terminate the application
    void FatalError(const char*);

    //Reports a bug and continues execution
    virtual void ReportBug(const char*) {}

    virtual void FileCreationCallback(void (* postBackupProcess)());

    static void WriteLineToLog(const char* format, ...);

    virtual void StartMemLog();
    virtual void StopMemLog();

protected:
    IDebugCallStack();
    virtual ~IDebugCallStack();

    static const char* TranslateExceptionCode(DWORD dwExcept);
    static void PutVersion(char* str, size_t length);

    bool m_bIsFatalError;
    static const char* const s_szFatalErrorCode;

    void (* m_postBackupProcess)();

    AZ::IO::HandleType m_memAllocFileHandle;
};



#endif // CRYINCLUDE_CRYSYSTEM_IDEBUGCALLSTACK_H
