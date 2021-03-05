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
#include <Launcher.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/RemoteStorageDrive.h>

#include <AzGameFramework/Application/GameApplication.h>

#include <CryLibrary.h>
#include <IConsole.h>
#include <ITimer.h>
#include <LegacyAllocator.h>
#include <ParseEngineConfig.h>

#include <Launcher_Traits_Platform.h>

#if defined(AZ_MONOLITHIC_BUILD)
extern "C" void CreateStaticModules(AZStd::vector<AZ::Module*>& modulesOut);
#endif //  defined(AZ_MONOLITHIC_BUILD)

// Add the "REMOTE_ASSET_PROCESSOR" define except in release
// this makes it so that asset processor functions.  Without this, all assets must be present and on local media
// with this, the asset processor can be used to remotely process assets.
#if !defined(_RELEASE)
#   define REMOTE_ASSET_PROCESSOR
#endif

namespace
{
#if AZ_TRAIT_LAUNCHER_USE_CRY_DYNAMIC_MODULE_HANDLE
    // mimics AZ::DynamicModuleHandle but uses CryLibrary under the hood,
    // which is necessary to properly load legacy Cry libraries on some platforms
    class DynamicModuleHandle
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicModuleHandle, AZ::OSAllocator, 0)

        static AZStd::unique_ptr<DynamicModuleHandle> Create(const char* fullFileName)
        {
            return AZStd::unique_ptr<DynamicModuleHandle>(aznew DynamicModuleHandle(fullFileName));
        }

        DynamicModuleHandle(const DynamicModuleHandle&) = delete;
        DynamicModuleHandle& operator=(const DynamicModuleHandle&) = delete;

        ~DynamicModuleHandle()
        {
            Unload();
        }

        // argument is strictly to match the API of AZ::DynamicModuleHandle
        bool Load(bool unused)
        { 
            AZ_UNUSED(unused);

            if (IsLoaded())
            {
                return true;
            }

            m_moduleHandle = CryLoadLibrary(m_fileName.c_str());
            return IsLoaded();
        }

        bool Unload()
        {
            if (!IsLoaded())
            {
                return false;
            }

            return CryFreeLibrary(m_moduleHandle);
        }

        bool IsLoaded() const 
        { 
            return m_moduleHandle != nullptr; 
        }

        const AZ::OSString& GetFilename() const 
        { 
            return m_fileName; 
        }

        template<typename Function>
        Function GetFunction(const char* functionName) const
        {
            if (IsLoaded())
            {
                return reinterpret_cast<Function>(CryGetProcAddress(m_moduleHandle, functionName));
            }
            else
            {
                return nullptr;
            }
        }


    private:
        DynamicModuleHandle(const char* fileFullName)
            : m_fileName()
            , m_moduleHandle(nullptr)
        {
            m_fileName = AZ::OSString::format("%s%s%s", 
                CrySharedLibraryPrefix, fileFullName, CrySharedLibraryExtension);
        }

        AZ::OSString m_fileName;
        HMODULE m_moduleHandle;
    };
#else
    // mimics AZ::DynamicModuleHandle but also calls InjectEnvironmentFunction on
    // the loaded module which is necessary to properly load legacy Cry libraries
    class DynamicModuleHandle
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicModuleHandle, AZ::OSAllocator, 0);

        static AZStd::unique_ptr<DynamicModuleHandle> Create(const char* fullFileName)
        {
            return AZStd::unique_ptr<DynamicModuleHandle>(aznew DynamicModuleHandle(fullFileName));
        }

        bool Load(bool isInitializeFunctionRequired)
        {
            const bool loaded = m_moduleHandle->Load(isInitializeFunctionRequired);
            if (loaded)
            {
                // We need to inject the environment first thing so that allocators are available immediately
                InjectEnvironmentFunction injectEnv = GetFunction<InjectEnvironmentFunction>(INJECT_ENVIRONMENT_FUNCTION);
                if (injectEnv)
                {
                    auto env = AZ::Environment::GetInstance();
                    injectEnv(env);
                }
            }
            return loaded;
        }

        bool Unload()
        {
            bool unloaded = m_moduleHandle->Unload();
            if (unloaded)
            {
                DetachEnvironmentFunction detachEnv = GetFunction<DetachEnvironmentFunction>(DETACH_ENVIRONMENT_FUNCTION);
                if (detachEnv)
                {
                    detachEnv();
                }
            }
            return unloaded;
        }

        template<typename Function>
        Function GetFunction(const char* functionName) const
        {
            return m_moduleHandle->GetFunction<Function>(functionName);
        }

    private:
        DynamicModuleHandle(const char* fileFullName)
            : m_moduleHandle(AZ::DynamicModuleHandle::Create(fileFullName))
        {
        }

        AZStd::unique_ptr<AZ::DynamicModuleHandle> m_moduleHandle;
    };
