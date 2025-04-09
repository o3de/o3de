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

#include "CryPath.h"

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Quality/QualitySystemBus.h>

#include "AZCoreLogSink.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/Archive/ArchiveFileIO.h>
#include <AzFramework/Archive/INestedArchive.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzCore/Console/ConsoleDataWrapper.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/Logging/MissingAssetLogger.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <CryCommon/LoadScreenBus.h>

#if defined(APPLE) || defined(LINUX)
#include <cstdlib>
#include <dlfcn.h>
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include <float.h>

#endif // WIN32

#include <AzCore/IO/FileIO.h>
#include <IAudioSystem.h>
#include <ICmdLine.h>
#include <ILog.h>
#include <IMovieSystem.h>
#include <IRenderer.h>

#include "LevelSystem/LevelSystem.h"
#include "LevelSystem/SpawnableLevelSystem.h"
#include "LocalizedStringManager.h"
#include "Log.h"
#include "SystemEventDispatcher.h"
#include "XConsole.h"
#include "XML/xml.h"
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzFramework/Archive/Archive.h>
#include <CrySystemBus.h>

#if defined(ANDROID)
#include <AzCore/Android/Utils.h>
#endif

#if defined(EXTERNAL_CRASH_REPORTING)
#include <CrashHandler.h>
#endif

// select the asset processor based on cvars and defines.
// uncomment the following and edit the path where it is instantiated if you'd like to use the test file client
// #define USE_TEST_FILE_CLIENT

#if defined(REMOTE_ASSET_PROCESSOR)
// Over here, we'd put the header to the Remote Asset Processor interface (as opposed to the Local built in version  below)
#include <AzFramework/Network/AssetProcessorConnection.h>
#endif

#ifdef WIN32
extern LONG WINAPI CryEngineExceptionFilterWER(struct _EXCEPTION_POINTERS* pExceptionPointers);
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_14
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif

//////////////////////////////////////////////////////////////////////////
#define DEFAULT_LOG_FILENAME "@log@/Log.txt"

#define CRYENGINE_ENGINE_FOLDER "Engine"

//////////////////////////////////////////////////////////////////////////
#define CRYENGINE_DEFAULT_LOCALIZATION_LANG "en-US"

#define LOCALIZATION_TRANSLATIONS_LIST_FILE_NAME "Libs/Localization/localization.xml"

#define AZ_TRACE_SYSTEM_WINDOW AZ::Debug::Trace::GetDefaultSystemWindow()

#ifdef WIN32
extern HMODULE gDLLHandle;
#endif

// static int g_sysSpecChanged = false;

struct SCVarsClientConfigSink : public ILoadConfigurationEntrySink
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
        // This method intentionally crashes, a lot.

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
                new char[128]; // testing the crash handler an exception in the cry memory allocation occurred
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
ICVar* CSystem::attachVariable(const char* szVarName, int* pContainer, const char* szComment, int dwFlags)
{
    IConsole* pConsole = GetIConsole();

    ICVar* pOldVar = pConsole->GetCVar(szVarName);
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
    assert(*pContainer == pVar->GetIVal());
    ++*pContainer;
    assert(*pContainer == pVar->GetIVal());
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

    // we are good to go
    return true;
}

void CSystem::ShutdownFileSystem()
{
    m_env.pFileIO = nullptr;
}

