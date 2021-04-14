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
#include "SystemInit.h"

#if defined(AZ_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#undef AZ_RESTRICTED_SECTION
#define SYSTEMINIT_CPP_SECTION_1 1
#define SYSTEMINIT_CPP_SECTION_2 2
#define SYSTEMINIT_CPP_SECTION_3 3
#define SYSTEMINIT_CPP_SECTION_4 4
#define SYSTEMINIT_CPP_SECTION_5 5
#define SYSTEMINIT_CPP_SECTION_6 6
#define SYSTEMINIT_CPP_SECTION_7 7
#define SYSTEMINIT_CPP_SECTION_8 8
#define SYSTEMINIT_CPP_SECTION_9 9
#define SYSTEMINIT_CPP_SECTION_10 10
#define SYSTEMINIT_CPP_SECTION_11 11
#define SYSTEMINIT_CPP_SECTION_12 12
#define SYSTEMINIT_CPP_SECTION_13 13
#define SYSTEMINIT_CPP_SECTION_14 14
#define SYSTEMINIT_CPP_SECTION_15 15
#define SYSTEMINIT_CPP_SECTION_16 16
#define SYSTEMINIT_CPP_SECTION_17 17
#endif

#if defined(MAP_LOADING_SLICING)
#include "SystemScheduler.h"
#endif // defined(MAP_LOADING_SLICING)
#include "CryLibrary.h"
#include "CryPath.h"
#include <StringUtils.h>
#include <IThreadManager.h>

#include "CryFontBus.h"

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <IEngineModule.h>
#include <CryExtension/CryCreateClassInstance.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/API/ApplicationAPI.h>
#include "AZCoreLogSink.h"
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Archive/INestedArchive.h>
#include <AzFramework/Archive/ArchiveFileIO.h>

#include <LoadScreenBus.h>
#include <LyShine/Bus/UiSystemBus.h>
#include <AzFramework/Logging/MissingAssetLogger.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/API/AtomActiveInterface.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/Utils.h>

#if defined(APPLE) || defined(LINUX)
#include <dlfcn.h>
#include <cstdlib>
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include <float.h>

// To enable profiling with vtune (https://software.intel.com/en-us/intel-vtune-amplifier-xe), make sure the line below is not commented out
//#define  PROFILE_WITH_VTUNE

#endif //WIN32

#include <I3DEngine.h>
#include <IRenderer.h>
#include <AzCore/IO/FileIO.h>
#include <IMovieSystem.h>
#include <ILog.h>
#include <IAudioSystem.h>
#include <ICmdLine.h>
#include <IProcess.h>
#include <LyShine/ILyShine.h>
#include <HMDBus.h>
#include "HMDCVars.h"

#include <AzFramework/Archive/Archive.h>
#include <Pak/CryPakUtils.h>
#include "XConsole.h"
#include "Log.h"
#include "XML/xml.h"
#include "StreamEngine/StreamEngine.h"
#include "PhysRenderer.h"
#include "LocalizedStringManager.h"
#include "SystemEventDispatcher.h"
#include "Statistics/LocalMemoryUsage.h"
#include "ThreadConfigManager.h"
#include "Validator.h"
#include "ServerThrottle.h"
#include "SystemCFG.h"
#include "AutoDetectSpec.h"
#include "ResourceManager.h"
#include "VisRegTest.h"
#include "MTSafeAllocator.h"
#include "NotificationNetwork.h"
#include "ExtensionSystem/CryFactoryRegistryImpl.h"
#include "ExtensionSystem/TestCases/TestExtensions.h"
#include "ProfileLogSystem.h"
#include "SoftCode/SoftCodeMgr.h"
#include "ZLibCompressor.h"
#include "ZLibDecompressor.h"
#include "ZStdDecompressor.h"
#include "LZ4Decompressor.h"
#include "OverloadSceneManager/OverloadSceneManager.h"
#include "ServiceNetwork.h"
#include "RemoteCommand.h"
#include "LevelSystem/LevelSystem.h"
#include "LevelSystem/SpawnableLevelSystem.h"
#include "ViewSystem/ViewSystem.h"
#include <CrySystemBus.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzFramework/Driller/DrillerConsoleAPI.h>

#include "PerfHUD.h"
#include "MiniGUI/MiniGUI.h"

#if USE_STEAM
#include "Steamworks/public/steam/steam_api.h"
#include "Steamworks/public/steam/isteamremotestorage.h"
#endif

#if defined(IOS)
#include "IOSConsole.h"
#endif

#if defined(ANDROID)
    #include <AzCore/Android/Utils.h>
    #include "AndroidConsole.h"
#if !defined(AZ_RELEASE_BUILD)
    #include "ThermalInfoAndroid.h"
#endif // !defined(AZ_RELEASE_BUILD)
#endif

#if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_IOS)
#include "MobileDetectSpec.h"
#endif

#include "IDebugCallStack.h"

#include "WindowsConsole.h"

#if defined(EXTERNAL_CRASH_REPORTING)
#include <CrashHandler.h>
#endif

// select the asset processor based on cvars and defines.
// uncomment the following and edit the path where it is instantiated if you'd like to use the test file client
//#define USE_TEST_FILE_CLIENT

#if defined(REMOTE_ASSET_PROCESSOR)
// Over here, we'd put the header to the Remote Asset Processor interface (as opposed to the Local built in version  below)
#   include <AzFramework/Network/AssetProcessorConnection.h>
#endif

// if we enable the built-in local version instead of remote:
#if defined(CRY_ENABLE_RC_HELPER)
#include "ResourceCompilerHelper.h"
#endif

#ifdef WIN32
extern LONG WINAPI CryEngineExceptionFilterWER(struct _EXCEPTION_POINTERS* pExceptionPointers);
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_14
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif

#if AZ_TRAIT_USE_CRY_SIGNAL_HANDLER

#include <execinfo.h>
#include <signal.h>
void CryEngineSignalHandler(int signal)
{
    char resolvedPath[_MAX_PATH];

    // it is assumed that @log@ points at the appropriate place (so for apple, to the user profile dir)
    if (AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath("@log@/crash.log", resolvedPath, _MAX_PATH))
    {
        fprintf(stderr, "Crash Signal Handler - logged to %s\n", resolvedPath);
        FILE* file = fopen(resolvedPath, "a");
        if (file)
        {
            char sTime[128];
            time_t ltime;
            time(&ltime);
            struct tm* today = localtime(&ltime);
            strftime(sTime, 40, "<%Y-%m-%d %H:%M:%S> ", today);
            fprintf(file, "%s: Error: signal %s:\n", sTime, strsignal(signal));
            fflush(file);
            void* array[100];
            int s = backtrace(array, 100);
            backtrace_symbols_fd(array, s, fileno(file));
            fclose(file);
            CryLogAlways("Successfully recorded crash file:  '%s'", resolvedPath);
            abort();
        }
    }

    CryLogAlways("Could not record crash file...");
    abort();
}

#endif // AZ_TRAIT_USE_CRY_SIGNAL_HANDLER

#if defined(USE_UNIXCONSOLE)
#if defined(LINUX) && !defined(ANDROID)
CUNIXConsole* pUnixConsole;
#endif
#endif // USE_UNIXCONSOLE

//////////////////////////////////////////////////////////////////////////
#define DEFAULT_LOG_FILENAME "@log@/Log.txt"

#define CRYENGINE_ENGINE_FOLDER "Engine"

//////////////////////////////////////////////////////////////////////////
#define CRYENGINE_DEFAULT_LOCALIZATION_LANG "en-US"

#define LOCALIZATION_TRANSLATIONS_LIST_FILE_NAME "Libs/Localization/localization.xml"

#define LOAD_LEGACY_RENDERER_FOR_EDITOR true // If you set this to false you must for now also set 'ed_useAtomNativeViewport' to true (see /Code/Sandbox/Editor/ViewManager.cpp)
#define LOAD_LEGACY_RENDERER_FOR_LAUNCHER false

//////////////////////////////////////////////////////////////////////////
// Where possible, these are defaults used to initialize cvars
// System.cfg can then be used to override them
// This includes the Game DLL, although it is loaded elsewhere

#define DLL_FONT           "CryFont"
#define DLL_3DENGINE       "Cry3DEngine"
#define DLL_RENDERER_DX9   "CryRenderD3D9"
#define DLL_RENDERER_DX11  "CryRenderD3D11"
#define DLL_RENDERER_DX12  "CryRenderD3D12"
#define DLL_RENDERER_METAL "CryRenderMetal"
#define DLL_RENDERER_GL    "CryRenderGL"
#define DLL_RENDERER_NULL  "CryRenderNULL"
#define DLL_SHINE          "LyShine"

//////////////////////////////////////////////////////////////////////////
#if defined(WIN32) || defined(LINUX) || defined(APPLE)
#   define DLL_MODULE_INIT_ISYSTEM "ModuleInitISystem"
#   define DLL_MODULE_SHUTDOWN_ISYSTEM "ModuleShutdownISystem"
#   define DLL_INITFUNC_RENDERER "PackageRenderConstructor"
#   define DLL_INITFUNC_SOUND "CreateSoundSystem"
#   define DLL_INITFUNC_FONT "CreateCryFontInterface"
#   define DLL_INITFUNC_3DENGINE "CreateCry3DEngine"
#   define DLL_INITFUNC_UI "CreateLyShineInterface"
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_1
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#   define DLL_MODULE_INIT_ISYSTEM (LPCSTR)2
#   define DLL_MODULE_SHUTDOWN_ISYSTEM (LPCSTR)3
#   define DLL_INITFUNC_RENDERER  (LPCSTR)1
#   define DLL_INITFUNC_RENDERER  (LPCSTR)1
#   define DLL_INITFUNC_SOUND     (LPCSTR)1
#   define DLL_INITFUNC_PHYSIC    (LPCSTR)1
#   define DLL_INITFUNC_FONT      (LPCSTR)1
#   define DLL_INITFUNC_3DENGINE  (LPCSTR)1
#   define DLL_INITFUNC_UI        (LPCSTR)1
#endif

#define AZ_TRACE_SYSTEM_WINDOW AZ::Debug::Trace::GetDefaultSystemWindow()

extern CMTSafeHeap* g_pPakHeap;

#ifdef WIN32
extern HMODULE gDLLHandle;
#endif

namespace
{
#if defined(AZ_PLATFORM_WINDOWS)
    // on windows, we lock our cache using a lockfile.  On other platforms this is not necessary since devices like ios, android, consoles cannot
    // run more than one game process that uses the same folder anyway.
    HANDLE g_cacheLock = INVALID_HANDLE_VALUE;
#endif
}

//static int g_sysSpecChanged = false;

struct SCVarsClientConfigSink
    : public ILoadConfigurationEntrySink
{
    virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, [[maybe_unused]] const char* szGroup)
    {
        gEnv->pConsole->SetClientDataProbeString(szKey, szValue);
    }
};

//////////////////////////////////////////////////////////////////////////
static inline void InlineInitializationProcessing([[maybe_unused]] const char* sDescription)
{
    assert(CryMemory::IsHeapValid());
    if (gEnv->pLog)
    {
        gEnv->pLog->UpdateLoadingScreen(0);
    }
}

//////////////////////////////////////////////////////////////////////////
AZ_PUSH_DISABLE_WARNING(4723, "-Wunknown-warning-option") // potential divide by 0 (needs to wrap the function)
static void CmdCrashTest(IConsoleCmdArgs* pArgs)
{
    assert(pArgs);

    if (pArgs->GetArgCount() == 2)
    {
        //This method intentionally crashes, a lot.

        int crashType = atoi(pArgs->GetArg(1));
        switch (crashType)
        {
        case 1:
        {
            int* p = 0;
            PREFAST_SUPPRESS_WARNING(6011) * p = 0xABCD;
        }
        break;
        case 2:
        {
            float a = 1.0f;
            memset(&a, 0, sizeof(a));
            float* b = &a;
            float c = 3;
            CryLog("%f", (c / *b));
        }
        break;
        case 3:
            while (true)
            {
                new char[10240];
            }
            break;
        case 4:
            CryFatalError("sys_crashtest 4");
            break;
        case 5:
            while (true)
            {
                new char[128];     //testing the crash handler an exception in the cry memory allocation occurred
            }
        case 6:
        {
            AZ_Assert(false, "Testing assert for testing crashes");
        }
        break;
        case 7:
            __debugbreak();
            break;
        case 8:
            CrySleep(1000 * 60 * 10);
            break;
        }
    }
}
AZ_POP_DISABLE_WARNING

#if USE_STEAM
//////////////////////////////////////////////////////////////////////////
static void CmdWipeSteamCloud(IConsoleCmdArgs* pArgs)
{
    if (!gEnv->pSystem->SteamInit())
    {
        return;
    }

    int32 fileCount = SteamRemoteStorage()->GetFileCount();
    for (int i = 0; i < fileCount; i++)
    {
        int32 size = 0;
        const char* name = SteamRemoteStorage()->GetFileNameAndSize(i, &size);
        bool success = SteamRemoteStorage()->FileDelete(name);
        CryLog("Deleting file: %s - success: %d", name, success);
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
struct SysSpecOverrideSink
    : public ILoadConfigurationEntrySink
{
    virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup)
    {
        ICVar* pCvar = gEnv->pConsole->GetCVar(szKey);

        if (pCvar)
        {
            const bool wasNotInConfig = ((pCvar->GetFlags() & VF_WASINCONFIG) == 0);
            bool applyCvar = wasNotInConfig;
            if (applyCvar == false)
            {
                // Special handling for sys_spec_full
                if (azstricmp(szKey, "sys_spec_full") == 0)
                {
                    // If it is set to 0 then ignore this request to set to something else
                    // If it is set to 0 then the user wants to changes system spec settings in system.cfg
                    if (pCvar->GetIVal() != 0)
                    {
                        applyCvar = true;
                    }
                }
                else
                {
                    // This could bypass the restricted/whitelisted cvar checks that exist elsewhere depending on
                    // the calling code so we also need check here before setting.
                    bool isConst = pCvar->IsConstCVar();
                    bool isCheat = ((pCvar->GetFlags() & (VF_CHEAT | VF_CHEAT_NOCHECK | VF_CHEAT_ALWAYS_CHECK)) != 0);
                    bool isReadOnly = ((pCvar->GetFlags() & VF_READONLY) != 0);
                    bool isDeprecated = ((pCvar->GetFlags() & VF_DEPRECATED) != 0);
                    bool allowApplyCvar = true;
                    bool whitelisted = true;

#if defined CVARS_WHITELIST
                    ICVarsWhitelist* cvarWhitelist = gEnv->pSystem->GetCVarsWhiteList();
                    whitelisted = cvarWhitelist ? cvarWhitelist->IsWhiteListed(szKey, true) : true;
#endif

                    if ((isConst || isCheat || isReadOnly) || isDeprecated)
                    {
                        allowApplyCvar = !isDeprecated && (gEnv->pSystem->IsDevMode()) || (gEnv->IsEditor());
                    }

                    if ((allowApplyCvar && whitelisted) || ALLOW_CONST_CVAR_MODIFICATIONS)
                    {
                        applyCvar = true;
                    }
                }
            }

            if (applyCvar)
            {
                pCvar->Set(szValue);
            }
            else
            {
                CryLogAlways("NOT VF_WASINCONFIG Ignoring cvar '%s' new value '%s' old value '%s' group '%s'", szKey, szValue, pCvar->GetString(), szGroup);
            }
        }
        else
        {
            CryLogAlways("Can't find cvar '%s' value '%s' group '%s'", szKey, szValue, szGroup);
        }
    }
};

#if !defined(CONSOLE)
struct SysSpecOverrideSinkConsole
    : public ILoadConfigurationEntrySink
{
    virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup)
    {
        // Ignore platform-specific cvars that should just be executed on the console
        if (azstricmp(szGroup, "Platform") == 0)
        {
            return;
        }

        ICVar* pCvar = gEnv->pConsole->GetCVar(szKey);
        if (pCvar)
        {
            pCvar->Set(szValue);
        }
        else
        {
            // If the cvar doesn't exist, calling this function only saves the value in case it's registered later where
            // at that point it will be set from the stored value. This is required because otherwise registering the
            // cvar bypasses any callbacks and uses values directly from the cvar group files.
            gEnv->pConsole->LoadConfigVar(szKey, szValue);
        }
    }
};
#endif

static ESystemConfigPlatform GetDevicePlatform()
{
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)
    return CONFIG_PC;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_2
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_PLATFORM_ANDROID)
    return CONFIG_ANDROID;
#elif defined(AZ_PLATFORM_IOS)
    return CONFIG_IOS;
#elif defined(AZ_PLATFORM_MAC)
    return CONFIG_OSX_METAL;
#else
    AZ_Assert(false, "Platform not supported");
    return CONFIG_INVALID_PLATFORM;
#endif
}

static void GetSpecConfigFileToLoad(ICVar* pVar, AZStd::string& cfgFile, ESystemConfigPlatform platform)
{
    switch (platform)
    {
    case CONFIG_PC:
        cfgFile = "pc";
        break;
    case CONFIG_ANDROID:
        cfgFile = "android";
        break;
    case CONFIG_IOS:
        cfgFile = "ios";
        break;
#if defined(AZ_PLATFORM_JASPER) || defined(TOOLS_SUPPORT_JASPER)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_3
#include AZ_RESTRICTED_FILE_EXPLICIT(SystemInit_cpp, jasper)
#endif
#if defined(AZ_PLATFORM_PROVO) || defined(TOOLS_SUPPORT_PROVO)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_3
#include AZ_RESTRICTED_FILE_EXPLICIT(SystemInit_cpp, provo)
#endif
#if defined(AZ_PLATFORM_SALEM) || defined(TOOLS_SUPPORT_SALEM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_3
#include AZ_RESTRICTED_FILE_EXPLICIT(SystemInit_cpp, salem)
#endif
    case CONFIG_OSX_METAL:
        cfgFile = "osx_metal";
        break;
    case CONFIG_OSX_GL:
        // Spec level is hardcoded for these platforms
        cfgFile = "";
        return;
    default:
        AZ_Assert(false, "Platform not supported");
        return;
    }

    switch (pVar->GetIVal())
    {
    case CONFIG_AUTO_SPEC:
        // Spec level is set for autodetection
        cfgFile = "";
        break;
    case CONFIG_LOW_SPEC:
        cfgFile += "_low.cfg";
        break;
    case CONFIG_MEDIUM_SPEC:
        cfgFile += "_medium.cfg";
        break;
    case CONFIG_HIGH_SPEC:
        cfgFile += "_high.cfg";
        break;
    case CONFIG_VERYHIGH_SPEC:
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_4
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
        cfgFile += "_veryhigh.cfg";
        break;
    default:
        AZ_Assert(false, "Invalid value for r_GraphicsQuality");
        break;
    }
}

static void LoadDetectedSpec(ICVar* pVar)
{
    CDebugAllowFileAccess ignoreInvalidFileAccess;
    SysSpecOverrideSink sysSpecOverrideSink;
    ILoadConfigurationEntrySink* pSysSpecOverrideSinkConsole = nullptr;

#if !defined(CONSOLE)
    SysSpecOverrideSinkConsole sysSpecOverrideSinkConsole;
    pSysSpecOverrideSinkConsole = &sysSpecOverrideSinkConsole;
#endif

    //  g_sysSpecChanged = true;
    static int no_recursive = false;
    if (no_recursive)
    {
        return;
    }
    no_recursive = true;

    int spec = pVar->GetIVal();
    ESystemConfigPlatform platform = GetDevicePlatform();
    if (gEnv->IsEditor())
    {
        ESystemConfigPlatform configPlatform = GetISystem()->GetConfigPlatform();
        // Check if the config platform is set first.
        if (configPlatform != CONFIG_INVALID_PLATFORM)
        {
            platform = configPlatform;
        }
    }

    AZStd::string configFile;
    GetSpecConfigFileToLoad(pVar, configFile, platform);
    if (configFile.length())
    {
        GetISystem()->LoadConfiguration(configFile.c_str(), platform == CONFIG_PC ? &sysSpecOverrideSink : pSysSpecOverrideSinkConsole);
    }
    else
    {
        // Automatically sets graphics quality - spec level autodetected for ios/android, hardcoded for all other platforms

        switch (platform)
        {
        case CONFIG_PC:
        {
            // TODO: add support for autodetection
            pVar->Set(CONFIG_VERYHIGH_SPEC);
            GetISystem()->LoadConfiguration("pc_veryhigh.cfg", &sysSpecOverrideSink);
            break;
        }
        case CONFIG_ANDROID:
        {
#if defined(AZ_PLATFORM_ANDROID)
            AZStd::string file;
            if (MobileSysInspect::GetAutoDetectedSpecName(file))
            {
                if (file == "android_low.cfg")
                {
                    pVar->Set(CONFIG_LOW_SPEC);
                }
                if (file == "android_medium.cfg")
                {
                    pVar->Set(CONFIG_MEDIUM_SPEC);
                }
                if (file == "android_high.cfg")
                {
                    pVar->Set(CONFIG_HIGH_SPEC);
                }
                if (file == "android_veryhigh.cfg")
                {
                    pVar->Set(CONFIG_VERYHIGH_SPEC);
                }
                GetISystem()->LoadConfiguration(file.c_str(), pSysSpecOverrideSinkConsole);
            }
            else
            {
                float totalRAM = MobileSysInspect::GetDeviceRamInGB();
                if (totalRAM < MobileSysInspect::LOW_SPEC_RAM)
                {
                    pVar->Set(CONFIG_LOW_SPEC);
                    GetISystem()->LoadConfiguration("android_low.cfg", pSysSpecOverrideSinkConsole);
                }
                else if (totalRAM < MobileSysInspect::MEDIUM_SPEC_RAM)
                {
                    pVar->Set(CONFIG_MEDIUM_SPEC);
                    GetISystem()->LoadConfiguration("android_medium.cfg", pSysSpecOverrideSinkConsole);
                }
                else if (totalRAM < MobileSysInspect::HIGH_SPEC_RAM)
                {
                    pVar->Set(CONFIG_HIGH_SPEC);
                    GetISystem()->LoadConfiguration("android_high.cfg", pSysSpecOverrideSinkConsole);
                }
                else
                {
                    pVar->Set(CONFIG_VERYHIGH_SPEC);
                    GetISystem()->LoadConfiguration("android_veryhigh.cfg", pSysSpecOverrideSinkConsole);
                }
            }
#endif
            break;
        }
        case CONFIG_IOS:
        {
#if defined(AZ_PLATFORM_IOS)
            AZStd::string file;
            if (MobileSysInspect::GetAutoDetectedSpecName(file))
            {
                if (file == "ios_low.cfg")
                {
                    pVar->Set(CONFIG_LOW_SPEC);
                }
                if (file == "ios_medium.cfg")
                {
                    pVar->Set(CONFIG_MEDIUM_SPEC);
                }
                if (file == "ios_high.cfg")
                {
                    pVar->Set(CONFIG_HIGH_SPEC);
                }
                if (file == "ios_veryhigh.cfg")
                {
                    pVar->Set(CONFIG_VERYHIGH_SPEC);
                }
                GetISystem()->LoadConfiguration(file.c_str(), pSysSpecOverrideSinkConsole);
            }
            else
            {
                pVar->Set(CONFIG_MEDIUM_SPEC);
                GetISystem()->LoadConfiguration("ios_medium.cfg", pSysSpecOverrideSinkConsole);
            }
#endif
            break;
        }
#if defined(AZ_PLATFORM_JASPER) || defined(TOOLS_SUPPORT_JASPER)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_5
#include AZ_RESTRICTED_FILE_EXPLICIT(SystemInit_cpp, jasper)
#endif
#if defined(AZ_PLATFORM_PROVO) || defined(TOOLS_SUPPORT_PROVO)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_5
#include AZ_RESTRICTED_FILE_EXPLICIT(SystemInit_cpp, provo)
#endif
#if defined(AZ_PLATFORM_SALEM) || defined(TOOLS_SUPPORT_SALEM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_5
#include AZ_RESTRICTED_FILE_EXPLICIT(SystemInit_cpp, salem)
#endif

        case CONFIG_OSX_GL:
        {
            pVar->Set(CONFIG_HIGH_SPEC);
            GetISystem()->LoadConfiguration("osx_gl.cfg", pSysSpecOverrideSinkConsole);
            break;
        }
        case CONFIG_OSX_METAL:
        {
            pVar->Set(CONFIG_HIGH_SPEC);
            GetISystem()->LoadConfiguration("osx_metal_high.cfg", pSysSpecOverrideSinkConsole);
            break;
        }
        default:
            AZ_Assert(false, "Platform not supported");
            break;
        }
    }

    // make sure editor specific settings are not changed
    if (gEnv->IsEditor())
    {
        GetISystem()->LoadConfiguration("editor.cfg");
    }

    bool bMultiGPUEnabled = false;
    if (gEnv->pRenderer)
    {
        gEnv->pRenderer->EF_Query(EFQ_MultiGPUEnabled, bMultiGPUEnabled);

#if defined(AZ_PLATFORM_ANDROID)
        AZStd::string gpuConfigFile;
        const AZStd::string& adapterDesc = gEnv->pRenderer->GetAdapterDescription();
        const AZStd::string& apiver = gEnv->pRenderer->GetApiVersion();

        if (!adapterDesc.empty())
        {
            MobileSysInspect::GetSpecForGPUAndAPI(adapterDesc, apiver, gpuConfigFile);
            GetISystem()->LoadConfiguration(gpuConfigFile.c_str(), pSysSpecOverrideSinkConsole);
        }
#endif
    }
    if (bMultiGPUEnabled)
    {
        GetISystem()->LoadConfiguration("mgpu.cfg");
    }

    // override cvars just loaded based on current API version/GPU

    GetISystem()->SetConfigSpec(static_cast<ESystemConfigSpec>(spec), platform, false);

    if (gEnv->p3DEngine)
    {
        gEnv->p3DEngine->GetMaterialManager()->RefreshMaterialRuntime();
    }

    no_recursive = false;
}

