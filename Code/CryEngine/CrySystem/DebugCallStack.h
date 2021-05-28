/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYSYSTEM_DEBUGCALLSTACK_H
#define CRYINCLUDE_CRYSYSTEM_DEBUGCALLSTACK_H
#pragma once


#include "IDebugCallStack.h"

#if defined (WIN32) || defined (WIN64)

//! Limits the maximal number of functions in call stack.
const int MAX_DEBUG_STACK_ENTRIES_FILE_DUMP = 12;

struct ISystem;

//!============================================================================
//!
//! DebugCallStack class, capture call stack information from symbol files.
//!
//!============================================================================
class DebugCallStack
    : public IDebugCallStack
{
public:
    DebugCallStack();
    virtual ~DebugCallStack();

    ISystem* GetSystem() { return m_pSystem; };

    virtual string GetModuleNameForAddr(void* addr);
    virtual void GetProcNameForAddr(void* addr, string& procName, void*& baseAddr, string& filename, int& line);
    virtual string GetCurrentFilename();

    void installErrorHandler(ISystem* pSystem);
    virtual int handleException(EXCEPTION_POINTERS* exception_pointer);

    virtual void ReportBug(const char*);

    void dumpCallStack(std::vector<string>& functions);

    void SetUserDialogEnable(const bool bUserDialogEnable);

    typedef std::map<void*, string> TModules;
protected:
    static void RemoveOldFiles();
    static void RemoveFile(const char* szFileName);

    static int PrintException(EXCEPTION_POINTERS* exception_pointer);
    static INT_PTR CALLBACK ExceptionDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ConfirmSaveDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

    void LogExceptionInfo(EXCEPTION_POINTERS* exception_pointer);
    bool BackupCurrentLevel();
    bool SaveCurrentLevel();
    int SubmitBug(EXCEPTION_POINTERS* exception_pointer);
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
    CONTEXT  m_context;

    TModules m_modules;
};

#endif //WIN32

#endif // CRYINCLUDE_CRYSYSTEM_DEBUGCALLSTACK_H