#endif // AZ_TRAIT_LAUNCHER_USE_CRY_DYNAMIC_MODULE_HANDLE

    void RunMainLoop(AzGameFramework::GameApplication& gameApplication)
    {
        // Ideally we'd just call GameApplication::RunMainLoop instead, but
        // we'd have to stop calling ISystem::UpdatePreTickBus / PostTickBus
        // directly, and instead have something subscribe to the TickBus in
        // order to call them, using order ComponentTickBus::TICK_FIRST - 1
        // and ComponentTickBus::TICK_LAST + 1 to ensure they get called at
        // the same time as they do now. Also, we'd need to pass a function
        // pointer to AzGameFramework::GameApplication::MainLoop that would
        // be used to call ITimer::GetFrameTime (unless we could also shift
        // our frame time to be managed by AzGameFramework::GameApplication
        // instead, which probably isn't going to happen anytime soon given
        // how many things depend on the ITimer interface).
        bool continueRunning = true;
        ISystem* system = gEnv ? gEnv->pSystem : nullptr;
        while (continueRunning)
        {
            // Pump the system event loop
            gameApplication.PumpSystemEventLoopUntilEmpty();

            // Update the AzFramework system tick bus
            gameApplication.TickSystem();

            // Pre-update CrySystem
            if (system)
            {
                system->UpdatePreTickBus();
            }

            // Update the AzFramework application tick bus
            gameApplication.Tick(gEnv->pTimer->GetFrameTime());

            // Post-update CrySystem
            if (system)
            {
                system->UpdatePostTickBus();
            }

            // Check for quit requests
            continueRunning = !gameApplication.WasExitMainLoopRequested() && continueRunning;
        }
    }
}


namespace LumberyardLauncher
{
    AZ_CVAR(bool, bg_ConnectToAssetProcessor, true, nullptr, AZ::ConsoleFunctorFlags::DontReplicate, "If true, the process will launch and connect to the asset processor");

    bool PlatformMainInfo::CopyCommandLine(int argc, char** argv)
    {
        for (int argIndex = 0; argIndex < argc; ++argIndex)
        {
            if (!AddArgument(argv[argIndex]))
            {
                return false;
            }
        }
        return true;
    }

    bool PlatformMainInfo::AddArgument(const char* arg)
    {
        AZ_Error("Launcher", arg, "Attempting to add a nullptr command line argument!");

        bool needsQuote = (strstr(arg, " ") != nullptr);
        bool needsSpace = (m_commandLine[0] != 0);

        // strip the previous null-term from the count to prevent double counting
        m_commandLineLen = (m_commandLineLen == 0) ? 0 : (m_commandLineLen - 1);

        // compute the expected length with the added argument
        size_t argLen = strlen(arg);
        size_t pendingLen = m_commandLineLen + argLen + 1 + (needsSpace ? 1 : 0) + (needsQuote ? 2 : 0); // +1 null-term, [+1 space], [+2 quotes]

        if (pendingLen >= AZ_COMMAND_LINE_LEN)
        {
            AZ_Assert(false, "Command line exceeds the %d character limit!", AZ_COMMAND_LINE_LEN);
            return false;
        }

        if (needsSpace)
        {
            m_commandLine[m_commandLineLen++] = ' ';
        }

        azsnprintf(m_commandLine + m_commandLineLen,
            AZ_COMMAND_LINE_LEN - m_commandLineLen,
            needsQuote ? "\"%s\"" : "%s",
            arg);

        // Inject the argument in the argument buffer to preserve/replicate argC and argV
        azstrncpy(&m_commandLineArgBuffer[m_nextCommandLineArgInsertPoint],
            AZ_COMMAND_LINE_LEN - m_nextCommandLineArgInsertPoint,
            arg,
            argLen+1);
        m_argV[m_argC++] = &m_commandLineArgBuffer[m_nextCommandLineArgInsertPoint];
        m_nextCommandLineArgInsertPoint += argLen + 1;

        m_commandLineLen = pendingLen;
        return true;
    }


