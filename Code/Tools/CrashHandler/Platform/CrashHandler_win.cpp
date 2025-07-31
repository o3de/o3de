/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Base Crashpad Windows implementation

#include <AzCore/PlatformIncl.h>

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>
#include <CrashHandler.h>
#include <client/crashpad_client.h>
#include <stdio.h>

namespace CrashHandler
{
    const char* crashHandlerPath = "ToolsCrashUploader.exe";
    static AZStd::string g_dumpCallstackFolder;

    const char* TranslateExceptionCode(DWORD dwExcept)
    {
        switch (dwExcept)
        {
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
            // case EXCEPTION_POSSIBLE_DEADLOCK: return "EXCEPTION_POSSIBLE_DEADLOCK";   break ;

        case STATUS_FLOAT_MULTIPLE_FAULTS:
            return "STATUS_FLOAT_MULTIPLE_FAULTS";
            break;
        case STATUS_FLOAT_MULTIPLE_TRAPS:
            return "STATUS_FLOAT_MULTIPLE_TRAPS";
            break;
        default:
            return "Unknown";
            break;
        }
    }

    bool HandleCrash(EXCEPTION_POINTERS* pex)
    {
        // Copy logs to the report folder
        {
            AZStd::string path;
            const char* logAlias = AZ::IO::FileIOBase::GetDirectInstance()->GetAlias("@log@");
            if (!logAlias)
            {
                logAlias = AZ::IO::FileIOBase::GetDirectInstance()->GetAlias("@products@");
            }

            if (logAlias)
            {
                path = logAlias;
                path += "\\Editor.log";
            }

            struct stat fileInfo;
            if (stat(path.c_str(), &fileInfo) == 0)
            {
                AZStd::string backupFileName = g_dumpCallstackFolder + "\\editor.log";
                AZStd::wstring fileNameW;
                AZStd::to_wstring(fileNameW, path.c_str());
                AZStd::wstring backupFileNameW;
                AZStd::to_wstring(backupFileNameW, backupFileName.c_str());
                CopyFileW(fileNameW.c_str(), backupFileNameW.c_str(), false);
            }
        }

        char excCode[4096];
        char excAddr[80];
        const char* excName;
        if (!pex)
        {
            excName = "Fatal Error";
            azstrcpy(excCode, AZ_ARRAY_SIZE(excCode), excName);
            azstrcpy(excAddr, AZ_ARRAY_SIZE(excAddr), "");
        }
        else
        {
            sprintf_s(excAddr, "0x%04X:0x%p", pex->ContextRecord->SegCs, pex->ExceptionRecord->ExceptionAddress);
            sprintf_s(excCode, "0x%08lX", pex->ExceptionRecord->ExceptionCode);
            excName = TranslateExceptionCode(pex->ExceptionRecord->ExceptionCode);
        }

        AZStd::string callstackMessage;
        callstackMessage.append(AZStd::string::format("Exception Code: %s\n", excCode));
        callstackMessage.append(AZStd::string::format("Exception Addr: %s\n", excAddr));
        callstackMessage.append(AZStd::string::format("Exception Name : %s\n", excName));

        if (pex && pex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            if (pex->ExceptionRecord->NumberParameters > 1)
            {
                ULONG_PTR iswrite = pex->ExceptionRecord->ExceptionInformation[0];
                DWORD64 accessAddr = pex->ExceptionRecord->ExceptionInformation[1];
                if (iswrite)
                {
                    callstackMessage.append(AZStd::string::format(
                        "Attempt to write data to address 0x%08llu\r\nThe memory could not be \"written\" \n\n", accessAddr));
                }
                else
                {
                    callstackMessage.append(AZStd::string::format(
                        "Attempt to read from address 0x%08llu\r\nThe memory could not be \"read\" \n\n", accessAddr));
                }
            }
        }
        else
        {
            callstackMessage.append("\n");
        }

        AZ::Debug::StackFrame frames[25];
        AZ::Debug::SymbolStorage::StackLine lines[AZ_ARRAY_SIZE(frames)];
        const unsigned int numFrames = AZ::Debug::StackRecorder::Record(frames, AZ_ARRAY_SIZE(frames), 3);
        if (numFrames)
        {
            AZ::Debug::SymbolStorage::DecodeFrames(frames, numFrames, lines);
            for (unsigned int i = 0; i < numFrames; i++)
            {
                callstackMessage.append(AZStd::string::format("%2d) %s\n", i, lines[i]));
            }
        }

        AZStd::string callstackFilePath = g_dumpCallstackFolder;
        callstackFilePath += "\\callstack.log";

        AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
        AZ::IO::FileIOBase::GetDirectInstance()->Open(callstackFilePath.c_str(), AZ::IO::GetOpenModeFromStringMode("w+t"), fileHandle);
        if (fileHandle != AZ::IO::InvalidHandle)
        {
            AZ::IO::FileIOBase::GetDirectInstance()->Write(fileHandle, callstackMessage.c_str(), callstackMessage.size());
            AZ::IO::FileIOBase::GetDirectInstance()->Flush(fileHandle);
            AZ::IO::FileIOBase::GetDirectInstance()->Close(fileHandle);
        }

        return false;
    }

