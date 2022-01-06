/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Support for Windows Error Reporting (WER)


#include "CrySystem_precompiled.h"

#ifdef WIN32

#include "System.h"
#include <AzCore/PlatformIncl.h>
#include <tchar.h>
#include "errorrep.h"
#include "ISystem.h"

#include <DbgHelp.h>

static WCHAR szPath[MAX_PATH + 1];
static WCHAR szFR[] = L"\\System32\\FaultRep.dll";

WCHAR* GetFullPathToFaultrepDll(void)
{
    UINT rc = GetSystemWindowsDirectoryW(szPath, ARRAYSIZE(szPath));
    if (rc == 0 || rc > ARRAYSIZE(szPath) - ARRAYSIZE(szFR) - 1)
    {
        return NULL;
    }

    wcscat_s(szPath, szFR);
    return szPath;
}


typedef BOOL (WINAPI * MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
    );

//////////////////////////////////////////////////////////////////////////
LONG WINAPI CryEngineExceptionFilterMiniDump(struct _EXCEPTION_POINTERS* pExceptionPointers, const char* szDumpPath, MINIDUMP_TYPE DumpType)
{
    // note:  In debug mode, this dll is loaded on startup anyway, so this should not incur an additional load unless it crashes
    // very early during startup.

    fflush(nullptr); // according to MSDN on fflush, calling fflush on null flushes all buffers.
    HMODULE hndDBGHelpDLL = LoadLibraryA("DBGHELP.DLL");

    if (!hndDBGHelpDLL)
    {
        CryLogAlways("Failed to record DMP file: Could not open DBGHELP.DLL");
        return EXCEPTION_CONTINUE_SEARCH;
    }

    MINIDUMPWRITEDUMP dumpFnPtr = (MINIDUMPWRITEDUMP)::GetProcAddress(hndDBGHelpDLL, "MiniDumpWriteDump");
    if (!dumpFnPtr)
    {
        CryLogAlways("Failed to record DMP file: Unable to find MiniDumpWriteDump in DBGHELP.DLL");
        return EXCEPTION_CONTINUE_SEARCH;
    }

    AZStd::wstring szDumpPathW;
    AZStd::to_wstring(szDumpPathW, szDumpPath);
    HANDLE hFile = ::CreateFileW(szDumpPathW.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        CryLogAlways("Failed to record DMP file: could not open file '%s' for writing - error code: %d", szDumpPath, GetLastError());
        return EXCEPTION_CONTINUE_SEARCH;
    }

    _MINIDUMP_EXCEPTION_INFORMATION ExInfo;
    ExInfo.ThreadId = ::GetCurrentThreadId();
    ExInfo.ExceptionPointers = pExceptionPointers;
    ExInfo.ClientPointers = NULL;

    BOOL bOK = dumpFnPtr(GetCurrentProcess(), GetCurrentProcessId(), hFile, DumpType, &ExInfo, NULL, NULL);
    ::CloseHandle(hFile);

    if (bOK)
    {
        CryLogAlways("Successfully recorded DMP file:  '%s'", szDumpPath);
        return EXCEPTION_EXECUTE_HANDLER; // SUCCESS!  you can execute your handlers now
    }
    else
    {
        CryLogAlways("Failed to record DMP file: '%s' - error code: %d", szDumpPath, GetLastError());
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

//////////////////////////////////////////////////////////////////////////
LONG WINAPI CryEngineExceptionFilterWER(struct _EXCEPTION_POINTERS* pExceptionPointers)
{
    if (g_cvars.sys_WER > 1)
    {
        AZ::IO::FixedMaxPath dumpPath{ "@log@/CE2Dump.dmp" };
        if (auto fileIoBase = AZ::IO::FileIOBase::GetInstance(); fileIoBase != nullptr)
        {
            dumpPath = fileIoBase->ResolvePath(dumpPath, "@log@/CE2Dump.dmp");
        }

        MINIDUMP_TYPE mdumpValue = (MINIDUMP_TYPE)(MiniDumpNormal);
        if (g_cvars.sys_WER > 1)
        {
            mdumpValue = (MINIDUMP_TYPE)(g_cvars.sys_WER - 2);
        }

        return CryEngineExceptionFilterMiniDump(pExceptionPointers, dumpPath.c_str(), mdumpValue);
    }

    LONG lRet = EXCEPTION_CONTINUE_SEARCH;
    WCHAR* psz = GetFullPathToFaultrepDll();
    if (psz)
    {
        HMODULE hFaultRepDll = LoadLibraryW(psz);
        if (hFaultRepDll)
        {
            pfn_REPORTFAULT pfn = (pfn_REPORTFAULT)GetProcAddress(hFaultRepDll, "ReportFault");
            if (pfn)
            {
                pfn(pExceptionPointers, 0);
                lRet = EXCEPTION_EXECUTE_HANDLER;
            }
            FreeLibrary(hFaultRepDll);
        }
    }
    return lRet;
}

#endif // WIN32