    const char* GetReturnCodeString(ReturnCode code)
    {
        switch (code)
        {
            case ReturnCode::Success:
                return "Success";

            case ReturnCode::ErrBootstrapMismatch:
                return "Mismatch detected between Launcher compiler defines and bootstrap values (LY_GAMEFOLDER/sys_game_folder).";

            case ReturnCode::ErrCommandLine:
                return "Failed to copy command line arguments";

            case ReturnCode::ErrResourceLimit:
                return "A resource limit failed to update";

            case ReturnCode::ErrAppDescriptor:
                return "Application descriptor file was not found";

            case ReturnCode::ErrCrySystemLib:
                return "Failed to load the CrySystem library";

            case ReturnCode::ErrCrySystemInterface:
                return "Failed to initialize the CrySystem Interface";

            case ReturnCode::ErrCryEnvironment:
                return "Failed to initialize the CryEngine global environment";

            case ReturnCode::ErrAssetProccessor:
                return "Failed to connect to AssetProcessor while the /Amazon/AzCore/Bootstrap/wait_for_connect value is 1\n."
                    "wait_for_connect can be set to 0 within the bootstrap to allow connecting to the AssetProcessor"
                    " to not be an error if unsuccessful.";
            default:
                return "Unknown error code";
        }
    }

    void CopySettingsRegistryToCrySystemInitParams(const AZ::SettingsRegistryInterface& registry, SSystemInitParams& params)
    {
        constexpr AZStd::string_view DefaultRemoteIp = "127.0.0.1";
        constexpr uint16_t DefaultRemotePort = 45643U;

        AZ::SettingsRegistryInterface::FixedValueString settingsKeyPrefix = AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey;
        AZ::SettingsRegistryInterface::FixedValueString settingsValueString;
        AZ::s64 settingsValueInt{};

        // remote filesystem
        if (registry.Get(settingsValueInt, settingsKeyPrefix + "/remote_filesystem"))
        {
            params.remoteFileIO = settingsValueInt != 0;
        }
        // remote port
        if(registry.Get(settingsValueInt, settingsKeyPrefix + "/remote_port"))
        {
            params.remotePort = aznumeric_cast<int>(settingsValueInt);
        }
        else
        {
            params.remotePort = DefaultRemotePort;
        }

        // remote ip
        if (registry.Get(settingsValueString, settingsKeyPrefix + "/remote_ip"))
        {
            azstrncpy(AZStd::data(params.remoteIP), AZStd::size(params.remoteIP), settingsValueString.c_str(), settingsValueString.size());
        }
        else
        {
            azstrncpy(AZStd::data(params.remoteIP), AZStd::size(params.remoteIP), DefaultRemoteIp.data(), DefaultRemoteIp.size());
        }

        // connect_to_remote - also supports <platform>_connect_to_remote override
        if (registry.Get(settingsValueInt, settingsKeyPrefix + "/" AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER "_connect_to_remote")
            || registry.Get(settingsValueInt, settingsKeyPrefix + "/connect_to_remote"))
        {
            params.connectToRemote = settingsValueInt != 0;
        }
#if !defined(DEDICATED_SERVER)
        // wait_for_connect - also supports <platform>_wait_for_connect override
        // Dedicated server does not depend on Asset Processor and assumes that assets are already prepared.
        if (registry.Get(settingsValueInt, settingsKeyPrefix + "/" AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER "_wait_for_connect")
            || registry.Get(settingsValueInt, settingsKeyPrefix + "/wait_for_connect"))
        {
            params.waitForConnection = settingsValueInt != 0;
        }
#endif // defined(DEDICATED_SERVER)

        // assets - also supports <platform>_assets override
        settingsValueString.clear();
        if (registry.Get(settingsValueString, settingsKeyPrefix + "/" AZ_TRAIT_OS_PLATFORM_CODENAME_LOWER "_assets")
            || registry.Get(settingsValueString, settingsKeyPrefix + "/assets"))
        {
            azstrncpy(AZStd::data(params.assetsPlatform), AZStd::size(params.assetsPlatform), settingsValueString.c_str(), settingsValueString.size());
        }

        // Project name - First tries sys_game_folder
        settingsValueString.clear();
        if (registry.Get(settingsValueString, settingsKeyPrefix + "/sys_game_folder"))
        {
            // sys_game_folder is the current way to do it
            azstrncpy(AZStd::data(params.gameFolderName), AZStd::size(params.gameFolderName), settingsValueString.c_str(), settingsValueString.size());
        }

        // assetProcessor_branch_token
        if (registry.Get(settingsValueInt, settingsKeyPrefix + "/assetProcessor_branch_token"))
        {
            azsnprintf(AZStd::data(params.branchToken), AZStd::size(params.branchToken), "0x%llx", settingsValueInt);
        }

        // Engine root path(also AppRoot path as well)
        settingsValueString.clear();
        if (registry.Get(settingsValueString, AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
        {
            azstrncpy(AZStd::data(params.rootPath), AZStd::size(params.rootPath), settingsValueString.c_str(), settingsValueString.size());
        }
        // Asset Cache Root path
        settingsValueString.clear();
        if (registry.Get(settingsValueString, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
        {
            azstrncpy(AZStd::data(params.rootPathCache), AZStd::size(params.rootPathCache), settingsValueString.c_str(), settingsValueString.size());
        }
        // Asset Cache Game path (Includes as part of path, the current project name)
        settingsValueString.clear();
        if (registry.Get(settingsValueString, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheGameFolder))
        {
            azstrncpy(AZStd::data(params.assetsPathCache), AZStd::size(params.assetsPathCache), settingsValueString.c_str(), settingsValueString.size());
            azstrncpy(AZStd::data(params.assetsPath), AZStd::size(params.assetsPath), settingsValueString.c_str(), settingsValueString.size());
        }
    }

    void CompileCriticalAssets();
    void CreateRemoteFileIO();

    bool ConnectToAssetProcessor()
    {
        bool connectedToAssetProcessor{};
        // When the AssetProcessor is already launched it should take less than a second to perform a connection
        // but when the AssetProcessor needs to be launch it could take up to 15 seconds to have the AssetProcessor initialize
        // and able to negotiate a connection when running a debug build
        // and to negotiate a connection
        // Setting the connectTimeout to 3 seconds if not set within the settings registry

        AzFramework::AssetSystem::ConnectionSettings connectionSettings;
        AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);

        connectionSettings.m_launchAssetProcessorOnFailedConnection = true;
        connectionSettings.m_connectionIdentifier = AzFramework::AssetSystem::ConnectionIdentifiers::Game;
        connectionSettings.m_loggingCallback = []([[maybe_unused]] AZStd::string_view logData)
        {
            AZ_TracePrintf("Launcher", "%.*s", aznumeric_cast<int>(logData.size()), logData.data());
        };

        AzFramework::AssetSystemRequestBus::BroadcastResult(connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);

        if (connectedToAssetProcessor)
        {
            AZ_TracePrintf("Launcher", "Connected to Asset Processor\n");
            CreateRemoteFileIO();
            CompileCriticalAssets();
        }

        return connectedToAssetProcessor;
    }

    //! Compiles the critical assets that are within the Engine directory of Lumberyard
    //! This code should be in a centralized location, but doesn't belong in AzFramework
    //! since it is specific to how Lumberyard projects has assets setup
    void CompileCriticalAssets()
    {
        // VERY early on, as soon as we can, request that the asset system make sure the following assets take priority over others,
        // so that by the time we ask for them there is a greater likelihood that they're already good to go.
        // these can be loaded later but are still important:
        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, "/texturemsg/");
        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, "engineassets/materials");
        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, "engineassets/geomcaches");
        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetBySearchTerm, "engineassets/objects");

