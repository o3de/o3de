/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DebugCallStack.h"
#include "CrySystem_precompiled.h"

#if defined(WIN32) || defined(WIN64)

#include "System.h"

#include <CryPath.h>
#include <IConsole.h>

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/NativeUI/NativeUIRequests.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/parallel/spin_mutex.h>

#include <DbgHelp.h>
#include <Shellapi.h>

#define MAX_PATH_LENGTH 1024

static HWND hwndException = 0;
static bool g_bUserDialog = true; // true=on crash show dialog box, false=supress user interaction

static constexpr const char* SettingKey_IssueReportLink = "/O3DE/Settings/Links/Issue/Create";
static constexpr const char* IssueReportLinkFallback = "https://github.com/o3de/o3de/issues/new/choose";

extern int prev_sys_float_exceptions;

DWORD g_idDebugThreads[10];
const char* g_nameDebugThreads[10];
int g_nDebugThreads = 0;
AZStd::spin_mutex g_lockThreadDumpList;

static bool IsFloatingPointException(EXCEPTION_POINTERS* pex);

extern LONG WINAPI CryEngineExceptionFilterWER(struct _EXCEPTION_POINTERS* pExceptionPointers);
extern LONG WINAPI
CryEngineExceptionFilterMiniDump(struct _EXCEPTION_POINTERS* pExceptionPointers, const char* szDumpPath, MINIDUMP_TYPE mdumpValue);

void MarkThisThreadForDebugging(const char* name);
void UnmarkThisThreadFromDebugging();
void UpdateFPExceptionsMaskForThreads();

//=============================================================================
CONTEXT CaptureCurrentContext()
{
    CONTEXT context;
    memset(&context, 0, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&context);

    return context;
}

LONG __stdcall CryUnhandledExceptionHandler(EXCEPTION_POINTERS* pex)
{
    return DebugCallStack::instance()->handleException(pex);
}

BOOL CALLBACK EnumModules(PCSTR ModuleName, DWORD64 BaseOfDll, PVOID UserContext)
{
    DebugCallStack::TModules& modules = *static_cast<DebugCallStack::TModules*>(UserContext);
    modules[(void*)BaseOfDll] = ModuleName;

    return TRUE;
}
//=============================================================================
// Class Statics
//=============================================================================

// Return single instance of class.
IDebugCallStack* IDebugCallStack::instance()
{
    static DebugCallStack sInstance;
    return &sInstance;
}

//------------------------------------------------------------------------------------------------------------------------
// Sets up the symbols for functions in the debug file.
//------------------------------------------------------------------------------------------------------------------------
DebugCallStack::DebugCallStack()
    : prevExceptionHandler(0)
    , m_pSystem(0)
    , m_nSkipNumFunctions(0)
    , m_bCrash(false)
    , m_szBugMessage(NULL)
{
}

DebugCallStack::~DebugCallStack()
{
}

void DebugCallStack::RemoveOldFiles()
{
    RemoveFile("error.log");
    RemoveFile("error.bmp");
    RemoveFile("error.dmp");
}

void DebugCallStack::RemoveFile(const char* szFileName)
{
    FILE* pFile = nullptr;
    azfopen(&pFile, szFileName, "r");
    const bool bFileExists = (pFile != NULL);

    if (bFileExists)
    {
        fclose(pFile);

        WriteLineToLog("Removing file \"%s\"...", szFileName);
        if (remove(szFileName) == 0)
        {
            WriteLineToLog("File successfully removed.");
        }
        else
        {
            WriteLineToLog("Couldn't remove file!");
        }
    }
}

void DebugCallStack::installErrorHandler(ISystem* pSystem)
{
    m_pSystem = pSystem;
    prevExceptionHandler = (void*)SetUnhandledExceptionFilter(CryUnhandledExceptionHandler);

    MarkThisThreadForDebugging("main");
}

//////////////////////////////////////////////////////////////////////////
void DebugCallStack::SetUserDialogEnable(const bool bUserDialogEnable)
{
    g_bUserDialog = bUserDialogEnable;
}

