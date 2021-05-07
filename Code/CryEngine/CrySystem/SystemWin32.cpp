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

#include "CrySystem_precompiled.h"
#include "System.h"
#include <time.h>

#include <IRenderer.h>
#include <IMovieSystem.h>
#include <ILog.h>
#include <CryLibrary.h>
#include <StringUtils.h>
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
#include "StreamEngine/StreamEngine.h"
#include "LocalizedStringManager.h"
#include "XML/XmlUtils.h"
#include "AutoDetectSpec.h"

#if defined(WIN32)
__pragma(comment(lib, "wininet.lib"))
__pragma(comment(lib, "Winmm.lib"))
#endif

#if defined(APPLE)
#include "SystemUtilsApple.h"
#endif

// this is the list of modules that can be loaded into the game process
// Each array element contains 2 strings: the name of the module (case-insensitive)
// and the name of the group the module belongs to
//////////////////////////////////////////////////////////////////////////
const char g_szGroupCore[] = "CryEngine";
const char* g_szModuleGroups[][2] = {
    {"Editor.exe", g_szGroupCore},
    {"CrySystem.dll", g_szGroupCore},
    {"CryFont.dll", g_szGroupCore},
};

//////////////////////////////////////////////////////////////////////////
void CSystem::SetAffinity()
{
    // the following code is only for Windows
#ifdef WIN32
    // set the process affinity
    ICVar* pcvAffinityMask = GetIConsole()->GetCVar("sys_affinity");
    if (!pcvAffinityMask)
    {
        pcvAffinityMask = REGISTER_INT("sys_affinity", 0, VF_NULL, "");
    }

    if (pcvAffinityMask)
    {
        unsigned nAffinity = pcvAffinityMask->GetIVal();
        if (nAffinity)
        {
            typedef BOOL (WINAPI * FnSetProcessAffinityMask)(IN HANDLE hProcess, IN DWORD_PTR dwProcessAffinityMask);
            HMODULE hKernel = CryLoadLibrary ("kernel32.dll");
            if (hKernel)
            {
                FnSetProcessAffinityMask SetProcessAffinityMask = (FnSetProcessAffinityMask)GetProcAddress(hKernel, "SetProcessAffinityMask");
                if (SetProcessAffinityMask && !SetProcessAffinityMask(GetCurrentProcess(), nAffinity))
                {
                    GetILog()->LogError("Error: Cannot set affinity mask %d, error code %d", nAffinity, GetLastError());
                }
                FreeLibrary (hKernel);
            }
        }
    }
#endif
}

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

