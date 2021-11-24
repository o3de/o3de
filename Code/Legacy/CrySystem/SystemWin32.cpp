/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "System.h"
#include <time.h>

#include <IRenderer.h>
#include <IMovieSystem.h>
#include <ILog.h>
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/std/allocator_stack.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SYSTEMWIN32_CPP_SECTION_1 1
#define SYSTEMWIN32_CPP_SECTION_2 2
#define SYSTEMWIN32_CPP_SECTION_3 3
#endif

#if defined(LINUX) || defined(APPLE)
#include <unistd.h>
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include <float.h>
#include <shellapi.h> // Needed for ShellExecute.
#include <Psapi.h>
#include <Aclapi.h>

#include <shlobj.h>
#endif

#include "IDebugCallStack.h"

#if defined(APPLE) || defined(LINUX)
#include <pwd.h>
#endif

#include "XConsole.h"
#include "LocalizedStringManager.h"
#include "XML/XmlUtils.h"

#if defined(WIN32)
__pragma(comment(lib, "wininet.lib"))
__pragma(comment(lib, "Winmm.lib"))
#endif

#if defined(APPLE)
#include <AzFramework/Utils/SystemUtilsApple.h>
#endif

// this is the list of modules that can be loaded into the game process
// Each array element contains 2 strings: the name of the module (case-insensitive)
// and the name of the group the module belongs to
//////////////////////////////////////////////////////////////////////////
const char g_szGroupCore[] = "CryEngine";
const char* g_szModuleGroups[][2] = {
    {"Editor.exe", g_szGroupCore},
    {"CrySystem.dll", g_szGroupCore}
};

#if defined(WIN32)
#pragma pack(push,1)
struct PEHeader_DLL
{
    DWORD signature;
    IMAGE_FILE_HEADER _head;
    IMAGE_OPTIONAL_HEADER opt_head;
    IMAGE_SECTION_HEADER* section_header;  // actual number in NumberOfSections
};
#pragma pack(pop)
#endif

//////////////////////////////////////////////////////////////////////////
const char* CSystem::GetUserName()
{
#if defined(WIN32) || defined(WIN64)
    static const int iNameBufferSize = 1024;
    static char szNameBuffer[iNameBufferSize];
    memset(szNameBuffer, 0, iNameBufferSize);

    DWORD dwSize = iNameBufferSize;
    wchar_t nameW[iNameBufferSize];
    ::GetUserNameW(nameW, &dwSize);
    AZStd::to_string(szNameBuffer, iNameBufferSize, { nameW, dwSize });
    return szNameBuffer;
#else
#if defined(LINUX)
    static uid_t uid = geteuid ();
    static struct passwd* pw = getpwuid (uid);
    if (pw)
    {
        return  (pw->pw_name);
    }
    else
    {
        return NULL;
    }
#elif defined(APPLE)
    static const int iNameBufferSize = 1024;
    static char szNameBuffer[iNameBufferSize];
    if(SystemUtilsApple::GetUserName(szNameBuffer, iNameBufferSize))
    {
        return szNameBuffer;
    }
    else
    {
        return "";
    }
#else
    return "";
#endif
#endif
}

//////////////////////////////////////////////////////////////////////////
int CSystem::GetApplicationInstance()
{
#ifdef WIN32
    // tools that declare themselves as in "tool mode" may not access @user@ and may also not lock it
    if (gEnv->IsInToolMode())
    {
        return 0;
    }

    // this code below essentially "locks" an instance of the USER folder to a specific running application
    if (m_iApplicationInstance == -1)
    {
        AZStd::wstring suffix;
        for (int instance = 0;; ++instance)
        {
            suffix = AZStd::wstring::format(L"O3DEApplication(%d)", instance);

            CreateMutexW(NULL, TRUE, suffix.c_str());
            // search for duplicates
            if (GetLastError() != ERROR_ALREADY_EXISTS)
            {
                m_iApplicationInstance = instance;
                break;
            }
        }
    }

    return m_iApplicationInstance;
#else
    return 0;
#endif
}

