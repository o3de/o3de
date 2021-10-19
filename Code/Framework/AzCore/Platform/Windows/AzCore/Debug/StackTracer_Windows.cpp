/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/std/parallel/mutex.h>

#include <tchar.h>
#include <stdio.h>
AZ_PUSH_DISABLE_WARNING(4091, "-Wmissing-declarations"); // 4091: 'typedef ': ignored on left of '' when no variable is declared
#   include <dbghelp.h>
AZ_POP_DISABLE_WARNING;
#pragma comment(lib, "version.lib")  // for "VerQueryValue"

#include <NTSecAPI.h> // for LdrDllNotification dll load hook (UNICODE_STRING)

namespace AZ {
#if defined(AZ_ENABLE_DEBUG_TOOLS)

    namespace Debug
    {
        // its important that this struct is POD
        struct DynamicallyLoadedModuleInfo
        {
            char    m_fileName[1024];
            char    m_name[256];
            AZ::u64 m_baseAddress;
            AZ::u32 m_size;
        };

        struct SymbolStorageDynamicallyLoadedModules
        {
            size_t m_size;
            DynamicallyLoadedModuleInfo m_modules[256];

            SymbolStorageDynamicallyLoadedModules()
                : m_size(0)
            {}

        };

        // Contains module information of modules loaded by process. We keep track of this so that when we need
        // symbol information when decoding frames we can load the symbols of any modules that may have been active
        // for that frame/allocation. Prior to tracking these, when the memory leak detection code ran at the end of
        // the process (when allocators are being destroyed, AFTER dynamic modules have been unloaded) we wouldn't have
        // the module symbols needed and wouldn't get correct callstacks.
        static EnvironmentVariable<SymbolStorageDynamicallyLoadedModules> s_dynamicallyLoadedModules = nullptr;

        // Mutex for use of s_dynamicallyLoadedModules so that if there's one thread decoding frames while another is
        // handling DLL load and adding to the list we don't have issues
        static EnvironmentVariable<AZStd::mutex> s_dynamicallyLoadedModulesMutex;

        static AZStd::mutex& GetDynamicallyLoadedModulesMutex()
        {
            if (!s_dynamicallyLoadedModulesMutex)
            {
                s_dynamicallyLoadedModulesMutex = Environment::CreateVariable<AZStd::mutex>(AZ_CRC("SymbolStorageDynamicallyLoadedModulesMutex", 0x2b7dbaf2));
                AZ_Assert(s_dynamicallyLoadedModulesMutex, "Unable to create SymbolStorageDynamicallyLoadedModulesMutex environment variable");
            }
            return *s_dynamicallyLoadedModulesMutex;
        }

        static SymbolStorageDynamicallyLoadedModules& GetRegisteredLoadedModules()
        {
            if (!s_dynamicallyLoadedModules)
            {
                s_dynamicallyLoadedModules = Environment::CreateVariable<SymbolStorageDynamicallyLoadedModules>(AZ_CRC("SymbolStorageDynamicallyLoadedModules", 0xecf96588));
                AZ_Assert(s_dynamicallyLoadedModules, "Unable to create SymbolStorageDynamicallyLoadedModules environment variable - dynamically loaded modules won't have symbols loaded!");
            }
            return *s_dynamicallyLoadedModules;
        }
       
        // This is a callback function as from MSDN documents.
        // it can be used to debug why symbols are not loading.  But you do have to enable the DEBUG flag during SymInit for it to work.
        BOOL CALLBACK SymRegisterCallbackProc64(__in HANDLE hProcess, __in ULONG ActionCode, __in_opt ULONG64 CallbackData, __in_opt ULONG64 UserContext)
        {
            UNREFERENCED_PARAMETER(hProcess);
            UNREFERENCED_PARAMETER(UserContext);
            PIMAGEHLP_CBA_EVENT evt;

            switch (ActionCode)
            {
            case CBA_EVENT:
                evt = (PIMAGEHLP_CBA_EVENT)CallbackData;
                printf("%s", evt->desc);
                break;

            default:
                return FALSE;
            }

            return TRUE;
        }
    } // namespace Debug


    // this first typedef is for the USER CALLBACK function. 
    typedef BOOL    (CALLBACK *PSYMBOL_REGISTERED_CALLBACK64)(_In_ HANDLE hProcess, _In_ ULONG ActionCode, _In_opt_ ULONG64 CallbackData, _In_opt_ ULONG64 UserContext);

    // these are the dll exports.
    typedef BOOL    (__stdcall * SymCleanup_t)(IN HANDLE hProcess);
    typedef PVOID   (__stdcall * SymFunctionTableAccess64_t)(HANDLE hProcess, DWORD64 AddrBase);
    typedef BOOL    (__stdcall * SymGetLineFromAddr64_t)(IN HANDLE hProcess, IN DWORD64 dwAddr, OUT PDWORD pdwDisplacement, OUT PIMAGEHLP_LINE64 Line);
    typedef DWORD64 (__stdcall * SymGetModuleBase64_t)(IN HANDLE hProcess, IN DWORD64 dwAddr);
    typedef BOOL    (__stdcall * SymGetModuleInfo64_t)(IN HANDLE hProcess, IN DWORD64 dwAddr, OUT IMAGEHLP_MODULE64* ModuleInfo);
    typedef DWORD   (__stdcall * SymGetOptions_t)(VOID);
    typedef BOOL    (__stdcall * SymGetSymFromAddr64_t)(IN HANDLE hProcess, IN DWORD64 dwAddr, OUT PDWORD64 pdwDisplacement, OUT PIMAGEHLP_SYMBOL64 Symbol);
    typedef BOOL    (__stdcall * SymInitialize_t)(IN HANDLE hProcess, IN PCSTR UserSearchPath, IN BOOL fInvadeProcess);
    typedef DWORD64 (__stdcall * SymLoadModule64_t)(IN HANDLE hProcess, IN HANDLE hFile, IN PSTR ImageName, IN PSTR ModuleName, IN DWORD64 BaseOfDll, IN DWORD SizeOfDll);
    typedef DWORD   (__stdcall * SymSetOptions_t)(IN DWORD SymOptions);
    typedef BOOL    (__stdcall * StackWalk64_t)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord,
        PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
        PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
    typedef DWORD (__stdcall WINAPI * UnDecorateSymbolName_t)(PCSTR DecoratedName, PSTR UnDecoratedName, DWORD UndecoratedLength, DWORD Flags);
    typedef BOOL (__stdcall WINAPI * SymGetSearchPath_t)(HANDLE hProcess, PSTR SearchPath, DWORD SearchPathLength);
    typedef BOOL(__stdcall WINAPI * SymRegisterCallback64_t)(HANDLE hProcess, PSYMBOL_REGISTERED_CALLBACK64 CallbackFunction, ULONG64 UserContext);