const SmallModuleInfo* FindModuleInfo(std::vector<SmallModuleInfo>& vec, const char* name)
{
    for (size_t i = 0; i < vec.size(); ++i)
    {
        if (!vec[i].name.compareNoCase(name))
        {
            return &vec[i];
        }
    }

    return 0;
}

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
    cry_strcpy(szNameBuffer, CryStringUtils::WStrToUTF8(nameW));
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
        string suffix;
        for (int instance = 0;; ++instance)
        {
            suffix.Format("(%d)", instance);

            CreateMutex(NULL, TRUE, "LumberyardApplication" + suffix);
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
    string suffix;
    int instance = 0;
    for (;; ++instance)
    {
        suffix.Format("(%d)", instance);

        CreateMutex(NULL, TRUE, logFilePath + suffix);
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

// these 2 functions are duplicated in System.cpp in editor
//////////////////////////////////////////////////////////////////////////
#if !defined(LINUX)
extern int CryStats(char* buf);
#endif
int CSystem::DumpMMStats(bool log)
{
#if defined(LINUX)
    return 0;
#else
    if (log)
    {
        char buf[1024];
        int n = CryStats(buf);
        GetILog()->Log(buf);
        return n;
    }
    else
    {
        return CryStats(NULL);
    };
#endif
};

//////////////////////////////////////////////////////////////////////////
struct CryDbgModule
{
    HANDLE heap;
    WIN_HMODULE handle;
    string name;
    DWORD dwSize;
};

//////////////////////////////////////////////////////////////////////////
void CSystem::DebugStats([[maybe_unused]] bool checkpoint, [[maybe_unused]] bool leaks)
{
#ifdef WIN32
    std::vector<CryDbgModule> dbgmodules;

    //////////////////////////////////////////////////////////////////////////
    // Use windows Performance Monitoring API to enumerate all modules of current process.
    //////////////////////////////////////////////////////////////////////////
    HANDLE hSnapshot;
    hSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPMODULE, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 me;
        memset (&me, 0, sizeof(me));
        me.dwSize = sizeof(me);

        if (Module32First (hSnapshot, &me))
        {
            // the sizes of each module group
            do
            {
                CryDbgModule module;
                module.handle = me.hModule;
                module.name = me.szModule;
                module.dwSize = me.modBaseSize;
                dbgmodules.push_back(module);
            } while (Module32Next(hSnapshot, &me));
        }
        CloseHandle (hSnapshot);
    }
    //////////////////////////////////////////////////////////////////////////
   
    int nolib = 0;

#ifdef _DEBUG
    ILog* log = GetILog();
    int totalal = 0;
    int totalbl = 0;
    int extrastats[10];
#endif

    int totalUsedInModules = 0;
    int countedMemoryModules = 0;
    for (int i = 0; i < (int)(dbgmodules.size()); i++)
    {
        if (!dbgmodules[i].handle)
        {
            CryLogAlways("WARNING: CSystem::DebugStats: NULL handle for %s", dbgmodules[i].name.c_str());
            nolib++;
            continue;
        }
        ;

        typedef int (* PFN_MODULEMEMORY)();
        PFN_MODULEMEMORY fpCryModuleGetAllocatedMemory = (PFN_MODULEMEMORY)::GetProcAddress((HMODULE)dbgmodules[i].handle, "CryModuleGetAllocatedMemory");
        if (fpCryModuleGetAllocatedMemory)
        {
            int allocatedMemory = fpCryModuleGetAllocatedMemory();
            totalUsedInModules += allocatedMemory;
            countedMemoryModules++;
            CryLogAlways("%8d K used in Module %s: ", allocatedMemory / 1024, dbgmodules[i].name.c_str());
        }

#ifdef _DEBUG
        typedef void (* PFNUSAGESUMMARY)(ILog* log, const char*, int*);
        typedef void (* PFNCHECKPOINT)();
        PFNUSAGESUMMARY fpu = (PFNUSAGESUMMARY)::GetProcAddress((HMODULE)dbgmodules[i].handle, "UsageSummary");
        PFNCHECKPOINT fpc = (PFNCHECKPOINT)::GetProcAddress((HMODULE)dbgmodules[i].handle, "CheckPoint");
        if (fpu && fpc)
        {
            if (checkpoint)
            {
                fpc();
            }
            else
            {
                extrastats[2] = (int)leaks;
                fpu(log, dbgmodules[i].name.c_str(), extrastats);
                totalal += extrastats[0];
                totalbl += extrastats[1];
            };
        }
        else
        {
            CryLogAlways("WARNING: CSystem::DebugStats: could not retrieve function from DLL %s", dbgmodules[i].name.c_str());
            nolib++;
        };
#endif

        typedef HANDLE(* PFNGETDLLHEAP)();
        PFNGETDLLHEAP fpg = (PFNGETDLLHEAP)::GetProcAddress((HMODULE)dbgmodules[i].handle, "GetDLLHeap");
        if (fpg)
        {
            dbgmodules[i].heap = fpg();
        }
        ;
    }
    ;

    CryLogAlways("-------------------------------------------------------");
    CryLogAlways("%8d K Total Memory Allocated in %d Modules", totalUsedInModules / 1024, countedMemoryModules);
#ifdef _DEBUG
    CryLogAlways("$8GRAND TOTAL: %d k, %d blocks (%d dlls not included)", totalal / 1024, totalbl, nolib);
    CryLogAlways("estimated debugalloc overhead: between %d k and %d k", totalbl * 36 / 1024, totalbl * 72 / 1024);
#endif

    //////////////////////////////////////////////////////////////////////////
    // Get HeapQueryInformation pointer if on windows XP.
    //////////////////////////////////////////////////////////////////////////
    typedef BOOL (WINAPI * FUNC_HeapQueryInformation)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T, PSIZE_T);
    FUNC_HeapQueryInformation pFnHeapQueryInformation = NULL;
    HMODULE hKernelInstance = CryLoadLibrary("Kernel32.dll");
    if (hKernelInstance)
    {
        pFnHeapQueryInformation = (FUNC_HeapQueryInformation)(::GetProcAddress(hKernelInstance, "HeapQueryInformation"));
    }
    //////////////////////////////////////////////////////////////////////////

    const int MAXHANDLES = 100;
    HANDLE handles[MAXHANDLES];
    int realnumh = GetProcessHeaps(MAXHANDLES, handles);
    char hinfo[1024];
    PROCESS_HEAP_ENTRY phe;
    CryLogAlways("$6--------------------- dump of windows heaps ---------------------");
    int nTotalC = 0, nTotalCP = 0, nTotalUC = 0, nTotalUCP = 0, totalo = 0;
    for (int i = 0; i < realnumh; i++)
    {
        HANDLE hHeap = handles[i];
        HeapCompact(hHeap, 0);
        hinfo[0] = 0;
        if (pFnHeapQueryInformation)
        {
            pFnHeapQueryInformation(hHeap, HeapCompatibilityInformation, hinfo, 1024, NULL);
        }
        else
        {
            for (int m = 0; m < (int)(dbgmodules.size()); m++)
            {
                if (dbgmodules[m].heap == handles[i])
                {
                    azstrcpy(hinfo, AZ_ARRAY_SIZE(hinfo), dbgmodules[m].name.c_str());
                }
            }
        }
        phe.lpData = NULL;
        int nCommitted = 0, nUncommitted = 0, nOverhead = 0;
        int nCommittedPieces = 0, nUncommittedPieces = 0;
#if !defined(NDEBUG)
        int nPrevRegionIndex = -1;
#endif
        while (HeapWalk(hHeap, &phe))
        {
            if (phe.wFlags & PROCESS_HEAP_REGION)
            {
                assert (++nPrevRegionIndex == phe.iRegionIndex);
                nCommitted += phe.Region.dwCommittedSize;
                nUncommitted +=  phe.Region.dwUnCommittedSize;
                assert (phe.cbData == 0 || (phe.wFlags & PROCESS_HEAP_ENTRY_BUSY));
            }
            else
            if (phe.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE)
            {
                nUncommittedPieces += phe.cbData;
            }
            else
            {
                //if (phe.wFlags & PROCESS_HEAP_ENTRY_BUSY)
                nCommittedPieces += phe.cbData;
            }


            {
                /*
                MEMORY_BASIC_INFORMATION mbi;
                if (VirtualQuery(phe.lpData, &mbi,sizeof(mbi)) == sizeof(mbi))
                {
                if (mbi.State == MEM_COMMIT)
                nCommittedPieces += phe.cbData;//mbi.RegionSize;
                //else
                //  nUncommitted += mbi.RegionSize;
                }
                else
                nCommittedPieces += phe.cbData;
                */
            }

            nOverhead += phe.cbOverhead;
        }

        CryLogAlways("* heap %8x: %6d (or ~%6d) K in use, %6d..%6d K uncommitted, %6d K overhead (%s)\n",
            handles[i], nCommittedPieces / 1024, nCommitted / 1024, nUncommittedPieces / 1024, nUncommitted / 1024, nOverhead / 1024, hinfo);

        nTotalC += nCommitted;
        nTotalCP += nCommittedPieces;
        nTotalUC += nUncommitted;
        nTotalUCP += nUncommittedPieces;
        totalo += nOverhead;
    }
    ;
    CryLogAlways("$6----------------- total in heaps: %d megs committed (win stats shows ~%d) (%d..%d uncommitted, %d k overhead) ---------------------", nTotalCP / 1024 / 1024, nTotalC / 1024 / 1024, nTotalUCP / 1024 / 1024, nTotalUC / 1024 / 1024, totalo / 1024);

#endif //WIN32
};