/////////////////////////////////////////////////////////////////////////////////
bool CSystem::InitFileSystem_LoadEngineFolders(const SSystemInitParams&)
{
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
bool CSystem::InitAudioSystem()
{
    if (!Audio::Gem::SystemRequestBus::HasHandlers())
    {
        // AudioSystem Gem has not been enabled for this project/configuration (e.g. Server).
        // This should not generate an error, but calling scope will warn.
        return false;
    }

    bool result = false;
    Audio::Gem::SystemRequestBus::BroadcastResult(result, &Audio::Gem::SystemRequestBus::Events::Initialize);
    if (result)
    {
        AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Audio System is initialized and ready!\n");
    }
    else
    {
        AZ_Error(AZ_TRACE_SYSTEM_WINDOW, result, "The Audio System did not initialize correctly!\n");
    }

    return result;
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
#endif // AZ_PLATFORM_ANDROID

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
    if (!m_env.pCryPak->OpenPacks(
            { sLocalizationFolder.c_str(), sLocalizationFolder.size() }, { sLocalizedPath.c_str(), sLocalizedPath.size() }, 0))
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
    AZStd::string sLocalizationFolder(
        AZStd::string().assign(PathUtil::GetLocalizationFolder(), 0, PathUtil::GetLocalizationFolder().size() - 1));

    if (!AZ::StringFunc::Equal(sLocalizationFolder, "Languages", false))
    {
        sLocalizationFolder = "@products@";
    }

    // load localized pak with crc32 filenames on consoles to save memory.
    AZStd::string sLocalizedPath = "loc.pak";

    if (!m_env.pCryPak->OpenPacks(sLocalizationFolder.c_str(), sLocalizedPath.c_str()))
    {
        // make sure the localized language is found - not really necessary, for TC
        AZ_Error(
            AZ_TRACE_SYSTEM_WINDOW,
            false,
            "Localized language content(%s) not available or modified from the original installation.",
            sLanguage);
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

namespace AZ
{
    // Purposely Null Uuid, so it isn't aggregated into the ConsoleDataWrapper Uuid
    AZ_TYPE_INFO_SPECIALIZE(AZ::ThreadSafety, AZ::Uuid{});

    AZ_TYPE_INFO_TEMPLATE_WITH_NAME(AZ::ConsoleDataWrapper, "ConsoleDataWrapper", "{1E5AB0FC-83FF-4715-BBE9-348B880613B0}", AZ_TYPE_INFO_CLASS, AZ_TYPE_INFO_AUTO);
} // namespace AZ

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

        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(
            command.c_str(), AZ::ConsoleSilentMode::Silent, AZ::ConsoleInvokedFrom::CryBinding);
    }

    static void OnVarChanged(ICVar* cvar)
    {
        AZ::CVarFixedString command = AZ::CVarFixedString::format("%s %s", cvar->GetName(), cvar->GetString());
        AZ::Interface<AZ::IConsole>::Get()->PerformCommand(
            command.c_str(), AZ::ConsoleSilentMode::Silent, AZ::ConsoleInvokedFrom::CryBinding);
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

        // only add CVar versions if they are not already present
        auto existingCVar = gEnv->pConsole->GetCVar(functor->GetName());
        if (!existingCVar)
        {
            const auto typeId = functor->GetTypeId();
            if (typeId != AZ::TypeId::CreateNull())
            {
                auto registerType = [&typeId, &functor, &cryFlags](auto value) -> bool
                {
                    using type = decltype(value);
                    using consoleDataWrapperType = AZ::ConsoleDataWrapper<type, ConsoleThreadSafety<type>>;
                    if (typeId == azrtti_typeid<type>() || typeId == azrtti_typeid<consoleDataWrapperType>())
                    {
                        functor->GetValue(value);
                        if constexpr (AZStd::is_same_v<type, bool>)
                        {
                            AZ::CVarFixedString valueString;
                            functor->GetValue(valueString);
                            return (
                                gEnv->pConsole->RegisterString(
                                    functor->GetName(),
                                    valueString.c_str(),
                                    cryFlags,
                                    functor->GetDesc(),
                                    AzConsoleToCryConsoleBinder::OnVarChanged) != nullptr);
                        }
                        else if constexpr (AZStd::is_integral_v<type>)
                        {
                            return (
                                gEnv->pConsole->RegisterInt(
                                    functor->GetName(),
                                    static_cast<int>(value),
                                    cryFlags,
                                    functor->GetDesc(),
                                    AzConsoleToCryConsoleBinder::OnVarChanged) != nullptr);
                        }
                        else if constexpr (AZStd::is_floating_point_v<type>)
                        {
                            return (
                                gEnv->pConsole->RegisterFloat(
                                    functor->GetName(),
                                    static_cast<float>(value),
                                    cryFlags,
                                    functor->GetDesc(),
                                    AzConsoleToCryConsoleBinder::OnVarChanged) != nullptr);
                        }
                    }
                    return false;
                };
                // register fundamental types
                bool registered = registerType(bool()) || registerType(AZ::s32()) || registerType(AZ::u32()) || registerType(float()) ||
                    registerType(double()) || registerType(AZ::s16()) || registerType(AZ::u16()) || registerType(AZ::s64()) ||
                    registerType(AZ::u64()) || registerType(AZ::s8()) || registerType(AZ::u8());

                if (!registered)
                {
                    // register all other types as strings, if possible
                    AZ::CVarFixedString value;
                    functor->GetValue(value);
                    gEnv->pConsole->RegisterString(
                        functor->GetName(), value.c_str(), cryFlags, functor->GetDesc(), AzConsoleToCryConsoleBinder::OnVarChanged);
                }
            }
            else
            {
                gEnv->pConsole->RemoveCommand(functor->GetName());
                gEnv->pConsole->AddCommand(functor->GetName(), AzConsoleToCryConsoleBinder::OnInvoke, cryFlags, functor->GetDesc());
            }
        }
        else
        {
            existingCVar->AddOnChangeFunctor(
                AZ::Name("AZCryBinder"),
                [existingCVar]()
                {
                    AzConsoleToCryConsoleBinder::OnVarChanged(existingCVar);
                });
        }
    }

    using CommandRegisteredHandler = AZ::IConsole::ConsoleCommandRegisteredEvent::Handler;
    static inline CommandRegisteredHandler s_commandRegisteredHandler = CommandRegisteredHandler(
        [](AZ::ConsoleFunctorBase* functor)
        {
            Visit(functor);
        });
};

// System initialization
/////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////
bool CSystem::Init(const SSystemInitParams& startupParams)
{
    // Temporary Fix for an issue accessing gEnv from this object instance. The gEnv is not resolving to the
    // global gEnv, instead its resolving an some uninitialized gEnv elsewhere (NULL). Since gEnv is
    // initialized to this instance's SSystemGlobalEnvironment (m_env), we will force set it again here
    // to m_env
    if (!gEnv)
    {
        gEnv = &m_env;
    }

    SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_INIT);
    gEnv->mMainThreadId = GetCurrentThreadId(); // Set this ASAP on startup

    InlineInitializationProcessing("CSystem::Init start");

    m_env.bNoAssertDialog = false;

    m_bNoCrashDialog = gEnv->IsDedicated();

    if (startupParams.bUnattendedMode)
    {
        m_bNoCrashDialog = true;
        m_env.bNoAssertDialog = true; // this also suppresses CryMessageBox
        g_cvars.sys_no_crash_dialog = true;
    }

#if defined(AZ_PLATFORM_LINUX)
    // Linux is all console for now and so no room for dialog boxes!
    m_env.bNoAssertDialog = true;
#endif

    m_pCmdLine = new CCmdLine(startupParams.szSystemCmdLine);

    // Init AZCoreLogSink. Don't suppress system output if we're running as an editor-server
    bool suppressSystemOutput = true;
    if (const ICmdLineArg* isEditorServerArg = m_pCmdLine->FindArg(eCLAT_Pre, "editorsv_isDedicated"))
    {
        bool editorsv_isDedicated = false;
        if (isEditorServerArg->GetBoolValue(editorsv_isDedicated) && editorsv_isDedicated)
        {
            suppressSystemOutput = false;
        }
    }
    AZCoreLogSink::Connect(suppressSystemOutput);

    // Registers all AZ Console Variables functors specified within CrySystem
    if (auto azConsole = AZ::Interface<AZ::IConsole>::Get(); azConsole)
    {
        azConsole->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());
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

            const ICmdLineArg* logfile = m_pCmdLine->FindArg(eCLAT_Pre, "logfile"); // see if the user specified a log name, if so use it
            if (logfile && strlen(logfile->GetValue()) > 0)
            {
                m_env.pLog->SetFileName(logfile->GetValue(), startupParams.autoBackupLogs);
            }
            else if (startupParams.sLogFileName) // otherwise see if the startup params has a log file name, if so use it
            {
                const AZStd::string sUniqueLogFileName = GetUniqueLogFileName(startupParams.sLogFileName);
                m_env.pLog->SetFileName(sUniqueLogFileName.c_str(), startupParams.autoBackupLogs);
            }
            else // use the default log name
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
        // Load config files
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
            [[maybe_unused]] bool audioInitResult = InitAudioSystem();
            // Getting false here is not an error, the engine may run fine without it so a warning here is sufficient.
            // But if there were errors internally during initialization, those would be reported above this.
            AZ_Warning(AZ_TRACE_SYSTEM_WINDOW, audioInitResult, "<Audio>: Running without any AudioSystem!");
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

        if (!gEnv->IsDedicated() && !gEnv->IsEditor())
        {
            AzFramework::InputSystemCursorRequestBus::Event(
                AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                AzFramework::SystemCursorState::ConstrainedAndHidden);
        }

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
        AZ_Printf(AZ_TRACE_SYSTEM_WINDOW, "Initializing additional systems\n");

        //////////////////////////////////////////////////////////////////////////
        // LEVEL SYSTEM
        m_pLevelSystem = new LegacyLevelSystem::SpawnableLevelSystem(this);

        InlineInitializationProcessing("CSystem::Init Level System");

        // Az to Cry console binding
        AZ::Interface<AZ::IConsole>::Get()->VisitRegisteredFunctors(
            [](AZ::ConsoleFunctorBase* functor)
            {
                AzConsoleToCryConsoleBinder::Visit(functor);
            });
        AzConsoleToCryConsoleBinder::s_commandRegisteredHandler.Connect(
            AZ::Interface<AZ::IConsole>::Get()->GetConsoleCommandRegisteredEvent());

        if (g_cvars.sys_float_exceptions > 0)
        {
            if (g_cvars.sys_float_exceptions == 3 && gEnv->IsEditor()) // Turn off float exceptions in editor if sys_float_exceptions = 3
            {
                g_cvars.sys_float_exceptions = 0;
            }
            if (g_cvars.sys_float_exceptions > 0)
            {
                AZ_TracePrintf(
                    AZ_TRACE_SYSTEM_WINDOW,
                    "Enabled float exceptions(sys_float_exceptions %d). This makes the performance slower.",
                    g_cvars.sys_float_exceptions);
            }
        }
        EnableFloatExceptions(g_cvars.sys_float_exceptions);
    }

    InlineInitializationProcessing("CSystem::Init End");

    // All CVARs should now be registered, load and apply quality settings for the default quality group
    // using device rules to auto-detected the correct quality level 
    AzFramework::QualitySystemEvents::Bus::Broadcast(
        &AzFramework::QualitySystemEvents::LoadDefaultQualityGroup,
        AzFramework::QualityLevel::LevelFromDeviceRules);

    // Send out EBus event
    EBUS_EVENT(CrySystemEventBus, OnCrySystemInitialized, *this, startupParams);

    // Execute any deferred commands that uses the CVar commands that were just registered
    AZ::Interface<AZ::IConsole>::Get()->ExecuteDeferredConsoleCommands();

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