    SymInitialize_t             g_SymInitialize;
    SymCleanup_t                g_SymCleanup;
    SymFunctionTableAccess64_t  g_SymFunctionTableAccess64;
    SymGetLineFromAddr64_t      g_SymGetLineFromAddr64;
    SymGetModuleBase64_t        g_SymGetModuleBase64;
    SymGetModuleInfo64_t        g_SymGetModuleInfo64;
    SymGetOptions_t             g_SymGetOptions;
    SymGetSymFromAddr64_t       g_SymGetSymFromAddr64;
    SymLoadModule64_t           g_SymLoadModule64;
    SymSetOptions_t             g_SymSetOptions;
    StackWalk64_t               g_StackWalk64;
    UnDecorateSymbolName_t      g_UnDecorateSymbolName;
    SymGetSearchPath_t          g_SymGetSearchPath;
    SymRegisterCallback64_t     g_SymRegisterCallback64;
    HMODULE                     g_dbgHelpDll;

#define LOAD_FUNCTION(A)    { g_##A = (A##_t)GetProcAddress(g_dbgHelpDll, #A); AZ_Assert(g_##A != 0, ("Can not load %s function!",#A)); }

    using namespace AZ::Debug;

    bool                        g_dbgHelpLoaded = false;
    AZStd::mutex                g_dbgLoadingMutex;
    HANDLE                      g_currentProcess = 0;   /// We deal with only one process for now.
    CRITICAL_SECTION            g_csDbgHelpDll;         /// All dbg help functions are single threaded, so we need to control the access.
    AZStd::fixed_vector<SymbolStorage::ModuleInfo, 2048> g_moduleInfo;
    
    // reserve 4k of scratch space so that we can get some callstack information without any allocations and as little stack frame usage as possible.
    const size_t                g_scratchSpaceSize = 2048;
    char                        g_reservedScratchSpace1[g_scratchSpaceSize];
    char                        g_reservedScratchSpace2[g_scratchSpaceSize];
    /**
     * WindowsSymbols - loads and manages windows symbols.
     */
    class WindowsSymbols
    {
        const char*     m_szSymPath;
        BOOL            m_isSymLoaded;


        // --------------------------------------------------------------------------------------------
        // Begin DLL load hook section
        // These structures and variables are for LdrDllNotification callback to work -
        // we listen for module loads on the process and we add their info to 
        // AZ::Debug::s_dynamicallyLoadedModules.  When decoding frames, we make sure to 
        // load up any symbols for dynamically loaded modules.
        // --------------------------------------------------------------------------------------------
        typedef const UNICODE_STRING * PCUNICODE_STRING;

        // Notification structures: https://msdn.microsoft.com/en-us/library/dd347460(v=vs.85).aspx

        typedef struct _LDR_DLL_LOADED_NOTIFICATION_DATA {
            ULONG Flags;                    //Reserved.
            PCUNICODE_STRING FullDllName;   //The full path name of the DLL module.
            PCUNICODE_STRING BaseDllName;   //The base file name of the DLL module.
            PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
            ULONG SizeOfImage;              //The size of the DLL image, in bytes.
        } LDR_DLL_LOADED_NOTIFICATION_DATA, *PLDR_DLL_LOADED_NOTIFICATION_DATA;

        typedef struct _LDR_DLL_UNLOADED_NOTIFICATION_DATA {
            ULONG Flags;                    //Reserved.
            PCUNICODE_STRING FullDllName;   //The full path name of the DLL module.
            PCUNICODE_STRING BaseDllName;   //The base file name of the DLL module.
            PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
            ULONG SizeOfImage;              //The size of the DLL image, in bytes.
        } LDR_DLL_UNLOADED_NOTIFICATION_DATA, *PLDR_DLL_UNLOADED_NOTIFICATION_DATA;

        typedef union _LDR_DLL_NOTIFICATION_DATA {
            LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
            LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
        } LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

        typedef const _LDR_DLL_NOTIFICATION_DATA * PCLDR_DLL_NOTIFICATION_DATA;

        // A notification callback function specified with the LdrRegisterDllNotification function. The loader calls this function when a DLL is first loaded.
        // https://msdn.microsoft.com/en-us/library/dd347460(v=vs.85).aspx 
        typedef VOID (CALLBACK * PLDR_DLL_NOTIFICATION_FUNCTION)(
            _In_     ULONG                       NotificationReason,
            _In_     PCLDR_DLL_NOTIFICATION_DATA NotificationData,
            _In_opt_ PVOID                       Context
            );

        // Registers for notification when a DLL is first loaded. This notification occurs before dynamic linking takes place. 
        // https://msdn.microsoft.com/en-us/library/dd347461(v=vs.85).aspx
        typedef NTSTATUS (NTAPI * PLDR_REGISTER_DLL_NOTIFICATION)(
            _In_     ULONG                          Flags,
            _In_     PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
            _In_opt_ PVOID                          Context,
            _Out_    PVOID                          *Cookie
        );

        // Cancels DLL load notification previously registered by calling the LdrRegisterDllNotification function.
        // https://msdn.microsoft.com/en-us/library/dd347462(v=vs.85).aspx
        typedef NTSTATUS (NTAPI * PLDR_UNREGISTER_DLL_NOTIFICATION)(
            _In_ PVOID Cookie
        );

        BOOL                                    m_dllListenersRegistered;
        size_t                                  m_numDynamicModulesLoaded;
        PVOID                                   m_dllListenerCookie;
        PLDR_REGISTER_DLL_NOTIFICATION          m_LdrRegisterDllNotification;
        PLDR_UNREGISTER_DLL_NOTIFICATION        m_LdrUnregisterDllNotification;

    public:
        bool ModuleSymbolsNeedRefreshed() const
        {
            AZStd::lock_guard<AZStd::mutex> locker(AZ::Debug::GetDynamicallyLoadedModulesMutex());
            AZ::Debug::SymbolStorageDynamicallyLoadedModules& loadedModules = AZ::Debug::GetRegisteredLoadedModules();
            size_t numLoadedModules = loadedModules.m_size;
            return numLoadedModules != m_numDynamicModulesLoaded;
        }

        void RefreshModuleSymbols()
        {
            LoadModules(g_currentProcess, GetProcessId(g_currentProcess));
        }