#ifdef WIN32
struct DumpHeap32Stats
{
    DumpHeap32Stats()
        : dwFree(0)
        , dwMoveable(0)
        , dwFixed(0)
        , dwUnknown(0)
    {
    }
    void operator += (const DumpHeap32Stats& right)
    {
        dwFree += right.dwFree;
        dwMoveable += right.dwMoveable;
        dwFixed += right.dwFixed;
        dwUnknown += right.dwUnknown;
    }
    DWORD dwFree;
    DWORD dwMoveable;
    DWORD dwFixed;
    DWORD dwUnknown;
};
static void DumpHeap32 (const HEAPLIST32& hl, DumpHeap32Stats& stats)
{
    HEAPENTRY32 he;
    memset (&he, 0, sizeof(he));
    he.dwSize = sizeof(he);

    if (Heap32First (&he, hl.th32ProcessID, hl.th32HeapID))
    {
        DumpHeap32Stats heap;
        do
        {
            if (he.dwFlags & LF32_FREE)
            {
                heap.dwFree += he.dwBlockSize;
            }
            else
            if (he.dwFlags & LF32_MOVEABLE)
            {
                heap.dwMoveable += he.dwBlockSize;
            }
            else
            if (he.dwFlags & LF32_FIXED)
            {
                heap.dwFixed += he.dwBlockSize;
            }
            else
            {
                heap.dwUnknown += he.dwBlockSize;
            }
        } while (Heap32Next (&he));

        CryLogAlways ("%08X  %6d %6d %6d (%d)", hl.th32HeapID, heap.dwFixed / 0x400, heap.dwFree / 0x400, heap.dwMoveable / 0x400, heap.dwUnknown / 0x400);
        stats += heap;
    }
    else
    {
        CryLogAlways ("%08X  empty or invalid");
    }
}