void MarkThisThreadForDebugging(const char* name)
{
    AZStd::scoped_lock lock(g_lockThreadDumpList);
    DWORD id = GetCurrentThreadId();
    if (g_nDebugThreads == sizeof(g_idDebugThreads) / sizeof(g_idDebugThreads[0]))
    {
        return;
    }
    for (int i = 0; i < g_nDebugThreads; i++)
    {
        if (g_idDebugThreads[i] == id)
        {
            return;
        }
    }
    g_nameDebugThreads[g_nDebugThreads] = name;
    g_idDebugThreads[g_nDebugThreads++] = id;
    ((CSystem*)gEnv->pSystem)->EnableFloatExceptions(g_cvars.sys_float_exceptions);
}

void UnmarkThisThreadFromDebugging()
{
    AZStd::scoped_lock lock(g_lockThreadDumpList);
    DWORD id = GetCurrentThreadId();
    for (int i = g_nDebugThreads - 1; i >= 0; i--)
    {
        if (g_idDebugThreads[i] == id)
        {
            memmove(g_idDebugThreads + i, g_idDebugThreads + i + 1, (g_nDebugThreads - 1 - i) * sizeof(g_idDebugThreads[0]));
            memmove(g_nameDebugThreads + i, g_nameDebugThreads + i + 1, (g_nDebugThreads - 1 - i) * sizeof(g_nameDebugThreads[0]));
            --g_nDebugThreads;
        }
    }
}