        static VOID CALLBACK LdrDllNotification(
            _In_     ULONG                       NotificationReason,
            _In_     PCLDR_DLL_NOTIFICATION_DATA NotificationData,
            _In_opt_ PVOID                       Context
        )
        {
            (void)Context;
            if (NotificationReason == 1)
            {
                // Module loaded
                AZStd::lock_guard<AZStd::mutex> locker(AZ::Debug::GetDynamicallyLoadedModulesMutex());
                AZ::Debug::SymbolStorageDynamicallyLoadedModules& loadedModules = AZ::Debug::GetRegisteredLoadedModules();

                if (loadedModules.m_size < AZ_ARRAY_SIZE(loadedModules.m_modules))
                {
                    AZ::Debug::DynamicallyLoadedModuleInfo& modInfo = loadedModules.m_modules[loadedModules.m_size];
                    size_t numCharsConverted;
                    wcstombs_s(&numCharsConverted, modInfo.m_fileName, AZ_ARRAY_SIZE(modInfo.m_fileName), NotificationData->Loaded.FullDllName->Buffer, AZStd::min(AZ_ARRAY_SIZE(modInfo.m_fileName) - 1, (size_t)NotificationData->Loaded.FullDllName->Length));
                    wcstombs_s(&numCharsConverted, modInfo.m_name, AZ_ARRAY_SIZE(modInfo.m_name), NotificationData->Loaded.BaseDllName->Buffer, AZStd::min(AZ_ARRAY_SIZE(modInfo.m_name) - 1, (size_t)NotificationData->Loaded.BaseDllName->Length));
                    modInfo.m_baseAddress = (AZ::u64)NotificationData->Loaded.DllBase;
                    modInfo.m_size = (AZ::u32)NotificationData->Loaded.SizeOfImage;

                    // it might already be in the list.   If it is, we'll just use the existing one and overwrite it instead of pushing the last one 
                    // so that it doesn't grow unbounded as plugins / etc are loaded and unloaded, which can happen via operating system hooks or even
                    // via external app launching.
                    DynamicallyLoadedModuleInfo* toWriteTo = nullptr;
                    for (int idx = 0; idx < loadedModules.m_size; ++idx)
                    {
                        if ((strcmp(loadedModules.m_modules[idx].m_fileName, modInfo.m_fileName) == 0) &&
                            (strcmp(loadedModules.m_modules[idx].m_name, modInfo.m_name) == 0))
                        {
                            toWriteTo = &loadedModules.m_modules[idx];
                            break;
                        }
                    }

                    if (toWriteTo)
                    {
                        // if we found one to replace,
                        // update existing entry using this one as scratch space:
                        memcpy(toWriteTo, &modInfo, sizeof(AZ::Debug::DynamicallyLoadedModuleInfo));
                    }
                    else
                    {
                        loadedModules.m_size++;
                    }
                }
                else
                {
                    AZ_TracePrintf("System", "Failed to store dynamic module information, increase size of AZ::Debug::SymbolStorageDynamicallyLoadedModules.m_modules to maintain stack tracer functionality!");
                }
            }
        }

        void RegisterModuleListeners()
        {
            if (m_dllListenersRegistered)
            {
                return;
            }

            const HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
            m_LdrRegisterDllNotification = reinterpret_cast<PLDR_REGISTER_DLL_NOTIFICATION>(GetProcAddress(hNtDll, "LdrRegisterDllNotification"));

            if (m_LdrRegisterDllNotification)
            {
                NTSTATUS status = m_LdrRegisterDllNotification(0, LdrDllNotification, 0, &m_dllListenerCookie);
                if (status == 0)
                {
                    m_dllListenersRegistered = true;
                }
                else
                {
                    AZ_TracePrintf("System", "Failed registering module load listeners - SymbolStorage frame decoding may be impacted.");
                }
            }
        }

        void UnregisterModuleListeners()
        {
            if (!m_dllListenersRegistered)
            {
                return;
            }

            const HMODULE hNtDll = GetModuleHandleW(L"ntdll.dll");
            m_LdrUnregisterDllNotification = reinterpret_cast<PLDR_UNREGISTER_DLL_NOTIFICATION>(GetProcAddress(hNtDll, "LdrUnregisterDllNotification"));

            if (m_LdrUnregisterDllNotification)
            {
                NTSTATUS status = m_LdrUnregisterDllNotification(m_dllListenerCookie);
                if (status == 0)
                {
                    m_dllListenersRegistered = false;
                }
                else
                {
                    AZ_TracePrintf("System", "Failed unregistering module load listeners");
                }
            }
        }

        // --------------------------------------------------------------------------------------------
        // End DLL load hook section
        // --------------------------------------------------------------------------------------------

    public:
        WindowsSymbols()
            : m_szSymPath(nullptr)
            , m_isSymLoaded(FALSE)
            , m_dllListenersRegistered(FALSE)
            , m_numDynamicModulesLoaded(0)
            , m_dllListenerCookie(nullptr)
            , m_LdrRegisterDllNotification(nullptr)
            , m_LdrUnregisterDllNotification(nullptr) {}

        const char* GetSymbolPath() const               { return m_szSymPath; }
        void        SetSymbolPath(const char* path)     { m_szSymPath = path; }