//////////////////////////////////////////////////////////////////////////
class CStringOrder
{
public:
    bool operator () (const char* szLeft, const char* szRight) const {return azstricmp(szLeft, szRight) < 0; }
};
typedef std::map<const char*, unsigned, CStringOrder> StringToSizeMap;
void AddSize (StringToSizeMap& mapSS, const char* szString, unsigned nSize)
{
    StringToSizeMap::iterator it = mapSS.find (szString);
    if (it == mapSS.end())
    {
        mapSS.insert (StringToSizeMap::value_type(szString, nSize));
    }
    else
    {
        it->second += nSize;
    }
}

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

//////////////////////////////////////////////////////////////////////////
void CSystem::DumpWinHeaps()
{
#ifdef WIN32
    //
    // Retrieve modules and log them; remember the process id

    HANDLE hSnapshot;
    hSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPMODULE, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        CryLogAlways ("Cannot get the module snapshot, error code %d", GetLastError());
        return;
    }

    DWORD dwProcessID = GetCurrentProcessId();

    MODULEENTRY32 me;
    memset (&me, 0, sizeof(me));
    me.dwSize = sizeof(me);

    if (Module32First (hSnapshot, &me))
    {
        // the sizes of each module group
        StringToSizeMap mapGroupSize;
        DWORD dwTotalModuleSize = 0;
        CryLogAlways ("base        size  module");
        do
        {
            dwProcessID = me.th32ProcessID;
            const char* szGroup = GetModuleGroup (me.szModule);
            CryLogAlways ("%08X %8X  %25s   - %s", me.modBaseAddr, me.modBaseSize, me.szModule, azstricmp(szGroup, "Other") ? szGroup : "");
            dwTotalModuleSize += me.modBaseSize;
            AddSize (mapGroupSize, szGroup, me.modBaseSize);
        } while (Module32Next(hSnapshot, &me));

        CryLogAlways ("------------------------------------");
        for (StringToSizeMap::iterator it = mapGroupSize.begin(); it != mapGroupSize.end(); ++it)
        {
            CryLogAlways ("         %6.3f Mbytes  - %s", double(it->second) / 0x100000, it->first);
        }
        CryLogAlways ("------------------------------------");
        CryLogAlways ("         %6.3f Mbytes  - TOTAL", double(dwTotalModuleSize) / 0x100000);
        CryLogAlways ("------------------------------------");
    }
    else
    {
        CryLogAlways ("No modules to dump");
    }

    CloseHandle (hSnapshot);

    //
    // Retrieve the heaps and dump each of them with a special function

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPHEAPLIST, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        CryLogAlways ("Cannot get the heap LIST snapshot, error code %d", GetLastError());
        return;
    }

    HEAPLIST32 hl;
    memset (&hl, 0, sizeof(hl));
    hl.dwSize = sizeof(hl);

    CryLogAlways ("__Heap__   fixed   free   move (unknown)");
    if (Heap32ListFirst (hSnapshot, &hl))
    {
        DumpHeap32Stats stats;
        do
        {
            DumpHeap32 (hl, stats);
        } while (Heap32ListNext (hSnapshot, &hl));

        CryLogAlways ("-------------------------------------------------");
        CryLogAlways ("$6          %6.3f %6.3f %6.3f (%.3f) Mbytes", double(stats.dwFixed) / 0x100000, double(stats.dwFree) / 0x100000, double(stats.dwMoveable) / 0x100000, double(stats.dwUnknown) / 0x100000);
        CryLogAlways ("-------------------------------------------------");
    }
    else
    {
        CryLogAlways ("No heaps to dump");
    }

    CloseHandle(hSnapshot);