void UpdateFPExceptionsMaskForThreads()
{
    int mask = -iszero(g_cvars.sys_float_exceptions);
    CONTEXT ctx;
    for (int i = 0; i < g_nDebugThreads; i++)
    {
        if (g_idDebugThreads[i] != GetCurrentThreadId())
        {
            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, g_idDebugThreads[i]);
            ctx.ContextFlags = CONTEXT_ALL;
            SuspendThread(hThread);
            GetThreadContext(hThread, &ctx);
#ifndef WIN64
            (ctx.FloatSave.ControlWord |= 7) &= ~5 | mask;
            (*(WORD*)(ctx.ExtendedRegisters + 24) |= 0x280) &= ~0x280 | mask;
#else
            (ctx.FltSave.ControlWord |= 7) &= ~5 | mask;
            (ctx.FltSave.MxCsr |= 0x280) &= ~0x280 | mask;
#endif
            SetThreadContext(hThread, &ctx);
            ResumeThread(hThread);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int DebugCallStack::handleException(EXCEPTION_POINTERS* exception_pointer)
{
    AZ_TracePrintf("Exit", "Exception with exit code: 0x%x", exception_pointer->ExceptionRecord->ExceptionCode);
    AZ::Debug::Trace::Instance().PrintCallstack("Exit");

    if (gEnv == NULL)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    ResetFPU(exception_pointer);

    prev_sys_float_exceptions = 0;
    const int cached_sys_float_exceptions = g_cvars.sys_float_exceptions;

    ((CSystem*)gEnv->pSystem)->EnableFloatExceptions(0);

    if (g_cvars.sys_WER)
    {
        gEnv->pLog->FlushAndClose();
        return CryEngineExceptionFilterWER(exception_pointer);
    }

    if (g_cvars.sys_no_crash_dialog)
    {
        DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
        SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    }

    m_bCrash = true;

    if (g_cvars.sys_no_crash_dialog)
    {
        DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
        SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
    }

    static bool firstTime = true;

    if (g_cvars.sys_dump_aux_threads)
    {
        for (int i = 0; i < g_nDebugThreads; i++)
        {
            if (g_idDebugThreads[i] != GetCurrentThreadId())
            {
                SuspendThread(OpenThread(THREAD_ALL_ACCESS, TRUE, g_idDebugThreads[i]));
            }
        }
    }

    // uninstall our exception handler.
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)prevExceptionHandler);

    if (!firstTime)
    {
        WriteLineToLog("Critical Exception! Called Multiple Times!");
        gEnv->pLog->FlushAndClose();
        // Exception called more then once.
        return EXCEPTION_EXECUTE_HANDLER;
    }

    // Print exception info:
    {
        char excCode[80];
        char excAddr[80];
        WriteLineToLog("<CRITICAL EXCEPTION>");
        sprintf_s(excAddr, "0x%04X:0x%p", exception_pointer->ContextRecord->SegCs, exception_pointer->ExceptionRecord->ExceptionAddress);
        sprintf_s(excCode, "0x%08lX", exception_pointer->ExceptionRecord->ExceptionCode);
        WriteLineToLog("Exception: %s, at Address: %s", excCode, excAddr);
    }

    firstTime = false;

    const UserPostExceptionChoice ret = SubmitBugAndAskToRecoverOrCrash(exception_pointer);

    if (ret != UserPostExceptionChoice::Recover)
    {
        CryEngineExceptionFilterWER(exception_pointer);
    }

    gEnv->pLog->FlushAndClose();

    if (exception_pointer->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
    {
        // This is non continuable exception. abort application now.
        exit(exception_pointer->ExceptionRecord->ExceptionCode);
    }

    if (ret == UserPostExceptionChoice::Exit)
    {
        // Immediate exit.
        // on windows, exit() and _exit() do all sorts of things, unfortuantely
        // TerminateProcess is the only way to die.
        TerminateProcess(
            GetCurrentProcess(), exception_pointer->ExceptionRecord->ExceptionCode); // we crashed, so don't return a zero exit code!
        // on linux based systems, _exit will not call ATEXIT and other things, which makes it more suitable for termination in an emergency
        // such as an unhandled exception. however, this function is a windows exception handler.
    }
    else if (ret == UserPostExceptionChoice::Recover)
    {
#ifndef WIN64
        exception_pointer->ContextRecord->FloatSave.StatusWord &= ~31;
        exception_pointer->ContextRecord->FloatSave.ControlWord |= 7;
        (*(WORD*)(exception_pointer->ContextRecord->ExtendedRegisters + 24) &= 31) |= 0x1F80;
#else
        exception_pointer->ContextRecord->FltSave.StatusWord &= ~31;
        exception_pointer->ContextRecord->FltSave.ControlWord |= 7;
        (exception_pointer->ContextRecord->FltSave.MxCsr &= 31) |= 0x1F80;
#endif
        firstTime = true;
        prevExceptionHandler = (void*)SetUnhandledExceptionFilter(CryUnhandledExceptionHandler);
        g_cvars.sys_float_exceptions = cached_sys_float_exceptions;
        ((CSystem*)gEnv->pSystem)->EnableFloatExceptions(g_cvars.sys_float_exceptions);
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    // Continue;
    return EXCEPTION_EXECUTE_HANDLER;
}

void DebugCallStack::ReportBug(const char* szErrorMessage)
{
    WriteLineToLog("Reporting bug: %s", szErrorMessage);

    m_szBugMessage = szErrorMessage;
    m_context = CaptureCurrentContext();
    SubmitBugAndAskToRecoverOrCrash(NULL);
    m_szBugMessage = NULL;
}

void DebugCallStack::dumpCallStack(AZStd::vector<AZStd::string>& funcs)
{
    WriteLineToLog("=============================================================================");
    int len = (int)funcs.size();
    for (int i = 0; i < len; i++)
    {
        const char* str = funcs[i].c_str();
        WriteLineToLog("%2d) %s", len - i, str);
    }
    WriteLineToLog("=============================================================================");
}

//////////////////////////////////////////////////////////////////////////
void DebugCallStack::SaveExceptionInfoAndShowUserReportDialogs(EXCEPTION_POINTERS* pex)
{
    AZStd::string path("");
    if ((gEnv) && (gEnv->pFileIO))
    {
        const char* logAlias = gEnv->pFileIO->GetAlias("@log@");
        if (!logAlias)
        {
            logAlias = gEnv->pFileIO->GetAlias("@products@");
        }
        if (logAlias)
        {
            path = logAlias;
            path += "\\";
        }
    }

    AZStd::string fileName = path;
    fileName += "error.log";

    struct stat fileInfo;
    AZStd::string timeStamp;
    AZStd::string backupPath;
    if (gEnv->IsDedicated())
    {
        backupPath = PathUtil::ToUnixPath(PathUtil::AddSlash(path + "DumpBackups"));
        gEnv->pFileIO->CreatePath(backupPath.c_str());

        if (stat(fileName.c_str(), &fileInfo) == 0)
        {
            // Backup log
            tm creationTime;
            localtime_s(&creationTime, &fileInfo.st_mtime);
            char tempBuffer[32];
            strftime(tempBuffer, sizeof(tempBuffer), "%d %b %Y (%H %M %S)", &creationTime);
            timeStamp = tempBuffer;

            AZStd::string backupFileName = backupPath + timeStamp + " error.log";
            AZStd::wstring fileNameW;
            AZStd::to_wstring(fileNameW, fileName.c_str());
            AZStd::wstring backupFileNameW;
            AZStd::to_wstring(backupFileNameW, backupFileName.c_str());
            CopyFileW(fileNameW.c_str(), backupFileNameW.c_str(), true);
        }
    }

    FILE* f = nullptr;
    azfopen(&f, fileName.c_str(), "wt");

    static char errorString[s_iCallStackSize];
    errorString[0] = 0;

    // Time and Version.
    char versionbuf[1024];
    azstrcpy(versionbuf, AZ_ARRAY_SIZE(versionbuf), "");
    PutVersion(versionbuf, AZ_ARRAY_SIZE(versionbuf));
    azstrcat(errorString, AZ_ARRAY_SIZE(errorString), versionbuf);
    azstrcat(errorString, AZ_ARRAY_SIZE(errorString), "\n");

    char excCode[MAX_WARNING_LENGTH];
    char excAddr[80];
    char desc[1024];
    char excDesc[MAX_WARNING_LENGTH];

    // make sure the mouse cursor is visible
    ShowCursor(TRUE);

    const char* excName;
    if (m_bIsFatalError || !pex)
    {
        const char* const szMessage = m_bIsFatalError ? s_szFatalErrorCode : m_szBugMessage;
        excName = szMessage;
        azstrcpy(excCode, AZ_ARRAY_SIZE(excCode), szMessage);
        azstrcpy(excAddr, AZ_ARRAY_SIZE(excAddr), "");
        azstrcpy(desc, AZ_ARRAY_SIZE(desc), "");
        azstrcpy(m_excModule, AZ_ARRAY_SIZE(m_excModule), "");
        azstrcpy(excDesc, AZ_ARRAY_SIZE(excDesc), szMessage);
    }
    else
    {
        sprintf_s(excAddr, "0x%04X:0x%p", pex->ContextRecord->SegCs, pex->ExceptionRecord->ExceptionAddress);
        sprintf_s(excCode, "0x%08lX", pex->ExceptionRecord->ExceptionCode);
        excName = TranslateExceptionCode(pex->ExceptionRecord->ExceptionCode);
        azstrcpy(desc, AZ_ARRAY_SIZE(desc), "");
        sprintf_s(excDesc, "%s\r\n%s", excName, desc);

        if (pex->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            if (pex->ExceptionRecord->NumberParameters > 1)
            {
                ULONG_PTR iswrite = pex->ExceptionRecord->ExceptionInformation[0];
                DWORD64 accessAddr = pex->ExceptionRecord->ExceptionInformation[1];
                if (iswrite)
                {
                    sprintf_s(desc, "Attempt to write data to address 0x%08llu\r\nThe memory could not be \"written\"", accessAddr);
                }
                else
                {
                    sprintf_s(desc, "Attempt to read from address 0x%08llu\r\nThe memory could not be \"read\"", accessAddr);
                }
            }
        }
    }

    WriteLineToLog("Exception Code: %s", excCode);
    WriteLineToLog("Exception Addr: %s", excAddr);
    WriteLineToLog("Exception Module: %s", m_excModule);
    WriteLineToLog("Exception Name  : %s", excName);
    WriteLineToLog("Exception Description: %s", desc);

    azstrcpy(m_excDesc, AZ_ARRAY_SIZE(m_excDesc), excDesc);
    azstrcpy(m_excAddr, AZ_ARRAY_SIZE(m_excAddr), excAddr);
    azstrcpy(m_excCode, AZ_ARRAY_SIZE(m_excCode), excCode);

    char errs[32768];
    sprintf_s(
        errs,
        "Exception Code: %s\nException Addr: %s\nException Module: %s\nException Description: %s, %s\n",
        excCode,
        excAddr,
        m_excModule,
        excName,
        desc);

    azstrcat(errs, AZ_ARRAY_SIZE(errs), "\nCall Stack Trace:\n");

    AZStd::vector<AZStd::string> funcs;
    {
        AZ::Debug::StackFrame frames[25];
        AZ::Debug::SymbolStorage::StackLine lines[AZ_ARRAY_SIZE(frames)];
        unsigned int numFrames = AZ::Debug::StackRecorder::Record(frames, AZ_ARRAY_SIZE(frames), 3);
        if (numFrames)
        {
            AZ::Debug::SymbolStorage::DecodeFrames(frames, numFrames, lines);
            for (unsigned int i = 0; i < numFrames; i++)
            {
                funcs.push_back(lines[i]);
            }
        }
        dumpCallStack(funcs);
        // Fill call stack.
        char str[s_iCallStackSize];
        azstrcpy(str, AZ_ARRAY_SIZE(str), "");
        for (unsigned int i = 0; i < funcs.size(); i++)
        {
            char temp[s_iCallStackSize];
            sprintf_s(temp, "%2zd) %s", funcs.size() - i, (const char*)funcs[i].c_str());
            azstrcat(str, AZ_ARRAY_SIZE(str), temp);
            azstrcat(str, AZ_ARRAY_SIZE(str), "\r\n");
            azstrcat(errs, AZ_ARRAY_SIZE(errs), temp);
            azstrcat(errs, AZ_ARRAY_SIZE(errs), "\n");
        }
        azstrcpy(m_excCallstack, AZ_ARRAY_SIZE(m_excCallstack), str);
    }

    azstrcat(errorString, AZ_ARRAY_SIZE(errorString), errs);

    if (f)
    {
        fwrite(errorString, strlen(errorString), 1, f);
        {
            if (g_cvars.sys_dump_aux_threads)
            {
                for (int i = 0; i < g_nDebugThreads; i++)
                {
                    if (g_idDebugThreads[i] != GetCurrentThreadId())
                    {
                        fprintf(f, "\n\nSuspended thread (%s):\n", g_nameDebugThreads[i]);
                        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, TRUE, g_idDebugThreads[i]);

                        // mirrors the AZ::Debug::Trace::PrintCallstack() functionality, but prints to a file
                        {
                            AZ::Debug::StackFrame frames[10];

                            // Without StackFrame explicit alignment frames array is aligned to 4 bytes
                            // which causes the stack tracing to fail.
                            AZ::Debug::SymbolStorage::StackLine lines[AZ_ARRAY_SIZE(frames)];

                            unsigned int numFrames = AZ::Debug::StackRecorder::Record(frames, AZ_ARRAY_SIZE(frames), 0, hThread);
                            if (numFrames)
                            {
                                AZ::Debug::SymbolStorage::DecodeFrames(frames, numFrames, lines);
                                for (unsigned int i2 = 0; i2 < numFrames; ++i2)
                                {
                                    fprintf(f, "%2d) %s\n", numFrames - i2, lines[i2]);
                                }
                            }
                        }

                        ResumeThread(hThread);
                    }
                }
            }
        }
        fflush(f);
        fclose(f);
    }

    if (pex)
    {
        MINIDUMP_TYPE mdumpValue = MiniDumpNormal;
        bool bDump = true;
        switch (g_cvars.sys_dump_type)
        {
        case 0:
            bDump = false;
            break;
        case 1:
            mdumpValue = MiniDumpNormal;
            break;
        case 2:
            mdumpValue = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithDataSegs);
            break;
        case 3:
            mdumpValue = MiniDumpWithFullMemory;
            break;
        default:
            mdumpValue = (MINIDUMP_TYPE)g_cvars.sys_dump_type;
            break;
        }
        if (bDump)
        {
            fileName = path + "error.dmp";

            if (gEnv->IsDedicated() && stat(fileName.c_str(), &fileInfo) == 0)
            {
                // Backup dump (use timestamp from error.log if available)
                if (timeStamp.empty())
                {
                    tm creationTime;
                    localtime_s(&creationTime, &fileInfo.st_mtime);
                    char tempBuffer[32];
                    strftime(tempBuffer, sizeof(tempBuffer), "%d %b %Y (%H %M %S)", &creationTime);
                    timeStamp = tempBuffer;
                }

                AZStd::string backupFileName = backupPath + timeStamp + " error.dmp";
                AZStd::wstring fileNameW;
                AZStd::to_wstring(fileNameW, fileName.c_str());
                AZStd::wstring backupFileNameW;
                AZStd::to_wstring(backupFileNameW, backupFileName.c_str());
                CopyFileW(fileNameW.c_str(), backupFileNameW.c_str(), true);
            }

            CryEngineExceptionFilterMiniDump(pex, fileName.c_str(), mdumpValue);
        }
    }

    // if no crash dialog don't even submit the bug
    if (m_postBackupProcess && g_cvars.sys_no_crash_dialog == 0 && g_bUserDialog)
    {
        m_postBackupProcess();
    }
    else if (auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get(); nativeUI != nullptr)
    {
        AZStd::string msg = AZStd::string::format(
            "O3DE has encountered an unexpected error.\n\nDo you want to manually report the issue on Github ?\nInformation about the "
            "crash are located in %s via error.log and error.dmp",
            path.c_str());
        constexpr bool showCancel = false;
        AZStd::string res = nativeUI->DisplayYesNoDialog("O3DE unexpected error", msg, showCancel);
        if (res == "Yes")
        {
            AZStd::wstring arg(path.begin(), path.end());
            ShellExecuteW(nullptr, L"open", arg.c_str(), NULL, NULL, SW_SHOWNORMAL);

            AZStd::string reportIssueUrl;
            if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                settingsRegistry->Get(reportIssueUrl, SettingKey_IssueReportLink);

            if (reportIssueUrl.empty())
                reportIssueUrl = IssueReportLinkFallback;

            arg = AZStd::wstring(reportIssueUrl.begin(), reportIssueUrl.end());
            ShellExecuteW(nullptr, L"open", arg.c_str(), NULL, NULL, SW_SHOWNORMAL);
        }
    }

    const bool bQuitting = !gEnv || !gEnv->pSystem || gEnv->pSystem->IsQuitting();

    if (g_cvars.sys_no_crash_dialog == 0 && g_bUserDialog && gEnv->IsEditor() && !bQuitting && pex)
    {
        BackupCurrentLevel();

        if (auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get(); nativeUI != nullptr)
        {
            constexpr bool showCancel = false;
            AZStd::string res = nativeUI->DisplayYesNoDialog(
                "Save your changes?",
                "Do you want to try to save your changes?\nAs O3DE is in a panic state, it might corrupt your data",
                showCancel);
            if (res == "Yes")
            {
                if (SaveCurrentLevel())
                    nativeUI->DisplayOkDialog("Save", "Level has been successfully saved!\r\nPress Ok to proceed.", showCancel);
                else
                    nativeUI->DisplayOkDialog("Save", "Error saving level.\r\nPress Ok to proceed.", showCancel);
            }
        }
    }

    if (g_cvars.sys_no_crash_dialog != 0 || !g_bUserDialog)
    {
        // terminate immediately - since we're in a crash, there is no point unwinding stack, we've already done access violation or worse.
        // calling exit will only cause further death down the line...
        TerminateProcess(GetCurrentProcess(), pex->ExceptionRecord->ExceptionCode);
    }
}