    const char* CrashHandlerBase::GetCrashHandlerExecutableName() const
    {
        return crashHandlerPath;
    }

    void CrashHandlerBase::GetOSAnnotations(CrashHandlerAnnotations& annotations) const
    {
        annotations["os"] = "windows";

        // Disable warning about GetVersion deprecation
        // Windows now encourages users to check for the specific capability they need rather
        // than relying on a version ID due to badly written/lazy code (If buildID >= NUM then I certainly can do x...)
        // In this case we really just want a build ID to report
        AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
        DWORD winVersion = GetVersion();
        AZ_POP_DISABLE_WARNING

        // VersionMajor lives in the low byte of the low word
        int versionMajor = winVersion & 0xFF;
        // VersionMinor lives in the high byte of the low word
        int versionMinor = (winVersion & (0xFF << 8)) >> 8;

        int osBuild = 0;
        if (winVersion < 0x80000000)
        {
            // BuildID lives in the high word
            osBuild = (winVersion & (0xFFFF << 16)) >> 16;
        }

        std::string annotationBuf;

        annotationBuf = std::to_string(versionMajor) + "." + std::to_string(versionMinor);
        annotations["os.version"] = annotationBuf;

        annotationBuf = std::to_string(osBuild);
        annotations["os.build"] = annotationBuf;

        const size_t kbSize = 1024;
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);
        annotationBuf = std::to_string(statex.dwMemoryLoad);
        annotations["vm.used"] = annotationBuf;
        annotationBuf = std::to_string(statex.ullTotalPhys / kbSize);
        annotations["vm.total"] = annotationBuf;
        annotationBuf = std::to_string(statex.ullAvailPhys / kbSize);
        annotations["vm.free"] = annotationBuf;
        annotationBuf = std::to_string(statex.ullTotalPageFile / kbSize);
        annotations["vm.swap.size"] = annotationBuf;

        RECT desktopRect;
        const HWND desktopWind = GetDesktopWindow();
        GetWindowRect(desktopWind, &desktopRect);
        annotationBuf = std::to_string(desktopRect.right) + "x" + std::to_string(desktopRect.bottom);
        annotations["resolution"] = annotationBuf;

        ULARGE_INTEGER freeBytes;
        auto queryResult = GetDiskFreeSpaceExA(".", &freeBytes, nullptr, nullptr);
        if (queryResult)
        {
            annotationBuf = std::to_string(freeBytes.QuadPart);
            annotations["disk_free"] = annotationBuf;
        }
        else
        {
            annotations["disk_free"] = "error";
        }
    }

    void CrashHandlerBase::ClientInitialized(crashpad::CrashpadClient& client, bool manualCrashSubmission) const
    {
        g_dumpCallstackFolder = g_dumpCallstackFolder.format("%s/reports", m_crashDbPath.c_str());
        if (manualCrashSubmission)
            client.SetFirstChanceExceptionHandler(HandleCrash);
    }
} // namespace CrashHandler