//////////////////////////////////////////////////////////////////////////
struct SCryEngineLanguageConfigLoader
    : public ILoadConfigurationEntrySink
{
    CSystem* m_pSystem;
    string m_language;
    string m_pakFile;

    SCryEngineLanguageConfigLoader(CSystem* pSystem) { m_pSystem = pSystem; }
    void Load(const char* sCfgFilename)
    {
        CSystemConfiguration cfg(sCfgFilename, m_pSystem, this); // Parse folders config file.
    }
    virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, [[maybe_unused]] const char* szGroup)
    {
        if (azstricmp(szKey, "Language") == 0)
        {
            m_language = szValue;
        }
        else if (azstricmp(szKey, "PAK") == 0)
        {
            m_pakFile = szValue;
        }
    }
    virtual void OnLoadConfigurationEntry_End() {}
};

//////////////////////////////////////////////////////////////////////////
#if !defined(AZ_MONOLITHIC_BUILD)

AZStd::unique_ptr<AZ::DynamicModuleHandle> CSystem::LoadDynamiclibrary(const char* dllName) const
{
    AZStd::unique_ptr<AZ::DynamicModuleHandle> handle = AZ::DynamicModuleHandle::Create(dllName);

    bool libraryLoaded = handle->Load(false);
    // We need to inject the environment first thing so that allocators are available immediately
    InjectEnvironmentFunction injectEnv = handle->GetFunction<InjectEnvironmentFunction>(INJECT_ENVIRONMENT_FUNCTION);
    if (injectEnv)
    {
        auto env = AZ::Environment::GetInstance();
        injectEnv(env);
    }

    if (!libraryLoaded)
    {
        handle.release();
    }
    return handle;
}

//////////////////////////////////////////////////////////////////////////
AZStd::unique_ptr<AZ::DynamicModuleHandle> CSystem::LoadDLL(const char* dllName)
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    AZ_TracePrintf(AZ_TRACE_SYSTEM_WINDOW, "Loading DLL: %s", dllName);

    AZStd::unique_ptr<AZ::DynamicModuleHandle> handle = LoadDynamiclibrary(dllName);

    if (!handle)
    {
#if defined(LINUX) || defined(APPLE)
        AZ_Assert(false, "Error loading dylib: %s, error :  %s\n", dllName, dlerror());
#else
        AZ_Assert(false, "Error loading dll: %s, error code %d", dllName, GetLastError());
#endif
        return handle;
    }

    //////////////////////////////////////////////////////////////////////////
    // After loading DLL initialize it by calling ModuleInitISystem
    //////////////////////////////////////////////////////////////////////////
    string moduleName = PathUtil::GetFileName(dllName);

    typedef void*(*PtrFunc_ModuleInitISystem)(ISystem* pSystem, const char* moduleName);
    PtrFunc_ModuleInitISystem pfnModuleInitISystem = handle->GetFunction<PtrFunc_ModuleInitISystem>(DLL_MODULE_INIT_ISYSTEM);
    if (pfnModuleInitISystem)
    {
        pfnModuleInitISystem(this, moduleName.c_str());
    }

    return handle;
}