bool DebugCallStack::BackupCurrentLevel()
{
    CSystem* pSystem = static_cast<CSystem*>(m_pSystem);
    if (pSystem && pSystem->GetUserCallback())
    {
        return pSystem->GetUserCallback()->OnBackupDocument();
    }

    return false;
}

bool DebugCallStack::SaveCurrentLevel()
{
    CSystem* pSystem = static_cast<CSystem*>(m_pSystem);
    if (pSystem && pSystem->GetUserCallback())
    {
        return pSystem->GetUserCallback()->OnSaveDocument();
    }

    return false;
}

DebugCallStack::UserPostExceptionChoice DebugCallStack::SubmitBugAndAskToRecoverOrCrash(EXCEPTION_POINTERS* exception_pointer)
{
    UserPostExceptionChoice ret = UserPostExceptionChoice::Exit;

    assert(!hwndException);

    RemoveOldFiles();

    AZ::Debug::Trace::Instance().PrintCallstack("", 2);

    SaveExceptionInfoAndShowUserReportDialogs(exception_pointer);

    if (IsFloatingPointException(exception_pointer))
    {
        ret = AskUserToRecoverOrCrash(exception_pointer);
    }

    return ret;
}

void DebugCallStack::ResetFPU(EXCEPTION_POINTERS* pex)
{
    if (IsFloatingPointException(pex))
    {
        // How to reset FPU: http://www.experts-exchange.com/Programming/System/Windows__Programming/Q_10310953.html
        _clearfp();
#ifndef WIN64
        pex->ContextRecord->FloatSave.ControlWord |= 0x2F;
        pex->ContextRecord->FloatSave.StatusWord &= ~0x8080;
#endif
    }
}