//////////////////////////////////////////////////////////////////////////
void CSystem::CreateSystemVars()
{
    assert(gEnv);
    assert(gEnv->pConsole);

#if AZ_LOADSCREENCOMPONENT_ENABLED
    m_game_load_screen_uicanvas_path = REGISTER_STRING("game_load_screen_uicanvas_path", "", 0, "Game load screen UiCanvas path.");
    m_level_load_screen_uicanvas_path = REGISTER_STRING("level_load_screen_uicanvas_path", "", 0, "Level load screen UiCanvas path.");
    m_game_load_screen_sequence_to_auto_play =
        REGISTER_STRING("game_load_screen_sequence_to_auto_play", "", 0, "Game load screen UiCanvas animation sequence to play on load.");
    m_level_load_screen_sequence_to_auto_play =
        REGISTER_STRING("level_load_screen_sequence_to_auto_play", "", 0, "Level load screen UiCanvas animation sequence to play on load.");
    m_game_load_screen_sequence_fixed_fps = REGISTER_FLOAT(
        "game_load_screen_sequence_fixed_fps", 60.0f, 0, "Fixed frame rate fed to updates of the game load screen sequence.");
    m_level_load_screen_sequence_fixed_fps = REGISTER_FLOAT(
        "level_load_screen_sequence_fixed_fps", 60.0f, 0, "Fixed frame rate fed to updates of the level load screen sequence.");
    m_game_load_screen_max_fps =
        REGISTER_FLOAT("game_load_screen_max_fps", 30.0f, 0, "Max frame rate to update the game load screen sequence.");
    m_level_load_screen_max_fps =
        REGISTER_FLOAT("level_load_screen_max_fps", 30.0f, 0, "Max frame rate to update the level load screen sequence.");
    m_game_load_screen_minimum_time = REGISTER_FLOAT(
        "game_load_screen_minimum_time",
        0.0f,
        0,
        "Minimum amount of time to show the game load screen. Important to prevent short loads from flashing the load screen. 0 means "
        "there is no limit.");
    m_level_load_screen_minimum_time = REGISTER_FLOAT(
        "level_load_screen_minimum_time",
        0.0f,
        0,
        "Minimum amount of time to show the level load screen. Important to prevent short loads from flashing the load screen. 0 means "
        "there is no limit.");
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED

    REGISTER_INT("cvDoVerboseWindowTitle", 0, VF_NULL, "");

    // Register an AZ Console command to quit the engine.
    // The command is available even in Release builds.
    static AZ::ConsoleFunctor<void, false> s_functorQuit(
        "quit",
        "Quit/Shutdown the engine",
        AZ::ConsoleFunctorFlags::AllowClientSet | AZ::ConsoleFunctorFlags::DontReplicate,
        AZ::TypeId::CreateNull(),
        []([[maybe_unused]] const AZ::ConsoleCommandContainer& params)
        {
            GetISystem()->Quit();
        });

    static AZ::ConsoleFunctor<void, false> s_functorCrash(
        "crash",
        "Crash the engine",
        AZ::ConsoleFunctorFlags::IsInvisible | AZ::ConsoleFunctorFlags::DontReplicate,
        AZ::TypeId::CreateNull(),
        []([[maybe_unused]] const AZ::ConsoleCommandContainer& params)
        {
            AZ_Crash();
        });

    m_sys_load_files_to_memory = REGISTER_STRING(
        "sys_load_files_to_memory",
        "shadercache.pak",
        0,
        "Specify comma separated list of filenames that need to be loaded to memory.\n"
        "Partial names also work. Eg. \"shader\" will load:\n"
        "shaders.pak, shadercache.pak, and shadercachestartup.pak");

#ifndef _RELEASE
    REGISTER_STRING_CB("sys_version", "", VF_CHEAT, "Override system file/product version", SystemVersionChanged);
#endif // #ifndef _RELEASE

    attachVariable("sys_PakSaveLevelResourceList", &g_cvars.archiveVars.nSaveLevelResourceList, "Save resource list when loading level");
    attachVariable("sys_PakLogInvalidFileAccess", &g_cvars.archiveVars.nLogInvalidFileAccess, "Log synchronous file access when in game");

    m_sysNoUpdate = REGISTER_INT(
        "sys_noupdate",
        0,
        VF_CHEAT,
        "Toggles updating of system with sys_script_debugger.\n"
        "Usage: sys_noupdate [0/1]\n"
        "Default is 0 (system updates during debug).");

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

    m_svDedicatedMaxRate = REGISTER_FLOAT(
        "sv_DedicatedMaxRate",
        30.0f,
        0,
        "Sets the maximum update rate when running as a dedicated server.\n"
        "Usage: sv_DedicatedMaxRate [5..500]\n"
        "Default is 30.");

    REGISTER_FLOAT(
        "sv_DedicatedCPUPercent",
        0.0f,
        0,
        "Sets the target CPU usage when running as a dedicated server, or disable this feature if it's zero.\n"
        "Usage: sv_DedicatedCPUPercent [0..100]\n"
        "Default is 0 (disabled).");
    REGISTER_FLOAT(
        "sv_DedicatedCPUVariance",
        10.0f,
        0,
        "Sets how much the CPU can vary from sv_DedicateCPU (up or down) without adjusting the framerate.\n"
        "Usage: sv_DedicatedCPUVariance [5..50]\n"
        "Default is 10.");

    m_sys_firstlaunch = REGISTER_INT("sys_firstlaunch", 0, 0, "Indicates that the game was run for the first time.");

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_12
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEMINIT_CPP_SECTION_17
#include AZ_RESTRICTED_FILE(SystemInit_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#define SYS_STREAMING_CPU_DEFAULT_VALUE 1
#define SYS_STREAMING_CPU_WORKER_DEFAULT_VALUE 5
#endif
#define DEFAULT_USE_OPTICAL_DRIVE_THREAD (gEnv->IsDedicated() ? 0 : 1)

    const char* localizeFolder = "Localization";
    g_cvars.sys_localization_folder = REGISTER_STRING_CB(
        "sys_localization_folder",
        localizeFolder,
        VF_NULL,
        "Sets the folder where to look for localized data.\n"
        "This cvar allows for backwards compatibility so localized data under the game folder can still be found.\n"
        "Usage: sys_localization_folder <folder name>\n"
        "Default: Localization\n",
        CSystem::OnLocalizationFolderCVarChanged);

    REGISTER_CVAR2("sys_float_exceptions", &g_cvars.sys_float_exceptions, 0, 0, "Use or not use floating point exceptions.");

    REGISTER_CVAR2("sys_update_profile_time", &g_cvars.sys_update_profile_time, 1.0f, 0, "Time to keep updates timings history for.");
    REGISTER_CVAR2(
        "sys_no_crash_dialog", &g_cvars.sys_no_crash_dialog, m_bNoCrashDialog, VF_NULL, "Whether to disable the crash dialog window");
    REGISTER_CVAR2(
        "sys_no_error_report_window",
        &g_cvars.sys_no_error_report_window,
        m_bNoErrorReportWindow,
        VF_NULL,
        "Whether to disable the error report list");
#if defined(_RELEASE)
    if (!gEnv->IsDedicated())
    {
        REGISTER_CVAR2("sys_WER", &g_cvars.sys_WER, 1, 0, "Enables Windows Error Reporting");
    }
#else
    REGISTER_CVAR2("sys_WER", &g_cvars.sys_WER, 0, 0, "Enables Windows Error Reporting");
#endif

    const int DEFAULT_DUMP_TYPE = 2;

    REGISTER_CVAR2(
        "sys_dump_type",
        &g_cvars.sys_dump_type,
        DEFAULT_DUMP_TYPE,
        VF_NULL,
        "Specifies type of crash dump to create - see MINIDUMP_TYPE in dbghelp.h for full list of values\n"
        "0: Do not create a minidump\n"
        "1: Create a small minidump (stacktrace)\n"
        "2: Create a medium minidump (+ some variables)\n"
        "3: Create a full minidump (+ all memory)\n");
    REGISTER_CVAR2(
        "sys_dump_aux_threads", &g_cvars.sys_dump_aux_threads, 1, VF_NULL, "Dumps callstacks of other threads in case of a crash");

#if (defined(WIN32) || defined(WIN64)) && defined(_RELEASE)
    const int DEFAULT_SYS_MAX_FPS = 0;
#else
    const int DEFAULT_SYS_MAX_FPS = -1;
#endif
    REGISTER_CVAR2(
        "sys_MaxFPS",
        &g_cvars.sys_MaxFPS,
        DEFAULT_SYS_MAX_FPS,
        VF_NULL,
        "Limits the frame rate to specified number n (if n>0 and if vsync is disabled).\n"
        " 0 = on PC if vsync is off auto throttles fps while in menu or game is paused (default)\n"
        "-1 = off");

    REGISTER_CVAR2(
        "sys_maxTimeStepForMovieSystem",
        &g_cvars.sys_maxTimeStepForMovieSystem,
        0.1f,
        VF_NULL,
        "Caps the time step for the movie system so that a cut-scene won't be jumped in the case of an extreme stall.");

    REGISTER_COMMAND(
        "sys_crashtest",
        CmdCrashTest,
        VF_CHEAT,
        "Make the game crash\n"
        "0=off\n"
        "1=null pointer exception\n"
        "2=floating point exception\n"
        "3=memory allocation exception\n"
        "4=cry fatal error is called\n"
        "5=memory allocation for small blocks\n"
        "6=assert\n"
        "7=debugbreak\n"
        "8=10min sleep");

    REGISTER_FLOAT("sys_scale3DMouseTranslation", 0.2f, 0, "Scales translation speed of supported 3DMouse devices.");
    REGISTER_FLOAT("sys_Scale3DMouseYPR", 0.05f, 0, "Scales rotation speed of supported 3DMouse devices.");

    REGISTER_INT("capture_frames", 0, 0, "Enables capturing of frames. 0=off, 1=on");
    REGISTER_STRING("capture_folder", "CaptureOutput", 0, "Specifies sub folder to write captured frames.");
    REGISTER_INT("capture_frame_once", 0, 0, "Makes capture single frame only");
    REGISTER_STRING("capture_file_name", "", 0, "If set, specifies the path and name to use for the captured frame");
    REGISTER_STRING(
        "capture_file_prefix", "", 0, "If set, specifies the prefix to use for the captured frame instead of the default 'Frame'.");

    m_gpu_particle_physics =
        REGISTER_INT("gpu_particle_physics", 0, VF_REQUIRE_APP_RESTART, "Enable GPU physics if available (0=off / 1=enabled).");
    assert(m_gpu_particle_physics);

    REGISTER_COMMAND(
        "LoadConfig",
        &LoadConfigurationCmd,
        0,
        "Load .cfg file from disk (from the {Game}/Config directory)\n"
        "e.g. LoadConfig lowspec.cfg\n"
        "Usage: LoadConfig <filename>");
    assert(m_env.pConsole);
    m_env.pConsole->CreateKeyBind("alt_keyboard_key_function_F12", "Screenshot");
    m_env.pConsole->CreateKeyBind("alt_keyboard_key_function_F11", "RecordClip");

    REGISTER_CVAR2("sys_trackview", &g_cvars.sys_trackview, 1, 0, "Enables TrackView Update");

    // Defines selected language.
    REGISTER_STRING_CB("g_language", "", VF_NULL, "Defines which language pak is loaded", CSystem::OnLanguageCVarChanged);

    // adding CVAR to toggle assert verbosity level
    const int defaultAssertValue = 1;
    REGISTER_CVAR2_CB(
        "sys_asserts",
        &g_cvars.sys_asserts,
        defaultAssertValue,
        VF_CHEAT,
        "0 = Suppress Asserts\n"
        "1 = Log Asserts\n"
        "2 = Show Assert Dialog\n"
        "3 = Crashes the Application on Assert\n"
        "Note: when set to '0 = Suppress Asserts', assert expressions are still evaluated. To turn asserts into a no-op, undefine "
        "AZ_ENABLE_TRACING and recompile.",
        OnAssertLevelCvarChanged);
    CSystem::SetAssertLevel(defaultAssertValue);

    REGISTER_CVAR2(
        "sys_error_debugbreak", &g_cvars.sys_error_debugbreak, 0, VF_CHEAT, "__debugbreak() if a VALIDATOR_ERROR_DBGBREAK message is hit");

    REGISTER_STRING("dlc_directory", "", 0, "Holds the path to the directory where DLC should be installed to and read from");

#if defined(WIN32) || defined(WIN64)
    REGISTER_INT("sys_screensaver_allowed", 0, VF_NULL, "Specifies if screen saver is allowed to start up while the game is running.");
#endif
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
