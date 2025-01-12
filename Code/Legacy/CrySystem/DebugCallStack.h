/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CRYINCLUDE_CRYSYSTEM_DEBUGCALLSTACK_H
#define CRYINCLUDE_CRYSYSTEM_DEBUGCALLSTACK_H
#pragma once

#include "IDebugCallStack.h"

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/map.h>

#if defined(WIN32) || defined(WIN64)

//! Limits the maximal number of functions in call stack.
const int MAX_DEBUG_STACK_ENTRIES_FILE_DUMP = 12;

struct ISystem;

//!============================================================================
//!
//! DebugCallStack class, capture call stack information from symbol files.
//!
//!============================================================================
class DebugCallStack : public IDebugCallStack
{
public:
    DebugCallStack();
    virtual ~DebugCallStack();

    ISystem* GetSystem()
    {
        return m_pSystem;
    };

    virtual AZStd::string GetModuleNameForAddr(void* addr) override;
    virtual void GetProcNameForAddr(void* addr, AZStd::string& procName, void*& baseAddr, AZStd::string& filename, int& line) override;
    virtual AZStd::string GetCurrentFilename() override;
    virtual int handleException(EXCEPTION_POINTERS* exception_pointer) override;
    virtual void ReportBug(const char*) override;

    void installErrorHandler(ISystem* pSystem);
    void dumpCallStack(AZStd::vector<AZStd::string>& functions);
    void SetUserDialogEnable(const bool bUserDialogEnable);

    typedef AZStd::map<void*, AZStd::string> TModules;

protected:
    enum class UserPostExceptionChoice
    {
        Exit,
        Recover // Only available if the exception type allows it (eg: floating point exception)
    };

    static void RemoveOldFiles();
    static void RemoveFile(const char* szFileName);

    static UserPostExceptionChoice AskUserToRecoverOrCrash(EXCEPTION_POINTERS* exception_pointer);

    void SaveExceptionInfoAndShowUserReportDialogs(EXCEPTION_POINTERS* exception_pointer);
    bool BackupCurrentLevel();
    bool SaveCurrentLevel();
    UserPostExceptionChoice SubmitBugAndAskToRecoverOrCrash(EXCEPTION_POINTERS* exception_pointer);
    void ResetFPU(EXCEPTION_POINTERS* pex);

    static const int s_iCallStackSize = 32768;

    char m_excLine[256];
    char m_excModule[128];

    char m_excDesc[MAX_WARNING_LENGTH];
    char m_excCode[MAX_WARNING_LENGTH];
    char m_excAddr[80];
    char m_excCallstack[s_iCallStackSize];

    void* prevExceptionHandler;

    bool m_bCrash;
    const char* m_szBugMessage;

    ISystem* m_pSystem;

    int m_nSkipNumFunctions;
    CONTEXT m_context;

    TModules m_modules;
};

#endif // WIN32

#endif // CRYINCLUDE_CRYSYSTEM_DEBUGCALLSTACK_H