AZStd::string DebugCallStack::GetModuleNameForAddr(void* addr)
{
    if (m_modules.empty())
    {
        return "[unknown]";
    }

    if (addr < m_modules.begin()->first)
    {
        return "[unknown]";
    }

    TModules::const_iterator it = m_modules.begin();
    TModules::const_iterator end = m_modules.end();
    for (; ++it != end;)
    {
        if (addr < it->first)
        {
            return (--it)->second;
        }
    }

    // if address is higher than the last module, we simply assume it is in the last module.
    return m_modules.rbegin()->second;
}

void DebugCallStack::GetProcNameForAddr(void* addr, AZStd::string& procName, void*& baseAddr, AZStd::string& filename, int& line)
{
    AZ::Debug::SymbolStorage::StackLine func, file, module;
    AZ::Debug::SymbolStorage::FindFunctionFromIP(addr, &func, &file, &module, line, baseAddr);
    procName = func;
    filename = file;
}

AZStd::string DebugCallStack::GetCurrentFilename()
{
    char fullpath[MAX_PATH_LENGTH + 1];
    AZ::Utils::GetExecutablePath(fullpath, MAX_PATH_LENGTH);
    return fullpath;
}

static bool IsFloatingPointException(EXCEPTION_POINTERS* pex)
{
    if (!pex)
    {
        return false;
    }

    DWORD exceptionCode = pex->ExceptionRecord->ExceptionCode;
    switch (exceptionCode)
    {
    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_UNDERFLOW:
    case STATUS_FLOAT_MULTIPLE_FAULTS:
    case STATUS_FLOAT_MULTIPLE_TRAPS:
        return true;

    default:
        return false;
    }
}