        BOOL LoadSymbols()
        {
            if (m_isSymLoaded)
            {
                return TRUE;
            }
            
            // if no override has been set by someone calling SetSymbolPath (which completely overrides this)
            // then at the very least, add paths to current folder, current executable folder and known modules.
            if (!m_szSymPath)
            {
                g_reservedScratchSpace1[0] = 0;
                char* finalPosition = g_reservedScratchSpace1;

                // Add the current execution path and the current process path
                DWORD pathLen = GetModuleFileNameA(nullptr, g_reservedScratchSpace1, AZ_ARRAY_SIZE(g_reservedScratchSpace1));
                if (pathLen > 0)
                {
                    char* exeDirEnd = g_reservedScratchSpace1 + pathLen;
                    AZStd::replace(g_reservedScratchSpace1, exeDirEnd, '\\', '/');
                    
                    // g_reservedScratchSpace1 currently contains full path to EXE. Modify to end the string after the last '/'
                    finalPosition = strrchr(g_reservedScratchSpace1, '/');
                    if (finalPosition)
                    {
                        *finalPosition = 0;
                    }
                }

                // Add the current folder
                azstrcat(g_reservedScratchSpace1, AZ_ARRAY_SIZE(g_reservedScratchSpace1), ";.");

                AZ::Debug::SymbolStorageDynamicallyLoadedModules& loadedModules = AZ::Debug::GetRegisteredLoadedModules();
                for (int j = 0; j < loadedModules.m_size; ++j)
                {
                    AZ::Debug::DynamicallyLoadedModuleInfo& module = loadedModules.m_modules[j];
                    azstrcpy(g_reservedScratchSpace2, g_scratchSpaceSize, module.m_fileName);
                    char* lastSlash = strrchr(g_reservedScratchSpace2, '\\');
                    if (lastSlash)
                    {
                        *lastSlash = 0; // cut off everything after and including the last backslash.
                        azstrcat(g_reservedScratchSpace2, g_scratchSpaceSize, ";");
                        // is it already there?

                        if (strstr(g_reservedScratchSpace1, g_reservedScratchSpace2) == nullptr)
                        {
                            if (strlen(g_reservedScratchSpace1) + strlen(g_reservedScratchSpace2) < g_scratchSpaceSize)
                            {
                                azstrcat(g_reservedScratchSpace1, g_scratchSpaceSize, g_reservedScratchSpace2);
                            }
                        }
                    }
                    
                }
            }

            // SymInitialize
            if (g_SymInitialize(g_currentProcess, m_szSymPath ? m_szSymPath : g_reservedScratchSpace1, FALSE) == FALSE)
            {
                AZ_TracePrintf("System", "failed to call SymInitialize! Error %d", GetLastError());
            }

            if (g_SymRegisterCallback64)
            {
                g_SymRegisterCallback64(g_currentProcess, Debug::SymRegisterCallbackProc64, 0);
            }

            DWORD symOptions = g_SymGetOptions();
            symOptions |= SYMOPT_LOAD_LINES;
            symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
            symOptions |= SYMOPT_OMAP_FIND_NEAREST;
            symOptions |= SYMOPT_DEFERRED_LOADS;

            //symOptions |= SYMOPT_DEBUG; // uncomment if you want the debug callback to print out why it cannot load syms.
            //symOptions |= SYMOPT_NO_PROMPTS;

            symOptions = g_SymSetOptions(symOptions);

            m_isSymLoaded = LoadModules(g_currentProcess, GetProcessId(g_currentProcess));
            return m_isSymLoaded;
        }

        AZ_INLINE void CleanUpSymbols()
        {
            m_isSymLoaded = false;
            g_SymCleanup(g_currentProcess);
        }

        AZ_FORCE_INLINE bool IsSymLoaded() const { return m_isSymLoaded ? true : false; }

        void DecodeFrames(const StackFrame* frames, unsigned int numFrames, SymbolStorage::StackLine* textLines)
        {
            const int maxNameLen = 1024;
            char buffer[sizeof(IMAGEHLP_SYMBOL64) + maxNameLen] = {0};
            IMAGEHLP_SYMBOL64& sym = *(IMAGEHLP_SYMBOL64*)buffer;

            EnterCriticalSection(&g_csDbgHelpDll);
            for (unsigned int i = 0; i < numFrames; ++i)
            {
                SymbolStorage::StackLine& textLine = textLines[i];
                unsigned int textLineSize = sizeof(SymbolStorage::StackLine);
                const StackFrame& frame = frames[i];
                textLine[0] = 0;

                if (!frame.IsValid())
                {
                    continue;
                }

                sym.SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
                sym.MaxNameLength = maxNameLen;

                DWORD64 pc = (DWORD64)frame.m_programCounter;

                IMAGEHLP_LINE64 line = {0};
                line.SizeOfStruct = sizeof(line);
                DWORD displacement;
                if (g_SymGetLineFromAddr64(g_currentProcess, pc, &displacement, &line) && line.FileName[0] != 0)
                {
                    azsnprintf(textLine, textLineSize, "%s (%ld) : ", line.FileName, line.LineNumber);
                }
                else
                {
                    IMAGEHLP_MODULE64 module;
                    if (GetModuleInfo(g_currentProcess, pc, &module) && module.ModuleName[0] != 0)
                    {
                        azsnprintf(textLine, textLineSize, "%p (%s) : ", (LPVOID)pc, module.ModuleName);
                    }
                    else
                    {
                        azsnprintf(textLine, textLineSize, "%p (module-name not available) : ", (LPVOID)pc);
                    }
                }

                DWORD64 offsetFromSmybol;
                if (g_SymGetSymFromAddr64(g_currentProcess, pc, &offsetFromSmybol, &sym))
                {
                    unsigned int len = (unsigned int)strlen(textLine);
                    g_UnDecorateSymbolName(sym.Name, &textLine[len], textLineSize - len, UNDNAME_COMPLETE);
                    if (textLine[0] == 0)
                    {
                        g_UnDecorateSymbolName(sym.Name, &textLine[len], textLineSize - len, UNDNAME_NAME_ONLY);
                    }
                    if (textLine[0] == 0)
                    {
                        azstrcpy(textLine, textLineSize, sym.Name);
                    }
                }
                else
                {
                    azstrcat(textLine, textLineSize, "(function-name not available)");
                }
            }

            LeaveCriticalSection(&g_csDbgHelpDll);
        }

        void FindFunctionFromIP(void* ip, SymbolStorage::StackLine* func_name, SymbolStorage::StackLine* file_name, SymbolStorage::StackLine* module_name, int& line_num, void*& baseAddr)
        {
            const int maxNameLen = 1024;    // includes decorations, will be trimmed before assigned to StackLine
            char buffer[sizeof(IMAGEHLP_SYMBOL64) + maxNameLen] = { 0 };
            IMAGEHLP_SYMBOL64& sym = *(IMAGEHLP_SYMBOL64*)buffer;
            sym.SizeOfStruct = sizeof(sym);
            sym.MaxNameLength = maxNameLen;
            sym.Name[0] = '\0';

            IMAGEHLP_LINE64 line = { 0 };
            line.SizeOfStruct = sizeof(line);

            EnterCriticalSection(&g_csDbgHelpDll);
            {
                DWORD displacement;
                if (g_SymGetLineFromAddr64(g_currentProcess, (DWORD64)ip, &displacement, &line))
                {
                    if (line.FileName[0])
                    {
                        azstrcpy(*file_name, sizeof(*file_name), line.FileName);
                        line_num = line.LineNumber;
                        baseAddr = (void*)((DWORD64)ip - displacement);
                    }
                }

                IMAGEHLP_MODULE64 module;
                if (GetModuleInfo(g_currentProcess, (DWORD64)ip, &module))
                {
                    if (module.ModuleName[0])
                    {
                        azstrcpy(*module_name, sizeof(*module_name), module.ModuleName);
                    }
                }

                DWORD64 offsetFromSymbol;
                if (g_SymGetSymFromAddr64(g_currentProcess, (DWORD64)ip, &offsetFromSymbol, &sym))
                {
                    if (sym.Name[0])
                    {
                        g_UnDecorateSymbolName(sym.Name, *func_name, sizeof(*func_name), UNDNAME_NAME_ONLY);
                    }
                }
            }
            LeaveCriticalSection(&g_csDbgHelpDll);
        }