// TODO:DLL  #endif //#if defined(AZ_HAS_DLL_SUPPORT) && !defined(AZ_MONOLITHIC_BUILD)
#endif //if !defined(AZ_MONOLITHIC_BUILD)
//////////////////////////////////////////////////////////////////////////
bool CSystem::LoadEngineDLLs()
{
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::UnloadDLL(const char* dllName)
{
    bool isSuccess = false;

    CCryNameCRC key(dllName);
    AZStd::unique_ptr<AZ::DynamicModuleHandle> empty;
    AZStd::unique_ptr<AZ::DynamicModuleHandle>& hModule = stl::find_in_map_ref(m_moduleDLLHandles, key, empty);
    if ((hModule) && (hModule->IsLoaded()))
    {
        DetachEnvironmentFunction detachEnv = hModule->GetFunction<DetachEnvironmentFunction>(DETACH_ENVIRONMENT_FUNCTION);
        if (detachEnv)
        {
            detachEnv();
        }

        isSuccess = hModule->Unload();
        hModule.release();
    }

    return isSuccess;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::InitializeEngineModule(const char* dllName, const char* moduleClassName, const SSystemInitParams& initParams)
{
    bool bResult = false;

    stack_string msg;
    msg = "Initializing ";
    AZStd::string dll = dllName;

    // Strip off Cry if the dllname is Cry<something>
    if (dll.find("Cry") == 0)
    {
        msg += dll.substr(3).c_str();
    }
    else
    {
        msg += dllName;
    }
    msg += "...";

    if (m_pUserCallback)
    {
        m_pUserCallback->OnInitProgress(msg.c_str());
    }
    AZ_TracePrintf(moduleClassName, "%s", msg.c_str());

    IMemoryManager::SProcessMemInfo memStart, memEnd;
    if (GetIMemoryManager())
    {
        GetIMemoryManager()->GetProcessMemInfo(memStart);
    }
    else
    {
        ZeroStruct(memStart);
    }

    stack_string dllfile = "";


#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_16
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else

    dllfile.append(dllName);

#if defined(LINUX)
    dllfile = "lib" + PathUtil::ReplaceExtension(dllfile, "so");
#ifndef LINUX
    dllfile.MakeLower();
#endif
#elif defined(AZ_PLATFORM_MAC)
    dllfile = "lib" + PathUtil::ReplaceExtension(dllfile, "dylib");
#elif defined(AZ_PLATFORM_IOS)
    PathUtil::RemoveExtension(dllfile);
#else
    dllfile = PathUtil::ReplaceExtension(dllfile, "dll");
#endif

#endif

#if !defined(AZ_MONOLITHIC_BUILD)

    m_moduleDLLHandles.insert(std::make_pair(dllfile.c_str(), LoadDLL(dllfile.c_str())));
    if (!m_moduleDLLHandles[dllfile.c_str()])
    {
        return bResult;
    }

#endif // #if !defined(AZ_MONOLITHIC_BUILD)

    AZStd::shared_ptr<IEngineModule> pModule;
    if (CryCreateClassInstance(moduleClassName, pModule))
    {
        bResult = pModule->Initialize(m_env, initParams);

        // After initializing the module, give it a chance to register any AZ console vars
        // declared within the module.
        pModule->RegisterConsoleVars();
    }

    if (GetIMemoryManager())
    {
        GetIMemoryManager()->GetProcessMemInfo(memEnd);

#if defined(AZ_ENABLE_TRACING)
        uint64 memUsed = memEnd.WorkingSetSize - memStart.WorkingSetSize;
#endif
        AZ_TracePrintf(AZ_TRACE_SYSTEM_WINDOW, "Initializing %s %s, MemUsage=%uKb", dllName, pModule ? "done" : "failed", uint32(memUsed / 1024));
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::UnloadEngineModule(const char* dllName, const char* moduleClassName)
{
    bool isSuccess = false;

    // Remove the factory.
    ICryFactoryRegistryImpl* const pReg = static_cast<ICryFactoryRegistryImpl*>(GetCryFactoryRegistry());

    if (pReg != nullptr)
    {
        ICryFactory* pICryFactory = pReg->GetFactory(moduleClassName);

        if (pICryFactory != nullptr)
        {
            pReg->UnregisterFactory(pICryFactory);
        }
    }

    stack_string msg;
    msg = "Unloading ";
    msg += dllName;
    msg += "...";

    AZ_TracePrintf(AZ_TRACE_SYSTEM_WINDOW, "%s", msg.c_str());

    stack_string dllfile = dllName;

#if defined(LINUX)
    dllfile = "lib" + PathUtil::ReplaceExtension(dllfile, "so");
#ifndef LINUX
    dllfile.MakeLower();
#endif
#elif defined(APPLE)
    dllfile = "lib" + PathUtil::ReplaceExtension(dllfile, "dylib");
#else
    dllfile = PathUtil::ReplaceExtension(dllfile, "dll");
#endif

#if !defined(AZ_MONOLITHIC_BUILD)
    isSuccess = UnloadDLL(dllfile.c_str());
#endif // #if !defined(AZ_MONOLITHIC_BUILD)

    return isSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::ShutdownModuleLibraries()
{
#if !defined(AZ_MONOLITHIC_BUILD)
    for (auto iterator = m_moduleDLLHandles.begin(); iterator != m_moduleDLLHandles.end(); ++iterator)
    {
        typedef void*( * PtrFunc_ModuleShutdownISystem )(ISystem* pSystem);

        PtrFunc_ModuleShutdownISystem pfnModuleShutdownISystem = iterator->second->GetFunction<PtrFunc_ModuleShutdownISystem>(DLL_MODULE_SHUTDOWN_ISYSTEM);
        if (pfnModuleShutdownISystem)
        {
            pfnModuleShutdownISystem(this);
        }
        if (iterator->second->IsLoaded())
        {
            iterator->second->Unload();
        }
        iterator->second.release();
    }

    m_moduleDLLHandles.clear();

#endif // !defined(AZ_MONOLITHIC_BUILD)
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::OpenRenderLibrary([[maybe_unused]] const char* t_rend, const SSystemInitParams& initParams)
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_6
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else

    if (gEnv->IsDedicated())
    {
        return OpenRenderLibrary(R_NULL_RENDERER, initParams);
    }

    if (AZ::Interface<AzFramework::AtomActiveInterface>::Get())
    {
        return OpenRenderLibrary(R_DX11_RENDERER, initParams);
    }
    else if (azstricmp(t_rend, "DX9") == 0)
    {
        return OpenRenderLibrary(R_DX9_RENDERER, initParams);
    }
    else if (azstricmp(t_rend, "DX11") == 0)
    {
        return OpenRenderLibrary(R_DX11_RENDERER, initParams);
    }
    else if (azstricmp(t_rend, "DX12") == 0)
    {
        return OpenRenderLibrary(R_DX12_RENDERER, initParams);
    }
    else if (azstricmp(t_rend, "GL") == 0)
    {
        return OpenRenderLibrary(R_GL_RENDERER, initParams);
    }
    else if (azstricmp(t_rend, "METAL") == 0)
    {
        return OpenRenderLibrary(R_METAL_RENDERER, initParams);
    }
    else if (azstricmp(t_rend, "NULL") == 0)
    {
        return OpenRenderLibrary(R_NULL_RENDERER, initParams);
    }

    AZ_Assert(false, "Unknown renderer type: %s", t_rend);
    return false;
#endif
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

#if defined(WIN32) || defined(WIN64)
wstring GetErrorStringUnsupportedGPU(const char* gpuName, unsigned int gpuVendorId, unsigned int gpuDeviceId)
{
    const size_t fullLangID = (size_t) GetKeyboardLayout(0);
    const size_t primLangID = fullLangID & 0x3FF;

    const wchar_t* pFmt = L"Unsupported video card detected! Continuing to run might lead to unexpected results or crashes. "
        L"Please check the manual for further information on hardware requirements.\n\n\"%S\" [vendor id = 0x%.4x, device id = 0x%.4x]";

    switch (primLangID)
    {
    case 0x04: // Chinese
    {
        static const wchar_t fmt[] = {0x5075, 0x6E2C, 0x5230, 0x4E0D, 0x652F, 0x63F4, 0x7684, 0x986F, 0x793A, 0x5361, 0xFF01, 0x7E7C, 0x7E8C, 0x57F7, 0x884C, 0x53EF, 0x80FD, 0x5C0E, 0x81F4, 0x7121, 0x6CD5, 0x9810, 0x671F, 0x7684, 0x7D50, 0x679C, 0x6216, 0x7576, 0x6A5F, 0x3002, 0x8ACB, 0x6AA2, 0x67E5, 0x8AAA, 0x660E, 0x66F8, 0x4E0A, 0x7684, 0x786C, 0x9AD4, 0x9700, 0x6C42, 0x4EE5, 0x53D6, 0x5F97, 0x66F4, 0x591A, 0x76F8, 0x95DC, 0x8CC7, 0x8A0A, 0x3002, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x5EE0, 0x5546, 0x7DE8, 0x865F, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x88DD, 0x7F6E, 0x7DE8, 0x865F, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }
    case 0x05: // Czech
    {
        static const wchar_t fmt[] = {0x0042, 0x0079, 0x006C, 0x0061, 0x0020, 0x0064, 0x0065, 0x0074, 0x0065, 0x006B, 0x006F, 0x0076, 0x00E1, 0x006E, 0x0061, 0x0020, 0x0067, 0x0072, 0x0061, 0x0066, 0x0069, 0x0063, 0x006B, 0x00E1, 0x0020, 0x006B, 0x0061, 0x0072, 0x0074, 0x0061, 0x002C, 0x0020, 0x006B, 0x0074, 0x0065, 0x0072, 0x00E1, 0x0020, 0x006E, 0x0065, 0x006E, 0x00ED, 0x0020, 0x0070, 0x006F, 0x0064, 0x0070, 0x006F, 0x0072, 0x006F, 0x0076, 0x00E1, 0x006E, 0x0061, 0x002E, 0x0020, 0x0050, 0x006F, 0x006B, 0x0072, 0x0061, 0x010D, 0x006F, 0x0076, 0x00E1, 0x006E, 0x00ED, 0x0020, 0x006D, 0x016F, 0x017E, 0x0065, 0x0020, 0x0076, 0x00E9, 0x0073, 0x0074, 0x0020, 0x006B, 0x0065, 0x0020, 0x006B, 0x0072, 0x0069, 0x0074, 0x0069, 0x0063, 0x006B, 0x00FD, 0x006D, 0x0020, 0x0063, 0x0068, 0x0079, 0x0062, 0x00E1, 0x006D, 0x0020, 0x006E, 0x0065, 0x0062, 0x006F, 0x0020, 0x006E, 0x0065, 0x0073, 0x0074, 0x0061, 0x0062, 0x0069, 0x006C, 0x0069, 0x0074, 0x011B, 0x0020, 0x0073, 0x0079, 0x0073, 0x0074, 0x00E9, 0x006D, 0x0075, 0x002E, 0x0020, 0x0050, 0x0159, 0x0065, 0x010D, 0x0074, 0x011B, 0x0074, 0x0065, 0x0020, 0x0073, 0x0069, 0x0020, 0x0070, 0x0072, 0x006F, 0x0073, 0x00ED, 0x006D, 0x0020, 0x0075, 0x017E, 0x0069, 0x0076, 0x0061, 0x0074, 0x0065, 0x006C, 0x0073, 0x006B, 0x006F, 0x0075, 0x0020, 0x0070, 0x0159, 0x00ED, 0x0072, 0x0075, 0x010D, 0x006B, 0x0075, 0x0020, 0x0070, 0x0072, 0x006F, 0x0020, 0x0070, 0x006F, 0x0064, 0x0072, 0x006F, 0x0062, 0x006E, 0x00E9, 0x0020, 0x0069, 0x006E, 0x0066, 0x006F, 0x0072, 0x006D, 0x0061, 0x0063, 0x0065, 0x0020, 0x006F, 0x0020, 0x0073, 0x0079, 0x0073, 0x0074, 0x00E9, 0x006D, 0x006F, 0x0076, 0x00FD, 0x0063, 0x0068, 0x0020, 0x0070, 0x006F, 0x017E, 0x0061, 0x0064, 0x0061, 0x0076, 0x0063, 0x00ED, 0x0063, 0x0068, 0x002E, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x0076, 0x0065, 0x006E, 0x0064, 0x006F, 0x0072, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x0064, 0x0065, 0x0076, 0x0069, 0x0063, 0x0065, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }
    case 0x07: // German
    {
        static const wchar_t fmt[] = {0x004E, 0x0069, 0x0063, 0x0068, 0x0074, 0x002D, 0x0075, 0x006E, 0x0074, 0x0065, 0x0072, 0x0073, 0x0074, 0x00FC, 0x0074, 0x007A, 0x0074, 0x0065, 0x0020, 0x0056, 0x0069, 0x0064, 0x0065, 0x006F, 0x006B, 0x0061, 0x0072, 0x0074, 0x0065, 0x0020, 0x0067, 0x0065, 0x0066, 0x0075, 0x006E, 0x0064, 0x0065, 0x006E, 0x0021, 0x0020, 0x0046, 0x006F, 0x0072, 0x0074, 0x0066, 0x0061, 0x0068, 0x0072, 0x0065, 0x006E, 0x0020, 0x006B, 0x0061, 0x006E, 0x006E, 0x0020, 0x007A, 0x0075, 0x0020, 0x0075, 0x006E, 0x0065, 0x0072, 0x0077, 0x0061, 0x0072, 0x0074, 0x0065, 0x0074, 0x0065, 0x006E, 0x0020, 0x0045, 0x0072, 0x0067, 0x0065, 0x0062, 0x006E, 0x0069, 0x0073, 0x0073, 0x0065, 0x006E, 0x0020, 0x006F, 0x0064, 0x0065, 0x0072, 0x0020, 0x0041, 0x0062, 0x0073, 0x0074, 0x00FC, 0x0072, 0x007A, 0x0065, 0x006E, 0x0020, 0x0066, 0x00FC, 0x0068, 0x0072, 0x0065, 0x006E, 0x002E, 0x0020, 0x0042, 0x0069, 0x0074, 0x0074, 0x0065, 0x0020, 0x006C, 0x0069, 0x0065, 0x0073, 0x0020, 0x0064, 0x0061, 0x0073, 0x0020, 0x004D, 0x0061, 0x006E, 0x0075, 0x0061, 0x006C, 0x0020, 0x0066, 0x00FC, 0x0072, 0x0020, 0x0077, 0x0065, 0x0069, 0x0074, 0x0065, 0x0072, 0x0065, 0x0020, 0x0049, 0x006E, 0x0066, 0x006F, 0x0072, 0x006D, 0x0061, 0x0074, 0x0069, 0x006F, 0x006E, 0x0065, 0x006E, 0x0020, 0x007A, 0x0075, 0x0020, 0x0048, 0x0061, 0x0072, 0x0064, 0x0077, 0x0061, 0x0072, 0x0065, 0x002D, 0x0041, 0x006E, 0x0066, 0x006F, 0x0072, 0x0064, 0x0065, 0x0072, 0x0075, 0x006E, 0x0067, 0x0065, 0x006E, 0x002E, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x0076, 0x0065, 0x006E, 0x0064, 0x006F, 0x0072, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x0064, 0x0065, 0x0076, 0x0069, 0x0063, 0x0065, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }
    case 0x0a: // Spanish
    {
        static const wchar_t fmt[] = {0x0053, 0x0065, 0x0020, 0x0068, 0x0061, 0x0020, 0x0064, 0x0065, 0x0074, 0x0065, 0x0063, 0x0074, 0x0061, 0x0064, 0x006F, 0x0020, 0x0075, 0x006E, 0x0061, 0x0020, 0x0074, 0x0061, 0x0072, 0x006A, 0x0065, 0x0074, 0x0061, 0x0020, 0x0067, 0x0072, 0x00E1, 0x0066, 0x0069, 0x0063, 0x0061, 0x0020, 0x006E, 0x006F, 0x0020, 0x0063, 0x006F, 0x006D, 0x0070, 0x0061, 0x0074, 0x0069, 0x0062, 0x006C, 0x0065, 0x002E, 0x0020, 0x0053, 0x0069, 0x0020, 0x0073, 0x0069, 0x0067, 0x0075, 0x0065, 0x0073, 0x0020, 0x0065, 0x006A, 0x0065, 0x0063, 0x0075, 0x0074, 0x0061, 0x006E, 0x0064, 0x006F, 0x0020, 0x0065, 0x006C, 0x0020, 0x006A, 0x0075, 0x0065, 0x0067, 0x006F, 0x002C, 0x0020, 0x0065, 0x0073, 0x0020, 0x0070, 0x006F, 0x0073, 0x0069, 0x0062, 0x006C, 0x0065, 0x0020, 0x0071, 0x0075, 0x0065, 0x0020, 0x0073, 0x0065, 0x0020, 0x0070, 0x0072, 0x006F, 0x0064, 0x0075, 0x007A, 0x0063, 0x0061, 0x006E, 0x0020, 0x0065, 0x0066, 0x0065, 0x0063, 0x0074, 0x006F, 0x0073, 0x0020, 0x0069, 0x006E, 0x0065, 0x0073, 0x0070, 0x0065, 0x0072, 0x0061, 0x0064, 0x006F, 0x0073, 0x0020, 0x006F, 0x0020, 0x0071, 0x0075, 0x0065, 0x0020, 0x0065, 0x006C, 0x0020, 0x0070, 0x0072, 0x006F, 0x0067, 0x0072, 0x0061, 0x006D, 0x0061, 0x0020, 0x0064, 0x0065, 0x006A, 0x0065, 0x0020, 0x0064, 0x0065, 0x0020, 0x0066, 0x0075, 0x006E, 0x0063, 0x0069, 0x006F, 0x006E, 0x0061, 0x0072, 0x002E, 0x0020, 0x0050, 0x006F, 0x0072, 0x0020, 0x0066, 0x0061, 0x0076, 0x006F, 0x0072, 0x002C, 0x0020, 0x0063, 0x006F, 0x006D, 0x0070, 0x0072, 0x0075, 0x0065, 0x0062, 0x0061, 0x0020, 0x0065, 0x006C, 0x0020, 0x006D, 0x0061, 0x006E, 0x0075, 0x0061, 0x006C, 0x0020, 0x0070, 0x0061, 0x0072, 0x0061, 0x0020, 0x006F, 0x0062, 0x0074, 0x0065, 0x006E, 0x0065, 0x0072, 0x0020, 0x006D, 0x00E1, 0x0073, 0x0020, 0x0069, 0x006E, 0x0066, 0x006F, 0x0072, 0x006D, 0x0061, 0x0063, 0x0069, 0x00F3, 0x006E, 0x0020, 0x0061, 0x0063, 0x0065, 0x0072, 0x0063, 0x0061, 0x0020, 0x0064, 0x0065, 0x0020, 0x006C, 0x006F, 0x0073, 0x0020, 0x0072, 0x0065, 0x0071, 0x0075, 0x0069, 0x0073, 0x0069, 0x0074, 0x006F, 0x0073, 0x0020, 0x0064, 0x0065, 0x006C, 0x0020, 0x0073, 0x0069, 0x0073, 0x0074, 0x0065, 0x006D, 0x0061, 0x002E, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x0076, 0x0065, 0x006E, 0x0064, 0x006F, 0x0072, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x0064, 0x0065, 0x0076, 0x0069, 0x0063, 0x0065, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }
    case 0x0c: // French
    {
        static const wchar_t fmt[] = {0x0041, 0x0074, 0x0074, 0x0065, 0x006E, 0x0074, 0x0069, 0x006F, 0x006E, 0x002C, 0x0020, 0x006C, 0x0061, 0x0020, 0x0063, 0x0061, 0x0072, 0x0074, 0x0065, 0x0020, 0x0076, 0x0069, 0x0064, 0x00E9, 0x006F, 0x0020, 0x0064, 0x00E9, 0x0074, 0x0065, 0x0063, 0x0074, 0x00E9, 0x0065, 0x0020, 0x006E, 0x2019, 0x0065, 0x0073, 0x0074, 0x0020, 0x0070, 0x0061, 0x0073, 0x0020, 0x0073, 0x0075, 0x0070, 0x0070, 0x006F, 0x0072, 0x0074, 0x00E9, 0x0065, 0x0020, 0x0021, 0x0020, 0x0050, 0x006F, 0x0075, 0x0072, 0x0073, 0x0075, 0x0069, 0x0076, 0x0072, 0x0065, 0x0020, 0x006C, 0x2019, 0x0061, 0x0070, 0x0070, 0x006C, 0x0069, 0x0063, 0x0061, 0x0074, 0x0069, 0x006F, 0x006E, 0x0020, 0x0070, 0x006F, 0x0075, 0x0072, 0x0072, 0x0061, 0x0069, 0x0074, 0x0020, 0x0065, 0x006E, 0x0067, 0x0065, 0x006E, 0x0064, 0x0072, 0x0065, 0x0072, 0x0020, 0x0064, 0x0065, 0x0073, 0x0020, 0x0069, 0x006E, 0x0073, 0x0074, 0x0061, 0x0062, 0x0069, 0x006C, 0x0069, 0x0074, 0x00E9, 0x0073, 0x0020, 0x006F, 0x0075, 0x0020, 0x0064, 0x0065, 0x0073, 0x0020, 0x0063, 0x0072, 0x0061, 0x0073, 0x0068, 0x0073, 0x002E, 0x0020, 0x0056, 0x0065, 0x0075, 0x0069, 0x006C, 0x006C, 0x0065, 0x007A, 0x0020, 0x0076, 0x006F, 0x0075, 0x0073, 0x0020, 0x0072, 0x0065, 0x0070, 0x006F, 0x0072, 0x0074, 0x0065, 0x0072, 0x0020, 0x0061, 0x0075, 0x0020, 0x006D, 0x0061, 0x006E, 0x0075, 0x0065, 0x006C, 0x0020, 0x0070, 0x006F, 0x0075, 0x0072, 0x0020, 0x0070, 0x006C, 0x0075, 0x0073, 0x0020, 0x0064, 0x2019, 0x0069, 0x006E, 0x0066, 0x006F, 0x0072, 0x006D, 0x0061, 0x0074, 0x0069, 0x006F, 0x006E, 0x0073, 0x0020, 0x0073, 0x0075, 0x0072, 0x0020, 0x006C, 0x0065, 0x0073, 0x0020, 0x0070, 0x0072, 0x00E9, 0x002D, 0x0072, 0x0065, 0x0071, 0x0075, 0x0069, 0x0073, 0x0020, 0x006D, 0x0061, 0x0074, 0x00E9, 0x0072, 0x0069, 0x0065, 0x006C, 0x002E, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x0076, 0x0065, 0x006E, 0x0064, 0x006F, 0x0072, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x0064, 0x0065, 0x0076, 0x0069, 0x0063, 0x0065, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }
    case 0x10: // Italian
    {
        static const wchar_t fmt[] = {0x00C8, 0x0020, 0x0073, 0x0074, 0x0061, 0x0074, 0x0061, 0x0020, 0x0072, 0x0069, 0x006C, 0x0065, 0x0076, 0x0061, 0x0074, 0x0061, 0x0020, 0x0075, 0x006E, 0x0061, 0x0020, 0x0073, 0x0063, 0x0068, 0x0065, 0x0064, 0x0061, 0x0020, 0x0067, 0x0072, 0x0061, 0x0066, 0x0069, 0x0063, 0x0061, 0x0020, 0x006E, 0x006F, 0x006E, 0x0020, 0x0073, 0x0075, 0x0070, 0x0070, 0x006F, 0x0072, 0x0074, 0x0061, 0x0074, 0x0061, 0x0021, 0x0020, 0x0053, 0x0065, 0x0020, 0x0073, 0x0069, 0x0020, 0x0063, 0x006F, 0x006E, 0x0074, 0x0069, 0x006E, 0x0075, 0x0061, 0x002C, 0x0020, 0x0073, 0x0069, 0x0020, 0x0070, 0x006F, 0x0074, 0x0072, 0x0065, 0x0062, 0x0062, 0x0065, 0x0072, 0x006F, 0x0020, 0x0076, 0x0065, 0x0072, 0x0069, 0x0066, 0x0069, 0x0063, 0x0061, 0x0072, 0x0065, 0x0020, 0x0072, 0x0069, 0x0073, 0x0075, 0x006C, 0x0074, 0x0061, 0x0074, 0x0069, 0x0020, 0x0069, 0x006E, 0x0061, 0x0074, 0x0074, 0x0065, 0x0073, 0x0069, 0x0020, 0x006F, 0x0020, 0x0063, 0x0072, 0x0061, 0x0073, 0x0068, 0x002E, 0x0020, 0x0043, 0x006F, 0x006E, 0x0073, 0x0075, 0x006C, 0x0074, 0x0061, 0x0020, 0x0069, 0x006C, 0x0020, 0x006D, 0x0061, 0x006E, 0x0075, 0x0061, 0x006C, 0x0065, 0x0020, 0x0070, 0x0065, 0x0072, 0x0020, 0x0075, 0x006C, 0x0074, 0x0065, 0x0072, 0x0069, 0x006F, 0x0072, 0x0069, 0x0020, 0x0069, 0x006E, 0x0066, 0x006F, 0x0072, 0x006D, 0x0061, 0x007A, 0x0069, 0x006F, 0x006E, 0x0069, 0x0020, 0x0073, 0x0075, 0x0069, 0x0020, 0x0072, 0x0065, 0x0071, 0x0075, 0x0069, 0x0073, 0x0069, 0x0074, 0x0069, 0x0020, 0x0064, 0x0069, 0x0020, 0x0073, 0x0069, 0x0073, 0x0074, 0x0065, 0x006D, 0x0061, 0x002E, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x0076, 0x0065, 0x006E, 0x0064, 0x006F, 0x0072, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x0064, 0x0065, 0x0076, 0x0069, 0x0063, 0x0065, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }
    case 0x11: // Japanese
    {
        static const wchar_t fmt[] = {0x30B5, 0x30DD, 0x30FC, 0x30C8, 0x3055, 0x308C, 0x3066, 0x3044, 0x306A, 0x3044, 0x30D3, 0x30C7, 0x30AA, 0x30AB, 0x30FC, 0x30C9, 0x304C, 0x691C, 0x51FA, 0x3055, 0x308C, 0x307E, 0x3057, 0x305F, 0xFF01, 0x0020, 0x3053, 0x306E, 0x307E, 0x307E, 0x7D9A, 0x3051, 0x308B, 0x3068, 0x4E88, 0x671F, 0x3057, 0x306A, 0x3044, 0x7D50, 0x679C, 0x3084, 0x30AF, 0x30E9, 0x30C3, 0x30B7, 0x30E5, 0x306E, 0x6050, 0x308C, 0x304C, 0x3042, 0x308A, 0x307E, 0x3059, 0x3002, 0x0020, 0x30DE, 0x30CB, 0x30E5, 0x30A2, 0x30EB, 0x306E, 0x5FC5, 0x8981, 0x52D5, 0x4F5C, 0x74B0, 0x5883, 0x3092, 0x3054, 0x78BA, 0x8A8D, 0x304F, 0x3060, 0x3055, 0x3044, 0x3002, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x30D9, 0x30F3, 0x30C0, 0x30FC, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x30C7, 0x30D0, 0x30A4, 0x30B9, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }
    case 0x15: // Polish
    {
        static const wchar_t fmt[] = {0x0057, 0x0079, 0x006B, 0x0072, 0x0079, 0x0074, 0x006F, 0x0020, 0x006E, 0x0069, 0x0065, 0x006F, 0x0062, 0x0073, 0x0142, 0x0075, 0x0067, 0x0069, 0x0077, 0x0061, 0x006E, 0x0105, 0x0020, 0x006B, 0x0061, 0x0072, 0x0074, 0x0119, 0x0020, 0x0067, 0x0072, 0x0061, 0x0066, 0x0069, 0x0063, 0x007A, 0x006E, 0x0105, 0x0021, 0x0020, 0x0044, 0x0061, 0x006C, 0x0073, 0x007A, 0x0065, 0x0020, 0x006B, 0x006F, 0x0072, 0x007A, 0x0079, 0x0073, 0x0074, 0x0061, 0x006E, 0x0069, 0x0065, 0x0020, 0x007A, 0x0020, 0x0070, 0x0072, 0x006F, 0x0064, 0x0075, 0x006B, 0x0074, 0x0075, 0x0020, 0x006D, 0x006F, 0x017C, 0x0065, 0x0020, 0x0073, 0x0070, 0x006F, 0x0077, 0x006F, 0x0064, 0x006F, 0x0077, 0x0061, 0x0107, 0x0020, 0x006E, 0x0069, 0x0065, 0x0070, 0x006F, 0x017C, 0x0105, 0x0064, 0x0061, 0x006E, 0x0065, 0x0020, 0x007A, 0x0061, 0x0063, 0x0068, 0x006F, 0x0077, 0x0061, 0x006E, 0x0069, 0x0065, 0x0020, 0x006C, 0x0075, 0x0062, 0x0020, 0x0077, 0x0073, 0x0074, 0x0072, 0x007A, 0x0079, 0x006D, 0x0061, 0x006E, 0x0069, 0x0065, 0x0020, 0x0070, 0x0072, 0x006F, 0x0067, 0x0072, 0x0061, 0x006D, 0x0075, 0x002E, 0x0020, 0x0041, 0x0062, 0x0079, 0x0020, 0x0075, 0x007A, 0x0079, 0x0073, 0x006B, 0x0061, 0x0107, 0x0020, 0x0077, 0x0069, 0x0119, 0x0063, 0x0065, 0x006A, 0x0020, 0x0069, 0x006E, 0x0066, 0x006F, 0x0072, 0x006D, 0x0061, 0x0063, 0x006A, 0x0069, 0x002C, 0x0020, 0x0073, 0x006B, 0x006F, 0x006E, 0x0073, 0x0075, 0x006C, 0x0074, 0x0075, 0x006A, 0x0020, 0x0073, 0x0069, 0x0119, 0x0020, 0x007A, 0x0020, 0x0069, 0x006E, 0x0073, 0x0074, 0x0072, 0x0075, 0x006B, 0x0063, 0x006A, 0x0105, 0x0020, 0x006F, 0x0062, 0x0073, 0x0142, 0x0075, 0x0067, 0x0069, 0x002E, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x0076, 0x0065, 0x006E, 0x0064, 0x006F, 0x0072, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x0064, 0x0065, 0x0076, 0x0069, 0x0063, 0x0065, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }
    case 0x19: // Russian
    {
        static const wchar_t fmt[] = {0x0412, 0x0430, 0x0448, 0x0430, 0x0020, 0x0432, 0x0438, 0x0434, 0x0435, 0x043E, 0x0020, 0x043A, 0x0430, 0x0440, 0x0442, 0x0430, 0x0020, 0x043D, 0x0435, 0x0020, 0x043F, 0x043E, 0x0434, 0x0434, 0x0435, 0x0440, 0x0436, 0x0438, 0x0432, 0x0430, 0x0435, 0x0442, 0x0441, 0x044F, 0x0021, 0x0020, 0x042D, 0x0442, 0x043E, 0x0020, 0x043C, 0x043E, 0x0436, 0x0435, 0x0442, 0x0020, 0x043F, 0x0440, 0x0438, 0x0432, 0x0435, 0x0441, 0x0442, 0x0438, 0x0020, 0x043A, 0x0020, 0x043D, 0x0435, 0x043F, 0x0440, 0x0435, 0x0434, 0x0441, 0x043A, 0x0430, 0x0437, 0x0443, 0x0435, 0x043C, 0x043E, 0x043C, 0x0443, 0x0020, 0x043F, 0x043E, 0x0432, 0x0435, 0x0434, 0x0435, 0x043D, 0x0438, 0x044E, 0x0020, 0x0438, 0x0020, 0x0437, 0x0430, 0x0432, 0x0438, 0x0441, 0x0430, 0x043D, 0x0438, 0x044E, 0x0020, 0x0438, 0x0433, 0x0440, 0x044B, 0x002E, 0x0020, 0x0414, 0x043B, 0x044F, 0x0020, 0x043F, 0x043E, 0x043B, 0x0443, 0x0447, 0x0435, 0x043D, 0x0438, 0x044F, 0x0020, 0x0438, 0x043D, 0x0444, 0x043E, 0x0440, 0x043C, 0x0430, 0x0446, 0x0438, 0x0438, 0x0020, 0x043E, 0x0020, 0x0441, 0x0438, 0x0441, 0x0442, 0x0435, 0x043C, 0x043D, 0x044B, 0x0445, 0x0020, 0x0442, 0x0440, 0x0435, 0x0431, 0x043E, 0x0432, 0x0430, 0x043D, 0x0438, 0x044F, 0x0445, 0x0020, 0x043E, 0x0431, 0x0440, 0x0430, 0x0442, 0x0438, 0x0442, 0x0435, 0x0441, 0x044C, 0x0020, 0x043A, 0x0020, 0x0440, 0x0443, 0x043A, 0x043E, 0x0432, 0x043E, 0x0434, 0x0441, 0x0442, 0x0432, 0x0443, 0x0020, 0x043F, 0x043E, 0x043B, 0x044C, 0x0437, 0x043E, 0x0432, 0x0430, 0x0442, 0x0435, 0x043B, 0x044F, 0x002E, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x0076, 0x0065, 0x006E, 0x0064, 0x006F, 0x0072, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x0064, 0x0065, 0x0076, 0x0069, 0x0063, 0x0065, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }
    case 0x1f: // Turkish
    {
        static const wchar_t fmt[] = {0x0044, 0x0065, 0x0073, 0x0074, 0x0065, 0x006B, 0x006C, 0x0065, 0x006E, 0x006D, 0x0065, 0x0079, 0x0065, 0x006E, 0x0020, 0x0062, 0x0069, 0x0072, 0x0020, 0x0065, 0x006B, 0x0072, 0x0061, 0x006E, 0x0020, 0x006B, 0x0061, 0x0072, 0x0074, 0x0131, 0x0020, 0x0061, 0x006C, 0x0067, 0x0131, 0x006C, 0x0061, 0x006E, 0x0064, 0x0131, 0x0021, 0x0020, 0x0044, 0x0065, 0x0076, 0x0061, 0x006D, 0x0020, 0x0065, 0x0074, 0x006D, 0x0065, 0x006B, 0x0020, 0x0062, 0x0065, 0x006B, 0x006C, 0x0065, 0x006E, 0x006D, 0x0065, 0x0064, 0x0069, 0x006B, 0x0020, 0x0073, 0x006F, 0x006E, 0x0075, 0x00E7, 0x006C, 0x0061, 0x0072, 0x0061, 0x0020, 0x0076, 0x0065, 0x0020, 0x00E7, 0x00F6, 0x006B, 0x006D, 0x0065, 0x006C, 0x0065, 0x0072, 0x0065, 0x0020, 0x0079, 0x006F, 0x006C, 0x0020, 0x0061, 0x00E7, 0x0061, 0x0062, 0x0069, 0x006C, 0x0069, 0x0072, 0x002E, 0x0020, 0x0044, 0x006F, 0x006E, 0x0061, 0x006E, 0x0131, 0x006D, 0x0020, 0x0067, 0x0065, 0x0072, 0x0065, 0x006B, 0x006C, 0x0069, 0x006C, 0x0069, 0x006B, 0x006C, 0x0065, 0x0072, 0x0069, 0x0020, 0x0069, 0x00E7, 0x0069, 0x006E, 0x0020, 0x006C, 0x00FC, 0x0074, 0x0066, 0x0065, 0x006E, 0x0020, 0x0072, 0x0065, 0x0068, 0x0062, 0x0065, 0x0072, 0x0069, 0x006E, 0x0069, 0x007A, 0x0065, 0x0020, 0x0062, 0x0061, 0x015F, 0x0076, 0x0075, 0x0072, 0x0075, 0x006E, 0x002E, 0x000A, 0x000A, 0x0022, 0x0025, 0x0053, 0x0022, 0x0020, 0x005B, 0x0076, 0x0065, 0x006E, 0x0064, 0x006F, 0x0072, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x002C, 0x0020, 0x0064, 0x0065, 0x0076, 0x0069, 0x0063, 0x0065, 0x0020, 0x0069, 0x0064, 0x0020, 0x003D, 0x0020, 0x0030, 0x0078, 0x0025, 0x002E, 0x0034, 0x0078, 0x005D, 0};
        pFmt = fmt;
        break;
    }

    case 0x09: // English
    default:
        break;
    }

    wchar_t msg[1024];
    msg[0] = L'\0';
    msg[sizeof(msg) / sizeof(msg[0]) - 1] = L'\0';
    azsnwprintf(msg, sizeof(msg) / sizeof(msg[0]) - 1, pFmt, gpuName, gpuVendorId, gpuDeviceId);

    return msg;
}
#endif

bool CSystem::OpenRenderLibrary(int type, const SSystemInitParams& initParams)
{
    LOADING_TIME_PROFILE_SECTION;
#if defined(WIN32) || defined(WIN64)
    if (!gEnv->IsDedicated())
    {
        unsigned int gpuVendorId = 0, gpuDeviceId = 0, totVidMem = 0;
        char gpuName[256];
        Win32SysInspect::DXFeatureLevel featureLevel = Win32SysInspect::DXFL_Undefined;
        Win32SysInspect::GetGPUInfo(gpuName, sizeof(gpuName), gpuVendorId, gpuDeviceId, totVidMem, featureLevel);

        if (m_env.IsEditor())
        {
#if defined(EXTERNAL_CRASH_REPORTING)
            CrashHandler::CrashHandlerBase::AddAnnotation("dx.feature.level", Win32SysInspect::GetFeatureLevelAsString(featureLevel));
            CrashHandler::CrashHandlerBase::AddAnnotation("gpu.name", gpuName);
            CrashHandler::CrashHandlerBase::AddAnnotation("gpu.vendorId", std::to_string(gpuVendorId));
            CrashHandler::CrashHandlerBase::AddAnnotation("gpu.deviceId", std::to_string(gpuDeviceId));
            CrashHandler::CrashHandlerBase::AddAnnotation("gpu.memory", std::to_string(totVidMem));
#endif
        }
        else
        {
            if (featureLevel < Win32SysInspect::DXFL_11_0)
            {
                const char logMsgFmt[] ("Unsupported GPU configuration!\n- %s (vendor = 0x%.4x, device = 0x%.4x)\n- Dedicated video memory: %d MB\n- Feature level: %s\n");
                AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, logMsgFmt, gpuName, gpuVendorId, gpuDeviceId, totVidMem >> 20, GetFeatureLevelAsString(featureLevel));

#if !defined(_RELEASE)
                const bool allowPrompts = m_env.pSystem->GetICmdLine()->FindArg(eCLAT_Pre, "noprompt") == 0;
#else
                const bool allowPrompts = true;
#endif // !defined(_RELEASE)
                if (allowPrompts)
                {
                    AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Asking user if they wish to continue...");
                    const int mbRes = MessageBoxW(0, GetErrorStringUnsupportedGPU(gpuName, gpuVendorId, gpuDeviceId).c_str(), L"Open 3D Engine", MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2 | MB_DEFAULT_DESKTOP_ONLY);
                    if (mbRes == IDCANCEL)
                    {
                        AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "User chose to cancel startup due to unsupported GPU.");
                        return false;
                    }
                }
                else
                {
#if !defined(_RELEASE)
                    const bool obeyGPUCheck = m_env.pSystem->GetICmdLine()->FindArg(eCLAT_Pre, "anygpu") == 0;
#else
                    const bool obeyGPUCheck = true;
#endif // !defined(_RELEASE)
                    if (obeyGPUCheck)
                    {
                        AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "No prompts allowed and unsupported GPU check active. Treating unsupported GPU as error and exiting.");
                        return false;
                    }
                }

                AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "User chose to continue despite unsupported GPU!");
            }
        }
    }
#endif
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_7
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif

    if (gEnv->IsDedicated())
    {
        type = R_NULL_RENDERER;
    }
    const char* libname = "";
    if (AZ::Interface<AzFramework::AtomActiveInterface>::Get())
    {
        libname = "CryRenderOther";
    }
    else if (type == R_DX9_RENDERER)
    {
        libname = DLL_RENDERER_DX9;
    }
    else if (type == R_DX11_RENDERER)
    {
        libname = DLL_RENDERER_DX11;
    }
    else if (type == R_DX12_RENDERER)
    {
        libname = DLL_RENDERER_DX12;
    }
    else if (type == R_NULL_RENDERER)
    {
        libname = DLL_RENDERER_NULL;
    }
    else if (type == R_GL_RENDERER)
    {
        libname = DLL_RENDERER_GL;
    }
    else if (type == R_METAL_RENDERER)
    {
        libname = DLL_RENDERER_METAL;
    }
    else
    {
        AZ_Assert(false, "Renderer did not initialize correctly; no valid renderer specified.");
        return false;
    }

    if (!InitializeEngineModule(libname, "EngineModule_CryRenderer", initParams))
    {
        return false;
    }

    if (!m_env.pRenderer)
    {
        AZ_Assert(false, "Renderer did not initialize correctly; it could not be found in the system environment.");
        return false;
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitConsole()
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    if (m_env.pConsole)
    {
        m_env.pConsole->Init(this);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
// attaches the given variable to the given container;
// recreates the variable if necessary
ICVar* CSystem::attachVariable (const char* szVarName, int* pContainer, const char* szComment, int dwFlags)
{
    IConsole* pConsole = GetIConsole();

    ICVar* pOldVar = pConsole->GetCVar (szVarName);
    int nDefault;
    if (pOldVar)
    {
        nDefault = pOldVar->GetIVal();
        pConsole->UnregisterVariable(szVarName, true);
    }

    // NOTE: maybe we should preserve the actual value of the variable across the registration,
    // because of the strange architecture of IConsole that converts int->float->int

    REGISTER_CVAR2(szVarName, pContainer, *pContainer, dwFlags, szComment);

    ICVar* pVar = pConsole->GetCVar(szVarName);

#ifdef _DEBUG
    // test if the variable really has this container
    assert (*pContainer == pVar->GetIVal());
    ++*pContainer;
    assert (*pContainer == pVar->GetIVal());
    --*pContainer;
#endif

    if (pOldVar)
    {
        // carry on the default value from the old variable anyway
        pVar->Set(nDefault);
    }
    return pVar;
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitRenderer(WIN_HINSTANCE hinst, WIN_HWND hwnd, const SSystemInitParams& initParams)
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    if (m_pUserCallback)
    {
        m_pUserCallback->OnInitProgress("Initializing Renderer...");
    }

    if (m_bEditor)
    {
        m_env.pConsole->GetCVar("r_Width");

        // save current screen width/height/bpp, so they can be restored on shutdown
        m_iWidth = m_env.pConsole->GetCVar("r_Width")->GetIVal();
        m_iHeight = m_env.pConsole->GetCVar("r_Height")->GetIVal();
        m_iColorBits = m_env.pConsole->GetCVar("r_ColorBits")->GetIVal();
    }

    if (!OpenRenderLibrary(m_rDriver->GetString(), initParams))
    {
        return false;
    }

#if defined(AZ_PLATFORM_IOS) || defined(AZ_PLATFORM_ANDROID)
    if (m_rWidthAndHeightAsFractionOfScreenSize->GetFlags() & VF_WASINCONFIG)
    {
        int displayWidth = 0;
        int displayHeight = 0;
        if (GetPrimaryPhysicalDisplayDimensions(displayWidth, displayHeight))
        {
            // Ideally we would probably want to clamp this at the source,
            // but I don't believe cvars support specifying a valid range.
            float scaleFactor = 1.0f;

            if(IsTablet())
            {
                scaleFactor = AZ::GetClamp(m_rTabletWidthAndHeightAsFractionOfScreenSize->GetFVal(), 0.1f, 1.0f);
            }
            else
            {
                scaleFactor = AZ::GetClamp(m_rWidthAndHeightAsFractionOfScreenSize->GetFVal(), 0.1f, 1.0f);
            }

            displayWidth *= scaleFactor;
            displayHeight *= scaleFactor;

            const int maxWidth = m_rMaxWidth->GetIVal();
            if (maxWidth > 0 && maxWidth < displayWidth)
            {
                const float widthScaleFactor = static_cast<float>(maxWidth) / static_cast<float>(displayWidth);
                displayWidth *= widthScaleFactor;
                displayHeight *= widthScaleFactor;
            }

            const int maxHeight = m_rMaxHeight->GetIVal();
            if (maxHeight > 0 && maxHeight < displayHeight)
            {
                const float heightScaleFactor = static_cast<float>(maxHeight) / static_cast<float>(displayHeight);
                displayWidth *= heightScaleFactor;
                displayHeight *= heightScaleFactor;
            }

            m_rWidth->Set(displayWidth);
            m_rHeight->Set(displayHeight);
        }
    }
#endif // defined(AZ_PLATFORM_IOS) || defined(AZ_PLATFORM_ANDROID)

    if (m_env.pRenderer)
    {
        // This is crucial as textures suffix are hard coded to context and we need to initialize
        // the texture semantics to look it up.
        m_env.pRenderer->InitTexturesSemantics();

#ifdef WIN32
        SCustomRenderInitArgs args;
        args.appStartedFromMediaCenter = strstr(initParams.szSystemCmdLine, "ReLaunchMediaCenter") != 0;

        m_hWnd = m_env.pRenderer->Init(0, 0, m_rWidth->GetIVal(), m_rHeight->GetIVal(), m_rColorBits->GetIVal(), m_rDepthBits->GetIVal(), m_rStencilBits->GetIVal(), m_rFullscreen->GetIVal() ? true : false, initParams.bEditor, hinst, hwnd, false, &args, initParams.bShaderCacheGen);
        //Timur, Not very clean code, we need to push new hwnd value to the system init params, so other modules can used when initializing.
        (const_cast<SSystemInitParams*>(&initParams))->hWnd = m_hWnd;


        bool retVal = (initParams.bShaderCacheGen || m_hWnd != 0);
        AZ_Assert(retVal, "Renderer failed to initialize correctly.");
        return retVal;
#else   // WIN32
        WIN_HWND h = m_env.pRenderer->Init(0, 0, m_rWidth->GetIVal(), m_rHeight->GetIVal(), m_rColorBits->GetIVal(), m_rDepthBits->GetIVal(), m_rStencilBits->GetIVal(), m_rFullscreen->GetIVal() ? true : false, initParams.bEditor, hinst, hwnd, false, nullptr, initParams.bShaderCacheGen);

#if (defined(LINUX) && !defined(AZ_PLATFORM_ANDROID))
        return true;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_8
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
        bool retVal = (initParams.bShaderCacheGen || h != 0);
        if (retVal)
        {
            return true;
        }

        AZ_Assert(false, "Renderer failed to initialize correctly.");
        return false;
#endif
#endif
    }
    return true;
}



/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitFileSystem()
{
    LOADING_TIME_PROFILE_SECTION;
    using namespace AzFramework::AssetSystem;

    if (m_pUserCallback)
    {
        m_pUserCallback->OnInitProgress("Initializing File System...");
    }

    // get the DirectInstance FileIOBase which should be the AZ::LocalFileIO
    m_env.pFileIO = AZ::IO::FileIOBase::GetDirectInstance();
    m_env.pResourceCompilerHelper = nullptr;

    m_env.pCryPak = AZ::Interface<AZ::IO::IArchive>::Get();
    m_env.pFileIO = AZ::IO::FileIOBase::GetInstance();
    AZ_Assert(m_env.pCryPak, "CryPak has not been initialized on AZ::Interface");
    AZ_Assert(m_env.pFileIO, "FileIOBase has not been initialized");

    if (m_bEditor)
    {
        m_env.pCryPak->RecordFileOpen(AZ::IO::IArchive::RFOM_EngineStartup);
    }

    //init crypak
    if (m_env.pCryPak->Init(""))
    {
#if !defined(_RELEASE)
        const ICmdLineArg* pakalias = m_pCmdLine->FindArg(eCLAT_Pre, "pakalias");
#else
        const ICmdLineArg* pakalias = nullptr;
#endif // !defined(_RELEASE)
        if (pakalias && strlen(pakalias->GetValue()) > 0)
        {
            m_env.pCryPak->ParseAliases(pakalias->GetValue());
        }
    }
    else
    {
        AZ_Assert(false, "Failed to initialize CryPak.");
        return false;
    }

    // Now that file systems are init, we will clear any events that have arrived
    // during file system init, so that systems do not reload assets that were already compiled in the
    // critical compilation section.

    AzFramework::LegacyAssetEventBus::ClearQueuedEvents();

    //we are good to go
    return true;
}


void CSystem::ShutdownFileSystem()
{
#if defined(AZ_PLATFORM_WINDOWS)
    if (g_cacheLock != INVALID_HANDLE_VALUE)
    {
        CloseHandle(g_cacheLock);
        g_cacheLock = INVALID_HANDLE_VALUE;
    }
#endif

    using namespace AZ::IO;

    FileIOBase* directInstance = FileIOBase::GetDirectInstance();
    FileIOBase* pakInstance = FileIOBase::GetInstance();

    if (directInstance == m_env.pFileIO)
    {
        // we only mess with file io if we own the instance that we installed.
        // if we dont' own the instance, then we never configured fileIO and we should not alter it.
        delete directInstance;
        FileIOBase::SetDirectInstance(nullptr);

        if (pakInstance != directInstance)
        {
            delete pakInstance;
            FileIOBase::SetInstance(nullptr);
        }
    }

    m_env.pFileIO = nullptr;
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitFileSystem_LoadEngineFolders(const SSystemInitParams& initParams)
{
    LOADING_TIME_PROFILE_SECTION;
    {
        ILoadConfigurationEntrySink* pCVarsWhiteListConfigSink = GetCVarsWhiteListConfigSink();
        LoadConfiguration(m_systemConfigName.c_str(), pCVarsWhiteListConfigSink);
        AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Loading system configuration from %s...", m_systemConfigName.c_str());
    }

#if defined(AZ_PLATFORM_ANDROID)
    AZ::Android::Utils::SetLoadFilesToMemory(m_sys_load_files_to_memory->GetString());
#endif

    GetISystem()->SetConfigPlatform(GetDevicePlatform());

#if defined(CRY_ENABLE_RC_HELPER)
    if (!m_env.pResourceCompilerHelper)
    {
        m_env.pResourceCompilerHelper = new CResourceCompilerHelper();
    }
#endif

    auto projectPath = AZ::Utils::GetProjectPath();
    AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Project Path: %s\n", projectPath.empty() ? "None specified" : projectPath.c_str());

    auto projectName = AZ::Utils::GetProjectName();
    AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Project Name: %s\n", projectName.empty() ? "None specified" : projectName.c_str());

    // simply open all paks if fast load pak can't be found
    if (!m_pResourceManager->LoadFastLoadPaks(true))
    {
        OpenBasicPaks();
    }


    // Load game-specific folder.
    LoadConfiguration("game.cfg");
    // Load the client/sever-specific configuration
    static const char* g_additionalConfig = gEnv->IsDedicated() ? "server_cfg" : "client_cfg";
    LoadConfiguration(g_additionalConfig, nullptr, false);

    if (initParams.bShaderCacheGen)
    {
        LoadConfiguration("shadercachegen.cfg");
    }
    // We do not use CVar groups on the consoles
    AddCVarGroupDirectory("Config/CVarGroups");

    return (true);
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::InitStreamEngine()
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    if (m_pUserCallback)
    {
        m_pUserCallback->OnInitProgress("Initializing Stream Engine...");
    }

    m_pStreamEngine = new CStreamEngine();

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitFont(const SSystemInitParams& initParams)
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    bool fontInited = false;
    AZ::CryFontCreationRequestBus::BroadcastResult(fontInited, &AZ::CryFontCreationRequests::CreateCryFont, m_env, initParams);
    if (!fontInited && !InitializeEngineModule(DLL_FONT, "EngineModule_CryFont", initParams))
    {
        return false;
    }

    if (!m_env.pCryFont)
    {
        AZ_Assert(false, "Font System did not initialize correctly; it could not be found in the system environment");
        return false;
    }

    if (gEnv->IsDedicated())
    {
        return true;
    }

    if (!LoadFontInternal(m_pIFont, "default"))
    {
        return false;
    }

    if (!LoadFontInternal(m_pIFontUi, "default-ui"))
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::Init3DEngine(const SSystemInitParams& initParams)
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    if (!InitializeEngineModule(DLL_3DENGINE, "EngineModule_Cry3DEngine", initParams))
    {
        return false;
    }

    if (!m_env.p3DEngine)
    {
        AZ_Assert(false, "3D Engine did not initialize correctly; it could not be found in the system environment");
        return false;
    }

    if (!m_env.p3DEngine->Init())
    {
        return false;
    }
    m_pProcess = m_env.p3DEngine;
    m_pProcess->SetFlags(PROC_3DENGINE);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::InitAudioSystem(const SSystemInitParams& initParams)
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    if (!Audio::Gem::AudioSystemGemRequestBus::HasHandlers())
    {
        // AudioSystem Gem has not been enabled for this project.
        // This should not generate an error, but calling scope will warn.
        return false;
    }

    bool useRealAudioSystem = false;
    if (!initParams.bPreview
        && !initParams.bShaderCacheGen
        && !initParams.bMinimal
        && !m_bDedicatedServer
        && m_sys_audio_disable->GetIVal() == 0)
    {
        useRealAudioSystem = true;
    }

    bool result = false;
    if (useRealAudioSystem)
    {
        Audio::Gem::AudioSystemGemRequestBus::BroadcastResult(result, &Audio::Gem::AudioSystemGemRequestBus::Events::Initialize, &initParams);
    }
    else
    {
        Audio::Gem::AudioSystemGemRequestBus::BroadcastResult(result, &Audio::Gem::AudioSystemGemRequestBus::Events::Initialize, nullptr);
    }

    if (result)
    {
        AZ_Assert(Audio::AudioSystemRequestBus::HasHandlers(),
            "Initialization of the Audio System succeeded, but the Audio System EBus is not connected!\n");
    }
    else
    {
        AZ_Error(AZ_TRACE_SYSTEM_WINDOW, result, "The Audio System did not initialize correctly!\n");
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::InitVTuneProfiler()
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

#ifdef PROFILE_WITH_VTUNE

    WIN_HMODULE hModule = LoadDLL("VTuneApi.dll");
    if (!hModule)
    {
        return false;
    }

        VTPause = (VTuneFunction) CryGetProcAddress(hModule, "VTPause");
        VTResume = (VTuneFunction) CryGetProcAddress(hModule, "VTResume");
        if (!VTPause || !VTResume)
        {
        AZ_Assert(false, "VTune did not initialize correctly.")
        return false;
    }
    else
    {
        AZ_TracePrintf(AZ_TRACE_SYSTEM_WINDOW, "VTune API Initialized");
    }
#endif //PROFILE_WITH_VTUNE

    return true;
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitShine([[maybe_unused]] const SSystemInitParams& initParams)
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());

    EBUS_EVENT(UiSystemBus, InitializeSystem);

    if (!m_env.pLyShine)
    {
        AZ_Error(AZ_TRACE_SYSTEM_WINDOW, false, "LYShine System did not initialize correctly. Please check that the LyShine gem is enabled for this project in ProjectConfigurator.");
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CSystem::InitLocalization()
{
    LOADING_TIME_PROFILE_SECTION(GetISystem());
    // Set the localization folder
    ICVar* pCVar = m_env.pConsole != 0 ? m_env.pConsole->GetCVar("sys_localization_folder") : 0;
    if (pCVar)
    {
        static_cast<AZ::IO::Archive* const>(m_env.pCryPak)->SetLocalizationFolder(g_cvars.sys_localization_folder->GetString());
    }

    // Removed line that assigned language based on a #define

    if (m_pLocalizationManager == nullptr)
    {
        m_pLocalizationManager = new CLocalizedStringsManager(this);
    }

    // Platform-specific implementation of getting the system language
    ILocalizationManager::EPlatformIndependentLanguageID languageID = m_pLocalizationManager->GetSystemLanguage();
    if (!m_pLocalizationManager->IsLanguageSupported(languageID))
    {
        languageID = ILocalizationManager::EPlatformIndependentLanguageID::ePILID_English_US;
    }

    string language = m_pLocalizationManager->LangNameFromPILID(languageID);
    m_pLocalizationManager->SetLanguage(language.c_str());
    if (m_pLocalizationManager->GetLocalizationFormat() == 1)
    {
        string translationsListXML = LOCALIZATION_TRANSLATIONS_LIST_FILE_NAME;
        m_pLocalizationManager->InitLocalizationData(translationsListXML);

        m_pLocalizationManager->LoadAllLocalizationData();
    }
    else
    {
        // if the language value cannot be found, let's default to the english pak
        OpenLanguagePak(language);
    }

    pCVar = m_env.pConsole != 0 ? m_env.pConsole->GetCVar("g_languageAudio") : 0;
    if (pCVar)
    {
        if (strlen(pCVar->GetString()) == 0)
        {
            pCVar->Set(language);
        }
        else
        {
            language = pCVar->GetString();
        }
    }
    OpenLanguageAudioPak(language);
}

void CSystem::OpenBasicPaks()
{
    static bool bBasicPaksLoaded = false;
    if (bBasicPaksLoaded)
    {
        return;
    }
    bBasicPaksLoaded = true;

    LOADING_TIME_PROFILE_SECTION;

    // open pak files
    constexpr AZStd::string_view paksFolder = "@assets@/*.pak"; // (@assets@ assumed)
    m_env.pCryPak->OpenPacks(paksFolder);

    InlineInitializationProcessing("CSystem::OpenBasicPaks OpenPacks( paksFolder.c_str() )");

    //////////////////////////////////////////////////////////////////////////
    // Open engine packs
    //////////////////////////////////////////////////////////////////////////

    const char* const assetsDir = "@assets@";
    const char* shaderCachePakDir = "@assets@/shadercache.pak";
    const char* shaderCacheStartupPakDir = "@assets@/shadercachestartup.pak";

    // After game paks to have same search order as with files on disk
    m_env.pCryPak->OpenPack(assetsDir, "Engine.pak");

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_15
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif

    m_env.pCryPak->OpenPack(assetsDir, shaderCachePakDir);
    m_env.pCryPak->OpenPack(assetsDir, shaderCacheStartupPakDir);
    m_env.pCryPak->OpenPack(assetsDir, "Shaders.pak");
    m_env.pCryPak->OpenPack(assetsDir, "ShadersBin.pak");

#ifdef AZ_PLATFORM_ANDROID
    // Load Android Obb files if available
    const char* obbStorage = AZ::Android::Utils::GetObbStoragePath();
    AZStd::string mainObbPath = AZStd::move(AZStd::string::format("%s/%s", obbStorage, AZ::Android::Utils::GetObbFileName(true)));
    AZStd::string patchObbPath = AZStd::move(AZStd::string::format("%s/%s", obbStorage, AZ::Android::Utils::GetObbFileName(false)));
    m_env.pCryPak->OpenPack(assetsDir, mainObbPath.c_str());
    m_env.pCryPak->OpenPack(assetsDir, patchObbPath.c_str());
#endif //AZ_PLATFORM_ANDROID

    InlineInitializationProcessing("CSystem::OpenBasicPaks OpenPacks( Engine... )");

    //////////////////////////////////////////////////////////////////////////
    // Open paks in MOD subfolders.
    //////////////////////////////////////////////////////////////////////////
#if !defined(_RELEASE)
    if (const ICmdLineArg* pModArg = GetICmdLine()->FindArg(eCLAT_Pre, "MOD"))
    {
        if (IsMODValid(pModArg->GetValue()))
        {
            AZStd::string modFolder = "Mods\\";
            modFolder += pModArg->GetValue();
            modFolder += "\\*.pak";
            GetIPak()->OpenPacks(assetsDir, modFolder, AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::INestedArchive::FLAGS_OVERRIDE_PAK);
        }
    }
#endif // !defined(_RELEASE)

    // Load paks required for game init to mem
    gEnv->pCryPak->LoadPakToMemory("Engine.pak", AZ::IO::IArchive::eInMemoryPakLocale_GPU);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::OpenLanguagePak(const char* sLanguage)
{
    // Don't attempt to open a language PAK file if the game doesn't have a
    // loc folder configured.
    bool projUsesLocalization = false;
    LocalizationManagerRequestBus::BroadcastResult(projUsesLocalization, &LocalizationManagerRequestBus::Events::ProjectUsesLocalization);
    if (!projUsesLocalization)
    {
        return;
    }

    // Initialize languages.

    // Omit the trailing slash!
    string sLocalizationFolder = PathUtil::GetLocalizationFolder();

    // load xml pak with full filenames to perform wildcard searches.
    string sLocalizedPath;
    GetLocalizedPath(sLanguage, sLocalizedPath);
    if (!m_env.pCryPak->OpenPacks({ sLocalizationFolder.c_str(), sLocalizationFolder.size() }, { sLocalizedPath.c_str(), sLocalizedPath.size() }, 0))
    {
        // make sure the localized language is found - not really necessary, for TC
        AZ_Printf("Localization", "Localized language content(%s) not available or modified from the original installation.", sLanguage);
    }
}


//////////////////////////////////////////////////////////////////////////
void CSystem::OpenLanguageAudioPak([[maybe_unused]] const char* sLanguage)
{
    // Don't attempt to open a language PAK file if the game doesn't have a
    // loc folder configured.
    bool projUsesLocalization = false;
    LocalizationManagerRequestBus::BroadcastResult(projUsesLocalization, &LocalizationManagerRequestBus::Events::ProjectUsesLocalization);
    if (!projUsesLocalization)
    {
        return;
    }

    // Initialize languages.

    int nPakFlags = 0;

    // Omit the trailing slash!
    string sLocalizationFolder(string().assign(PathUtil::GetLocalizationFolder(), 0, PathUtil::GetLocalizationFolder().size() - 1));

    if (sLocalizationFolder.compareNoCase("Languages") == 0)
    {
        sLocalizationFolder = "@assets@";
    }

    // load localized pak with crc32 filenames on consoles to save memory.
    string sLocalizedPath = "loc.pak";

    if (!m_env.pCryPak->OpenPacks(sLocalizationFolder.c_str(), sLocalizedPath.c_str(), nPakFlags))
    {
        // make sure the localized language is found - not really necessary, for TC
        AZ_Error(AZ_TRACE_SYSTEM_WINDOW, false, "Localized language content(%s) not available or modified from the original installation.", sLanguage);
    }
}


string GetUniqueLogFileName(string logFileName)
{
    string logFileNamePrefix = logFileName;
    if ((logFileNamePrefix[0] != '@') && (AzFramework::StringFunc::Path::IsRelative(logFileNamePrefix)))
    {
        logFileNamePrefix = "@log@/";
        logFileNamePrefix += logFileName;
    }

    char resolvedLogFilePathBuffer[AZ_MAX_PATH_LEN] = { 0 };
    AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(logFileNamePrefix.c_str(), resolvedLogFilePathBuffer, AZ_MAX_PATH_LEN);

    int instance = gEnv->pSystem->GetApplicationLogInstance(resolvedLogFilePathBuffer);

    if (instance == 0)
    {
        return logFileNamePrefix;
    }

    string logFileExtension;
    size_t extensionIndex = logFileName.find_last_of('.');
    if (extensionIndex != string::npos)
    {
        logFileExtension = logFileName.substr(extensionIndex, logFileName.length() - extensionIndex);
        logFileNamePrefix = logFileName.substr(0, extensionIndex);
    }

    logFileName.Format("%s(%d)%s", logFileNamePrefix.c_str(), instance, logFileExtension.c_str());

    return logFileName;
}


#if defined(WIN32) || defined(WIN64)
static wstring GetErrorStringUnsupportedCPU()
{
    static const wchar_t s_EN[] = L"Unsupported CPU detected. CPU needs to support SSE, SSE2, SSE3 and SSE4.1.";
    static const wchar_t s_FR[] = { 0 };
    static const wchar_t s_RU[] = { 0 };
    static const wchar_t s_ES[] = { 0 };
    static const wchar_t s_DE[] = { 0 };
    static const wchar_t s_IT[] = { 0 };

    const size_t fullLangID = (size_t) GetKeyboardLayout(0);
    const size_t primLangID = fullLangID & 0x3FF;
    const wchar_t* pFmt = s_EN;

    /*switch (primLangID)
    {
    case 0x07: // German
        pFmt = s_DE;
        break;
    case 0x0a: // Spanish
        pFmt = s_ES;
        break;
    case 0x0c: // French
        pFmt = s_FR;
        break;
    case 0x10: // Italian
        pFmt = s_IT;
        break;
    case 0x19: // Russian
        pFmt = s_RU;
        break;
    case 0x09: // English
    default:
        break;
    }*/
    wchar_t msg[1024];
    msg[0] = L'\0';
    msg[sizeof(msg) / sizeof(msg[0]) - 1] = L'\0';
    azsnwprintf(msg, sizeof(msg) / sizeof(msg[0]) - 1, pFmt);
    return msg;
}
#endif

static bool CheckCPURequirements([[maybe_unused]] CCpuFeatures* pCpu, [[maybe_unused]] CSystem* pSystem)
{
#if defined(WIN32) || defined(WIN64)
    if (!gEnv->IsDedicated())
    {
        if (!(pCpu->hasSSE() && pCpu->hasSSE2() && pCpu->hasSSE3() && pCpu->hasSSE41()))
        {
            AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Unsupported CPU! Need SSE, SSE2, SSE3 and SSE4.1 instructions to be available.");

#if !defined(_RELEASE)
            const bool allowPrompts = pSystem->GetICmdLine()->FindArg(eCLAT_Pre, "noprompt") == 0;
#else
            const bool allowPrompts = true;
#endif // !defined(_RELEASE)
            if (allowPrompts)
            {
                AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Asking user if they wish to continue...");
                const int mbRes = MessageBoxW(0, GetErrorStringUnsupportedCPU().c_str(), L"Open 3D Engine", MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2 | MB_DEFAULT_DESKTOP_ONLY);
                if (mbRes == IDCANCEL)
                {
                    AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "User chose to cancel startup.");
                    return false;
                }
            }
            else
            {
#if !defined(_RELEASE)
                const bool obeyCPUCheck = pSystem->GetICmdLine()->FindArg(eCLAT_Pre, "anycpu") == 0;
#else
                const bool obeyCPUCheck = true;
#endif // !defined(_RELEASE)
                if (obeyCPUCheck)
                {
                    AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "No prompts allowed and unsupported CPU check active. Treating unsupported CPU as error and exiting.");
                    return false;
                }
            }

            AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "User chose to continue despite unsupported CPU!");
        }
    }
#endif
    return true;
}


class AzConsoleToCryConsoleBinder final
{
public:
    static void OnInvoke(IConsoleCmdArgs* args)
    {
        std::string command = args->GetCommandLine();
        const size_t delim = command.find_first_of('=');
        if (delim != std::string::npos)
        {
            // All Cry executed cfg files will come in through this pathway in addition to regular commands
            // We strip out the = sign at this layer to maintain compatibility with cvars that use the '=' as a separator
            // Swap the '=' character for a space
            command[delim] = ' ';
        }

        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(command.c_str(), AZ::ConsoleSilentMode::Silent, AZ::ConsoleInvokedFrom::CryBinding);
    }

    static void OnVarChanged(ICVar* cvar)
    {
        AZ::CVarFixedString command = AZ::CVarFixedString::format("%s %s", cvar->GetName(), cvar->GetString());
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(command.c_str(), AZ::ConsoleSilentMode::Silent, AZ::ConsoleInvokedFrom::CryBinding);
    }

    static void Visit(AZ::ConsoleFunctorBase* functor)
    {
        if (gEnv->pConsole == nullptr)
        {
            AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Cry console was NULL while attempting to register Az CVars and CFuncs.\n");
            return;
        }

        int32_t cryFlags = VF_NET_SYNCED;
        if ((functor->GetFlags() & AZ::ConsoleFunctorFlags::DontReplicate) != AZ::ConsoleFunctorFlags::Null)
        {
            cryFlags = VF_NULL;
        }
        if ((functor->GetFlags() & AZ::ConsoleFunctorFlags::ServerOnly) != AZ::ConsoleFunctorFlags::Null)
        {
            cryFlags |= VF_DEDI_ONLY;
        }
        if ((functor->GetFlags() & AZ::ConsoleFunctorFlags::ReadOnly) != AZ::ConsoleFunctorFlags::Null)
        {
            cryFlags |= VF_READONLY;
        }
        if ((functor->GetFlags() & AZ::ConsoleFunctorFlags::IsCheat) != AZ::ConsoleFunctorFlags::Null)
        {
            cryFlags |= VF_CHEAT;
        }
        if ((functor->GetFlags() & AZ::ConsoleFunctorFlags::IsInvisible) != AZ::ConsoleFunctorFlags::Null)
        {
            cryFlags |= VF_INVISIBLE;
        }
        if ((functor->GetFlags() & AZ::ConsoleFunctorFlags::IsDeprecated) != AZ::ConsoleFunctorFlags::Null)
        {
            cryFlags |= VF_DEPRECATED;
        }
        if ((functor->GetFlags() & AZ::ConsoleFunctorFlags::NeedsReload) != AZ::ConsoleFunctorFlags::Null)
        {
            cryFlags |= VF_REQUIRE_APP_RESTART;
        }
        if ((functor->GetFlags() & AZ::ConsoleFunctorFlags::AllowClientSet) != AZ::ConsoleFunctorFlags::Null)
        {
            cryFlags |= VF_DEV_ONLY;
        }

        gEnv->pConsole->RemoveCommand(functor->GetName());
        if (functor->GetTypeId() != AZ::TypeId::CreateNull())
        {
            AZ::CVarFixedString value;
            functor->GetValue(value);
            gEnv->pConsole->RegisterString(functor->GetName(), value.c_str(), cryFlags, functor->GetDesc(), AzConsoleToCryConsoleBinder::OnVarChanged);
        }
        else
        {
            gEnv->pConsole->AddCommand(functor->GetName(), AzConsoleToCryConsoleBinder::OnInvoke, cryFlags, functor->GetDesc());
        }
    }

    using CommandRegisteredHandler = AZ::IConsole::ConsoleCommandRegisteredEvent::Handler;
    static inline CommandRegisteredHandler s_commandRegisteredHandler = CommandRegisteredHandler([](AZ::ConsoleFunctorBase* functor) { Visit(functor); });
};

// System initialization
/////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::Init(const SSystemInitParams& startupParams)
{
#if AZ_TRAIT_USE_CRY_SIGNAL_HANDLER
    signal(SIGSEGV, CryEngineSignalHandler);
    signal(SIGTRAP, CryEngineSignalHandler);
    signal(SIGILL, CryEngineSignalHandler);
#endif // AZ_TRAIT_USE_CRY_SIGNAL_HANDLER

    // Temporary Fix for an issue accessing gEnv from this object instance. The gEnv is not resolving to the
    // global gEnv, instead its resolving an some uninitialized gEnv elsewhere (NULL). Since gEnv is
    // initialized to this instance's SSystemGlobalEnvironment (m_env), we will force set it again here
    // to m_env
    if (!gEnv)
    {
        gEnv = &m_env;
    }

    LOADING_TIME_PROFILE_SECTION;

    SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_INIT);
    gEnv->mMainThreadId = GetCurrentThreadId();         //Set this ASAP on startup

    InlineInitializationProcessing("CSystem::Init start");
    m_szCmdLine = startupParams.szSystemCmdLine;

    m_env.szCmdLine = m_szCmdLine.c_str();
    m_env.bTesting = startupParams.bTesting;
    m_env.bNoAssertDialog = startupParams.bTesting;
    m_env.bNoRandomSeed = startupParams.bNoRandom;
    m_bShaderCacheGenMode = startupParams.bShaderCacheGen;

    m_bNoCrashDialog = gEnv->IsDedicated();

    if (startupParams.bUnattendedMode)
    {
        m_bNoCrashDialog = true;
        m_env.bNoAssertDialog = true; //this also suppresses CryMessageBox
        g_cvars.sys_no_crash_dialog = true;
    }

#if defined(AZ_PLATFORM_LINUX)
    // Linux is all console for now and so no room for dialog boxes!
    m_env.bNoAssertDialog = true;
#endif

    m_pCmdLine = new CCmdLine(startupParams.szSystemCmdLine);

    AZCoreLogSink::Connect();

    // Registers all AZ Console Variables functors specified within CrySystem
    if (auto azConsole = AZ::Interface<AZ::IConsole>::Get(); azConsole)
    {
        azConsole->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
    }
    
    if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry)
    {
        AZ::SettingsRegistryInterface::FixedValueString assetPlatform;
        if (!AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, assetPlatform,
            AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "assets"))
        {
            assetPlatform = AzFramework::OSPlatformToDefaultAssetPlatform(AZ_TRAIT_OS_PLATFORM_CODENAME);
            AZ_Warning(AZ_TRACE_SYSTEM_WINDOW, false, R"(A valid asset platform is missing in "%s/assets" key in the SettingsRegistry.)""\n"
                R"(This typically done by setting he "assets" field in the bootstrap.cfg for within a .setreg file)""\n"
                R"(A fallback of %s will be used.)",
                AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey,
                assetPlatform.c_str());
        }

        m_systemConfigName = "system_" AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER "_";
        m_systemConfigName += assetPlatform.c_str();
        m_systemConfigName += ".cfg";
    }

    AZ_Assert(CryMemory::IsHeapValid(), "Memory heap must be valid before continuing SystemInit.");

#ifdef EXTENSION_SYSTEM_INCLUDE_TESTCASES
    TestExtensions(&CCryFactoryRegistryImpl::Access());
#endif

    //_controlfp(0, _EM_INVALID|_EM_ZERODIVIDE | _PC_64 );

#if defined(WIN32) || defined(WIN64)
    // check OS version - we only want to run on XP or higher - talk to Martin Mittring if you want to change this
    {
        OSVERSIONINFO osvi;

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
AZ_PUSH_DISABLE_WARNING(4996, "-Wunknown-warning-option")
        GetVersionExA(&osvi);
AZ_POP_DISABLE_WARNING

        bool bIsWindowsXPorLater = osvi.dwMajorVersion > 5 || (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1);

        if (!bIsWindowsXPorLater)
        {
            AZ_Error(AZ_TRACE_SYSTEM_WINDOW, false, "Open 3D Engine requires an OS version of Windows XP or later.");
            return false;
        }
    }
#endif

    m_pResourceManager->Init();

    // Get file version information.
    QueryVersionInfo();
    DetectGameFolderAccessRights();

    m_hInst = (WIN_HINSTANCE)startupParams.hInstance;
    m_hWnd = (WIN_HWND)startupParams.hWnd;

    m_bEditor = startupParams.bEditor;
    m_bPreviewMode = startupParams.bPreview;
    m_bTestMode = startupParams.bTestMode;
    m_pUserCallback = startupParams.pUserCallback;
    m_bMinimal = startupParams.bMinimal;

#if defined(CVARS_WHITELIST)
    m_pCVarsWhitelist = startupParams.pCVarsWhitelist;
#endif // defined(CVARS_WHITELIST)
    m_bDedicatedServer = startupParams.bDedicatedServer;
    m_currentLanguageAudio = "";

    memcpy(gEnv->pProtectedFunctions, startupParams.pProtectedFunctions, sizeof(startupParams.pProtectedFunctions));

#if !defined(CONSOLE)
    m_env.SetIsEditor(m_bEditor);
    m_env.SetIsEditorGameMode(false);
    m_env.SetIsEditorSimulationMode(false);
#endif

    m_env.SetToolMode(startupParams.bToolMode);
    m_env.bIsOutOfMemory = false;

    if (m_bEditor)
    {
        m_bInDevMode = true;
    }

    if (!gEnv->IsDedicated())
    {
        const ICmdLineArg* crashdialog = m_pCmdLine->FindArg(eCLAT_Post, "sys_no_crash_dialog");
        if (crashdialog)
        {
            m_bNoCrashDialog = true;
        }
    }

    if (!startupParams.pValidator)
    {
        m_pDefaultValidator = new SDefaultValidator(this);
        m_pValidator = m_pDefaultValidator;
    }
    else
    {
        m_pValidator = startupParams.pValidator;
    }

#if !defined(_RELEASE)
    if (!m_bDedicatedServer)
    {
        const ICmdLineArg* dedicated = m_pCmdLine->FindArg(eCLAT_Pre, "dedicated");
        if (dedicated)
        {
            m_bDedicatedServer = true;
        }
    }
#endif // !defined(_RELEASE)

#if !defined(CONSOLE)
    gEnv->SetIsDedicated(m_bDedicatedServer);
#endif

#if !defined(CONSOLE)
#if !defined(_RELEASE)
    bool isDaemonMode = (m_pCmdLine->FindArg(eCLAT_Pre, "daemon") != 0);
#endif // !defined(_RELEASE)

#if defined(USE_DEDICATED_SERVER_CONSOLE)

#if !defined(_RELEASE)
    bool isSimpleConsole = (m_pCmdLine->FindArg(eCLAT_Pre, "simple_console") != 0);

    if (!(isDaemonMode || isSimpleConsole))
#endif // !defined(_RELEASE)
    {
#if defined(USE_UNIXCONSOLE)
        CUNIXConsole* pConsole = new CUNIXConsole();
#if defined(LINUX)
        pUnixConsole = pConsole;
#endif
#elif defined(USE_IOSCONSOLE)
        CIOSConsole* pConsole = new CIOSConsole();
#elif defined(USE_WINDOWSCONSOLE)
        CWindowsConsole* pConsole = new CWindowsConsole();
#elif defined(USE_ANDROIDCONSOLE)
        CAndroidConsole* pConsole = new CAndroidConsole();
#else
        CNULLConsole* pConsole = new CNULLConsole(false);
#endif
        m_pTextModeConsole = static_cast<ITextModeConsole*>(pConsole);

        if (m_pUserCallback == nullptr && m_bDedicatedServer)
        {
            char headerString[128];
            m_pUserCallback = pConsole;
            pConsole->SetRequireDedicatedServer(true);

            azstrcpy(
                headerString,
                AZ_ARRAY_SIZE(headerString),
                "Open 3D Engine - "
#if defined(LINUX)
                "Linux "
#elif defined(MAC)
                "MAC "
#elif defined(IOS)
                "iOS "
#endif
                "Dedicated Server"
                " - Version ");

            char* str = headerString + strlen(headerString);
            GetProductVersion().ToString(str, sizeof(headerString) - (str - headerString));
            pConsole->SetHeader(headerString);
        }
    }
#if !defined(_RELEASE)
    else
#endif
#endif

#if !(defined(USE_DEDICATED_SERVER_CONSOLE) && defined(_RELEASE))
    {
        CNULLConsole* pConsole = new CNULLConsole(isDaemonMode);
        m_pTextModeConsole = pConsole;

        if (m_pUserCallback == nullptr && m_bDedicatedServer)
        {
            m_pUserCallback = pConsole;
        }
    }
#endif

#endif // !defined(CONSOLE)

    {
        EBUS_EVENT(CrySystemEventBus, OnCrySystemPreInitialize, *this, startupParams);

        //////////////////////////////////////////////////////////////////////////
        // File system, must be very early
        //////////////////////////////////////////////////////////////////////////
        if (!InitFileSystem())
        {
            return false;
        }
        //////////////////////////////////////////////////////////////////////////
        InlineInitializationProcessing("CSystem::Init InitFileSystem");

        m_missingAssetLogger = AZStd::make_unique<AzFramework::MissingAssetLogger>();

        //////////////////////////////////////////////////////////////////////////
        // Logging is only available after file system initialization.
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.pLog)
        {
            m_env.pLog = new CLog(this);
            if (startupParams.pLogCallback)
            {
                m_env.pLog->AddCallback(startupParams.pLogCallback);
            }

            const ICmdLineArg* logfile = m_pCmdLine->FindArg(eCLAT_Pre, "logfile"); //see if the user specified a log name, if so use it
            if (logfile && strlen(logfile->GetValue()) > 0)
            {
                m_env.pLog->SetFileName(logfile->GetValue(), startupParams.autoBackupLogs);
            }
            else if (startupParams.sLogFileName)    //otherwise see if the startup params has a log file name, if so use it
            {
                const string sUniqueLogFileName = GetUniqueLogFileName(startupParams.sLogFileName);
                m_env.pLog->SetFileName(sUniqueLogFileName.c_str(), startupParams.autoBackupLogs);
            }
            else//use the default log name
            {
                m_env.pLog->SetFileName(DEFAULT_LOG_FILENAME, startupParams.autoBackupLogs);
            }
        }
        else
        {
            m_env.pLog = startupParams.pLog;
        }

        // The log backup system expects the version number to be the first line of the log
        // so we log this immediately after setting the log filename
        LogVersion();

        //here we should be good to ask Crypak to do something

        // Initialise after pLog and CPU feature initialization
        // AND after console creation (Editor only)
        // May need access to engine folder .pak files
        gEnv->pThreadManager->GetThreadConfigManager()->LoadConfig("config/engine_core.thread_config");

        if (m_bEditor)
        {
            gEnv->pThreadManager->GetThreadConfigManager()->LoadConfig("config/engine_sandbox.thread_config");
        }

        // Setup main thread
        void* pThreadHandle = 0; // Let system figure out thread handle
        gEnv->pThreadManager->RegisterThirdPartyThread(pThreadHandle, "Main");
        m_env.pProfileLogSystem = new CProfileLogSystem();

        bool devModeEnable = true;

#if defined(_RELEASE)
        // disable devmode by default in release builds outside the editor
        devModeEnable = m_bEditor;
#endif

        // disable devmode in launcher if someone really wants to (even in non release builds)
        if (!m_bEditor && m_pCmdLine->FindArg(eCLAT_Pre, "nodevmode"))
        {
            devModeEnable = false;
        }

        SetDevMode(devModeEnable);

        //////////////////////////////////////////////////////////////////////////
        // CREATE NOTIFICATION NETWORK
        //////////////////////////////////////////////////////////////////////////
        m_pNotificationNetwork = nullptr;
#ifndef _RELEASE
    #ifndef LINUX

        if (!startupParams.bMinimal)
        {
            m_pNotificationNetwork = CNotificationNetwork::Create();
        }
    #endif//LINUX
#endif // _RELEASE

        InlineInitializationProcessing("CSystem::Init NotificationNetwork");

        //////////////////////////////////////////////////////////////////////////
        // CREATE CONSOLE
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bSkipConsole)
        {
            m_env.pConsole = new CXConsole;

            if (startupParams.pPrintSync)
            {
                m_env.pConsole->AddOutputPrintSink(startupParams.pPrintSync);
            }
        }

        //////////////////////////////////////////////////////////////////////////

        if (m_pUserCallback)
        {
            m_pUserCallback->OnInit(this);
        }

        m_env.pLog->RegisterConsoleVariables();

        GetIRemoteConsole()->RegisterConsoleVariables();

        if (!startupParams.bSkipConsole)
        {
            // Register system console variables.
            CreateSystemVars();

            // Register Audio-related system CVars
            CreateAudioVars();

            // Register any AZ CVar commands created above with the AZ Console system.
            AZ::ConsoleFunctorBase*& deferredHead = AZ::ConsoleFunctorBase::GetDeferredHead();
            AZ::Interface<AZ::IConsole>::Get()->LinkDeferredFunctors(deferredHead);

            // Callback
            if (m_pUserCallback && m_env.pConsole)
            {
                m_pUserCallback->OnConsoleCreated(m_env.pConsole);
            }

            // Let listeners know its safe to register cvars
            EBUS_EVENT(CrySystemEventBus, OnCrySystemCVarRegistry);
        }


        // Set this as soon as the system cvars got initialized.
        static_cast<AZ::IO::Archive* const>(m_env.pCryPak)->SetLocalizationFolder(g_cvars.sys_localization_folder->GetString());

        InlineInitializationProcessing("CSystem::Init Create console");

        if (!startupParams.bSkipRenderer)
        {
            CreateRendererVars(startupParams);
        }

        // Need to load the engine.pak that includes the config files needed during initialization
        m_env.pCryPak->OpenPack("@assets@", "Engine.pak");
#if defined(AZ_PLATFORM_ANDROID) || defined(AZ_PLATFORM_IOS)
        MobileSysInspect::LoadDeviceSpecMapping();
#endif

        InitFileSystem_LoadEngineFolders(startupParams);

#if !defined(RELEASE) || defined(RELEASE_LOGGING)
        // now that the system cfgs have been loaded, we can start the remote console
        GetIRemoteConsole()->Update();
#endif

        // CPU features detection.
        m_pCpu = new CCpuFeatures;
        m_pCpu->Detect();
        m_env.pi.numCoresAvailableToProcess = m_pCpu->GetCPUCount();
        m_env.pi.numLogicalProcessors = m_pCpu->GetLogicalCPUCount();

        // Check hard minimum CPU requirements
        if (!CheckCPURequirements(m_pCpu, this))
        {
            return false;
        }

        if (!startupParams.bSkipConsole)
        {
            LogSystemInfo();
        }

        InlineInitializationProcessing("CSystem::Init Load Engine Folders");

        //////////////////////////////////////////////////////////////////////////
        //Load config files
        //////////////////////////////////////////////////////////////////////////

        int curSpecVal = 0;
        ICVar* pSysSpecCVar = gEnv->pConsole->GetCVar("r_GraphicsQuality");
        if (gEnv->pSystem->IsDevMode())
        {
            if (pSysSpecCVar && pSysSpecCVar->GetFlags() & VF_WASINCONFIG)
            {
                curSpecVal = pSysSpecCVar->GetIVal();
                pSysSpecCVar->SetFlags(pSysSpecCVar->GetFlags() | VF_SYSSPEC_OVERWRITE);
            }
        }

        // tools may not interact with @user@
        if (!gEnv->IsInToolMode())
        {
            if (m_pCmdLine->FindArg(eCLAT_Pre, "ResetProfile") == 0)
            {
                LoadConfiguration("@user@/game.cfg", 0, false);
            }
        }

        // If sys spec variable was specified, is not 0, and we are in devmode restore the value from before loading game.cfg
        // This enables setting of a specific sys_spec outside menu and game.cfg
        if (gEnv->pSystem->IsDevMode())
        {
            if (pSysSpecCVar && curSpecVal && curSpecVal != pSysSpecCVar->GetIVal())
            {
                pSysSpecCVar->Set(curSpecVal);
            }
        }

        {
            ILoadConfigurationEntrySink* pCVarsWhiteListConfigSink = GetCVarsWhiteListConfigSink();

            // We have to load this file again since first time we did it without devmode
            LoadConfiguration(m_systemConfigName.c_str(), pCVarsWhiteListConfigSink);
            // Optional user defined overrides
            LoadConfiguration("user.cfg", pCVarsWhiteListConfigSink);

            if (!startupParams.bSkipRenderer)
            {
                // Load the hmd.cfg if it exists. This will enable optional stereo rendering.
                LoadConfiguration("hmd.cfg");
            }

            if (startupParams.bShaderCacheGen)
            {
                LoadConfiguration("shadercachegen.cfg", pCVarsWhiteListConfigSink);
            }

#if defined(ENABLE_STATS_AGENT)
            if (m_pCmdLine->FindArg(eCLAT_Pre, "useamblecfg"))
            {
                LoadConfiguration("amble.cfg", pCVarsWhiteListConfigSink);
            }
#endif
        }

#if defined(PERFORMANCE_BUILD)
        LoadConfiguration("performance.cfg");
#endif

        //////////////////////////////////////////////////////////////////////////
        if (g_cvars.sys_asserts == 0)
        {
            gEnv->bIgnoreAllAsserts = true;
        }
        if (g_cvars.sys_asserts == 2)
        {
            gEnv->bNoAssertDialog = true;
        }

        //////////////////////////////////////////////////////////////////////////
        // Stream Engine
        //////////////////////////////////////////////////////////////////////////
        AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Stream Engine Initialization");
        InitStreamEngine();
        InlineInitializationProcessing("CSystem::Init StreamEngine");


        {
            if (m_pCmdLine->FindArg(eCLAT_Pre, "NullRenderer"))
            {
                m_env.pConsole->LoadConfigVar("r_Driver", "NULL");
            }
            else if (m_pCmdLine->FindArg(eCLAT_Pre, "DX11"))
            {
                m_env.pConsole->LoadConfigVar("r_Driver", "DX11");
            }
            else if (m_pCmdLine->FindArg(eCLAT_Pre, "GL"))
            {
                m_env.pConsole->LoadConfigVar("r_Driver", "GL");
            }
        }

        LogBuildInfo();

        InlineInitializationProcessing("CSystem::Init LoadConfigurations");

        m_env.pOverloadSceneManager = new COverloadSceneManager;

        if (m_bDedicatedServer && m_rDriver)
        {
            m_sSavedRDriver = m_rDriver->GetString();
            m_rDriver->Set("NULL");
        }

#if defined(WIN32) || defined(WIN64)
        if (!startupParams.bSkipRenderer)
        {
            if (azstricmp(m_rDriver->GetString(), "Auto") == 0)
            {
                m_rDriver->Set("DX11");
            }
        }

        if (gEnv->IsEditor() && azstricmp(m_rDriver->GetString(), "DX12") == 0)
        {
            AZ_Warning(AZ_TRACE_SYSTEM_WINDOW, false, "DX12 mode is not supported in the editor. Reverting to DX11 mode.");
            m_rDriver->Set("DX11");
        }
#endif

#ifdef WIN32
        if ((g_cvars.sys_WER) && (!startupParams.bMinimal))
        {
            SetUnhandledExceptionFilter(CryEngineExceptionFilterWER);
        }
#endif


        //////////////////////////////////////////////////////////////////////////
        // Localization
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bMinimal)
        {
            InitLocalization();
        }
        InlineInitializationProcessing("CSystem::Init InitLocalizations");

        //////////////////////////////////////////////////////////////////////////
        // RENDERER
        //////////////////////////////////////////////////////////////////////////
        const bool loadLegacyRenderer = gEnv->IsEditor() ?
                                        LOAD_LEGACY_RENDERER_FOR_EDITOR :
                                        LOAD_LEGACY_RENDERER_FOR_LAUNCHER;
        if (loadLegacyRenderer && !startupParams.bSkipRenderer)
        {
            AZ_Assert(CryMemory::IsHeapValid(), "CryMemory must be valid before initializing renderer.");
            AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Renderer initialization");

            if (!InitRenderer(m_hInst, m_hWnd, startupParams))
            {
                return false;
            }
            AZ_Assert(CryMemory::IsHeapValid(), "CryMemory must be valid after initializing renderer.");
            if (m_env.pRenderer)
            {
                bool bMultiGPUEnabled = false;
                m_env.pRenderer->EF_Query(EFQ_MultiGPUEnabled, bMultiGPUEnabled);
                if (bMultiGPUEnabled)
                {
                    LoadConfiguration("mgpu.cfg");
                }
            }

            InlineInitializationProcessing("CSystem::Init InitRenderer");

            if (m_env.pCryFont)
            {
                m_env.pCryFont->SetRendererProperties(m_env.pRenderer);
            }

            AZ_Assert(m_env.pRenderer || startupParams.bSkipRenderer, "The renderer did not initialize correctly.");
        }

#if !defined(AZ_RELEASE_BUILD) && defined(AZ_PLATFORM_ANDROID)
        m_thermalInfoHandler = AZStd::make_unique<ThermalInfoAndroidHandler>();
#endif

        if (g_cvars.sys_rendersplashscreen && !startupParams.bEditor && !startupParams.bShaderCacheGen)
        {
            if (m_env.pRenderer)
            {
                LOADING_TIME_PROFILE_SECTION_NAMED("Rendering Splash Screen");
                ITexture* pTex = m_env.pRenderer->EF_LoadTexture(g_cvars.sys_splashscreen, FT_DONT_STREAM | FT_NOMIPS | FT_USAGE_ALLOWREADSRGB);
                //check the width and height as extra verification hack. This texture is loaded before the replace me, so there is
                //no backup if it fails to load.
                if (pTex && pTex->GetWidth() && pTex->GetHeight())
                {
                    const int splashWidth = pTex->GetWidth();
                    const int splashHeight = pTex->GetHeight();

                    const int screenWidth = m_env.pRenderer->GetOverlayWidth();
                    const int screenHeight = m_env.pRenderer->GetOverlayHeight();

                    const float scaleX = (float)screenWidth / (float)splashWidth;
                    const float scaleY = (float)screenHeight / (float)splashHeight;

                    float scale = 1.0f;
                    switch (g_cvars.sys_splashScreenScaleMode)
                    {
                    case SSystemCVars::SplashScreenScaleMode_Fit:
                    {
                        scale = AZStd::GetMin(scaleX, scaleY);
                    }
                    break;
                    case SSystemCVars::SplashScreenScaleMode_Fill:
                    {
                        scale = AZStd::GetMax(scaleX, scaleY);
                    }
                    break;
                    }

                    const float w = splashWidth * scale;
                    const float h = splashHeight * scale;
                    const float x = (screenWidth - w) * 0.5f;
                    const float y = (screenHeight - h) * 0.5f;

                    const float vx = (800.0f / (float) screenWidth);
                    const float vy = (600.0f / (float) screenHeight);

                    m_env.pRenderer->SetViewport(0, 0, screenWidth, screenHeight);

                    // Skip splash screen rendering
                    if (!AZ::Interface<AzFramework::AtomActiveInterface>::Get())
                    {
                        // make sure it's rendered in full screen mode when triple buffering is enabled as well
                        for (size_t n = 0; n < 3; n++)
                        {
                            m_env.pRenderer->BeginFrame();
                            m_env.pRenderer->SetCullMode(R_CULL_NONE);
                            m_env.pRenderer->SetState(GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | GS_NODEPTHTEST);
                            m_env.pRenderer->Draw2dImageStretchMode(true);
                            m_env.pRenderer->Draw2dImage(x * vx, y * vy, w * vx, h * vy, pTex->GetTextureID(), 0.0f, 1.0f, 1.0f, 0.0f);
                            m_env.pRenderer->Draw2dImageStretchMode(false);
                            m_env.pRenderer->EndFrame();
                        }
                    }
#if defined(AZ_PLATFORM_IOS) || defined(AZ_PLATFORM_MAC)
                    // Pump system events in order to update the screen
                    AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::PumpSystemEventLoopUntilEmpty);
#endif

                    pTex->Release();
                }

        #if defined(AZ_PLATFORM_ANDROID)
                bool engineSplashEnabled = (g_cvars.sys_rendersplashscreen != 0);
                if (engineSplashEnabled)
                {
                    AZ::Android::Utils::DismissSplashScreen();
                }
        #endif
            }
            else
            {
                AZ_Warning(AZ_TRACE_SYSTEM_WINDOW, false, "Could not load startscreen image: %s.", g_cvars.sys_splashscreen);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // Open basic pak files after intro movie playback started
        //////////////////////////////////////////////////////////////////////////
        OpenBasicPaks();

        //////////////////////////////////////////////////////////////////////////
        // AUDIO
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bMinimal)
        {
            if (InitAudioSystem(startupParams))
            {
                // Pump the Log - Audio initialization happened on a non-main thread, there may be log messages queued up.
                gEnv->pLog->Update();
            }
            else
            {
                // Failure to initialize audio system is no longer a fatal or an error.  A warning is sufficient.
                AZ_Warning(AZ_TRACE_SYSTEM_WINDOW, false, "<Audio>: Running without any AudioSystem!");
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // FONT
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bSkipFont)
        {
            AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Font initialization");
            if (!InitFont(startupParams))
            {
                return false;
            }
        }

        InlineInitializationProcessing("CSystem::Init InitFonts");

        // The last update to the loading screen message was 'Initializing CryFont...'
        // Compiling the default system textures can be the lengthiest portion of
        // editor initialization, so it is useful to inform users that they are waiting on
        // the necessary default textures to compile, and that they are not waiting on CryFont.
        if (m_pUserCallback)
        {
            m_pUserCallback->OnInitProgress("First time asset processing - may take a minute...");
        }

        //////////////////////////////////////////////////////////////////////////
        // POST RENDERER
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bSkipRenderer && m_env.pRenderer)
        {
            m_env.pRenderer->PostInit();

            if (!startupParams.bShaderCacheGen)
            {
                // try to do a flush to keep the renderer busy during loading
                m_env.pRenderer->TryFlush();
            }
        }
        InlineInitializationProcessing("CSystem::Init Renderer::PostInit");

#ifdef SOFTCODE_SYSTEM_ENABLED
        m_env.pSoftCodeMgr = new SoftCodeMgr();
#else
        m_env.pSoftCodeMgr = nullptr;
#endif

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // System cursor
        //////////////////////////////////////////////////////////////////////////
        // - Dedicated server is in console mode by default (system cursor is always shown when console is)
        // - System cursor is always visible by default in Editor (we never start directly in Game Mode)
        // - System cursor has to be enabled manually by the Game if needed; the custom UiCursor will typically be used instead

        if (!gEnv->IsDedicated() &&
            m_env.pRenderer &&
            !gEnv->IsEditor() &&
            !startupParams.bTesting &&
            !m_pCmdLine->FindArg(eCLAT_Pre, "nomouse"))
        {
            AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                            &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                            AzFramework::SystemCursorState::ConstrainedAndHidden);
        }

        //////////////////////////////////////////////////////////////////////////
        // TIME
        //////////////////////////////////////////////////////////////////////////
        AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Time initialization");
        if (!m_Time.Init())
        {
            AZ_Assert(false, "Failed to initialize CTimer instance.");
            return false;
        }
        m_Time.ResetTimer();

        //////////////////////////////////////////////////////////////////////////
        // UI. Should be after input and hardware mouse
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bShaderCacheGen && !m_bDedicatedServer)
        {
            AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "UI system initialization");
            INDENT_LOG_DURING_SCOPE();
            if (!InitShine(startupParams))
            {
                return false;
            }
        }

        InlineInitializationProcessing("CSystem::Init InitShine");

        //////////////////////////////////////////////////////////////////////////
        // Create MiniGUI
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bMinimal)
        {
            minigui::IMiniGUIPtr pMiniGUI;
            if (CryCreateClassInstanceForInterface(cryiidof<minigui::IMiniGUI>(), pMiniGUI))
            {
                m_pMiniGUI = pMiniGUI.get();
                m_pMiniGUI->Init();
            }
        }

        InlineInitializationProcessing("CSystem::Init InitMiniGUI");

        //////////////////////////////////////////////////////////////////////////
        // CONSOLE
        //////////////////////////////////////////////////////////////////////////
        if (!InitConsole())
        {
            return false;
        }

        //////////////////////////////////////////////////////////////////////////
        // Init 3d engine
        //////////////////////////////////////////////////////////////////////////
        if (loadLegacyRenderer && !startupParams.bSkipRenderer && !startupParams.bShaderCacheGen)
        {
            AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Initializing 3D Engine");
            INDENT_LOG_DURING_SCOPE();

            if (!Init3DEngine(startupParams))
            {
                return false;
            }

            // try flush to keep renderer busy
            if (m_env.pRenderer)
            {
                m_env.pRenderer->TryFlush();
            }

            InlineInitializationProcessing("CSystem::Init Init3DEngine");
        }

        //////////////////////////////////////////////////////////////////////////
        // SERVICE NETWORK
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bSkipNetwork && !startupParams.bMinimal)
        {
            m_env.pServiceNetwork = new CServiceNetwork();
        }

        //////////////////////////////////////////////////////////////////////////
        // REMOTE COMMAND SYTSTEM
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bSkipNetwork && !startupParams.bMinimal)
        {
            m_env.pRemoteCommandManager = new CRemoteCommandManager();
        }

        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // VR SYSTEM INITIALIZATION
        //////////////////////////////////////////////////////////////////////////
        if ((!startupParams.bSkipRenderer) && (!startupParams.bMinimal))
        {
            if (m_pUserCallback)
            {
                m_pUserCallback->OnInitProgress("Initializing VR Systems...");
            }

            AZStd::vector<AZ::VR::HMDInitBus*> devices;
            AZ::VR::HMDInitRequestBus::EnumerateHandlers([&devices](AZ::VR::HMDInitBus* device)
                {
                    devices.emplace_back(device);
                    return true;
                });

            // Order the devices so that devices that only support one type of HMD are ordered first as we want
            // to use any device-specific drivers over more general ones.
            std::sort(devices.begin(), devices.end(), [](AZ::VR::HMDInitBus* left, AZ::VR::HMDInitBus* right)
                {
                    return left->GetInitPriority() > right->GetInitPriority();
                });

            //Start up a job to init the HMDs since they may take a while to start up
            AZ::JobContext* jobContext = nullptr;
            EBUS_EVENT_RESULT(jobContext, AZ::JobManagerBus, GetGlobalContext);
            AZ::Job* hmdJob = AZ::CreateJobFunction([devices]()
                    {
                        // Loop through the attached devices and attempt to initialize them. We'll use the first
                        // one that succeeds since we currently only support a single HMD.
                        for (AZ::VR::HMDInitBus* device : devices)
                        {
                            if (device->AttemptInit())
                            {
                                // At this point if any device connected to the HMDDeviceRequestBus then we are good to go for VR.
                                EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, OutputHMDInfo);
                                EBUS_EVENT(AZ::VR::HMDDeviceRequestBus, EnableDebugging, false);

                                // Since this was a job and we may have beaten the level's output_to_hmd cvar initialization
                                // We just want to retrigger the callback to this cvar
                                ICVar* outputToHMD = gEnv->pConsole->GetCVar("output_to_hmd");
                                if (outputToHMD != nullptr)
                                {
                                    int outputToHMDVal = gEnv->pConsole->GetCVar("output_to_hmd")->GetIVal();
                                    gEnv->pConsole->GetCVar("output_to_hmd")->Set(outputToHMDVal);
                                }
                                break;
                            }
                        }
                    }, true, jobContext);
            hmdJob->Start();
        }

        if (m_pUserCallback)
        {
            m_pUserCallback->OnInitProgress("Initializing additional systems...");
        }
        AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Initializing additional systems");


        InlineInitializationProcessing("CSystem::Init AIInit");

        //////////////////////////////////////////////////////////////////////////
        // LEVEL SYSTEM
        bool usePrefabSystemForLevels = false;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(
            usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

        if (usePrefabSystemForLevels)
        {
            m_pLevelSystem = new LegacyLevelSystem::SpawnableLevelSystem(this);
        }
        else
        {
            // [LYN-2376] Remove once legacy slice support is removed
            m_pLevelSystem = new LegacyLevelSystem::CLevelSystem(this, ILevelSystem::GetLevelsDirectoryName());
        }

        InlineInitializationProcessing("CSystem::Init Level System");

        //////////////////////////////////////////////////////////////////////////
        // VIEW SYSTEM (must be created after m_pLevelSystem)
        m_pViewSystem = new LegacyViewSystem::CViewSystem(this);

        InlineInitializationProcessing("CSystem::Init View System");

        //////////////////////////////////////////////////////////////////////////
        // Zlib compressor
        m_pIZLibCompressor = new CZLibCompressor();

        InlineInitializationProcessing("CSystem::Init ZLibCompressor");

        //////////////////////////////////////////////////////////////////////////
        // Zlib decompressor
        m_pIZLibDecompressor = new CZLibDecompressor();

        InlineInitializationProcessing("CSystem::Init ZLibDecompressor");

        //////////////////////////////////////////////////////////////////////////
        // LZ4 decompressor
        m_pILZ4Decompressor = new CLZ4Decompressor();

        InlineInitializationProcessing("CSystem::Init LZ4Decompressor");

        //////////////////////////////////////////////////////////////////////////
        // ZStd decompressor
        m_pIZStdDecompressor = new CZStdDecompressor();

        InlineInitializationProcessing("CSystem::Init ZStdDecompressor");
        //////////////////////////////////////////////////////////////////////////
        // Create PerfHUD
        //////////////////////////////////////////////////////////////////////////

#if defined(USE_PERFHUD)
        if (!gEnv->bTesting && !gEnv->IsInToolMode())
        {
            //Create late in Init so that associated CVars have already been created
            ICryPerfHUDPtr pPerfHUD;
            if (CryCreateClassInstanceForInterface(cryiidof<ICryPerfHUD>(), pPerfHUD))
            {
                m_pPerfHUD = pPerfHUD.get();
                m_pPerfHUD->Init();
            }
        }
#endif


        //////////////////////////////////////////////////////////////////////////
        // Initialize task threads.
        //////////////////////////////////////////////////////////////////////////
        if (!startupParams.bSkipRenderer)
        {
            m_pThreadTaskManager->InitThreads();

            SetAffinity();
            AZ_Assert(CryMemory::IsHeapValid(), "CryMemory heap must be valid before initializing VTune.");


            if (strstr(startupParams.szSystemCmdLine, "-VTUNE") != 0 || g_cvars.sys_vtune != 0)
            {
                if (!InitVTuneProfiler())
                {
                    return false;
                }
            }
        }

        InlineInitializationProcessing("CSystem::Init InitTaskThreads");

        if (m_env.pLyShine)
        {
            m_env.pLyShine->PostInit();
        }

        InlineInitializationProcessing("CSystem::Init InitLmbrAWS");

        // Az to Cry console binding
        AZ::Interface<AZ::IConsole>::Get()->VisitRegisteredFunctors([](AZ::ConsoleFunctorBase* functor) { AzConsoleToCryConsoleBinder::Visit(functor); });
        AzConsoleToCryConsoleBinder::s_commandRegisteredHandler.Connect(AZ::Interface<AZ::IConsole>::Get()->GetConsoleCommandRegisteredEvent());

        // final tryflush to be sure that all framework init request have been processed
        if (!startupParams.bShaderCacheGen && m_env.pRenderer)
        {
            m_env.pRenderer->TryFlush();
        }

#if !defined(RELEASE)
        m_env.pLocalMemoryUsage = new CLocalMemoryUsage();
#else
        m_env.pLocalMemoryUsage = nullptr;
#endif

        if (g_cvars.sys_float_exceptions > 0)
        {
            if (g_cvars.sys_float_exceptions == 3 && gEnv->IsEditor()) // Turn off float exceptions in editor if sys_float_exceptions = 3
            {
                g_cvars.sys_float_exceptions = 0;
            }
            if (g_cvars.sys_float_exceptions > 0)
            {
                AZ_TracePrintf(AZ_TRACE_SYSTEM_WINDOW, "Enabled float exceptions(sys_float_exceptions %d). This makes the performance slower.", g_cvars.sys_float_exceptions);
            }
        }
        EnableFloatExceptions(g_cvars.sys_float_exceptions);

        MarkThisThreadForDebugging("Main");
    }

    InlineInitializationProcessing("CSystem::Init End");

#if defined(IS_PROSDK)
    SDKEvaluation::InitSDKEvaluation(gEnv, &m_pUserCallback);
#endif

    InlineInitializationProcessing("CSystem::Init End");

    if (gEnv->IsDedicated())
    {
        SCVarsClientConfigSink CVarsClientConfigSink;
        LoadConfiguration("client.cfg", &CVarsClientConfigSink);
    }

    // All CVars should be registered by this point, we must now flush the cvar groups
    LoadDetectedSpec(m_sys_GraphicsQuality);

    //Connect to the render bus
    AZ::RenderNotificationsBus::Handler::BusConnect();

    // Send out EBus event
    EBUS_EVENT(CrySystemEventBus, OnCrySystemInitialized, *this, startupParams);

    // Verify that the Maestro Gem initialized the movie system correctly. This can be removed if and when Maestro is not a required Gem
    if (gEnv->IsEditor() && !gEnv->pMovieSystem)
    {
        AZ_Assert(false, "Error initializing the Cinematic System. Please check that the Maestro Gem is enabled for this project.");
        return false;
    }

    if (ISystemEventDispatcher* systemEventDispatcher = GetISystemEventDispatcher())
    {
        systemEventDispatcher->OnSystemEvent(ESYSTEM_EVENT_GAME_POST_INIT, 0, 0);
        systemEventDispatcher->OnSystemEvent(ESYSTEM_EVENT_GAME_POST_INIT_DONE, 0, 0);
    }

    m_bInitializedSuccessfully = true;

    return (true);
}


static void LoadConfigurationCmd(IConsoleCmdArgs* pParams)
{
    assert(pParams);

    if (pParams->GetArgCount() != 2)
    {
        gEnv->pLog->LogError("LoadConfiguration failed, one parameter needed");
        return;
    }

    ILoadConfigurationEntrySink* pCVarsWhiteListConfigSink = GetISystem()->GetCVarsWhiteListConfigSink();
    GetISystem()->LoadConfiguration(string("Config/") + pParams->GetArg(1), pCVarsWhiteListConfigSink);
}


// --------------------------------------------------------------------------------------------------------------------------

static string ConcatPath(const char* szPart1, const char* szPart2)
{
    if (szPart1[0] == 0)
    {
        return szPart2;
    }

    string ret;

    ret.reserve(strlen(szPart1) + 1 + strlen(szPart2));

    ret = szPart1;
    ret += "/";
    ret += szPart2;

    return ret;
}

static void ScreenshotCmd(IConsoleCmdArgs* pParams)
{
    assert(pParams);

    uint32 dwCnt = pParams->GetArgCount();

    if (dwCnt <= 1)
    {
        if (!gEnv->IsEditing())
        {
            // open console one line only

            //line should lie within title safe area, so calculate overscan border
            Vec2 overscanBorders = Vec2(0.0f, 0.0f);
            gEnv->pRenderer->EF_Query(EFQ_OverscanBorders, overscanBorders);
            float yDelta = /*((float)gEnv->pRenderer->GetHeight())*/ 600.0f * overscanBorders.y;

            //set console height depending on top/bottom overscan border
            gEnv->pConsole->ShowConsole(true, (int)(16 + yDelta));
            gEnv->pConsole->SetInputLine("Screenshot ");
        }
        else
        {
            gEnv->pLog->LogWithType(ILog::eInputResponse, "Screenshot <annotation> missing - no screenshot was done");
        }
    }
    else
    {
        static int iScreenshotNumber = -1;

        const char* szPrefix = "Screenshot";
        uint32 dwPrefixSize = strlen(szPrefix);

        char path[AZ::IO::IArchive::MaxPath];
        path[sizeof(path) - 1] = 0;
        gEnv->pCryPak->AdjustFileName("@user@/ScreenShots", path, AZ_ARRAY_SIZE(path), AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FLAGS_FOR_WRITING);

        if (iScreenshotNumber == -1)       // first time - find max number to start
        {
            auto pCryPak = gEnv->pCryPak;

            AZ::IO::ArchiveFileIterator handle = pCryPak->FindFirst((string(path) + "/*").c_str());       // mastercd folder
            if (handle)
            {
                do
                {
                    int iCurScreenshotNumber;

                    if (_strnicmp(handle.m_filename.data(), szPrefix, dwPrefixSize) == 0)
                    {
                        int iCnt = azsscanf(handle.m_filename.data() + dwPrefixSize, "%d", &iCurScreenshotNumber);

                        if (iCnt)
                        {
                            iScreenshotNumber = max(iCurScreenshotNumber, iScreenshotNumber);
                        }
                    }

                    handle = pCryPak->FindNext(handle);
                } while (handle);
                pCryPak->FindClose(handle);
            }
        }

        ++iScreenshotNumber;

        char szNumber[16];
        azsprintf(szNumber, "%.4d ", iScreenshotNumber);

        string sScreenshotName = string(szPrefix) + szNumber;

        for (uint32 dwI = 1; dwI < dwCnt; ++dwI)
        {
            if (dwI > 1)
            {
                sScreenshotName += "_";
            }

            sScreenshotName += pParams->GetArg(dwI);
        }

        sScreenshotName.replace("\\", "_");
        sScreenshotName.replace("/", "_");
        sScreenshotName.replace(":", "_");
        sScreenshotName.replace(".", "_");

        gEnv->pConsole->ShowConsole(false);

        CSystem* pCSystem = (CSystem*)(gEnv->pSystem);
        pCSystem->GetDelayedScreeenshot() = string(path) + "/" + sScreenshotName;// to delay a screenshot call for a frame
    }
}

// Helper to maintain backwards compatibility with our CVar but not force our new code to
// pull in CryCommon by routing through an environment variable
void CmdSetAwsLogLevel(IConsoleCmdArgs* pArgs)
{
    static const char* const logLevelEnvVar = "sys_SetLogLevel";
    static AZ::EnvironmentVariable<int> logVar = AZ::Environment::CreateVariable<int>(logLevelEnvVar);
    if (pArgs->GetArgCount() > 1)
    {
        int logLevel = atoi(pArgs->GetArg(1));
        *logVar = logLevel;
        AZ_TracePrintf("AWSLogging", "Log level set to %d", *logVar);
    }
}

static void SysRestoreSpecCmd(IConsoleCmdArgs* pParams)
{
    assert(pParams);

    if (pParams->GetArgCount() == 2)
    {
        const char* szArg = pParams->GetArg(1);

        ICVar* pCVar = gEnv->pConsole->GetCVar("sys_spec_Full");

        if (!pCVar)
        {
            gEnv->pLog->LogWithType(ILog::eInputResponse, "sys_RestoreSpec: no action");     // e.g. running Editor in shder compile mode
            return;
        }

        ICVar::EConsoleLogMode mode = ICVar::eCLM_Off;

        if (azstricmp(szArg, "test") == 0)
        {
            mode = ICVar::eCLM_ConsoleAndFile;
        }
        else if (azstricmp(szArg, "test*") == 0)
        {
            mode = ICVar::eCLM_FileOnly;
        }
        else if (azstricmp(szArg, "info") == 0)
        {
            mode = ICVar::eCLM_FullInfo;
        }

        if (mode != ICVar::eCLM_Off)
        {
            bool bFileOrConsole = (mode == ICVar::eCLM_FileOnly || mode == ICVar::eCLM_FullInfo);

            if (bFileOrConsole)
            {
                gEnv->pLog->LogToFile(" ");
            }
            else
            {
                CryLog(" ");
            }

            int iSysSpec = pCVar->GetRealIVal();

            if (iSysSpec == -1)
            {
                iSysSpec = ((CSystem*)gEnv->pSystem)->GetMaxConfigSpec();

                if (bFileOrConsole)
                {
                    gEnv->pLog->LogToFile("   sys_spec = Custom (assuming %d)", iSysSpec);
                }
                else
                {
                    gEnv->pLog->LogWithType(ILog::eInputResponse, "   $3sys_spec = $6Custom (assuming %d)", iSysSpec);
                }
            }
            else
            {
                if (bFileOrConsole)
                {
                    gEnv->pLog->LogToFile("   sys_spec = %d", iSysSpec);
                }
                else
                {
                    gEnv->pLog->LogWithType(ILog::eInputResponse, "   $3sys_spec = $6%d", iSysSpec);
                }
            }

            pCVar->DebugLog(iSysSpec, mode);

            if (bFileOrConsole)
            {
                gEnv->pLog->LogToFile(" ");
            }
            else
            {
                gEnv->pLog->LogWithType(ILog::eInputResponse, " ");
            }

            return;
        }
        else if (strcmp(szArg, "apply") == 0)
        {
            const char* szPrefix = "sys_spec_";

            ESystemConfigSpec originalSpec = CONFIG_AUTO_SPEC;
            ESystemConfigPlatform originalPlatform = GetDevicePlatform();

            if (gEnv->IsEditor())
            {
                originalSpec = gEnv->pSystem->GetConfigSpec(true);
            }

            std::vector<const char*> cmds;

            cmds.resize(gEnv->pConsole->GetSortedVars(0, 0, szPrefix));
            gEnv->pConsole->GetSortedVars(&cmds[0], cmds.size(), szPrefix);

            gEnv->pLog->LogWithType(IMiniLog::eInputResponse, " ");

            std::vector<const char*>::const_iterator it, end = cmds.end();

            for (it = cmds.begin(); it != end; ++it)
            {
                const char* szName = *it;

                if (azstricmp(szName, "sys_spec_Full") == 0)
                {
                    continue;
                }

                pCVar = gEnv->pConsole->GetCVar(szName);
                assert(pCVar);

                if (!pCVar)
                {
                    continue;
                }

                bool bNeeded = pCVar->GetIVal() != pCVar->GetRealIVal();

                gEnv->pLog->LogWithType(IMiniLog::eInputResponse, " $3%s = $6%d ... %s",
                    szName, pCVar->GetIVal(),
                    bNeeded ? "$4restored" : "valid");

                if (bNeeded)
                {
                    pCVar->Set(pCVar->GetIVal());
                }
            }

            gEnv->pLog->LogWithType(IMiniLog::eInputResponse, " ");

            if (gEnv->IsEditor())
            {
                gEnv->pSystem->SetConfigSpec(originalSpec, originalPlatform, true);
            }
            return;
        }
    }

    gEnv->pLog->LogWithType(ILog::eInputResponse, "ERROR: sys_RestoreSpec invalid arguments");
}

void CmdDrillToFile(IConsoleCmdArgs* pArgs)
{
    if (azstricmp(pArgs->GetArg(0), "DrillerStop") == 0)
    {
        EBUS_EVENT(AzFramework::DrillerConsoleCommandBus, StopDrillerSession, AZ::Crc32("DefaultDrillerSession"));
    }
    else
    {
        if (pArgs->GetArgCount() > 1)
        {
            AZ::Debug::DrillerManager::DrillerListType drillersToEnable;
            for (int iArg = 1; iArg < pArgs->GetArgCount(); ++iArg)
            {
                if (azstricmp(pArgs->GetArg(iArg), "Replica") == 0)
                {
                    drillersToEnable.push_back();
                    drillersToEnable.back().id = AZ::Crc32("ReplicaDriller");
                }
                else if (azstricmp(pArgs->GetArg(iArg), "Carrier") == 0)
                {
                    drillersToEnable.push_back();
                    drillersToEnable.back().id = AZ::Crc32("CarrierDriller");
                }
                else
                {
                    CryLogAlways("Driller %s not supported.", pArgs->GetArg(iArg));
                }
            }
            EBUS_EVENT(AzFramework::DrillerConsoleCommandBus, StartDrillerSession, drillersToEnable, AZ::Crc32("DefaultDrillerSession"));
        }
        else
        {
            CryLogAlways("Syntax: DrillerStart [Driller1] [Driller2] [...]");
            CryLogAlways("Supported Drillers:");
            CryLogAlways("    Carrier");
            CryLogAlways("    Replica");
        }
    }
}

void ChangeLogAllocations(ICVar* pVal)
{
    g_iTraceAllocations = pVal->GetIVal();

    if (g_iTraceAllocations == 2)
    {
        IDebugCallStack::instance()->StartMemLog();
    }
    else
    {
        IDebugCallStack::instance()->StopMemLog();
    }
}

static void VisRegTest(IConsoleCmdArgs* pParams)
{
    CSystem* pCSystem = (CSystem*)(gEnv->pSystem);
    CVisRegTest*& visRegTest = pCSystem->GetVisRegTestPtrRef();
    if (!visRegTest)
    {
        visRegTest = new CVisRegTest();
    }

    visRegTest->Init(pParams);
}

//////////////////////////////////////////////////////////////////////////
void CSystem::CreateSystemVars()
{
    assert(gEnv);
    assert(gEnv->pConsole);

    // Register DLL names as cvars before we load them
    //
    EVarFlags dllFlags = (EVarFlags)0;
    m_sys_dll_response_system = REGISTER_STRING("sys_dll_response_system", 0, dllFlags,                 "Specifies the DLL to load for the dynamic response system");

    m_sys_initpreloadpacks = REGISTER_STRING("sys_initpreloadpacks", "", 0,     "Specifies the paks for an engine initialization");
    m_sys_menupreloadpacks = REGISTER_STRING("sys_menupreloadpacks", 0, 0,      "Specifies the paks for a main menu loading");

#ifndef _RELEASE
    m_sys_resource_cache_folder = REGISTER_STRING("sys_resource_cache_folder", "Editor\\ResourceCache", 0, "Folder for resource compiled locally. Managed by Sandbox.");
#endif

#if AZ_LOADSCREENCOMPONENT_ENABLED
    m_game_load_screen_uicanvas_path = REGISTER_STRING("game_load_screen_uicanvas_path", "", 0, "Game load screen UiCanvas path.");
    m_level_load_screen_uicanvas_path = REGISTER_STRING("level_load_screen_uicanvas_path", "", 0, "Level load screen UiCanvas path.");
    m_game_load_screen_sequence_to_auto_play = REGISTER_STRING("game_load_screen_sequence_to_auto_play", "", 0, "Game load screen UiCanvas animation sequence to play on load.");
    m_level_load_screen_sequence_to_auto_play = REGISTER_STRING("level_load_screen_sequence_to_auto_play", "", 0, "Level load screen UiCanvas animation sequence to play on load.");
    m_game_load_screen_sequence_fixed_fps = REGISTER_FLOAT("game_load_screen_sequence_fixed_fps", 60.0f, 0, "Fixed frame rate fed to updates of the game load screen sequence.");
    m_level_load_screen_sequence_fixed_fps = REGISTER_FLOAT("level_load_screen_sequence_fixed_fps", 60.0f, 0, "Fixed frame rate fed to updates of the level load screen sequence.");
    m_game_load_screen_max_fps = REGISTER_FLOAT("game_load_screen_max_fps", 30.0f, 0, "Max frame rate to update the game load screen sequence.");
    m_level_load_screen_max_fps = REGISTER_FLOAT("level_load_screen_max_fps", 30.0f, 0, "Max frame rate to update the level load screen sequence.");
    m_game_load_screen_minimum_time = REGISTER_FLOAT("game_load_screen_minimum_time", 0.0f, 0, "Minimum amount of time to show the game load screen. Important to prevent short loads from flashing the load screen. 0 means there is no limit.");
    m_level_load_screen_minimum_time = REGISTER_FLOAT("level_load_screen_minimum_time", 0.0f, 0, "Minimum amount of time to show the level load screen. Important to prevent short loads from flashing the load screen. 0 means there is no limit.");
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED

    REGISTER_INT("cvDoVerboseWindowTitle", 0, VF_NULL, "");

    m_pCVarQuit = REGISTER_INT("ExitOnQuit", 1, VF_NULL, "");

    // Register an AZ Console command to quit the engine.
    // The command is available even in Release builds.
    static AZ::ConsoleFunctor<void, false> s_functorQuit
    (
        "quit",
        "Quit/Shutdown the engine",
        AZ::ConsoleFunctorFlags::AllowClientSet | AZ::ConsoleFunctorFlags::DontReplicate,
        AZ::TypeId::CreateNull(),
        []([[maybe_unused]] const AZ::ConsoleCommandContainer& params) { GetISystem()->Quit(); }
    );

    m_sys_load_files_to_memory = REGISTER_STRING("sys_load_files_to_memory", "shadercache.pak", 0,
            "Specify comma separated list of filenames that need to be loaded to memory.\n"
            "Partial names also work. Eg. \"shader\" will load:\n"
            "shaders.pak, shadercache.pak, and shadercachestartup.pak");

#ifndef _RELEASE
    REGISTER_STRING_CB("sys_version", "", VF_CHEAT, "Override system file/product version", SystemVersionChanged);
#endif // #ifndef _RELEASE

    m_cvAIUpdate = REGISTER_INT("ai_NoUpdate", 0, VF_CHEAT, "Disables AI system update when 1");

    m_iTraceAllocations = g_iTraceAllocations;
    REGISTER_CVAR2_CB("sys_logallocations", &m_iTraceAllocations, m_iTraceAllocations, VF_DUMPTODISK, "Save allocation call stack", ChangeLogAllocations);

    m_cvMemStats = REGISTER_INT("MemStats", 0, 0,
            "0/x=refresh rate in milliseconds\n"
            "Use 1000 to switch on and 0 to switch off\n"
            "Usage: MemStats [0..]");
    m_cvMemStatsThreshold = REGISTER_INT ("MemStatsThreshold", 32000, VF_NULL, "");
    m_cvMemStatsMaxDepth = REGISTER_INT("MemStatsMaxDepth", 4, VF_NULL, "");

    attachVariable("sys_PakReadSlice", &g_cvars.archiveVars.nReadSlice, "If non-0, means number of kilobytes to use to read files in portions. Should only be used on Win9x kernels");

    attachVariable("sys_PakInMemorySizeLimit", &g_cvars.archiveVars.nInMemoryPerPakSizeLimit, "Individual pak size limit for being loaded into memory (MB)");
    attachVariable("sys_PakTotalInMemorySizeLimit", &g_cvars.archiveVars.nTotalInMemoryPakSizeLimit, "Total limit (in MB) for all in memory paks");
    attachVariable("sys_PakLoadCache", &g_cvars.archiveVars.nLoadCache, "Load in memory paks from _LoadCache folder");
    attachVariable("sys_PakLoadModePaks", &g_cvars.archiveVars.nLoadModePaks, "Load mode switching paks from modes folder");
    attachVariable("sys_PakStreamCache", &g_cvars.archiveVars.nStreamCache, "Load in memory paks for faster streaming (cgf_cache.pak,dds_cache.pak)");
    attachVariable("sys_PakSaveTotalResourceList", &g_cvars.archiveVars.nSaveTotalResourceList, "Save resource list");
    attachVariable("sys_PakSaveLevelResourceList", &g_cvars.archiveVars.nSaveLevelResourceList, "Save resource list when loading level");
    attachVariable("sys_PakSaveFastLoadResourceList", &g_cvars.archiveVars.nSaveFastloadResourceList, "Save resource list during initial loading");
    attachVariable("sys_PakSaveMenuCommonResourceList", &g_cvars.archiveVars.nSaveMenuCommonResourceList, "Save resource list during front end menu flow");
    attachVariable("sys_PakMessageInvalidFileAccess", &g_cvars.archiveVars.nMessageInvalidFileAccess, "Message Box synchronous file access when in game");
    attachVariable("sys_PakLogInvalidFileAccess", &g_cvars.archiveVars.nLogInvalidFileAccess, "Log synchronous file access when in game");
#ifndef _RELEASE
    attachVariable("sys_PakLogAllFileAccess", &g_cvars.archiveVars.nLogAllFileAccess, "Log all file access allowing you to easily see whether a file has been loaded directly, or which pak file.");
#endif
    attachVariable("sys_PakValidateFileHash", &g_cvars.archiveVars.nValidateFileHashes, "Validate file hashes in pak files for collisions");
    attachVariable("sys_LoadFrontendShaderCache", &g_cvars.archiveVars.nLoadFrontendShaderCache, "Load frontend shader cache (on/off)");
    attachVariable("sys_UncachedStreamReads", &g_cvars.archiveVars.nUncachedStreamReads, "Enable stream reads via an uncached file handle");
    attachVariable("sys_PakDisableNonLevelRelatedPaks", &g_cvars.archiveVars.nDisableNonLevelRelatedPaks, "Disables all paks that are not required by specific level; This is used with per level splitted assets.");
    attachVariable("sys_PakWarnOnPakAccessFailures", &g_cvars.archiveVars.nWarnOnPakAccessFails, "If 1, access failure for Paks is treated as a warning, if zero it is only a log message.");


    {
        int nDefaultRenderSplashScreen = 1;
#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_10
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
        REGISTER_CVAR2("sys_rendersplashscreen", &g_cvars.sys_rendersplashscreen, nDefaultRenderSplashScreen, VF_NULL,
            "Render the splash screen during game initialization");
        REGISTER_CVAR2("sys_splashscreenscalemode", &g_cvars.sys_splashScreenScaleMode, static_cast<int>(SSystemCVars::SplashScreenScaleMode_Fill), VF_NULL,
            "0 - scale to fit (letterbox)\n"
            "1 - scale to fill (cropped)\n"
            "Default is 1");
        REGISTER_CVAR2("sys_splashscreen", &g_cvars.sys_splashscreen, "EngineAssets/Textures/startscreen.tif", VF_NULL,
            "The splash screen to render during game initialization");
    }

    static const int fileSystemCaseSensitivityDefault = 0;
    REGISTER_CVAR2("sys_FilesystemCaseSensitivity", &g_cvars.sys_FilesystemCaseSensitivity, fileSystemCaseSensitivityDefault, VF_NULL,
        "0 - CryPak lowercases all input file names\n"
        "1 - CryPak preserves file name casing\n"
        "Default is 1");

    REGISTER_CVAR2("sys_deferAudioUpdateOptim", &g_cvars.sys_deferAudioUpdateOptim, 1, VF_NULL,
        "0 - disable optimisation\n"
        "1 - enable optimisation\n"
        "Default is 1");

#if USE_STEAM
#ifndef RELEASE
    REGISTER_CVAR2("sys_steamAppId", &g_cvars.sys_steamAppId, 0, VF_NULL, "steam appId used for development testing");
    REGISTER_COMMAND("sys_wipeSteamCloud", CmdWipeSteamCloud, VF_CHEAT, "Delete all files from steam cloud for this user");
#endif // RELEASE
    REGISTER_CVAR2("sys_useSteamCloudForPlatformSaving", &g_cvars.sys_useSteamCloudForPlatformSaving, 0, VF_NULL, "Use steam cloud for save games and profile on PC (instead of the user folder)");
#endif

    m_sysNoUpdate = REGISTER_INT("sys_noupdate", 0, VF_CHEAT,
            "Toggles updating of system with sys_script_debugger.\n"
            "Usage: sys_noupdate [0/1]\n"
            "Default is 0 (system updates during debug).");

    m_sysWarnings = REGISTER_INT("sys_warnings", 0, 0,
            "Toggles printing system warnings.\n"
            "Usage: sys_warnings [0/1]\n"
            "Default is 0 (off).");

#if defined(_RELEASE) && defined(CONSOLE) && !defined(ENABLE_LW_PROFILERS)
    enum
    {
        e_sysKeyboardDefault = 0
    };
#else
    enum
    {
        e_sysKeyboardDefault = 1
    };
#endif
    m_sysKeyboard = REGISTER_INT("sys_keyboard", e_sysKeyboardDefault, 0,
            "Enables keyboard.\n"
            "Usage: sys_keyboard [0/1]\n"
            "Default is 1 (on).");

    m_svDedicatedMaxRate = REGISTER_FLOAT("sv_DedicatedMaxRate", 30.0f, 0,
            "Sets the maximum update rate when running as a dedicated server.\n"
            "Usage: sv_DedicatedMaxRate [5..500]\n"
            "Default is 30.");

    REGISTER_FLOAT("sv_DedicatedCPUPercent", 0.0f, 0,
        "Sets the target CPU usage when running as a dedicated server, or disable this feature if it's zero.\n"
        "Usage: sv_DedicatedCPUPercent [0..100]\n"
        "Default is 0 (disabled).");
    REGISTER_FLOAT("sv_DedicatedCPUVariance", 10.0f, 0,
        "Sets how much the CPU can vary from sv_DedicateCPU (up or down) without adjusting the framerate.\n"
        "Usage: sv_DedicatedCPUVariance [5..50]\n"
        "Default is 10.");

    m_cvSSInfo =  REGISTER_INT("sys_SSInfo", 0, 0,
            "Show SourceSafe information (Name,Comment,Date) for file errors."
            "Usage: sys_SSInfo [0/1]\n"
            "Default is 0 (off)");

    m_cvEntitySuppressionLevel = REGISTER_INT("e_EntitySuppressionLevel", 0, 0,
            "Defines the level at which entities are spawned.\n"
            "Entities marked with lower level will not be spawned - 0 means no level.\n"
            "Usage: e_EntitySuppressionLevel [0-infinity]\n"
            "Default is 0 (off)");

#if defined(WIN32) || defined(WIN64)
    const uint32 nJobSystemDefaultCoreNumber = 8;
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_11
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    const uint32 nJobSystemDefaultCoreNumber = 4;
#endif
    m_sys_GraphicsQuality = REGISTER_INT_CB("r_GraphicsQuality", 0, VF_ALWAYSONCHANGE,
        "Specifies the system cfg spec. 1=low, 2=med, 3=high, 4=very high)",
        LoadDetectedSpec);

    m_sys_firstlaunch = REGISTER_INT("sys_firstlaunch", 0, 0,
            "Indicates that the game was run for the first time.");

    m_sys_main_CPU = REGISTER_INT("sys_main_CPU", 0, 0,
            "Specifies the physical CPU index main will run on");

    m_sys_TaskThread_CPU[0] = REGISTER_INT("sys_TaskThread0_CPU", 3, 0,
            "Specifies the physical CPU index taskthread0 will run on");

    m_sys_TaskThread_CPU[1] = REGISTER_INT("sys_TaskThread1_CPU", 5, 0,
            "Specifies the physical CPU index taskthread1 will run on");

    m_sys_TaskThread_CPU[2] = REGISTER_INT("sys_TaskThread2_CPU", 4, 0,
            "Specifies the physical CPU index taskthread2 will run on");

    m_sys_TaskThread_CPU[3] = REGISTER_INT("sys_TaskThread3_CPU", 3, 0,
            "Specifies the physical CPU index taskthread3 will run on");

    m_sys_TaskThread_CPU[4] = REGISTER_INT("sys_TaskThread4_CPU", 2, 0,
            "Specifies the physical CPU index taskthread4 will run on");

    m_sys_TaskThread_CPU[5] = REGISTER_INT("sys_TaskThread5_CPU", 1, 0,
            "Specifies the physical CPU index taskthread5 will run on");

    //if physics thread is excluded all locks inside are mapped to NO_LOCK
    //var must be not visible to accidentally get enabled
#if defined(EXCLUDE_PHYSICS_THREAD)
    m_sys_physics_CPU = REGISTER_INT("sys_physics_CPU_disabled", 0, 0,
            "Specifies the physical CPU index physics will run on");
#else
    m_sys_physics_CPU = REGISTER_INT("sys_physics_CPU", 1, 0,
            "Specifies the physical CPU index physics will run on");
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_12
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif

    m_sys_min_step = REGISTER_FLOAT("sys_min_step", 0.01f, 0,
            "Specifies the minimum physics step in a separate thread");
    m_sys_max_step = REGISTER_FLOAT("sys_max_step", 0.05f, 0,
            "Specifies the maximum physics step in a separate thread");

    // used in define MEMORY_DEBUG_POINT()
    m_sys_memory_debug = REGISTER_INT("sys_memory_debug", 0, VF_CHEAT,
            "Enables to activate low memory situation is specific places in the code (argument defines which place), 0=off");

    REGISTER_CVAR2("sys_vtune", &g_cvars.sys_vtune, 0, VF_NULL, "");

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_17
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#   define SYS_STREAMING_CPU_DEFAULT_VALUE 1
#   define SYS_STREAMING_CPU_WORKER_DEFAULT_VALUE 5
#endif
    REGISTER_CVAR2("sys_streaming_CPU", &g_cvars.sys_streaming_cpu, SYS_STREAMING_CPU_DEFAULT_VALUE, VF_NULL, "Specifies the physical CPU file IO thread run on");
    REGISTER_CVAR2("sys_streaming_CPU_worker", &g_cvars.sys_streaming_cpu_worker, SYS_STREAMING_CPU_WORKER_DEFAULT_VALUE, VF_NULL, "Specifies the physical CPU file IO worker thread/s run on");
    REGISTER_CVAR2("sys_streaming_memory_budget", &g_cvars.sys_streaming_memory_budget, 10 * 1024, VF_NULL, "Temp memory streaming system can use in KB");
    REGISTER_CVAR2("sys_streaming_max_finalize_per_frame", &g_cvars.sys_streaming_max_finalize_per_frame, 0, VF_NULL,
        "Maximum stream finalizing calls per frame to reduce the CPU impact on main thread (0 to disable)");
    REGISTER_CVAR2("sys_streaming_max_bandwidth", &g_cvars.sys_streaming_max_bandwidth, 0, VF_NULL, "Enables capping of max streaming bandwidth in MB/s");
    REGISTER_CVAR2("az_streaming_stats", &g_cvars.az_streaming_stats, 0, VF_NULL, "Show stats from AZ::IO::Streamer\n"
        "0=off\n"
        "1=on\n"
    );
    REGISTER_CVAR2("sys_streaming_debug", &g_cvars.sys_streaming_debug, 0, VF_NULL, "Enable streaming debug information\n"
        "0=off\n"
        "1=Streaming Stats\n"
        "2=File IO\n"
        "3=Request Order\n"
        "4=Write to Log\n"
        "5=Stats per extension\n"
        );
    REGISTER_CVAR2("sys_streaming_requests_grouping_time_period", &g_cvars.sys_streaming_requests_grouping_time_period, 2, VF_NULL, // Vlad: 2 works better than 4 visually, should be be re-tested when streaming pak's activated
        "Streaming requests are grouped by request time and then sorted by disk offset");
    REGISTER_CVAR2("sys_streaming_debug_filter", &g_cvars.sys_streaming_debug_filter, 0, VF_NULL, "Set streaming debug information filter.\n"
        "0=all\n"
        "1=Texture\n"
        "2=Geometry\n"
        "3=Terrain\n"
        "4=Animation\n"
        "5=Music\n"
        "6=Sound\n"
        "7=Shader\n"
        );
    g_cvars.sys_streaming_debug_filter_file_name = REGISTER_STRING("sys_streaming_debug_filter_file_name", "", VF_CHEAT,
            "Set streaming debug information filter");
    REGISTER_CVAR2("sys_streaming_debug_filter_min_time", &g_cvars.sys_streaming_debug_filter_min_time, 0.f, VF_NULL, "Show only slow items.");
    REGISTER_CVAR2("sys_streaming_resetstats", &g_cvars.sys_streaming_resetstats, 0, VF_NULL,
        "Reset all the streaming stats");
#define DEFAULT_USE_OPTICAL_DRIVE_THREAD (gEnv->IsDedicated() ? 0 : 1)
    REGISTER_CVAR2("sys_streaming_use_optical_drive_thread", &g_cvars.sys_streaming_use_optical_drive_thread, DEFAULT_USE_OPTICAL_DRIVE_THREAD, VF_NULL,
        "Allow usage of an extra optical drive thread for faster streaming from 2 medias");

    const char* localizeFolder = "Localization";
    g_cvars.sys_localization_folder = REGISTER_STRING_CB("sys_localization_folder", localizeFolder, VF_NULL,
            "Sets the folder where to look for localized data.\n"
            "This cvar allows for backwards compatibility so localized data under the game folder can still be found.\n"
            "Usage: sys_localization_folder <folder name>\n"
            "Default: Localization\n",
            CSystem::OnLocalizationFolderCVarChanged);

    REGISTER_CVAR2("sys_streaming_in_blocks", &g_cvars.sys_streaming_in_blocks, 1, VF_NULL,
        "Streaming of large files happens in blocks");

#if (defined(WIN32) || defined(WIN64)) && !defined(_RELEASE)
    REGISTER_CVAR2("sys_float_exceptions", &g_cvars.sys_float_exceptions, 3, 0, "Use or not use floating point exceptions.");
#else // Float exceptions by default disabled for console builds.
    REGISTER_CVAR2("sys_float_exceptions", &g_cvars.sys_float_exceptions, 0, 0, "Use or not use floating point exceptions.");
#endif

    REGISTER_CVAR2("sys_update_profile_time", &g_cvars.sys_update_profile_time, 1.0f, 0, "Time to keep updates timings history for.");
    REGISTER_CVAR2("sys_no_crash_dialog", &g_cvars.sys_no_crash_dialog, m_bNoCrashDialog, VF_NULL, "Whether to disable the crash dialog window");
    REGISTER_CVAR2("sys_no_error_report_window", &g_cvars.sys_no_error_report_window, m_bNoErrorReportWindow, VF_NULL, "Whether to disable the error report list");
#if defined(_RELEASE)
    if (!gEnv->IsDedicated())
    {
        REGISTER_CVAR2("sys_WER", &g_cvars.sys_WER, 1, 0, "Enables Windows Error Reporting");
    }
#else
    REGISTER_CVAR2("sys_WER", &g_cvars.sys_WER, 0, 0, "Enables Windows Error Reporting");
#endif

#ifdef USE_HTTP_WEBSOCKETS
    REGISTER_CVAR2("sys_simple_http_base_port", &g_cvars.sys_simple_http_base_port, 1880, VF_REQUIRE_APP_RESTART,
        "sets the base port for the simple http server to run on, defaults to 1880");
#endif

    const int DEFAULT_DUMP_TYPE = 2;

    REGISTER_CVAR2("sys_dump_type", &g_cvars.sys_dump_type, DEFAULT_DUMP_TYPE, VF_NULL,
        "Specifies type of crash dump to create - see MINIDUMP_TYPE in dbghelp.h for full list of values\n"
        "0: Do not create a minidump\n"
        "1: Create a small minidump (stacktrace)\n"
        "2: Create a medium minidump (+ some variables)\n"
        "3: Create a full minidump (+ all memory)\n"
        );
    REGISTER_CVAR2("sys_dump_aux_threads", &g_cvars.sys_dump_aux_threads, 1, VF_NULL, "Dumps callstacks of other threads in case of a crash");

    REGISTER_CVAR2("sys_limit_phys_thread_count", &g_cvars.sys_limit_phys_thread_count, 1, VF_NULL, "Limits p_num_threads to physical CPU count - 1");

#if (defined(WIN32) || defined(WIN64)) && defined(_RELEASE)
    const int DEFAULT_SYS_MAX_FPS = 0;
#else
    const int DEFAULT_SYS_MAX_FPS = -1;
#endif
    REGISTER_CVAR2("sys_MaxFPS", &g_cvars.sys_MaxFPS, DEFAULT_SYS_MAX_FPS, VF_NULL, "Limits the frame rate to specified number n (if n>0 and if vsync is disabled).\n"
        " 0 = on PC if vsync is off auto throttles fps while in menu or game is paused (default)\n"
        "-1 = off");

    REGISTER_CVAR2("sys_maxTimeStepForMovieSystem", &g_cvars.sys_maxTimeStepForMovieSystem, 0.1f, VF_NULL, "Caps the time step for the movie system so that a cut-scene won't be jumped in the case of an extreme stall.");

    REGISTER_CVAR2("sys_force_installtohdd_mode", &g_cvars.sys_force_installtohdd_mode, 0, VF_NULL, "Forces install to HDD mode even when doing DVD emulation");

    REGISTER_CVAR2("sys_report_files_not_found_in_paks", &g_cvars.sys_report_files_not_found_in_paks, 0, VF_NULL, "Reports when files are searched for in paks and not found. 1 = log, 2 = warning, 3 = error");

    m_sys_preload = REGISTER_INT("sys_preload", 0, 0, "Preload Game Resources");
    REGISTER_COMMAND("sys_crashtest", CmdCrashTest, VF_CHEAT, "Make the game crash\n"
        "0=off\n"
        "1=null pointer exception\n"
        "2=floating point exception\n"
        "3=memory allocation exception\n"
        "4=cry fatal error is called\n"
        "5=memory allocation for small blocks\n"
        "6=assert\n"
        "7=debugbreak\n"
        "8=10min sleep"
        );

    REGISTER_FLOAT("sys_scale3DMouseTranslation", 0.2f, 0, "Scales translation speed of supported 3DMouse devices.");
    REGISTER_FLOAT("sys_Scale3DMouseYPR", 0.05f, 0, "Scales rotation speed of supported 3DMouse devices.");

    REGISTER_INT("capture_frames", 0, 0, "Enables capturing of frames. 0=off, 1=on");
    REGISTER_STRING("capture_folder", "CaptureOutput", 0, "Specifies sub folder to write captured frames.");
    REGISTER_INT("capture_frame_once", 0, 0, "Makes capture single frame only");
    REGISTER_STRING("capture_file_name", "", 0, "If set, specifies the path and name to use for the captured frame");
    REGISTER_STRING("capture_file_prefix", "", 0, "If set, specifies the prefix to use for the captured frame instead of the default 'Frame'.");

    m_gpu_particle_physics = REGISTER_INT("gpu_particle_physics", 0, VF_REQUIRE_APP_RESTART, "Enable GPU physics if available (0=off / 1=enabled).");
    assert(m_gpu_particle_physics);

    REGISTER_COMMAND("LoadConfig", &LoadConfigurationCmd, 0,
        "Load .cfg file from disk (from the {Game}/Config directory)\n"
        "e.g. LoadConfig lowspec.cfg\n"
        "Usage: LoadConfig <filename>");
    assert(m_env.pConsole);
    m_env.pConsole->CreateKeyBind("alt_keyboard_key_function_F12", "Screenshot");
    m_env.pConsole->CreateKeyBind("alt_keyboard_key_function_F11", "RecordClip");

    // screenshot functionality in system as console
    REGISTER_COMMAND("Screenshot", &ScreenshotCmd, VF_BLOCKFRAME,
        "Create a screenshot with annotation\n"
        "e.g. Screenshot beach scene with shark\n"
        "Usage: Screenshot <annotation text>");

    /*
        // experimental feature? - needs to be created very early
        m_sys_filecache = REGISTER_INT("sys_FileCache",0,0,
            "To speed up loading from non HD media\n"
            "0=off / 1=enabled");
    */
    REGISTER_CVAR2("sys_AI", &g_cvars.sys_ai, 1, 0, "Enables AI Update");
    REGISTER_CVAR2("sys_physics", &g_cvars.sys_physics, 1, 0, "Enables Physics Update");
    REGISTER_CVAR2("sys_entities", &g_cvars.sys_entitysystem, 1, 0, "Enables Entities Update");
    REGISTER_CVAR2("sys_trackview", &g_cvars.sys_trackview, 1, 0, "Enables TrackView Update");

    //Defines selected language.
    REGISTER_STRING_CB("g_language", "", VF_NULL, "Defines which language pak is loaded", CSystem::OnLanguageCVarChanged);
    REGISTER_STRING_CB("g_languageAudio", "", VF_NULL, "Will automatically match g_language setting unless specified otherwise", CSystem::OnLanguageAudioCVarChanged);

    REGISTER_COMMAND("sys_RestoreSpec", &SysRestoreSpecCmd, 0,
        "Restore or test the cvar settings of game specific spec settings,\n"
        "'test*' and 'info' log to the log file only\n"
        "Usage: sys_RestoreSpec [test|test*|apply|info]");

    REGISTER_COMMAND("VisRegTest", &VisRegTest, 0, "Run visual regression test.\n"
        "Usage: VisRegTest [<name>=test] [<config>=visregtest.xml] [quit=false]");

#if defined(WIN32)
    REGISTER_CVAR2("sys_display_threads", &g_cvars.sys_display_threads, 0, 0, "Displays Thread info");
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_13
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif

    // adding CVAR to toggle assert verbosity level
    const int defaultAssertValue = 1;
    REGISTER_CVAR2_CB("sys_asserts", &g_cvars.sys_asserts, defaultAssertValue, VF_CHEAT,
        "0 = Suppress Asserts\n"
        "1 = Log Asserts\n"
        "2 = Show Assert Dialog\n"
        "Note: when set to '0 = Suppress Asserts', assert expressions are still evaluated. To turn asserts into a no-op, undefine AZ_ENABLE_TRACING and recompile.",
        OnAssertLevelCvarChanged);
    CSystem::SetAssertLevel(defaultAssertValue);

    REGISTER_CVAR2("sys_error_debugbreak", &g_cvars.sys_error_debugbreak, 0, VF_CHEAT, "__debugbreak() if a VALIDATOR_ERROR_DBGBREAK message is hit");

    // [VR]
    AZ::VR::HMDCVars::Register();

    REGISTER_STRING("dlc_directory", "", 0, "Holds the path to the directory where DLC should be installed to and read from");

#if defined(MAP_LOADING_SLICING)
    CreateSystemScheduler(this);
#endif // defined(MAP_LOADING_SLICING)

#if defined(WIN32) || defined(WIN64)
    REGISTER_INT("sys_screensaver_allowed", 0, VF_NULL, "Specifies if screen saver is allowed to start up while the game is running.");
#endif

    // Since the UI Canvas Editor is incomplete, we have a variable to enable it.
    // By default it is now enabled. Modify system.cfg or game.cfg to disable it
    REGISTER_INT("sys_enableCanvasEditor", 1, VF_NULL, "Enables the UI Canvas Editor");

    REGISTER_COMMAND_DEV_ONLY("DrillerStart", CmdDrillToFile, VF_DEV_ONLY, "Start a driller capture.");
    REGISTER_COMMAND_DEV_ONLY("DrillerStop", CmdDrillToFile, VF_DEV_ONLY, "Stop a driller capture.");

    REGISTER_COMMAND("sys_SetLogLevel", CmdSetAwsLogLevel, 0, "Set AWS log level [0 - 6].");
}

//////////////////////////////////////////////////////////////////////////
void CSystem::CreateAudioVars()
{
    assert(gEnv);
    assert(gEnv->pConsole);

    m_sys_audio_disable = REGISTER_INT("sys_audio_disable", 0, VF_REQUIRE_APP_RESTART,
            "Specifies whether to use the NULLAudioSystem in place of the regular AudioSystem\n"
            "Usage: sys_audio_disable [0/1]\n"
            "0: use regular AudioSystem.\n"
            "1: use NullAudioSystem (disable all audio functionality).\n"
            "Default: 0 (enable audio functionality)");
}

/////////////////////////////////////////////////////////////////////
void CSystem::AddCVarGroupDirectory(const string& sPath)
{
    CryLog("creating CVarGroups from directory '%s' ...", sPath.c_str());
    INDENT_LOG_DURING_SCOPE();

    AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst(ConcatPath(sPath, "*.cfg").c_str());

    if (!handle)
    {
        return;
    }

    do
    {
        if ((handle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
        {
            if (handle.m_filename != "." && handle.m_filename != "..")
            {
                AddCVarGroupDirectory(ConcatPath(sPath, handle.m_filename.data()));
            }
        }
        else
        {
            string sFilePath = ConcatPath(sPath, handle.m_filename.data());

            string sCVarName = sFilePath;
            PathUtil::RemoveExtension(sCVarName);

            if (m_env.pConsole != 0)
            {
                ((CXConsole*)m_env.pConsole)->RegisterCVarGroup(PathUtil::GetFile(sCVarName), sFilePath);
            }
        }
    } while (handle = gEnv->pCryPak->FindNext(handle));

    gEnv->pCryPak->FindClose(handle);
}

bool CSystem::RegisterErrorObserver(IErrorObserver* errorObserver)
{
    return stl::push_back_unique(m_errorObservers, errorObserver);
}

bool CSystem::UnregisterErrorObserver(IErrorObserver* errorObserver)
{
    return stl::find_and_erase(m_errorObservers, errorObserver);
}

void CSystem::OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber)
{
    if (g_cvars.sys_asserts == 0)
    {
        return;
    }

    std::vector<IErrorObserver*>::const_iterator end = m_errorObservers.end();
    for (std::vector<IErrorObserver*>::const_iterator it = m_errorObservers.begin(); it != end; ++it)
    {
        (*it)->OnAssert(condition, message, fileName, fileLineNumber);
    }
    if (g_cvars.sys_asserts > 1)
    {
        CryFatalError("<assert> %s\r\n%s\r\n%s (%d)\r\n", condition, message, fileName, fileLineNumber);
    }
}

void CSystem::OnFatalError(const char* message)
{
    std::vector<IErrorObserver*>::const_iterator end = m_errorObservers.end();
    for (std::vector<IErrorObserver*>::const_iterator it = m_errorObservers.begin(); it != end; ++it)
    {
        (*it)->OnFatalError(message);
    }
}

bool CSystem::IsAssertDialogVisible() const
{
    return m_bIsAsserting;
}

void CSystem::SetAssertVisible(bool bAssertVisble)
{
    m_bIsAsserting = bAssertVisble;
}

bool CSystem::LoadFontInternal(IFFont*& font, const string& fontName)
{
    font = m_env.pCryFont->NewFont(fontName);
    if (!font)
    {
        AZ_Assert(false, "Could not instantiate the default font.");
        return false;
    }

    //////////////////////////////////////////////////////////////////////////
    string szFontPath = "Fonts/" + fontName + ".font";

    if (!font->Load(szFontPath.c_str()))
    {
        AZ_Error(AZ_TRACE_SYSTEM_WINDOW, false, "Could not load font: %s.  Make sure the program is running from the correct working directory.", szFontPath.c_str());
        return false;
    }

    return true;
}
