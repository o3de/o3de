/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "DebugCallStack.h"

#if defined(WIN32) || defined(WIN64)

#include <IConsole.h>
#include <CryPath.h>
#include "System.h"

#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Debug/EventTraceDrillerBus.h>
#include <AzCore/std/parallel/spin_mutex.h>
#include <AzCore/Utils/Utils.h>

#define VS_VERSION_INFO                 1
#define IDD_CRITICAL_ERROR              101
#define IDB_CONFIRM_SAVE                102
#define IDB_DONT_SAVE                   103
#define IDD_CONFIRM_SAVE_LEVEL          127
#define IDB_CRASH_FACE                  128
#define IDD_EXCEPTION                   245
#define IDC_CALLSTACK                   1001
#define IDC_EXCEPTION_CODE              1002
#define IDC_EXCEPTION_ADDRESS           1003
#define IDC_EXCEPTION_MODULE            1004
#define IDC_EXCEPTION_DESC              1005
#define IDB_EXIT                        1008
#define IDB_IGNORE                      1010
__pragma(comment(lib, "version.lib"))

//! Needs one external of DLL handle.
extern HMODULE gDLLHandle;

#include <DbgHelp.h>

#define MAX_PATH_LENGTH 1024
#define MAX_SYMBOL_LENGTH 512

static HWND hwndException = 0;
static bool g_bUserDialog = true;         // true=on crash show dialog box, false=supress user interaction

static int  PrintException(EXCEPTION_POINTERS* pex);

static bool IsFloatingPointException(EXCEPTION_POINTERS* pex);

extern LONG WINAPI CryEngineExceptionFilterWER(struct _EXCEPTION_POINTERS* pExceptionPointers);
extern LONG WINAPI CryEngineExceptionFilterMiniDump(struct _EXCEPTION_POINTERS* pExceptionPointers, const char* szDumpPath, MINIDUMP_TYPE mdumpValue);

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


BOOL CALLBACK EnumModules(
    PCSTR   ModuleName,
    DWORD64 BaseOfDll,
    PVOID   UserContext)
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
}

//////////////////////////////////////////////////////////////////////////
void DebugCallStack::SetUserDialogEnable(const bool bUserDialogEnable)
{
    g_bUserDialog = bUserDialogEnable;
}


DWORD g_idDebugThreads[10];
const char* g_nameDebugThreads[10];
int g_nDebugThreads = 0;
AZStd::spin_mutex g_lockThreadDumpList;

