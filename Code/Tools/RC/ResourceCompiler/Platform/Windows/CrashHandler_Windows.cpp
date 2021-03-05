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


#include "ResourceCompiler_precompiled.h"
#include "CrashHandler_Windows.h"

#include <IConsole.h>
#include "IResourceCompilerHelper.h"

#include "CryFixedString.h"

#include <DbgHelp.h>

#define DBGHELP_DLL_NAME "dbghelp.dll"


//=============================================================================
CrashHandler::CrashHandler()
{
    m_dumpFilename[0] = '\0';

    BusConnect();
}

//=============================================================================
CrashHandler::~CrashHandler()
{
    BusDisconnect();
}

//=============================================================================
bool CrashHandler::OnException(const char* /*message*/)
{
    if (m_dumpFilename[0])
    {
        WriteMinidump();
    }
    return false;   // continue error handler execution
}

//=============================================================================
void CrashHandler::SetDumpFile(const char* const dumpFilename)
{
    if (dumpFilename && dumpFilename[0])
    {
        azstrncpy(m_dumpFilename, AZ_ARRAY_SIZE(m_dumpFilename), dumpFilename, strlen(dumpFilename));
    }
}

//=============================================================================
void CrashHandler::WriteMinidump()
{
    // load any version we can.  Win now distributes the dll, and all the functions that are used have been around since winxp
    HMODULE hDll = LoadLibraryA(DBGHELP_DLL_NAME);

    CryFixedStringT<MAX_PATH + 200> strResult;

    if (hDll)
    {
        // based on DbgHelp.h
        typedef BOOL (WINAPI * MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
            CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
            CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
            CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

        MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress(hDll, "MiniDumpWriteDump");
        if (pDump)
        {
            {
                // create the file
                HANDLE hFile = ::CreateFile(m_dumpFilename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

                if (hFile != INVALID_HANDLE_VALUE)
                {
                    _MINIDUMP_EXCEPTION_INFORMATION ExInfo;

                    ExInfo.ThreadId = ::GetCurrentThreadId();
                    ExInfo.ExceptionPointers = (PEXCEPTION_POINTERS)AZ::Debug::Trace::GetNativeExceptionInfo();
                    ExInfo.ClientPointers = NULL;

                    // write the dump
                    MINIDUMP_TYPE const mdumpValue = (MINIDUMP_TYPE)
                        (MiniDumpNormal                             // include call stack, thread info, etc
                         | MiniDumpWithIndirectlyReferencedMemory   // try to find pointers on the stack and dump memory near where they're pointing
                         | MiniDumpWithDataSegs);                   // dump global variables (like gEnv)
                    BOOL const bOK = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdumpValue, &ExInfo, NULL, NULL);
                    if (bOK)
                    {
                        strResult.Format("Saved crash dump file to '%s'", m_dumpFilename);
                    }
                    else
                    {
                        strResult.Format("Failed to save crash dump file to '%s' (error %d)", m_dumpFilename, GetLastError());
                    }
                    ::CloseHandle(hFile);
                }
                else
                {
                    strResult.Format("Failed to create crash dump file '%s' (error %d)", m_dumpFilename, GetLastError());
                }
            }
        }
        else
        {
            strResult = "Failed to save crash dump file because " DBGHELP_DLL_NAME " is too old";
        }
    }
    else
    {
        strResult = "Failed to save crash dump file because " DBGHELP_DLL_NAME " is not found";
    }

    if (!strResult.empty())
    {
        fprintf(stderr, "%s\r\n", strResult.c_str());
    }
}