DebugCallStack::UserPostExceptionChoice DebugCallStack::AskUserToRecoverOrCrash(EXCEPTION_POINTERS* exception_pointer)
{
    if (exception_pointer->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
    {
        return UserPostExceptionChoice::Exit;
    }

    UserPostExceptionChoice res = UserPostExceptionChoice::Exit;
    if (auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get(); nativeUI != nullptr)
    {
        DebugCallStack* pDCS = static_cast<DebugCallStack*>(DebugCallStack::instance());
        AZStd::string msg = AZStd::string::format(
            "O3DE encountered an error but can recover from it.\nDo you want to try to recover ?\n\nException Code: %s\nException Addr: "
            "%s\nException Module: %s\nException Description: %s\nCallstack:\n%.600s",
            pDCS->m_excCode,
            pDCS->m_excAddr,
            pDCS->m_excModule,
            pDCS->m_excDesc,
            pDCS->m_excCallstack);

        constexpr bool showCancel = false;
        AZStd::string dialogRes = nativeUI->DisplayYesNoDialog("Try to recover?", msg, showCancel);
        if (dialogRes == "Yes")
        {
            res = UserPostExceptionChoice::Recover;
        }
    }
    return res;
}

#else
void MarkThisThreadForDebugging(const char*)
{
}
void UnmarkThisThreadFromDebugging()
{
}
void UpdateFPExceptionsMaskForThreads()
{
}
#endif // WIN32