    private:
        BOOL GetModuleInfo(HANDLE hProcess, DWORD64 baseAddr, IMAGEHLP_MODULE64* pModuleInfo)
        {
            if (g_SymGetModuleInfo64 == NULL)
            {
                return FALSE;
            }
            pModuleInfo->SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
            return g_SymGetModuleInfo64(hProcess, baseAddr, (IMAGEHLP_MODULE64*) pModuleInfo);
        }

        DWORD LoadModule(HANDLE hProcess, LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size)
        {
            DWORD result = ERROR_SUCCESS;

            // Check if module is already loaded
            for (size_t i = 0; i < g_moduleInfo.size(); ++i)
            {
                if (strcmp(g_moduleInfo[i].m_fileName, img) == 0)
                {
                    return result;
                }
            }

            CHAR szImg[1024];
            CHAR szMod[1024];
            azstrcpy(szImg, AZ_ARRAY_SIZE(szImg), img);
            azstrcpy(szMod, AZ_ARRAY_SIZE(szMod), mod);

            if (g_SymLoadModule64(hProcess, 0, szImg, szMod, baseAddr, size) == 0)
            {
                result = GetLastError();
            }
            ULONGLONG fileVersion = 0;
            if (szImg != NULL)
            {
                // try to retrieve the file-version:
                VS_FIXEDFILEINFO* fInfo = NULL;
                DWORD dwHandle;
                DWORD dwSize = GetFileVersionInfoSizeA(szImg, &dwHandle);
                if (dwSize > 0)
                {
                    LPVOID vData = malloc(dwSize);
                    if (vData != NULL)
                    {
                        if (GetFileVersionInfoA(szImg, dwHandle, dwSize, vData) != 0)
                        {
                            UINT len;
                            TCHAR szSubBlock[] = L"\\";
                            if (VerQueryValueW(vData, szSubBlock, (LPVOID*) &fInfo, &len) == 0)
                            {
                                fInfo = NULL;
                            }
                            else
                            {
                                fileVersion = ((ULONGLONG)fInfo->dwFileVersionLS) + ((ULONGLONG)fInfo->dwFileVersionMS << 32);
                            }
                        }
                        free(vData);
                    }
                }

                // Retrive some additional-infos about the module
                IMAGEHLP_MODULE64 Module;
                const char* szSymType = "-unknown-";
                if (GetModuleInfo(hProcess, baseAddr, &Module) != FALSE)
                {
                    switch (Module.SymType)
                    {
                    case SymNone:
                        szSymType = "-nosymbols-";
                        break;
                    case SymCoff:
                        szSymType = "COFF";
                        break;
                    case SymCv:
                        szSymType = "CV";
                        break;
                    case SymPdb:
                        szSymType = "PDB";
                        break;
                    case SymExport:
                        szSymType = "-exported-";
                        break;
                    case SymDeferred:
                        szSymType = "-deferred-";
                        break;
                    case SymSym:
                        szSymType = "SYM";
                        break;
                    case 8: //SymVirtual:
                        szSymType = "Virtual";
                        break;
                    case 9: // SymDia:
                        szSymType = "DIA";
                        break;
                    }
                }

                // find insert position
                if (g_moduleInfo.size() <  g_moduleInfo.capacity())
                {
                    g_moduleInfo.push_back();
                    SymbolStorage::ModuleInfo& modInfo = g_moduleInfo.back();
                    modInfo.m_baseAddress = (u64)baseAddr;
                    azstrcpy(modInfo.m_modName, AZ_ARRAY_SIZE(modInfo.m_modName), szMod);
                    azstrcpy(modInfo.m_fileName, AZ_ARRAY_SIZE(modInfo.m_fileName), img);
                    modInfo.m_fileVersion = (u64)fileVersion;
                    modInfo.m_size = size;
                }
                else
                {
                    // Warn that we can't store information about all modules... if we move this AZCore we can allocate this dynamically.
                    AZ_TracePrintf("System", "Failed to store module information, increase size of g_moduleInfo to maintain stack tracer functionality!");
                }
            }
            return result;
        }

        // **************************************** ToolHelp32 ************************
#define MAX_MODULE_NAME32 255
#define TH32CS_SNAPMODULE   0x00000008
#pragma pack( push, 8 )
        typedef struct tagMODULEENTRY32
        {
            DWORD   dwSize;
            DWORD   th32ModuleID;   // This module
            DWORD   th32ProcessID;  // owning process
            DWORD   GlblcntUsage;   // Global usage count on the module
            DWORD   ProccntUsage;   // Module usage count in th32ProcessID's context
            BYTE* modBaseAddr;      // Base address of module in th32ProcessID's context
            DWORD   modBaseSize;    // Size in bytes of module starting at modBaseAddr
            HMODULE hModule;        // The hModule of this module in th32ProcessID's context
            char    szModule[MAX_MODULE_NAME32 + 1];
            char    szExePath[MAX_PATH];
        } MODULEENTRY32;
        typedef MODULEENTRY32*  PMODULEENTRY32;
        typedef MODULEENTRY32*  LPMODULEENTRY32;
#pragma pack( pop )

        bool GetModuleListTH32(HANDLE hProcess, DWORD pid)
        {
            // CreateToolhelp32Snapshot()
            typedef HANDLE (__stdcall * tCT32S)(DWORD dwFlags, DWORD th32ProcessID);
            // Module32First()
            typedef BOOL (__stdcall * tM32F)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);
            // Module32Next()
            typedef BOOL (__stdcall * tM32N)(HANDLE hSnapshot, LPMODULEENTRY32 lpme);

            // try both dlls...
            const TCHAR* dllname[] = { L"kernel32.dll", L"tlhelp32.dll" };
            HINSTANCE hToolhelp = NULL;
            tCT32S pCT32S = NULL;
            tM32F pM32F = NULL;
            tM32N pM32N = NULL;

            HANDLE hSnap;
            MODULEENTRY32 me;
            me.dwSize = sizeof(me);
            BOOL keepGoing;
            size_t i;

            for (i = 0; i < (sizeof(dllname) / sizeof(dllname[0])); i++)
            {
                hToolhelp = LoadLibrary(dllname[i]);
                if (hToolhelp == NULL)
                {
                    continue;
                }
                pCT32S = (tCT32S)GetProcAddress(hToolhelp, "CreateToolhelp32Snapshot");
                pM32F = (tM32F)GetProcAddress(hToolhelp, "Module32First");
                pM32N = (tM32N)GetProcAddress(hToolhelp, "Module32Next");
                if ((pCT32S != NULL) && (pM32F != NULL) && (pM32N != NULL))
                {
                    break; // found the functions!
                }
                FreeLibrary(hToolhelp);
                hToolhelp = NULL;
            }

