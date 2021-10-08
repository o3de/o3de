/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <platform.h>
#include <ISystem.h>
#include <Random.h>
#include <IConsole.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/Utils.h>


// Section dictionary
#if defined(AZ_RESTRICTED_PLATFORM)
#define PLATFORM_IMPL_H_SECTION_TRAITS 1
#define PLATFORM_IMPL_H_SECTION_CRYLOWLATENCYSLEEP 2
#define PLATFORM_IMPL_H_SECTION_CRYGETFILEATTRIBUTES 3
#define PLATFORM_IMPL_H_SECTION_CRYSETFILEATTRIBUTES 4
#define PLATFORM_IMPL_H_SECTION_CRY_FILE_ATTRIBUTE_STUBS 5
#define PLATFORM_IMPL_H_SECTION_CRY_SYSTEM_FUNCTIONS 6
#define PLATFORM_IMPL_H_SECTION_VIRTUAL_ALLOCATORS 7
#endif

struct SSystemGlobalEnvironment* gEnv = nullptr;

// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_IMPL_H_SECTION_TRAITS
    #include AZ_RESTRICTED_FILE(platform_impl_h)
#endif

#if defined(WIN32) || defined(WIN64)
void CryPureCallHandler()
{
    CryFatalError("Pure function call");
}

void CryInvalidParameterHandler(
    [[maybe_unused]] const wchar_t* expression,
    [[maybe_unused]] const wchar_t* function,
    [[maybe_unused]] const wchar_t* file,
    [[maybe_unused]] unsigned int line,
    [[maybe_unused]] uintptr_t pReserved)
{
    //size_t i;
    //char sFunc[128];
    //char sExpression[128];
    //char sFile[128];
    //wcstombs_s( &i,sFunc,sizeof(sFunc),function,_TRUNCATE );
    //wcstombs_s( &i,sExpression,sizeof(sExpression),expression,_TRUNCATE );
    //wcstombs_s( &i,sFile,sizeof(sFile),file,_TRUNCATE );
    //CryFatalError( "Invalid parameter detected in function %s. File: %s Line: %d, Expression: %s",sFunc,sFile,line,sExpression );
    CryFatalError("Invalid parameter detected in CRT function\n");
}

void InitCRTHandlers()
{
    _set_purecall_handler(CryPureCallHandler);
    _set_invalid_parameter_handler(CryInvalidParameterHandler);
}
#else
void InitCRTHandlers() {}
#endif

//////////////////////////////////////////////////////////////////////////
// This is an entry to DLL initialization function that must be called for each loaded module
//////////////////////////////////////////////////////////////////////////
extern "C" AZ_DLL_EXPORT void ModuleInitISystem(ISystem* pSystem, [[maybe_unused]] const char* moduleName)
{
    if (gEnv) // Already registered.
    {
        return;
    }

    InitCRTHandlers();

    if (pSystem) // DONT REMOVE THIS. ITS FOR RESOURCE COMPILER!!!!
    {
        gEnv = pSystem->GetGlobalEnvironment();
        assert(gEnv);
        
        if (!AZ::Environment::IsReady() || (AZ::Environment::GetInstance() != gEnv->pSharedEnvironment))
        {
            AZ::Environment::Attach(gEnv->pSharedEnvironment);
            AZ::AllocatorManager::Instance();  // Force the AllocatorManager to instantiate and register any allocators defined in data sections
        }
    } // if pSystem
}

extern "C" AZ_DLL_EXPORT void ModuleShutdownISystem([[maybe_unused]] ISystem* pSystem)
{
    // Unregister with AZ environment.
    AZ::Environment::Detach();
}

extern "C" AZ_DLL_EXPORT void InjectEnvironment(void* env)
{
    static bool injected = false;
    if (!injected)
    {
        AZ::Environment::Attach(reinterpret_cast<AZ::EnvironmentInstance>(env));
        AZ::AllocatorManager::Instance();  // Force the AllocatorManager to instantiate and register any allocators defined in data sections
        injected = true;
    }
}

extern "C" AZ_DLL_EXPORT void DetachEnvironment()
{
    AZ::Environment::Detach();
}

void* GetModuleInitISystemSymbol()
{
    return reinterpret_cast<void*>(&ModuleInitISystem);
}
void* GetModuleShutdownISystemSymbol()
{
    return reinterpret_cast<void*>(&ModuleShutdownISystem);
}
void* GetInjectEnvironmentSymbol()
{
    return reinterpret_cast<void*>(&InjectEnvironment);
}
void* GetDetachEnvironmentSymbol()
{
    return reinterpret_cast<void*>(&DetachEnvironment);
}

bool g_bProfilerEnabled = false;

//////////////////////////////////////////////////////////////////////////
// global random number generator used by cry_random functions
CRndGen CryRandom_Internal::g_random_generator;


//////////////////////////////////////////////////////////////////////////

// when using STL Port _STLP_DEBUG and _STLP_DEBUG_TERMINATE - avoid actually
// crashing (default terminator seems to kill the thread, which isn't nice).
#ifdef _STLP_DEBUG_TERMINATE

# ifdef __stl_debug_terminate
#  undef __stl_debug_terminate
# endif

void __stl_debug_terminate(void)
{
    assert(0 && "STL Debug Error");
}
#endif

#ifdef _STLP_DEBUG_MESSAGE

# ifdef __stl_debug_message
#  undef __stl_debug_message
# endif

