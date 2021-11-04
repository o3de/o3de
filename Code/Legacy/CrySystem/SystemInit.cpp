/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "CrySystem_precompiled.h"
#include "System.h"

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

#include "CryLibrary.h"
#include "CryPath.h"

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/IO/LocalFileIO.h>

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

#include <IRenderer.h>
#include <AzCore/IO/FileIO.h>
#include <IMovieSystem.h>
#include <ILog.h>
#include <IAudioSystem.h>
#include <ICmdLine.h>
#include <IProcess.h>
#include <LyShine/ILyShine.h>
#include <HMDBus.h>

#include <AzFramework/Archive/Archive.h>
#include "XConsole.h"
#include "Log.h"
#include "XML/xml.h"
#include "LocalizedStringManager.h"
#include "SystemEventDispatcher.h"
#include "LevelSystem/LevelSystem.h"
#include "LevelSystem/SpawnableLevelSystem.h"
#include "ViewSystem/ViewSystem.h"
#include <CrySystemBus.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>

#if defined(ANDROID)
    #include <AzCore/Android/Utils.h>
#endif

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

//////////////////////////////////////////////////////////////////////////
#define DEFAULT_LOG_FILENAME "@log@/Log.txt"

#define CRYENGINE_ENGINE_FOLDER "Engine"

//////////////////////////////////////////////////////////////////////////
#define CRYENGINE_DEFAULT_LOCALIZATION_LANG "en-US"