            if (hToolhelp == NULL)
            {
                return FALSE;
            }
            hSnap = pCT32S(TH32CS_SNAPMODULE, pid);
            if (hSnap == (HANDLE) -1)
            {
                return FALSE;
            }
            keepGoing = !!pM32F(hSnap, &me);
            int cnt = 0;
            while (keepGoing)
            {
                LoadModule(hProcess, me.szExePath, me.szModule, (DWORD64) me.modBaseAddr, me.modBaseSize);
                cnt++;
                keepGoing = !!pM32N(hSnap, &me);
            }
            // Load dynamically loaded modules
            {
                // note, you may not call LoadModule while this mutex is locked since it may invoke the loader notify
                // which would also try to lock it!
                AZ::Debug::SymbolStorageDynamicallyLoadedModules loadedModules;
                {
                    // scoped for lock_guard
                    AZStd::lock_guard<AZStd::mutex> locker(AZ::Debug::GetDynamicallyLoadedModulesMutex());
                    // note well that this is operator=, we are copying
                    loadedModules = AZ::Debug::GetRegisteredLoadedModules();
                    m_numDynamicModulesLoaded = loadedModules.m_size;
                }
                
                for (int j = 0; j < loadedModules.m_size; ++j)
                {
                    AZ::Debug::DynamicallyLoadedModuleInfo& module = loadedModules.m_modules[j];
                    LoadModule(hProcess, module.m_fileName, module.m_name, (DWORD64) module.m_baseAddress, (DWORD) module.m_size);
                    cnt++;
                }
            }
            CloseHandle(hSnap);
            FreeLibrary(hToolhelp);
            if (cnt <= 0)
            {
                return FALSE;
            }
            return TRUE;
        } // GetModuleListTH32

        // **************************************** PSAPI ************************
        typedef struct _MODULEINFO
        {
            LPVOID lpBaseOfDll;
            DWORD SizeOfImage;
            LPVOID EntryPoint;
        } MODULEINFO, * LPMODULEINFO;

        BOOL GetModuleListPSAPI(HANDLE hProcess)
        {
            // EnumProcessModules()
            typedef BOOL (__stdcall * tEPM)(HANDLE hProcess, HMODULE* lphModule, DWORD cb, LPDWORD lpcbNeeded);
            // GetModuleFileNameEx()
            typedef DWORD (__stdcall * tGMFNE)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize);
            // GetModuleBaseName()
            typedef DWORD (__stdcall * tGMBN)(HANDLE hProcess, HMODULE hModule, LPSTR lpFilename, DWORD nSize);
            // GetModuleInformation()
            typedef BOOL (__stdcall * tGMI)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO pmi, DWORD nSize);

            HINSTANCE hPsapi;
            tEPM pEPM;
            tGMFNE pGMFNE;
            tGMBN pGMBN;
            tGMI pGMI;

            DWORD i;
            //ModuleEntry e;
            DWORD cbNeeded;
            MODULEINFO mi;
            HMODULE* hMods = 0;
            char* tt = NULL;
            char* tt2 = NULL;
            const SIZE_T TTBUFLEN = 8096;
            int cnt = 0;

            hPsapi = LoadLibraryW(L"psapi.dll");
            if (hPsapi == NULL)
            {
                return FALSE;
            }

            pEPM = (tEPM) GetProcAddress(hPsapi, "EnumProcessModules");
            pGMFNE = (tGMFNE) GetProcAddress(hPsapi, "GetModuleFileNameExA");
            pGMBN = (tGMFNE) GetProcAddress(hPsapi, "GetModuleBaseNameA");
            pGMI = (tGMI) GetProcAddress(hPsapi, "GetModuleInformation");
            if ((pEPM == NULL) || (pGMFNE == NULL) || (pGMBN == NULL) || (pGMI == NULL))
            {
                // we couldn't find all functions
                FreeLibrary(hPsapi);
                return FALSE;
            }

            hMods = (HMODULE*) malloc(sizeof(HMODULE) * (TTBUFLEN / sizeof(HMODULE)));
            tt = (char*) malloc(sizeof(char) * TTBUFLEN);
            tt2 = (char*) malloc(sizeof(char) * TTBUFLEN);
            if ((hMods == NULL) || (tt == NULL) || (tt2 == NULL))
            {
                goto cleanup;
            }

            if (!pEPM(hProcess, hMods, TTBUFLEN, &cbNeeded))
            {
                //_ftprintf(fLogFile, _T("%lu: EPM failed, GetLastError = %lu\n"), g_dwShowCount, gle );
                goto cleanup;
            }

            if (cbNeeded > TTBUFLEN)
            {
                //_ftprintf(fLogFile, _T("%lu: More than %lu module handles. Huh?\n"), g_dwShowCount, lenof( hMods ) );
                goto cleanup;
            }

            for (i = 0; i < cbNeeded / sizeof hMods[0]; i++)
            {
                // base address, size
                pGMI(hProcess, hMods[i], &mi, sizeof mi);
                // image file name
                tt[0] = 0;
                pGMFNE(hProcess, hMods[i], tt, TTBUFLEN);
                // module name
                tt2[0] = 0;
                pGMBN(hProcess, hMods[i], tt2, TTBUFLEN);

                DWORD dwRes = LoadModule(hProcess, tt, tt2, (DWORD64) mi.lpBaseOfDll, mi.SizeOfImage);
                if (dwRes != ERROR_SUCCESS)
                {
                    //AZ_ERROR()
                    //this->m_parent->OnDbgHelpErr("LoadModule", dwRes, 0);
                    return FALSE;
                }
                cnt++;
            }
            // Load dynamically loaded modules
            {
                // note, you may not call LoadModule while this mutex is locked since it may invoke the loader notify
                // which would also try to lock it!
                AZ::Debug::SymbolStorageDynamicallyLoadedModules loadedModules;
                {
                    // scoped for lock_guard
                    AZStd::lock_guard<AZStd::mutex> locker(AZ::Debug::GetDynamicallyLoadedModulesMutex());
                    // note well that this is operator=, we are copying
                    loadedModules = AZ::Debug::GetRegisteredLoadedModules();
                    m_numDynamicModulesLoaded = loadedModules.m_size;
                }
                for (int j = 0; j < loadedModules.m_size; ++j)
                {
                    AZ::Debug::DynamicallyLoadedModuleInfo& module = loadedModules.m_modules[j];
                    LoadModule(hProcess, module.m_fileName, module.m_name, (DWORD64) module.m_baseAddress, (DWORD) module.m_size);
                    cnt++;
                }
            }