#endif
}

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
            cry_strcpy(szBuffer, (char*)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }
        else
        {
            return 0;
        }

        return szBuffer;
    }
#else
    return 0;

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

    LogSystemInfo();

    OutputDebugString(szBuffer);
#ifdef WIN32
    OnFatalError(szBuffer);
    if (!g_cvars.sys_no_crash_dialog)
    {
        ::MessageBox(NULL, szBuffer, "Open 3D Engine Error", MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
    }

    // Dump callstack.
#endif
#if defined (WIN32)
    //Triggers a fatal error, so the DebugCallstack can create the error.log and terminate the application
    IDebugCallStack::instance()->FatalError(szBuffer);
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMWIN32_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(SystemWin32_cpp)
#endif

    CryDebugBreak();

    // app can not continue
#ifdef _DEBUG

#if defined(WIN32) && !defined(WIN64)
    DEBUG_BREAK;
#endif

#else

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
    for (int i = 0; i < numFrames; i++)
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

//////////////////////////////////////////////////////////////////////////
// Support relaunching for windows media center edition.
//////////////////////////////////////////////////////////////////////////
#if defined(WIN32)
#if (_WIN32_WINNT < 0x0501)
#define SM_MEDIACENTER          87
#endif
bool CSystem::ReLaunchMediaCenter()
{
    // Skip if not running on a Media Center
    if (GetSystemMetrics(SM_MEDIACENTER) == 0)
    {
        return false;
    }

    // Get the path to Media Center
    char szExpandedPath[AZ_MAX_PATH_LEN];
    if (!ExpandEnvironmentStrings("%SystemRoot%\\ehome\\ehshell.exe", szExpandedPath, AZ_MAX_PATH_LEN))
    {
        return false;
    }

    // Skip if ehshell.exe doesn't exist
    if (GetFileAttributes(szExpandedPath) == 0xFFFFFFFF)
    {
        return false;
    }

    // Launch ehshell.exe
    INT_PTR result = (INT_PTR)ShellExecute(NULL, TEXT("open"), szExpandedPath, NULL, NULL, SW_SHOWNORMAL);
    return (result > 32);
}
#else
bool CSystem::ReLaunchMediaCenter()
{
    return false;
}
#endif //defined(WIN32)

//////////////////////////////////////////////////////////////////////////
#if defined(WIN32)
void CSystem::LogSystemInfo()
{
    //////////////////////////////////////////////////////////////////////
    // Write the system informations to the log
    //////////////////////////////////////////////////////////////////////

    char szBuffer[1024];
    char szProfileBuffer[128];
    char szLanguageBuffer[64];
    //char szCPUModel[64];

    MEMORYSTATUSEX MemoryStatus;
    MemoryStatus.dwLength = sizeof(MemoryStatus);

    DEVMODE DisplayConfig;
    OSVERSIONINFO OSVerInfo;
    OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    // log Windows type
    Win32SysInspect::GetOS(m_env.pi.winVer, m_env.pi.win64Bit, szBuffer, sizeof(szBuffer));
    CryLogAlways(szBuffer);

    // log system language
    GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SENGLANGUAGE, szLanguageBuffer, sizeof(szLanguageBuffer));
    azsprintf(szBuffer, "System language: %s", szLanguageBuffer);
    CryLogAlways(szBuffer);

    // log Windows directory
    GetWindowsDirectory(szBuffer, sizeof(szBuffer));
    string str = "Windows Directory: \"";
    str += szBuffer;
    str += "\"";
    CryLogAlways(str);

    //////////////////////////////////////////////////////////////////////
    // Send system time & date
    //////////////////////////////////////////////////////////////////////

    str = "Local time is ";
    azstrtime(szBuffer);
    str += szBuffer;
    str += " ";
    _strdate_s(szBuffer);
    str += szBuffer;
    azsprintf(szBuffer, ", system running for %lu minutes", GetTickCount() / 60000);
    str += szBuffer;
    CryLogAlways(str);

    //////////////////////////////////////////////////////////////////////
    // Send system memory status
    //////////////////////////////////////////////////////////////////////

    GlobalMemoryStatusEx(&MemoryStatus);
    azsprintf(szBuffer, "%I64dMB physical memory installed, %I64dMB available, %I64dMB virtual memory installed, %ld percent of memory in use",
        MemoryStatus.ullTotalPhys  / 1048576 + 1,
        MemoryStatus.ullAvailPhys / 1048576,
        MemoryStatus.ullTotalVirtual / 1048576,
        MemoryStatus.dwMemoryLoad);
    CryLogAlways(szBuffer);

    if (GetISystem()->GetIMemoryManager())
    {
        IMemoryManager::SProcessMemInfo memCounters;
        GetISystem()->GetIMemoryManager()->GetProcessMemInfo(memCounters);

        uint64 PagefileUsage = memCounters.PagefileUsage;
        uint64 PeakPagefileUsage = memCounters.PeakPagefileUsage;
        uint64 WorkingSetSize = memCounters.WorkingSetSize;
        azsprintf(szBuffer, "PageFile usage: %I64dMB, Working Set: %I64dMB, Peak PageFile usage: %I64dMB,",
            (uint64)PagefileUsage / (1024 * 1024),
            (uint64)WorkingSetSize / (1024 * 1024),
            (uint64)PeakPagefileUsage / (1024 * 1024));
        CryLogAlways(szBuffer);
    }

    //////////////////////////////////////////////////////////////////////
    // Send display settings
    //////////////////////////////////////////////////////////////////////

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &DisplayConfig);
    GetPrivateProfileString("boot.description", "display.drv",
        "(Unknown graphics card)", szProfileBuffer, sizeof(szProfileBuffer),
        "system.ini");
    azsprintf(szBuffer, "Current display mode is %lux%lux%lu, %s",
        DisplayConfig.dmPelsWidth, DisplayConfig.dmPelsHeight,
        DisplayConfig.dmBitsPerPel, szProfileBuffer);
    CryLogAlways(szBuffer);

    //////////////////////////////////////////////////////////////////////
    // Send input device configuration
    //////////////////////////////////////////////////////////////////////

    str = "";
    // Detect the keyboard type
    switch (GetKeyboardType(0))
    {
    case 1:
        str = "IBM PC/XT (83-key)";
        break;
    case 2:
        str = "ICO (102-key)";
        break;
    case 3:
        str = "IBM PC/AT (84-key)";
        break;
    case 4:
        str = "IBM enhanced (101/102-key)";
        break;
    case 5:
        str = "Nokia 1050";
        break;
    case 6:
        str = "Nokia 9140";
        break;
    case 7:
        str = "Japanese";
        break;
    default:
        str = "Unknown";
        break;
    }

    // Any mouse attached ?
    if (!GetSystemMetrics(SM_MOUSEPRESENT))
    {
        CryLogAlways(str + " keyboard and no mouse installed");
    }
    else
    {
        azsprintf(szBuffer, " keyboard and %i+ button mouse installed",
            GetSystemMetrics(SM_CMOUSEBUTTONS));
        CryLogAlways(str + szBuffer);
    }

    CryLogAlways("--------------------------------------------------------------------------------");
}
#else
void CSystem::LogSystemInfo()
{
}
#endif

#if (defined(WIN32) || defined(WIN64))
//////////////////////////////////////////////////////////////////////////
bool CSystem::GetWinGameFolder(char* szMyDocumentsPath, int maxPathSize)
{
    bool bSucceeded  = false;
    // check Vista and later OS first

    HMODULE shell32 = LoadLibraryA("Shell32.dll");
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
                cry_strcpy(szMyDocumentsPath, maxPathSize, CryStringUtils::WStrToUTF8(wMyDocumentsPath));
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
            cry_strcpy(szMyDocumentsPath, maxPathSize, CryStringUtils::WStrToUTF8(wMyDocumentsPath));
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
    dwRes = GetNamedSecurityInfo(".", SE_FILE_OBJECT,
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
