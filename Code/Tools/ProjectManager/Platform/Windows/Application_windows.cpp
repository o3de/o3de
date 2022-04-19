/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Application.h>

#include <DbgHelp.h>
#include <errorrep.h>

#include <AzCore/IO/FileIO.h>

namespace O3DE::ProjectManager
{
    LONG WINAPI CreateMiniDump(struct _EXCEPTION_POINTERS* exceptionPointers)
    {
        static const AZStd::string dumpDirectoryAlias{ "@log@/" };
        static const AZStd::string dumpFileName{ "o3de.dmp" };
        static const AZStd::string dumpAliasPath{ dumpDirectoryAlias + dumpFileName };

        AZ::IO::FixedMaxPath dumpPath{ dumpAliasPath };
        if (auto fileIoBase = AZ::IO::FileIOBase::GetInstance(); fileIoBase != nullptr)
        {
            dumpPath = fileIoBase->ResolvePath(dumpPath, dumpAliasPath.c_str());
        }

        // FixedMaxPath can append '\x1' to the end of the path string which causes CreateFileW to fail,
        // Truncate anything after the end of the file name
        AZStd::string dumpPathFull{ dumpPath.String() };
        size_t fileNameStart = dumpPathFull.rfind(dumpFileName);
        AZStd::string dumpPathStr(dumpPathFull.begin(), dumpPathFull.begin() + (fileNameStart + dumpFileName.length()));

        const MINIDUMP_TYPE dumpType = MINIDUMP_TYPE::MiniDumpNormal;

        fflush(nullptr);
        HMODULE hndDBGHelpDLL = LoadLibraryA("DBGHELP.DLL");

        if (!hndDBGHelpDLL)
        {
            AZ_Warning("ProjectManager", false, "Failed to record DMP file: Could not open DBGHELP.DLL");
            return EXCEPTION_CONTINUE_SEARCH;
        }

        typedef BOOL(WINAPI * MiniDumpWriteDump)(
            HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE dumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION exceptionParam,
            CONST PMINIDUMP_USER_STREAM_INFORMATION userStreamParam, CONST PMINIDUMP_CALLBACK_INFORMATION callbackParam);

        MiniDumpWriteDump dumpFnPtr = reinterpret_cast<MiniDumpWriteDump>(GetProcAddress(hndDBGHelpDLL, "MiniDumpWriteDump"));
        if (!dumpFnPtr)
        {
            AZ_Warning("ProjectManager", false, "Failed to record DMP file: Unable to find MiniDumpWriteDump in DBGHELP.DLL");
            return EXCEPTION_CONTINUE_SEARCH;
        }

        AZStd::wstring dumpPathWStr;
        AZStd::to_wstring(dumpPathWStr, dumpPathStr);
        HANDLE fileHandle =
            CreateFileW(dumpPathWStr.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE)
        {
            AZ_Warning(
                "ProjectManager", false,
                "Failed to record DMP file: could not open file '%s' for writing, "
                "attempting to write to executable directory - error code: %d",
                dumpPathStr.c_str(), GetLastError());

            // Attempt to open a file in the executable directory as backup
            AZStd::to_wstring(dumpPathWStr, dumpFileName);
            fileHandle =
                CreateFileW(dumpPathWStr.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                AZ_Warning(
                    "ProjectManager", false, "Failed to record DMP file: could not open file '%s' for writing - error code: %d",
                    dumpPathStr.c_str(), GetLastError());
                return EXCEPTION_CONTINUE_SEARCH;
            }
        }

        _MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
        exceptionInfo.ThreadId = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = exceptionPointers;
        exceptionInfo.ClientPointers = NULL;

        BOOL dumpSuccessful = dumpFnPtr(GetCurrentProcess(), GetCurrentProcessId(), fileHandle, dumpType, &exceptionInfo, NULL, NULL);
        CloseHandle(fileHandle);

        if (dumpSuccessful)
        {
            AZ_Warning("ProjectManager", false, "Successfully recorded DMP file:  '%s'", dumpPathStr.c_str());
            return EXCEPTION_EXECUTE_HANDLER;
        }
        else
        {
            AZ_Warning("ProjectManager", false, "Failed to record DMP file: '%s' - error code: %d", dumpPathStr.c_str(), GetLastError());
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }

    void SetupCrashHandler()
    {
        SetUnhandledExceptionFilter(CreateMiniDump);
    }

} // namespace O3DE::ProjectManager