cleanup:
            if (hPsapi != NULL)
            {
                FreeLibrary(hPsapi);
            }
            if (tt2 != NULL)
            {
                free(tt2);
            }
            if (tt != NULL)
            {
                free(tt);
            }
            if (hMods != NULL)
            {
                free(hMods);
            }

            return cnt != 0;
        } // GetModuleListPSAPI



        BOOL LoadModules(HANDLE hProcess, DWORD dwProcessId)
        {
            // first try toolhelp32
            if (GetModuleListTH32(hProcess, dwProcessId))
            {
                return TRUE;
            }
            // then try psapi
            return GetModuleListPSAPI(hProcess);
        }
    };

    WindowsSymbols& GetSymbols()
    {
        static WindowsSymbols windowsSymbols;
        return windowsSymbols;
    }

    //=========================================================================
    // Load DbgHelp.dll and the functions we need from it.
    // [7/29/2009]
    //=========================================================================
    void    LoadDbgHelp()
    {
        AZStd::lock_guard<AZStd::mutex> lock(g_dbgLoadingMutex);

        if (!g_dbgHelpLoaded)
        {
            if (g_dbgHelpDll == NULL) // if not already loaded, try to load a default-one
            {
                // In some scenarios we may have built on a machine that is < Win10 but targetting Win10
                // In that scenario, we may try to load and older dbghelp.dll which could cause issues
                // To overcome this, we try to load dbghelp.dll from the Win 10 SDK folder, if that doesn't
                // work, load the default.
                g_dbgHelpDll = LoadLibraryW(LR"(C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\dbghelp.dll)");
                if (g_dbgHelpDll == NULL)
                {
                    g_dbgHelpDll = LoadLibrary(L"dbghelp.dll");
                }
            }
            if (g_dbgHelpDll == NULL)
            {
                return;
            }

            LOAD_FUNCTION(SymInitialize);
            LOAD_FUNCTION(SymCleanup);
            LOAD_FUNCTION(SymFunctionTableAccess64);
            LOAD_FUNCTION(SymGetLineFromAddr64);
            LOAD_FUNCTION(SymGetModuleBase64);
            LOAD_FUNCTION(SymGetModuleInfo64);
            LOAD_FUNCTION(SymGetOptions);
            LOAD_FUNCTION(SymGetSymFromAddr64);
            LOAD_FUNCTION(SymLoadModule64);
            LOAD_FUNCTION(SymSetOptions);
            LOAD_FUNCTION(StackWalk64);
            LOAD_FUNCTION(UnDecorateSymbolName);
            LOAD_FUNCTION(SymGetSearchPath);
            LOAD_FUNCTION(SymRegisterCallback64);
            g_currentProcess = GetCurrentProcess();

            InitializeCriticalSection(&g_csDbgHelpDll);
            // In memory allocation case (usually tools) we might have high contention,
            // using spin lock will improve performance.
            SetCriticalSectionSpinCount(&g_csDbgHelpDll, 4000);

            // We need to load the symbols even if we just do back-trace recording.
            GetSymbols().LoadSymbols();

            g_dbgHelpLoaded = true;
        }
    }

    //=========================================================================
    // Free dbghelp.dll
    // [7/29/2009]
    //=========================================================================
    void    FreeDbgHelp()
    {
        if (g_dbgHelpLoaded)
        {
            GetSymbols().CleanUpSymbols();

            FreeLibrary(g_dbgHelpDll);
            g_dbgHelpDll = 0;

            g_SymInitialize = 0;
            g_SymCleanup = 0;
            g_SymFunctionTableAccess64 = 0;
            g_SymGetLineFromAddr64 = 0;
            g_SymGetModuleBase64 = 0;
            g_SymGetModuleInfo64 = 0;
            g_SymGetOptions = 0;
            g_SymGetSymFromAddr64 = 0;
            g_SymLoadModule64 = 0;
            g_SymSetOptions = 0;
            g_StackWalk64 = 0;
            g_UnDecorateSymbolName = 0;
            g_SymGetSearchPath = 0;

            g_currentProcess = 0;

            DeleteCriticalSection(&g_csDbgHelpDll);

            g_dbgHelpLoaded = false;
        }
    }

#else
    using namespace AZ::Debug;
#endif

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Stack Recorder
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    // Record
    // [7/29/2009]
    //=========================================================================
    unsigned int
    StackRecorder::Record(StackFrame* frames, unsigned int maxNumOfFrames, unsigned int suppressCount, void* nativeThread)
    {
        unsigned int numFrames = 0;

#if defined(AZ_ENABLE_DEBUG_TOOLS)
        if (nativeThread == NULL)
        {
            ++suppressCount; // Skip current call
            PVOID myFrames[50] = { nullptr };

            static_assert(AZ_ARRAY_SIZE(myFrames) <= 0xffff, "RtlCaptureStackBackTrace only supports a maximum of MAXUSHORT (0xffff) frames captured.");
            AZ_Assert(maxNumOfFrames <= AZ_ARRAY_SIZE(myFrames), "You need to increase the size of myFrames array %u (needed %zu)!", maxNumOfFrames, AZ_ARRAY_SIZE(myFrames));
            
            numFrames = RtlCaptureStackBackTrace(suppressCount, maxNumOfFrames, myFrames, NULL);
            for (unsigned int frame = 0; frame < numFrames; ++frame)
            {
                frames[frame].m_programCounter = (uintptr_t)myFrames[frame];
            }
        }
        else
        {
            if (!g_dbgHelpLoaded)
            {
                LoadDbgHelp();
            }

            HANDLE hThread = nativeThread;
            CONTEXT alignas(8) context; // Without this alignment the function randomly crashes in release.
            context.ContextFlags = CONTEXT_ALL;
            GetThreadContext(hThread, &context);

            STACKFRAME64 sf;
            memset(&sf, 0, sizeof(STACKFRAME64));
            DWORD imageType = IMAGE_FILE_MACHINE_AMD64;

            sf.AddrPC.Offset = context.Rip;
            sf.AddrPC.Mode = AddrModeFlat;
            sf.AddrFrame.Offset = context.Rsp;
            sf.AddrFrame.Mode = AddrModeFlat;
            sf.AddrStack.Offset = context.Rsp;
            sf.AddrStack.Mode = AddrModeFlat;

            EnterCriticalSection(&g_csDbgHelpDll);
            for (s32 frame = -static_cast<s32>(suppressCount); frame < static_cast<s32>(maxNumOfFrames); ++frame)
            {
                if (!g_StackWalk64(imageType, g_currentProcess, hThread, &sf, &context, 0, g_SymFunctionTableAccess64, g_SymGetModuleBase64, 0))
                {
                    break;
                }

                if (sf.AddrPC.Offset == sf.AddrReturn.Offset)
                {
                    // "StackWalk64-Endless-Callstack!"
                    break;
                }

                if (frame >= 0)
                {
                    frames[numFrames++].m_programCounter = sf.AddrPC.Offset;
                }
            }

            LeaveCriticalSection(&g_csDbgHelpDll);
        }        
#else
        AZ_UNUSED(frames);
        AZ_UNUSED(maxNumOfFrames);
        AZ_UNUSED(suppressCount);
        AZ_UNUSED(nativeThread);
#endif // AZ_ENABLE_DEBUG_TOOLS

        return numFrames;
    }

    unsigned int StackConverter::FromNative(StackFrame* frames, unsigned int maxNumOfFrames, void* nativeContext)
    {
        unsigned int numFrames = 0;

#if defined(AZ_ENABLE_DEBUG_TOOLS)
        if (!g_dbgHelpLoaded)
        {
            LoadDbgHelp();
        }

        HANDLE hThread;
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hThread, 0, false, DUPLICATE_SAME_ACCESS);       

        PCONTEXT nativeContextType = reinterpret_cast<PCONTEXT>(nativeContext);
        STACKFRAME64 sf;
        memset(&sf, 0, sizeof(STACKFRAME64));

        DWORD imageType = IMAGE_FILE_MACHINE_AMD64;

        sf.AddrPC.Offset = nativeContextType->Rip;
        sf.AddrPC.Mode = AddrModeFlat;
        sf.AddrFrame.Offset = nativeContextType->Rsp;
        sf.AddrFrame.Mode = AddrModeFlat;
        sf.AddrStack.Offset = nativeContextType->Rsp;
        sf.AddrStack.Mode = AddrModeFlat;

        EnterCriticalSection(&g_csDbgHelpDll);
        for (unsigned int frame = 0; frame < maxNumOfFrames; ++frame)
        {
            if (!g_StackWalk64(imageType, g_currentProcess, hThread, &sf, nativeContext, 0, g_SymFunctionTableAccess64, g_SymGetModuleBase64, 0))
            {
                break;
            }

            if (sf.AddrPC.Offset == sf.AddrReturn.Offset)
            {
                // "StackWalk64-Endless-Callstack!"
                break;
            }

            frames[numFrames++].m_programCounter = sf.AddrPC.Offset;
        }

        LeaveCriticalSection(&g_csDbgHelpDll);