#define LOCALIZATION_TRANSLATIONS_LIST_FILE_NAME "Libs/Localization/localization.xml"

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
            *p = 0xABCD;
        }
        break;
        case 2:
        {
            float a = 1.0f;
            memset(&a, 0, sizeof(a));
            [[maybe_unused]] float* b = &a;
            [[maybe_unused]] float c = 3;
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
                    // This could bypass the restricted cvar checks that exist elsewhere depending on
                    // the calling code so we also need check here before setting.
                    bool isConst = pCvar->IsConstCVar();
                    bool isCheat = ((pCvar->GetFlags() & (VF_CHEAT | VF_CHEAT_NOCHECK | VF_CHEAT_ALWAYS_CHECK)) != 0);
                    bool isReadOnly = ((pCvar->GetFlags() & VF_READONLY) != 0);
                    bool isDeprecated = ((pCvar->GetFlags() & VF_DEPRECATED) != 0);
                    bool allowApplyCvar = true;

                    if ((isConst || isCheat || isReadOnly) || isDeprecated)
                    {
                        allowApplyCvar = !isDeprecated && (gEnv->pSystem->IsDevMode()) || (gEnv->IsEditor());
                    }

                    if ((allowApplyCvar) || ALLOW_CONST_CVAR_MODIFICATIONS)
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
    AZStd::string moduleName = PathUtil::GetFileName(dllName);

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

    AZ::Crc32 key(dllName);
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

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitConsole()
{
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
    int nDefault = 0;
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
bool CSystem::InitFileSystem()
{
    using namespace AzFramework::AssetSystem;

    if (m_pUserCallback)
    {
        m_pUserCallback->OnInitProgress("Initializing File System...");
    }

    // get the DirectInstance FileIOBase which should be the AZ::LocalFileIO
    m_env.pFileIO = AZ::IO::FileIOBase::GetDirectInstance();

    m_env.pCryPak = AZ::Interface<AZ::IO::IArchive>::Get();
    m_env.pFileIO = AZ::IO::FileIOBase::GetInstance();
    AZ_Assert(m_env.pCryPak, "CryPak has not been initialized on AZ::Interface");
    AZ_Assert(m_env.pFileIO, "FileIOBase has not been initialized");

    if (m_bEditor)
    {
        m_env.pCryPak->RecordFileOpen(AZ::IO::IArchive::RFOM_EngineStartup);
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
bool CSystem::InitFileSystem_LoadEngineFolders(const SSystemInitParams&)
{
    LoadConfiguration(m_systemConfigName.c_str());
    AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Loading system configuration from %s...", m_systemConfigName.c_str());

#if defined(AZ_PLATFORM_ANDROID)
    AZ::Android::Utils::SetLoadFilesToMemory(m_sys_load_files_to_memory->GetString());
#endif

    GetISystem()->SetConfigPlatform(GetDevicePlatform());

    auto projectPath = AZ::Utils::GetProjectPath();
    AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Project Path: %s\n", projectPath.empty() ? "None specified" : projectPath.c_str());

    auto projectName = AZ::Utils::GetProjectName();
    AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Project Name: %s\n", projectName.empty() ? "None specified" : projectName.c_str());

    OpenPlatformPaks();

    // Load game-specific folder.
    LoadConfiguration("game.cfg");
    // Load the client/sever-specific configuration
    static const char* g_additionalConfig = gEnv->IsDedicated() ? "server_cfg" : "client_cfg";
    LoadConfiguration(g_additionalConfig, nullptr, false);

    // We do not use CVar groups on the consoles
    AddCVarGroupDirectory("Config/CVarGroups");

    return (true);
}

//////////////////////////////////////////////////////////////////////////
bool CSystem::InitAudioSystem(const SSystemInitParams& initParams)
{
    if (!Audio::Gem::AudioSystemGemRequestBus::HasHandlers())
    {
        // AudioSystem Gem has not been enabled for this project.
        // This should not generate an error, but calling scope will warn.
        return false;
    }

    bool useRealAudioSystem = false;
    if (!initParams.bPreview
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

//////////////////////////////////////////////////////////////////////////
void CSystem::InitLocalization()
{
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

    AZStd::string language = m_pLocalizationManager->LangNameFromPILID(languageID);
    m_pLocalizationManager->SetLanguage(language.c_str());
    if (m_pLocalizationManager->GetLocalizationFormat() == 1)
    {
        AZStd::string translationsListXML = LOCALIZATION_TRANSLATIONS_LIST_FILE_NAME;
        m_pLocalizationManager->InitLocalizationData(translationsListXML.c_str());

        m_pLocalizationManager->LoadAllLocalizationData();
    }
    else
    {
        // if the language value cannot be found, let's default to the english pak
        OpenLanguagePak(language.c_str());
    }

    if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
    {
        AZ::CVarFixedString languageAudio;
        console->GetCvarValue("g_languageAudio", languageAudio);
        if (languageAudio.empty())
        {
            console->PerformCommand(AZStd::string::format("g_languageAudio %s", language.c_str()).c_str());
        }
        else
        {
            language.assign(languageAudio.data(), languageAudio.size());
        }
    }
    OpenLanguageAudioPak(language.c_str());
}

void CSystem::OpenPlatformPaks()
{
    static bool bPlatformPaksLoaded = false;
    if (bPlatformPaksLoaded)
    {
        return;
    }
    bPlatformPaksLoaded = true;

    //////////////////////////////////////////////////////////////////////////
    // Open engine packs
    //////////////////////////////////////////////////////////////////////////


#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_15
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif

#ifdef AZ_PLATFORM_ANDROID
    const char* const assetsDir = "@products@";
    // Load Android Obb files if available
    const char* obbStorage = AZ::Android::Utils::GetObbStoragePath();
    AZStd::string mainObbPath = AZStd::move(AZStd::string::format("%s/%s", obbStorage, AZ::Android::Utils::GetObbFileName(true)));
    AZStd::string patchObbPath = AZStd::move(AZStd::string::format("%s/%s", obbStorage, AZ::Android::Utils::GetObbFileName(false)));
    m_env.pCryPak->OpenPack(assetsDir, mainObbPath.c_str());
    m_env.pCryPak->OpenPack(assetsDir, patchObbPath.c_str());
#endif //AZ_PLATFORM_ANDROID

    InlineInitializationProcessing("CSystem::OpenPlatformPaks OpenPacks( Engine... )");
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
    AZStd::string sLocalizationFolder = PathUtil::GetLocalizationFolder();

    // load xml pak with full filenames to perform wildcard searches.
    AZStd::string sLocalizedPath;
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

    // Omit the trailing slash!
    AZStd::string sLocalizationFolder(AZStd::string().assign(PathUtil::GetLocalizationFolder(), 0, PathUtil::GetLocalizationFolder().size() - 1));

    if (!AZ::StringFunc::Equal(sLocalizationFolder, "Languages", false))
    {
        sLocalizationFolder = "@products@";
    }

    // load localized pak with crc32 filenames on consoles to save memory.
    AZStd::string sLocalizedPath = "loc.pak";

    if (!m_env.pCryPak->OpenPacks(sLocalizationFolder.c_str(), sLocalizedPath.c_str()))
    {
        // make sure the localized language is found - not really necessary, for TC
        AZ_Error(AZ_TRACE_SYSTEM_WINDOW, false, "Localized language content(%s) not available or modified from the original installation.", sLanguage);
    }
}


AZStd::string GetUniqueLogFileName(AZStd::string logFileName)
{
    AZStd::string logFileNamePrefix = logFileName;
    if ((logFileNamePrefix[0] != '@') && (AzFramework::StringFunc::Path::IsRelative(logFileNamePrefix.c_str())))
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

    AZStd::string logFileExtension;
    size_t extensionIndex = logFileName.find_last_of('.');
    if (extensionIndex != AZStd::string::npos)
    {
        logFileExtension = logFileName.substr(extensionIndex, logFileName.length() - extensionIndex);
        logFileNamePrefix = logFileName.substr(0, extensionIndex);
    }

    logFileName = AZStd::string::format("%s(%d)%s", logFileNamePrefix.c_str(), instance, logFileExtension.c_str()).c_str();

    return logFileName;
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

    SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_INIT);
    gEnv->mMainThreadId = GetCurrentThreadId();         //Set this ASAP on startup

    InlineInitializationProcessing("CSystem::Init start");

    m_env.bNoAssertDialog = false;

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
                R"(This typically done by setting the "assets" field within a .setreg file)""\n"
                R"(A fallback of %s will be used.)",
                AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey,
                assetPlatform.c_str());
        }

        m_systemConfigName = "system_" AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER "_";
        m_systemConfigName += assetPlatform.c_str();
        m_systemConfigName += ".cfg";
    }

#if defined(WIN32) || defined(WIN64)
    // check OS version - we only want to run on XP or higher - talk to Martin Mittring if you want to change this
    {
        OSVERSIONINFO osvi;

        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
AZ_PUSH_DISABLE_WARNING(4996, "-Wunknown-warning-option")
        GetVersionExW(&osvi);
AZ_POP_DISABLE_WARNING

        bool bIsWindowsXPorLater = osvi.dwMajorVersion > 5 || (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1);

        if (!bIsWindowsXPorLater)
        {
            AZ_Error(AZ_TRACE_SYSTEM_WINDOW, false, "Open 3D Engine requires an OS version of Windows XP or later.");
            return false;
        }
    }
#endif

    // Get file version information.
    QueryVersionInfo();
    DetectGameFolderAccessRights();

    m_bEditor = startupParams.bEditor;
    m_bPreviewMode = startupParams.bPreview;
    m_bTestMode = startupParams.bTestMode;
    m_pUserCallback = startupParams.pUserCallback;

    m_bDedicatedServer = startupParams.bDedicatedServer;
    m_currentLanguageAudio = "";

#if !defined(CONSOLE)
    m_env.SetIsEditor(m_bEditor);
    m_env.SetIsEditorGameMode(false);
    m_env.SetIsEditorSimulationMode(false);
#endif

    m_env.SetToolMode(startupParams.bToolMode);

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
                const AZStd::string sUniqueLogFileName = GetUniqueLogFileName(startupParams.sLogFileName);
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

        InitFileSystem_LoadEngineFolders(startupParams);

#if !defined(RELEASE) || defined(RELEASE_LOGGING)
        // now that the system cfgs have been loaded, we can start the remote console
        GetIRemoteConsole()->Update();
#endif

        InlineInitializationProcessing("CSystem::Init Load Engine Folders");

        //////////////////////////////////////////////////////////////////////////
        //Load config files
        //////////////////////////////////////////////////////////////////////////

        // tools may not interact with @user@
        if (!gEnv->IsInToolMode())
        {
            if (m_pCmdLine->FindArg(eCLAT_Pre, "ResetProfile") == 0)
            {
                LoadConfiguration("@user@/game.cfg", 0, false);
            }
        }

        {
            // We have to load this file again since first time we did it without devmode
            LoadConfiguration(m_systemConfigName.c_str());
            // Optional user defined overrides
            LoadConfiguration("user.cfg");

#if defined(ENABLE_STATS_AGENT)
            if (m_pCmdLine->FindArg(eCLAT_Pre, "useamblecfg"))
            {
                LoadConfiguration("amble.cfg");
            }
#endif
        }

        //////////////////////////////////////////////////////////////////////////
        if (g_cvars.sys_asserts == 0)
        {
            gEnv->bIgnoreAllAsserts = true;
        }
        if (g_cvars.sys_asserts == 2)
        {
            gEnv->bNoAssertDialog = true;
        }

        LogBuildInfo();

        InlineInitializationProcessing("CSystem::Init LoadConfigurations");

#ifdef WIN32
        if (g_cvars.sys_WER)
        {
            SetUnhandledExceptionFilter(CryEngineExceptionFilterWER);
        }
#endif

        //////////////////////////////////////////////////////////////////////////
        // Localization
        //////////////////////////////////////////////////////////////////////////
        {
            InitLocalization();
        }
        InlineInitializationProcessing("CSystem::Init InitLocalizations");

        //////////////////////////////////////////////////////////////////////////
        // Open basic pak files after intro movie playback started
        //////////////////////////////////////////////////////////////////////////
        OpenPlatformPaks();

        //////////////////////////////////////////////////////////////////////////
        // AUDIO
        //////////////////////////////////////////////////////////////////////////
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


        // Compiling the default system textures can be the lengthiest portion of
        // editor initialization, so it is useful to inform users that they are waiting on
        // the necessary default textures to compile.
        if (m_pUserCallback)
        {
            m_pUserCallback->OnInitProgress("First time asset processing - may take a minute...");
        }

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        // System cursor
        //////////////////////////////////////////////////////////////////////////
        // - Dedicated server is in console mode by default (system cursor is always shown when console is)
        // - System cursor is always visible by default in Editor (we never start directly in Game Mode)
        // - System cursor has to be enabled manually by the Game if needed; the custom UiCursor will typically be used instead

        if (!gEnv->IsDedicated() &&
            !gEnv->IsEditor())
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

        // CONSOLE
        //////////////////////////////////////////////////////////////////////////
        if (!InitConsole())
        {
            return false;
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

        if (m_env.pLyShine)
        {
            m_env.pLyShine->PostInit();
        }

        InlineInitializationProcessing("CSystem::Init InitLmbrAWS");

        // Az to Cry console binding
        AZ::Interface<AZ::IConsole>::Get()->VisitRegisteredFunctors([](AZ::ConsoleFunctorBase* functor) { AzConsoleToCryConsoleBinder::Visit(functor); });
        AzConsoleToCryConsoleBinder::s_commandRegisteredHandler.Connect(AZ::Interface<AZ::IConsole>::Get()->GetConsoleCommandRegisteredEvent());

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

    // Send out EBus event
    EBUS_EVENT(CrySystemEventBus, OnCrySystemInitialized, *this, startupParams);

    // Execute any deferred commands that uses the CVar commands that were just registered
    AZ::Interface<AZ::IConsole>::Get()->ExecuteDeferredConsoleCommands();

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

    return true;
}


static void LoadConfigurationCmd(IConsoleCmdArgs* pParams)
{
    assert(pParams);

    if (pParams->GetArgCount() != 2)
    {
        gEnv->pLog->LogError("LoadConfiguration failed, one parameter needed");
        return;
    }

    GetISystem()->LoadConfiguration((AZStd::string("Config/") + pParams->GetArg(1)).c_str());
}


// --------------------------------------------------------------------------------------------------------------------------

static AZStd::string ConcatPath(const char* szPart1, const char* szPart2)
{
    if (szPart1[0] == 0)
    {
        return szPart2;
    }

    AZStd::string ret;

    ret.reserve(strlen(szPart1) + 1 + strlen(szPart2));

    ret = szPart1;
    ret += "/";
    ret += szPart2;

    return ret;
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
    attachVariable("sys_UncachedStreamReads", &g_cvars.archiveVars.nUncachedStreamReads, "Enable stream reads via an uncached file handle");
    attachVariable("sys_PakDisableNonLevelRelatedPaks", &g_cvars.archiveVars.nDisableNonLevelRelatedPaks, "Disables all paks that are not required by specific level; This is used with per level splitted assets.");
    attachVariable("sys_PakWarnOnPakAccessFailures", &g_cvars.archiveVars.nWarnOnPakAccessFails, "If 1, access failure for Paks is treated as a warning, if zero it is only a log message.");

    static const int fileSystemCaseSensitivityDefault = 0;
    REGISTER_CVAR2("sys_FilesystemCaseSensitivity", &g_cvars.sys_FilesystemCaseSensitivity, fileSystemCaseSensitivityDefault, VF_NULL,
        "0 - CryPak lowercases all input file names\n"
        "1 - CryPak preserves file name casing\n"
        "Default is 1");

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

#if (defined(WIN32) || defined(WIN64)) && defined(_DEBUG)
    REGISTER_CVAR2("sys_float_exceptions", &g_cvars.sys_float_exceptions, 2, 0, "Use or not use floating point exceptions.");
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

    /*
        // experimental feature? - needs to be created very early
        m_sys_filecache = REGISTER_INT("sys_FileCache",0,0,
            "To speed up loading from non HD media\n"
            "0=off / 1=enabled");
    */
    REGISTER_CVAR2("sys_AI", &g_cvars.sys_ai, 1, 0, "Enables AI Update");
    REGISTER_CVAR2("sys_entities", &g_cvars.sys_entitysystem, 1, 0, "Enables Entities Update");
    REGISTER_CVAR2("sys_trackview", &g_cvars.sys_trackview, 1, 0, "Enables TrackView Update");

    //Defines selected language.
    REGISTER_STRING_CB("g_language", "", VF_NULL, "Defines which language pak is loaded", CSystem::OnLanguageCVarChanged);

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
        "3 = Crashes the Application on Assert\n"
        "Note: when set to '0 = Suppress Asserts', assert expressions are still evaluated. To turn asserts into a no-op, undefine AZ_ENABLE_TRACING and recompile.",
        OnAssertLevelCvarChanged);
    CSystem::SetAssertLevel(defaultAssertValue);

    REGISTER_CVAR2("sys_error_debugbreak", &g_cvars.sys_error_debugbreak, 0, VF_CHEAT, "__debugbreak() if a VALIDATOR_ERROR_DBGBREAK message is hit");

    REGISTER_STRING("dlc_directory", "", 0, "Holds the path to the directory where DLC should be installed to and read from");

#if defined(WIN32) || defined(WIN64)
    REGISTER_INT("sys_screensaver_allowed", 0, VF_NULL, "Specifies if screen saver is allowed to start up while the game is running.");
#endif

    // Since the UI Canvas Editor is incomplete, we have a variable to enable it.
    // By default it is now enabled. Modify system.cfg or game.cfg to disable it
    REGISTER_INT("sys_enableCanvasEditor", 1, VF_NULL, "Enables the UI Canvas Editor");

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
void CSystem::AddCVarGroupDirectory(const AZStd::string& sPath)
{
    CryLog("creating CVarGroups from directory '%s' ...", sPath.c_str());
    INDENT_LOG_DURING_SCOPE();

    AZ::IO::ArchiveFileIterator handle = gEnv->pCryPak->FindFirst(ConcatPath(sPath.c_str(), "*.cfg").c_str());

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
                AddCVarGroupDirectory(ConcatPath(sPath.c_str(), handle.m_filename.data()));
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