        // some are specifically extra important and will cause issues if missing completely:
        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::CompileAssetSync, "engineassets/objects/default.cgf");
    }

    //! Remote FileIO to use as a Virtual File System
    //! Communication of FileIOBase operations occur through an AssetProcessor connection
    void CreateRemoteFileIO()
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::s64 allowRemoteFilesystem{};

        AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, allowRemoteFilesystem,
            AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "remote_filesystem");
        if (allowRemoteFilesystem != 0)
        {

            // The SetInstance calls below will assert if this has already been set and we don't clear first
            // Application::StartCommon will set a LocalFileIO base first.
            // This provides an opportunity for the RemoteFileIO to override the direct instance
            auto remoteFileIo = new AZ::IO::RemoteFileIO(AZ::IO::FileIOBase::GetDirectInstance()); // Wrap AZ:I::LocalFileIO the direct instance
            AZ::IO::FileIOBase::SetDirectInstance(nullptr);
            // Wrap AZ:IO::LocalFileIO the direct instance
            AZ::IO::FileIOBase::SetDirectInstance(remoteFileIo);
        }
    }

    //! Add the GameProjectName and Launcher build target name into the settings registry
    void AddGameProjectNameToSettingsRegistry(AZ::SettingsRegistryInterface& settingsRegistry, AZ::CommandLine& commandLine)
    {
        auto gameProjectNameKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/sys_game_folder", AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey);
        AZ::SettingsRegistryInterface::FixedValueString bootstrapGameProjectName;
        settingsRegistry.Get(bootstrapGameProjectName, gameProjectNameKey);

        const AZStd::string_view gameProjectName = GetGameProjectName();
        AZ::SettingsRegistryInterface::FixedValueString gameProjectCommandLineOverride = R"(--regset=)";
        gameProjectCommandLineOverride += gameProjectNameKey;
        gameProjectCommandLineOverride += '=';
        gameProjectCommandLineOverride += gameProjectName;

        // Inject the Project Name into the CommandLine parameters, so that the Setting Registry
        // always is set to the launcher's project name whenever the command line is merged into the Settings Registry
        // This happens several times through application such as in GameApplication::Start
        AZ::CommandLine::ParamContainer commandLineArgs;
        commandLine.Dump(commandLineArgs);
        commandLineArgs.emplace_back(gameProjectCommandLineOverride);
        commandLine.Parse(commandLineArgs);

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_CommandLine(settingsRegistry, commandLine, false);
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(settingsRegistry);

        const AZStd::string_view buildTargetName = LumberyardLauncher::GetBuildTargetName();
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(settingsRegistry, buildTargetName);

        // Output a trace message if the sys_game_folder value read from the boostrap.cfg file doesn't match the value of the
        // LY_GAMEFOLDER define built into the Launcher.
        // This isn't any kind of error or ever warning, but is used as an informational message to the user
        // that the launcher will use the always used the injected LY_GAMEFOLDER define
        if (bootstrapGameProjectName != gameProjectName)
        {
            AZ_TracePrintf("Launcher", R"(The game project "%s" read into the Settings Registry from the bootstrap.cfg file)"
                R"( does not match the LY_GAMEFOLDER define "%.*s")" "\n",
                bootstrapGameProjectName.c_str(), aznumeric_cast<int>(gameProjectName.size()), gameProjectName.data());
        }

        AZ_TracePrintf("Launcher", R"(The game project name of "%.*s" is the value of the LY_GAMEFOLDER define.)" "\n"
            R"(That value has been successfully set into the Settings Registry at key "%s/sys_game_folder" for Launcher target "%.*s")" "\n",
            aznumeric_cast<int>(gameProjectName.size()), gameProjectName.data(),
            AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey,
            aznumeric_cast<int>(buildTargetName.size()), buildTargetName.data());
    }

    ReturnCode Run(const PlatformMainInfo& mainInfo)
    {
        if (mainInfo.m_updateResourceLimits 
            && !mainInfo.m_updateResourceLimits())
        {
            return ReturnCode::ErrResourceLimit;
        }

        // Game Application (AzGameFramework)
        int     gameArgC = mainInfo.m_argC;
        char**  gameArgV = const_cast<char**>(mainInfo.m_argV);
        int*    argCParam = (gameArgC > 0) ? &gameArgC : nullptr;
        char*** argVParam = (gameArgC > 0) ? &gameArgV : nullptr;

        AzGameFramework::GameApplication gameApplication(argCParam, argVParam);
        // The settings registry has been created by the AZ::ComponentApplication constructor at this point
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            // Settings registry must be available at this point in order to continue
            return ReturnCode::ErrValidation;
        }

        // Inject the ${LY_GAMEFOLDER} project name define that from the Launcher build target
        // into the settings registry
        AddGameProjectNameToSettingsRegistry(*settingsRegistry, *gameApplication.GetAzCommandLine()); 

        bool applyAppRootOverride = (AZ_TRAIT_LAUNCHER_SET_APPROOT_OVERRIDE == 1);
        AZ::SettingsRegistryInterface::FixedValueString pathToAssets;
        if (!settingsRegistry->Get(pathToAssets, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
        {
            // Default to mainInfo.m_appResource if the cache root folder is missing from the Settings Registry
            pathToAssets = mainInfo.m_appResourcesPath;
            AZ_Error("Launcher", false, "Unable to retrieve asset cache root folder from the settings registry at json pointer path %s",
                AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);

        }
        else
        {
            AZ_TracePrintf("Launcher", "The asset cache folder of %s has been successfully read from the Settings Registry\n",
                pathToAssets.c_str());
        }

        CryAllocatorsRAII cryAllocatorsRAII;
    #if AZ_TRAIT_LAUNCHER_ALLOW_CMDLINE_APPROOT_OVERRIDE
        char appRootOverride[AZ_MAX_PATH_LEN] = { 0 };
        {
            // Search for the app root argument (--app-root <PATH>) where <PATH> is the app root path to set for the application
            const static char* appRootArgPrefix = "--app-root";
            size_t appRootArgPrefixLen = strlen(appRootArgPrefix);

            const char* appRootArg = nullptr;

            char cmdLineCopy[AZ_COMMAND_LINE_LEN] = { 0 };
            azstrncpy(cmdLineCopy, AZ_COMMAND_LINE_LEN, mainInfo.m_commandLine, mainInfo.m_commandLineLen);

            const char* delimiters = " ";
            char* nextToken = nullptr;
            char* token = azstrtok(cmdLineCopy, 0, delimiters, &nextToken);
            while (token != NULL)
            {
                if (azstrnicmp(appRootArgPrefix, token, appRootArgPrefixLen) == 0)
                {
                    appRootArg = azstrtok(nullptr, 0, delimiters, &nextToken);
                    break;
                }
                token = azstrtok(nullptr, 0, delimiters, &nextToken);
            }

            if (appRootArg)
            {
                AZStd::string_view appRootArgView = appRootArg;
                size_t afterStartQuotes = appRootArgView.find_first_not_of(R"(")");
                if (afterStartQuotes != AZStd::string_view::npos)
                {
                    appRootArgView.remove_prefix(afterStartQuotes);
                }
                size_t beforeEndQuotes = appRootArgView.find_last_not_of(R"(")");
                if (beforeEndQuotes != AZStd::string_view::npos)
                {
                    appRootArgView.remove_suffix(appRootArgView.size() - (beforeEndQuotes + 1));
                }
                appRootArgView.copy(appRootOverride, AZ_MAX_PATH_LEN);
                appRootOverride[appRootArgView.size()] = '\0';
                pathToAssets = appRootOverride;
                applyAppRootOverride = true;
            }
        }
    #endif // AZ_TRAIT_LAUNCHER_ALLOW_CMDLINE_APPROOT_OVERRIDE

        // System Init Params ("Legacy" Lumberyard)
        SSystemInitParams systemInitParams;
        memset(&systemInitParams, 0, sizeof(SSystemInitParams));

        {
            AzGameFramework::GameApplication::StartupParameters gameApplicationStartupParams;

            if (applyAppRootOverride)
            {
                // NOTE: setting this on android doesn't work when assets are packaged in the APK
                gameApplicationStartupParams.m_appRootOverride = pathToAssets.c_str();
            }

            if (mainInfo.m_allocator)
            {
                gameApplicationStartupParams.m_allocator = mainInfo.m_allocator;
            }
            else if (AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
            {
                gameApplicationStartupParams.m_allocator = &AZ::AllocatorInstance<AZ::OSAllocator>::Get();
            }

        #if defined(AZ_MONOLITHIC_BUILD)
            gameApplicationStartupParams.m_createStaticModulesCallback = CreateStaticModules;
            gameApplicationStartupParams.m_loadDynamicModules = false;
        #endif // defined(AZ_MONOLITHIC_BUILD)

            CopySettingsRegistryToCrySystemInitParams(*settingsRegistry, systemInitParams);

            gameApplication.Start({}, gameApplicationStartupParams);

#if defined(REMOTE_ASSET_PROCESSOR)
            bool allowedEngineConnection = !systemInitParams.bToolMode && !systemInitParams.bTestMode && bg_ConnectToAssetProcessor;

            //connect to the asset processor using the bootstrap values
            bool connectedToAssetProcessor = false;
            if (allowedEngineConnection)
            {
                if (!ConnectToAssetProcessor())
                {
                    AZ::s64 waitForConnect{};
                    AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, waitForConnect,
                        AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "wait_for_connect");
                    if (waitForConnect != 0)
                    {
                        AZ_Error("Launcher", false, "Failed to connect to AssetProcessor.");
                        return ReturnCode::ErrAssetProccessor;
                    }
                }
            }
#endif
            AZ_Assert(AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady(), "System allocator was not created or creation failed.");
            //Initialize the Debug trace instance to create necessary environment variables
            AZ::Debug::Trace::Instance().Init();
        }

        if (mainInfo.m_onPostAppStart)
        {
            mainInfo.m_onPostAppStart();
        }

        azstrncpy(systemInitParams.szSystemCmdLine, sizeof(systemInitParams.szSystemCmdLine), 
            mainInfo.m_commandLine, mainInfo.m_commandLineLen);

        systemInitParams.pSharedEnvironment = AZ::Environment::GetInstance();
        systemInitParams.sLogFileName = GetLogFilename();
        systemInitParams.hInstance = mainInfo.m_instance;
        systemInitParams.hWnd = mainInfo.m_window;
        systemInitParams.pPrintSync = mainInfo.m_printSink;

        if (strstr(mainInfo.m_commandLine, "-norandom"))
        {
            systemInitParams.bNoRandom = true;
        }

        systemInitParams.bDedicatedServer = IsDedicatedServer();

        if (systemInitParams.remoteFileIO)
        {
            AZ_TracePrintf("Launcher", "Application is configured for VFS");
            AZ_TracePrintf("Launcher", "Log and cache files will be written to the Cache directory on your host PC");

            const char* message = "If your game does not run, check any of the following:\n"
                                  "\t- Verify the remote_ip address is correct in bootstrap.cfg";

            if (mainInfo.m_additionalVfsResolution)
            {
                AZ_TracePrintf("Launcher", "%s\n%s", message, mainInfo.m_additionalVfsResolution)
            }
            else 
            {
                AZ_TracePrintf("Launcher", "%s", message)
            }
        }
        else
        {
            AZ_TracePrintf("Launcher", "Application is configured to use device local files at %s\n", systemInitParams.rootPath);
            AZ_TracePrintf("Launcher", "Log and cache files will be written to device storage\n");

            const char* writeStorage = mainInfo.m_appWriteStoragePath;
            if (writeStorage)
            {
                AZ_TracePrintf("Launcher", "User Storage will be set to %s/user\n", writeStorage);
                azsnprintf(systemInitParams.userPath, AZ_MAX_PATH_LEN, "%s/user", writeStorage);
            }
        }

        // Create CrySystem.
    #if !defined(AZ_MONOLITHIC_BUILD)
        AZStd::unique_ptr<DynamicModuleHandle> crySystemLibrary;
        PFNCREATESYSTEMINTERFACE CreateSystemInterface = nullptr;

        crySystemLibrary = DynamicModuleHandle::Create("CrySystem");
        if (crySystemLibrary->Load(false))
        {
            CreateSystemInterface = crySystemLibrary->GetFunction<PFNCREATESYSTEMINTERFACE>("CreateSystemInterface");
            if (CreateSystemInterface)
            {
                systemInitParams.pSystem = CreateSystemInterface(systemInitParams);
            }
        }
    #else
        systemInitParams.pSystem = CreateSystemInterface(systemInitParams);
    #endif // !defined(AZ_MONOLITHIC_BUILD)

        ReturnCode status = ReturnCode::Success;

        if (systemInitParams.pSystem)
        {
            // Process queued events before main loop.
            AZ::TickBus::ExecuteQueuedEvents();

#if !defined(SYS_ENV_AS_STRUCT)
            gEnv = systemInitParams.pSystem->GetGlobalEnvironment();
        #endif // !defined(SYS_ENV_AS_STRUCT)

            if (gEnv && gEnv->pConsole)
            {
                // Execute autoexec.cfg to load the initial level
                gEnv->pConsole->ExecuteString("exec autoexec.cfg");

                gEnv->pSystem->ExecuteCommandLine(false);

                // Run the main loop
                RunMainLoop(gameApplication);
            }
            else
            {
                status = ReturnCode::ErrCryEnvironment;
            }
        }
        else
        {
            status = ReturnCode::ErrCrySystemInterface;
        }

    #if !defined(AZ_MONOLITHIC_BUILD)
        crySystemLibrary.reset(nullptr);
    #endif // !defined(AZ_MONOLITHIC_BUILD)

        gameApplication.Stop();
        AZ::Debug::Trace::Instance().Destroy();

        return status;
    }
}