#else
        AZ_UNUSED(frames);
        AZ_UNUSED(maxNumOfFrames);
        AZ_UNUSED(nativeContext);
#endif

        return numFrames;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Symbol Storage
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //=========================================================================
    // SymbolStorage
    // [7/29/2009]
    //=========================================================================
    void SymbolStorage::LoadModuleData(const void* moduleInfoData, unsigned int moduleInfoDataSize)
    {
#if defined(AZ_DEBUG_BUILD)
        LoadDbgHelp();
        (void)moduleInfoData;
        (void)moduleInfoDataSize;
#else
        (void)moduleInfoData;
        (void)moduleInfoDataSize;
#endif
    }

    //=========================================================================
    // StoreModuleInfoData
    // [7/29/2009]
    //=========================================================================
    void
    SymbolStorage::StoreModuleInfoData(void*, unsigned int)
    {
    }


    //=========================================================================
    // GetNumLoadedModules
    // [7/29/2009]
    //=========================================================================
    unsigned int
    SymbolStorage::GetNumLoadedModules()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        if (!g_dbgHelpLoaded)
        {
            LoadDbgHelp();
        }

        return (unsigned int)g_moduleInfo.size();
#else
        return 0;
#endif
    }

    //=========================================================================
    // GetModuleInfo
    // [7/29/2009]
    //=========================================================================
    const SymbolStorage::ModuleInfo*
    SymbolStorage::GetModuleInfo(unsigned int moduleIndex)
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        if (!g_dbgHelpLoaded)
        {
            LoadDbgHelp();
        }

        return &g_moduleInfo[moduleIndex];
#else
        (void)moduleIndex;
        return 0;
#endif
    }

    //=========================================================================
    // SetMapFilename
    // [7/29/2009]
    //=========================================================================
    void
    SymbolStorage::SetMapFilename(const char* fileName)
    {
        (void)fileName;
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        GetSymbols().SetSymbolPath(fileName);

        if (g_dbgHelpLoaded)
        {
            FreeDbgHelp();
        }
        LoadDbgHelp();
#endif
    }

    //=========================================================================
    // GetMapFilename
    // [7/29/2009]
    //=========================================================================
    const char*
    SymbolStorage::GetMapFilename()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        return GetSymbols().GetSymbolPath();
#else
        return NULL;
#endif
    }

    //=========================================================================
    // RegisterModuleListeners
    //=========================================================================
    void
    SymbolStorage::RegisterModuleListeners()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        GetSymbols().RegisterModuleListeners();
#endif
    }

    //=========================================================================
    // UnregisterModuleListeners
    //=========================================================================
    void
    SymbolStorage::UnregisterModuleListeners()
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        GetSymbols().UnregisterModuleListeners();
#endif
    }

    //=========================================================================
    // DecodeFrames
    // [7/29/2009]
    //=========================================================================
    void
    SymbolStorage::DecodeFrames(const StackFrame* frames, unsigned int numFrames, StackLine* textLines)
    {
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        if (!g_dbgHelpLoaded)
        {
            LoadDbgHelp();
        }

        WindowsSymbols& symbols = GetSymbols();
        if (symbols.IsSymLoaded())
        {
            if (symbols.ModuleSymbolsNeedRefreshed())
            {
                symbols.RefreshModuleSymbols();
            }
            symbols.DecodeFrames(frames, numFrames, textLines);
        }

#else
        (void)frames;
        (void)numFrames;
        (void)textLines;
#endif
    }

    //=========================================================================
    // FindFunctionFromIP
    // [4/25/2018]
    //=========================================================================
    void
    SymbolStorage::FindFunctionFromIP(void* address, StackLine* func, StackLine* file, StackLine* module, int& line, void*& baseAddr)
    {
        *func[0] = '\0';
        *file[0] = '\0';
        *module[0] = '\0';
        line = 0;
        baseAddr = address;
#if defined(AZ_ENABLE_DEBUG_TOOLS)
        if (!g_dbgHelpLoaded)
        {
            LoadDbgHelp();
        }

        WindowsSymbols& symbols = GetSymbols();
        if (symbols.IsSymLoaded())
        {
            if (symbols.ModuleSymbolsNeedRefreshed())
            {
                symbols.RefreshModuleSymbols();
            }
            symbols.FindFunctionFromIP(address, func, file, module, line, baseAddr);
        }
#endif
    }
} // namespace AZ