void __stl_debug_message(const char* format_str, ...)
{
    va_list __args;
    va_start(__args, format_str);
#ifdef WIN32
    char __buffer [4096];
    azvsnprintf(__buffer, sizeof(__buffer) / sizeof(char), format_str, __args);
    OutputDebugStringA(__buffer);
#endif
    gEnv->pLog->LogV(ILog::eErrorAlways, format_str, __args);
    va_end(__args);
}
#endif //_STLP_DEBUG_MESSAGE


#if defined(WIN32) || defined(WIN64)
#include <intrin.h>
#endif

#if defined(APPLE) || defined(LINUX)
#include "CryAssert_impl.h"
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_IMPL_H_SECTION_CRY_SYSTEM_FUNCTIONS
    #include AZ_RESTRICTED_FILE(platform_impl_h)
#endif

#if defined (_WIN32)

#include "CryAssert_impl.h"

//////////////////////////////////////////////////////////////////////////
void CrySleep(unsigned int dwMilliseconds)
{
    AZ_PROFILE_FUNCTION(System);
    Sleep(dwMilliseconds);
}

//////////////////////////////////////////////////////////////////////////
int CryMessageBox([[maybe_unused]] const char* lpText, [[maybe_unused]] const char* lpCaption, [[maybe_unused]] unsigned int uType)
{
#ifdef WIN32
    ICVar* const pCVar = gEnv && gEnv->pConsole ? gEnv->pConsole->GetCVar("sys_no_crash_dialog") : NULL;
    if ((pCVar && pCVar->GetIVal() != 0) || (gEnv && gEnv->bNoAssertDialog))
    {
        return 0;
    }
    AZStd::wstring lpTextW;
    AZStd::to_wstring(lpTextW, lpText);
    AZStd::wstring lpCaptionW;
    AZStd::to_wstring(lpCaptionW, lpCaption);
    return MessageBoxW(NULL, lpTextW.c_str(), lpCaptionW.c_str(), uType);
#else
    return 0;
#endif
}

// Initializes root folder of the game, optionally returns exe and path name.
void InitRootDir(char szExeFileName[], uint nExeSize, char szExeRootName[], uint nRootSize)
{
    char szPath[_MAX_PATH];
    AZ::Utils::GetExecutablePathReturnType ret = AZ::Utils::GetExecutablePath(szPath, _MAX_PATH);
    AZ_Assert(ret.m_pathStored == AZ::Utils::ExecutablePathResult::Success, "The path to the current executable exceeds the expected length");
    const size_t nLen = strnlen(szPath, _MAX_PATH);

    // Find path above exe name and deepest folder.
    bool firstIteration = true;
    for (size_t n = nLen - 1; n > 0; n--)
    {
        if (szPath[n] == '\\')
        {
            szPath[n] = 0;

            if (firstIteration)
            {
                // Return exe path
                if (szExeRootName)
                {
                    azstrncpy(szExeRootName, nRootSize, szPath + n + 1, nLen - n - 1);
                }

                // Return exe name
                if (szExeFileName)
                {
                    azstrncpy(szExeFileName, nExeSize, szPath + n, nLen - n);
                }
                firstIteration = false;
            }
            // Check if the engineroot exists
            azstrcat(szPath, AZ_ARRAY_SIZE(szPath), "\\engine.json");
            WIN32_FILE_ATTRIBUTE_DATA data;
            wchar_t szPathW[_MAX_PATH];
            AZStd::to_wstring(szPathW, _MAX_PATH, szPath);
            BOOL res = GetFileAttributesExW(szPathW, GetFileExInfoStandard, &data);
            if (res != 0 && data.dwFileAttributes != INVALID_FILE_ATTRIBUTES)
            {
                // Found file
                szPath[n] = 0;
                SetCurrentDirectoryW(szPathW);
                break;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CrySetFileAttributes(const char* lpFileName, uint32 dwFileAttributes)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_IMPL_H_SECTION_CRYSETFILEATTRIBUTES
    #include AZ_RESTRICTED_FILE(platform_impl_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    AZStd::wstring lpFileNameW;
    AZStd::to_wstring(lpFileNameW, lpFileName);
    return SetFileAttributes(lpFileNameW.c_str(), dwFileAttributes) != 0;
#endif
}

//////////////////////////////////////////////////////////////////////////
threadID CryGetCurrentThreadId()
{
    return GetCurrentThreadId();
}

#endif // _WIN32

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_IMPL_H_SECTION_CRY_FILE_ATTRIBUTE_STUBS
    #include AZ_RESTRICTED_FILE(platform_impl_h)
#endif

#if defined(AZ_PLATFORM_WINDOWS)
int64 CryGetTicks()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

int64 CryGetTicksPerSec()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Threads implementation. For static linking it must be declared inline otherwise creating multiple symbols
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define THR_INLINE inline

//////////////////////////////////////////////////////////////////////////
inline void CryDebugStr([[maybe_unused]] const char* format, ...)
{
    /*
     #ifdef CRYSYSTEM_EXPORTS
     va_list    ArgList;
     char       szBuffer[65535];
     va_start(ArgList, format);
     azvsnprintf(szBuffer,sizeof(szBuffer)-1, format, ArgList);
     va_end(ArgList);
     azstrcat(szBuffer,"\n");
     OutputDebugString(szBuffer);
     #endif
     */
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION PLATFORM_IMPL_H_SECTION_VIRTUAL_ALLOCATORS
    #include AZ_RESTRICTED_FILE(platform_impl_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#endif