void MarkThisThreadForDebugging(const char* name)
{
    EBUS_EVENT(AZ::Debug::EventTraceDrillerSetupBus, SetThreadName, AZStd::this_thread::get_id(), name);

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

extern int prev_sys_float_exceptions;
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
            (ctx.FltSave.MxCsr |= 0x280) &= ~0x280  | mask;
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
    AZ::Debug::Trace::PrintCallstack("Exit");

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

    const int ret = SubmitBug(exception_pointer);

    if (ret != IDB_IGNORE)
    {
        CryEngineExceptionFilterWER(exception_pointer);
    }

    gEnv->pLog->FlushAndClose();

    if (exception_pointer->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
    {
        // This is non continuable exception. abort application now.
        exit(exception_pointer->ExceptionRecord->ExceptionCode);
    }

    //typedef long (__stdcall *ExceptionFunc)(EXCEPTION_POINTERS*);
    //ExceptionFunc prevFunc = (ExceptionFunc)prevExceptionHandler;
    //return prevFunc( (EXCEPTION_POINTERS*)exception_pointer );
    if (ret == IDB_EXIT)
    {
        // Immediate exit.
        // on windows, exit() and _exit() do all sorts of things, unfortuantely
        // TerminateProcess is the only way to die.
        TerminateProcess(GetCurrentProcess(), exception_pointer->ExceptionRecord->ExceptionCode);  // we crashed, so don't return a zero exit code!
        // on linux based systems, _exit will not call ATEXIT and other things, which makes it more suitable for termination in an emergency such
        // as an unhandled exception.
        // however, this function is a windows exception handler.
    }
    else if (ret == IDB_IGNORE)
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
    SubmitBug(NULL);
    m_szBugMessage = NULL;
}

void DebugCallStack::dumpCallStack(std::vector<AZStd::string>& funcs)
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
void DebugCallStack::LogExceptionInfo(EXCEPTION_POINTERS* pex)
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
            path += "/";
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
    sprintf_s(errs, "Exception Code: %s\nException Addr: %s\nException Module: %s\nException Description: %s, %s\n",
        excCode, excAddr, m_excModule, excName, desc);


    azstrcat(errs, AZ_ARRAY_SIZE(errs), "\nCall Stack Trace:\n");

    std::vector<AZStd::string> funcs;
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

    //if no crash dialog don't even submit the bug
    if (m_postBackupProcess && g_cvars.sys_no_crash_dialog == 0 && g_bUserDialog)
    {
        m_postBackupProcess();
    }
    else
    {
        // lawsonn: Disabling the JIRA-based crash reporter for now
        // we'll need to deal with it our own way, pending QA.
        // if you're customizing the engine this is also your opportunity to deal with it.
        if (g_cvars.sys_no_crash_dialog != 0 || !g_bUserDialog)
        {
            // ------------ place custom crash handler here ---------------------
            // it should launch an executable!
            /// by  this time, error.bmp will be in the engine root folder
            // error.log and error.dmp will also be present in the engine root folder
            // if your error dumper wants those, it should zip them up and send them or offer to do so.
            // ------------------------------------------------------------------
        }
    }
    const bool bQuitting = !gEnv || !gEnv->pSystem || gEnv->pSystem->IsQuitting();

    //[AlexMcC|16.04.10] When the engine is shutting down, MessageBox doesn't display a box
    // and immediately returns IDYES. Avoid this by just not trying to save if we're quitting.
    // Don't ask to save if this isn't a real crash (a real crash has exception pointers)
    if (g_cvars.sys_no_crash_dialog == 0 && g_bUserDialog && gEnv->IsEditor() && !bQuitting && pex)
    {
        BackupCurrentLevel();

        const INT_PTR res = DialogBoxParam(gDLLHandle, MAKEINTRESOURCE(IDD_CONFIRM_SAVE_LEVEL), NULL, DebugCallStack::ConfirmSaveDialogProc, NULL);
        if (res == IDB_CONFIRM_SAVE)
        {
            if (SaveCurrentLevel())
            {
                MessageBoxW(NULL, L"Level has been successfully saved!\r\nPress Ok to terminate Editor.", L"Save", MB_OK);
            }
            else
            {
                MessageBoxW(NULL, L"Error saving level.\r\nPress Ok to terminate Editor.", L"Save", MB_OK | MB_ICONWARNING);
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


INT_PTR CALLBACK DebugCallStack::ExceptionDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static EXCEPTION_POINTERS* pex;

    static char errorString[32768] = "";

    switch (message)
    {
    case WM_INITDIALOG:
    {
        pex = (EXCEPTION_POINTERS*)lParam;
        HWND h;

        if (pex->ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
        {
            // Disable continue button for non continuable exceptions.
            //h = GetDlgItem( hwndDlg,IDB_CONTINUE );
            //if (h) EnableWindow( h,FALSE );
        }

        DebugCallStack* pDCS = static_cast<DebugCallStack*>(DebugCallStack::instance());

        h = GetDlgItem(hwndDlg, IDC_EXCEPTION_DESC);
        if (h)
        {
            SendMessage(h, EM_REPLACESEL, FALSE, (LONG_PTR)pDCS->m_excDesc);
        }

        h = GetDlgItem(hwndDlg, IDC_EXCEPTION_CODE);
        if (h)
        {
            SendMessage(h, EM_REPLACESEL, FALSE, (LONG_PTR)pDCS->m_excCode);
        }

        h = GetDlgItem(hwndDlg, IDC_EXCEPTION_MODULE);
        if (h)
        {
            SendMessage(h, EM_REPLACESEL, FALSE, (LONG_PTR)pDCS->m_excModule);
        }

        h = GetDlgItem(hwndDlg, IDC_EXCEPTION_ADDRESS);
        if (h)
        {
            SendMessage(h, EM_REPLACESEL, FALSE, (LONG_PTR)pDCS->m_excAddr);
        }

        // Fill call stack.
        HWND callStack = GetDlgItem(hwndDlg, IDC_CALLSTACK);
        if (callStack)
        {
            SendMessage(callStack, WM_SETTEXT, FALSE, (LPARAM)pDCS->m_excCallstack);
        }

        if (hwndException)
        {
            DestroyWindow(hwndException);
            hwndException = 0;
        }

        if (IsFloatingPointException(pex))
        {
            EnableWindow(GetDlgItem(hwndDlg, IDB_IGNORE), TRUE);
        }
    }
    break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDB_EXIT:
        case IDB_IGNORE:
            // Fall through.

            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK DebugCallStack::ConfirmSaveDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, [[maybe_unused]] LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        // The user might be holding down the spacebar while the engine crashes.
        // If we don't remove keyboard focus from this dialog, the keypress will
        // press the default button before the dialog actually appears, even if
        // the user has already released the key, which is bad.
        SetFocus(NULL);
    } break;
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDB_CONFIRM_SAVE:         // Fall through
        case IDB_DONT_SAVE:
        {
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
        }
    } break;
    }

    return FALSE;
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

int DebugCallStack::SubmitBug(EXCEPTION_POINTERS* exception_pointer)
{
    int ret = IDB_EXIT;

    assert(!hwndException);

    RemoveOldFiles();

    AZ::Debug::Trace::PrintCallstack("", 2);

    LogExceptionInfo(exception_pointer);

    if (IsFloatingPointException(exception_pointer))
    {
        //! Print exception dialog.
        ret = PrintException(exception_pointer);
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
    for (; ++it != end; )
    {
        if (addr < it->first)
        {
            return (--it)->second;
        }
    }

    //if address is higher than the last module, we simply assume it is in the last module.
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

int DebugCallStack::PrintException(EXCEPTION_POINTERS* exception_pointer)
{
    return (int)DialogBoxParam(gDLLHandle, MAKEINTRESOURCE(IDD_CRITICAL_ERROR), NULL, DebugCallStack::ExceptionDialogProc, (LPARAM)exception_pointer);
}

#else
void MarkThisThreadForDebugging(const char*) {}
void UnmarkThisThreadFromDebugging() {}
void UpdateFPExceptionsMaskForThreads() {}
#endif //WIN32