int CSystem::GetApplicationLogInstance([[maybe_unused]] const char* logFilePath)
{
#if AZ_TRAIT_OS_USE_WINDOWS_MUTEX
    AZStd::wstring suffix;
    int instance = 0;
    for (;; ++instance)
    {
        suffix = AZStd::wstring::format(L"%s(%d)", logFilePath, instance);

        CreateMutexW(NULL, TRUE, suffix.c_str());
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            break;
        }
    }
    return instance;
#else
    return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
struct CryDbgModule
{
    HANDLE heap;
    WIN_HMODULE handle;
    AZStd::string name;
    DWORD dwSize;
};

#ifdef WIN32

//////////////////////////////////////////////////////////////////////////
const char* GetModuleGroup (const char* szString)
{
    for (unsigned i = 0; i < sizeof(g_szModuleGroups) / sizeof(g_szModuleGroups[0]); ++i)
    {
        if (azstricmp(szString, g_szModuleGroups[i][0]) == 0)
        {
            return g_szModuleGroups[i][1];
        }
    }
    return "Other";
}

#endif

// Make system error message string
//////////////////////////////////////////////////////////////////////////
//! \return pointer to the null terminated error string or 0
static const char* GetLastSystemErrorMessage()
{
#ifdef WIN32
    DWORD dwError = GetLastError();

    static char szBuffer[512]; // function will return pointer to this buffer

    if (dwError)
    {
        LPVOID lpMsgBuf = 0;

        if (FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL))
        {
            azstrcpy(szBuffer, AZ_ARRAY_SIZE(szBuffer), (char*)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
        else
        {
            return 0;
        }

        return szBuffer;
    }
#endif //WIN32
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::FatalError(const char* format, ...)
{
    // Guard against reentrancy - out of memory fatal errors can become reentrant since logging can try to alloc.
    static bool currentlyReportingError = false;
    if (currentlyReportingError == true)
    {
        return;
    }
    currentlyReportingError = true;

    // format message
    va_list ArgList;
    char szBuffer[MAX_WARNING_LENGTH];
    const char* sPrefix = "";
    azstrcpy(szBuffer, MAX_WARNING_LENGTH, sPrefix);
    va_start(ArgList, format);
    azvsnprintf(szBuffer + strlen(sPrefix), MAX_WARNING_LENGTH - strlen(sPrefix), format, ArgList);
    va_end(ArgList);

    // get system error message before any attempt to write into log
    const char* szSysErrorMessage = GetLastSystemErrorMessage();

    CryLogAlways("=============================================================================");
    CryLogAlways("*ERROR");
    CryLogAlways("=============================================================================");
    // write both messages into log
    CryLogAlways("%s", szBuffer);

    if (szSysErrorMessage)
    {
        CryLogAlways("Last System Error: %s", szSysErrorMessage);
    }

    if (GetUserCallback())
    {
        GetUserCallback()->OnError(szBuffer);
    }

    assert(szBuffer[0] >= ' ');
    //  strcpy(szBuffer,szBuffer+1);    // remove verbosity tag since it is not supported by ::MessageBox

    AZ::Debug::Platform::OutputToDebugger("CrySystem", szBuffer);

#ifdef WIN32
    OnFatalError(szBuffer);
    if (!g_cvars.sys_no_crash_dialog)
    {
        AZStd::wstring szBufferW;
        AZStd::to_wstring(szBufferW, szBuffer);
        ::MessageBoxW(NULL, szBufferW.c_str(), L"Open 3D Engine Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    }

    // Dump callstack.
    IDebugCallStack::instance()->FatalError(szBuffer);
#endif

    // app can not continue
    AZ::Debug::Trace::Break();

#ifdef _DEBUG
    #if defined(WIN32) || defined(WIN64)
        _flushall();
        // on windows, _exit does all sorts of things which can cause cleanup to fail during a crash, we need to terminate instead.
        TerminateProcess(GetCurrentProcess(), 1);
    #endif

    #if defined(AZ_RESTRICTED_PLATFORM)
        #define AZ_RESTRICTED_SECTION SYSTEMWIN32_CPP_SECTION_2
        #include AZ_RESTRICTED_FILE(SystemWin32_cpp)
    #endif
    #if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
        #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #else
        _exit(1);
    #endif
#endif
}

void CSystem::ReportBug([[maybe_unused]] const char* format, ...)
{
#if defined (WIN32)
    va_list ArgList;
    char szBuffer[MAX_WARNING_LENGTH];
    const char* sPrefix = "";
    azstrcpy(szBuffer, MAX_WARNING_LENGTH, sPrefix);
    va_start(ArgList, format);
    azvsnprintf(szBuffer + strlen(sPrefix), MAX_WARNING_LENGTH - strlen(sPrefix), format, ArgList);
    va_end(ArgList);

    IDebugCallStack::instance()->ReportBug(szBuffer);
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::debug_GetCallStack(const char** pFunctions, int& nCount)
{
#if defined(WIN32)
    using namespace AZ::Debug;

    int nMaxCount = nCount;
    StackFrame* frames = (StackFrame*)AZ_ALLOCA(sizeof(StackFrame)*nMaxCount);
    unsigned int numFrames = StackRecorder::Record(frames, nMaxCount, 1);
    SymbolStorage::StackLine* textLines = (SymbolStorage::StackLine*)AZ_ALLOCA(sizeof(SymbolStorage::StackLine)*nMaxCount);
    SymbolStorage::DecodeFrames(frames, numFrames, textLines);
    for (unsigned int i = 0; i < numFrames; i++)
    {
        pFunctions[i] = textLines[i];
    }
    nCount = numFrames;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMWIN32_CPP_SECTION_3
#include AZ_RESTRICTED_FILE(SystemWin32_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    AZ_UNUSED(pFunctions);
    nCount = 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
void CSystem::debug_LogCallStack(int nMaxFuncs, [[maybe_unused]] int nFlags)
{
    if (nMaxFuncs > 32)
    {
        nMaxFuncs = 32;
    }
    // Print call stack for each find.
    const char* funcs[32];
    int nCount = nMaxFuncs;
    GetISystem()->debug_GetCallStack(funcs, nCount);
    for (int i = 1; i < nCount; i++) // start from 1 to skip this function.
    {
        CryLogAlways("    %02d) %s", i, funcs[i]);
    }
}

#if (defined(WIN32) || defined(WIN64))
//////////////////////////////////////////////////////////////////////////
bool CSystem::GetWinGameFolder(char* szMyDocumentsPath, int maxPathSize)
{
    bool bSucceeded  = false;
    // check Vista and later OS first

    HMODULE shell32 = LoadLibraryW(L"Shell32.dll");
    if (shell32)
    {
        typedef long (__stdcall * T_SHGetKnownFolderPath)(REFKNOWNFOLDERID rfid, unsigned long dwFlags, void* hToken, wchar_t** ppszPath);
        T_SHGetKnownFolderPath SHGetKnownFolderPath = (T_SHGetKnownFolderPath)GetProcAddress(shell32, "SHGetKnownFolderPath");
        if (SHGetKnownFolderPath)
        {
            // We must be running Vista or newer
            wchar_t* wMyDocumentsPath;
            HRESULT hr = SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_CREATE | KF_FLAG_DONT_UNEXPAND, NULL, &wMyDocumentsPath);
            bSucceeded = SUCCEEDED(hr);
            if (bSucceeded)
            {
                // Convert from UNICODE to UTF-8
                AZStd::to_string(szMyDocumentsPath, maxPathSize, wMyDocumentsPath);
                CoTaskMemFree(wMyDocumentsPath);
            }
        }
        FreeLibrary(shell32);
    }

    if (!bSucceeded)
    {
        // check pre-vista OS if not succeeded before
        wchar_t wMyDocumentsPath[AZ_MAX_PATH_LEN];
        bSucceeded = SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, wMyDocumentsPath));
        if (bSucceeded)
        {
            AZStd::to_string(szMyDocumentsPath, maxPathSize, wMyDocumentsPath);
        }
    }

    return bSucceeded;
}
#endif

//////////////////////////////////////////////////////////////////////////
void CSystem::DetectGameFolderAccessRights()
{
    // This code is trying to figure out if the current folder we are now running under have write access.
    // By default assume folder is not writable.
    // If folder is writable game.log is saved there, otherwise it is saved in user documents folder.

#if defined(WIN32)

    DWORD DesiredAccess = FILE_GENERIC_WRITE;
    DWORD GrantedAccess = 0;
    DWORD dwRes = 0;
    PACL pDACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    HANDLE hClientToken = 0;
    PRIVILEGE_SET PrivilegeSet;
    DWORD PrivilegeSetLength = sizeof(PrivilegeSet);
    BOOL bAccessStatus = FALSE;

    // Get a pointer to the existing DACL.
    dwRes = GetNamedSecurityInfoW(L".", SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
            NULL, NULL, &pDACL, NULL, &pSD);

    if (ERROR_SUCCESS != dwRes)
    {
        //
        assert(0);
    }

    if (!ImpersonateSelf(SecurityIdentification))
    {
        return;
    }
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hClientToken) && hClientToken != 0)
    {
        return;
    }

    GENERIC_MAPPING GenMap;
    GenMap.GenericRead = FILE_GENERIC_READ;
    GenMap.GenericWrite = FILE_GENERIC_WRITE;
    GenMap.GenericExecute = FILE_GENERIC_EXECUTE;
    GenMap.GenericAll = FILE_ALL_ACCESS;

    MapGenericMask(&DesiredAccess, &GenMap);
    if (!AccessCheck(pSD, hClientToken, DesiredAccess, &GenMap, &PrivilegeSet, &PrivilegeSetLength, &GrantedAccess, &bAccessStatus))
    {
        RevertToSelf();
        CloseHandle(hClientToken);
        return;
    }
    CloseHandle(hClientToken);
    RevertToSelf();

    if (bAccessStatus)
    {
        m_bGameFolderWritable = true;
    }
#elif defined(MOBILE)
    char cwd[AZ_MAX_PATH_LEN];

    if (getcwd(cwd, AZ_MAX_PATH_LEN) != NULL)
    {
        if (0 == access(cwd, W_OK))
        {
            m_bGameFolderWritable = true;
        }
    }
#endif //WIN32
}

/////////////////////////////////`/////////////////////////////////////////
void CSystem::EnableFloatExceptions([[maybe_unused]] int type)
{
#ifndef _RELEASE

#if defined(WIN32)

#if defined(WIN32) && !defined(WIN64)

    // Optimization
    // Enable DAZ/FZ
    // Denormals Are Zeros
    // Flush-to-Zero

    _controlfp(_DN_FLUSH, _MCW_DN);

#endif //#if defined(WIN32) && !defined(WIN64)

AZ_PUSH_DISABLE_WARNING(4996, "-Wunknown-warning-option")
    _controlfp(_DN_FLUSH, _MCW_DN);

    if (type == 0)
    {
        // mask all floating exceptions off.
        _controlfp(_EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW | _EM_INVALID | _EM_DENORMAL | _EM_ZERODIVIDE, _MCW_EM);
    }
    else
    {
        // Clear pending exceptions
        _fpreset();

        if (type == 1)
        {
            // enable just the most important fp-exceptions.
            _controlfp(_EM_INEXACT | _EM_UNDERFLOW | _EM_OVERFLOW, _MCW_EM); // Enable floating point exceptions.
        }

        if (type == 2)
        {
            // enable ALL floating point exceptions.
            _controlfp(_EM_INEXACT, _MCW_EM);
        }
    }
AZ_POP_DISABLE_WARNING

#endif //#if defined(WIN32) && !defined(WIN64)

#ifdef WIN32
    _mm_setcsr(_mm_getcsr() & ~0x280 | (type > 0 ? 0 : 0x280));
#endif

    #endif //_RELEASE
}
