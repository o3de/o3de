/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Launcher.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Component/ComponentApplicationLifecycle.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/BudgetTracker.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/IO/RemoteStorageDrive.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/Windowing/WindowBus.h>

#include <AzGameFramework/Application/GameApplication.h>

#include <ISystem.h>

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

void CVar_OnViewportPosition(const AZ::Vector2& value);

namespace
{
    void CVar_OnViewportResize(const AZ::Vector2& value);

    AZ_CVAR(AZ::Vector2, r_viewportSize, AZ::Vector2::CreateZero(), CVar_OnViewportResize, AZ::ConsoleFunctorFlags::DontReplicate,
        "The default size for the launcher viewport, 0 0 means full screen");

    void CVar_OnViewportResize(const AZ::Vector2& value)
    {
        AzFramework::NativeWindowHandle windowHandle = nullptr;
        AzFramework::WindowSystemRequestBus::BroadcastResult(windowHandle, &AzFramework::WindowSystemRequestBus::Events::GetDefaultWindowHandle);
        AzFramework::WindowSize newSize = AzFramework::WindowSize(aznumeric_cast<int32_t>(value.GetX()), aznumeric_cast<int32_t>(value.GetY()));
        AzFramework::WindowRequestBus::Broadcast(&AzFramework::WindowRequestBus::Events::ResizeClientArea, newSize, AzFramework::WindowPosOptions());
    }

    AZ_CVAR(AZ::Vector2, r_viewportPos, AZ::Vector2::CreateZero(), CVar_OnViewportPosition, AZ::ConsoleFunctorFlags::DontReplicate,
        "The default position for the launcher viewport, 0 0 means top left corner of your main desktop");

    void ExecuteConsoleCommandFile(AzFramework::Application& application)
    {
        const AZStd::string_view customConCmdKey = "console-command-file";
        const AZ::CommandLine* commandLine = application.GetCommandLine();
        AZStd::size_t numSwitchValues = commandLine->GetNumSwitchValues(customConCmdKey);
        if (numSwitchValues > 0)
        {
            // The expectations for command line parameters is that the "last one wins"
            // That way it allows users and test scripts to override previous command line options by just listing them later on the invocation line
            const AZStd::string& consoleCmd = commandLine->GetSwitchValue(customConCmdKey, numSwitchValues - 1);
            if (!consoleCmd.empty())
            {
                AZ::Interface<AZ::IConsole>::Get()->ExecuteConfigFile(consoleCmd.c_str());
            }
        }
    }

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
        ISystem* system = gEnv ? gEnv->pSystem : nullptr;
        while (!gameApplication.WasExitMainLoopRequested())
        {
            // Pump the system event loop
            gameApplication.PumpSystemEventLoopUntilEmpty();

            if (gameApplication.WasExitMainLoopRequested())
            {
                break;
            }

            // Update the AzFramework system tick bus
            gameApplication.TickSystem();

            // Pre-update CrySystem
            if (system)
            {
                system->UpdatePreTickBus();
            }

            // Update the AzFramework application tick bus
            gameApplication.Tick();

            // Post-update CrySystem
            if (system)
            {
                system->UpdatePostTickBus();
            }
        }
    }
}


namespace O3DELauncher
{
    inline constexpr AZStd::string_view LauncherTypeTag = "/O3DE/Runtime/LauncherType";
    inline constexpr AZStd::string_view LauncherFilenameTag = "launcher";

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

        if (needsQuote) // Branching instead of using a ternary on the format string to avoid warning 4774 (format literal expected)
        {
            azsnprintf(m_commandLine + m_commandLineLen,
                AZ_COMMAND_LINE_LEN - m_commandLineLen,
                "\"%s\"",
                arg);
        }
        else
        {
            azsnprintf(m_commandLine + m_commandLineLen,
                AZ_COMMAND_LINE_LEN - m_commandLineLen,
                "%s",
                arg);
        }

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
                return "Failed to initialize the global environment";

            case ReturnCode::ErrAssetProccessor:
                return "Failed to connect to AssetProcessor while the /Amazon/AzCore/Bootstrap/wait_for_connect value is 1\n."
                    "wait_for_connect can be set to 0 within the bootstrap to allow connecting to the AssetProcessor"
                    " to not be an error if unsuccessful.";
            default:
                return "Unknown error code";
        }
    }

    void CreateRemoteFileIO();

    // This function make sure the launcher has signaled the "CriticalAssetsCompiled"
    // lifecycle event as well as to load the "assetcatalog.xml" file if it exists
    void CompileCriticalAssets()
    {
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            // Reload the assetcatalog.xml at this point again
            // Start Monitoring Asset changes over the network and load the AssetCatalog.
            // Note: When using VFS this is the first time catalog will be loaded using remote's catalog file.
            auto LoadCatalog = [settingsRegistry](AZ::Data::AssetCatalogRequests* assetCatalogRequests)
            {
                if (AZ::IO::FixedMaxPath assetCatalogPath;
                    settingsRegistry->Get(assetCatalogPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder))
                {
                    assetCatalogPath /= "assetcatalog.xml";
                    assetCatalogRequests->LoadCatalog(assetCatalogPath.c_str());
                }
            };
            AZ::Data::AssetCatalogRequestBus::Broadcast(AZStd::move(LoadCatalog));

            AZ_TracePrintf("Launcher", "CriticalAssetsCompiled\n");

            // Broadcast that critical assets are ready
            AZ::ComponentApplicationLifecycle::SignalEvent(*settingsRegistry, "CriticalAssetsCompiled", R"({})");
        }
    }

    // If the connect option is false, this function will return true
    // to make sure the Launcher passes the connected to AP check
    // If REMOTE_ASSET_PROCESSOR is not defined, then the launcher doesn't need
    // to connect to the AssetProcessor and therefore this function returns true
    bool ConnectToAssetProcessor([[maybe_unused]] bool connect)
    {
        bool connectedToAssetProcessor = true;
#if defined(REMOTE_ASSET_PROCESSOR)
        if (connect)
        {
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
            }
        }

#endif
        CompileCriticalAssets();
        return connectedToAssetProcessor;
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
            using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

            // Application::StartCommon will set a LocalFileIO base first.
            // This provides an opportunity for the RemoteFileIO to override the direct instance
            auto remoteFileIo = new AZ::IO::RemoteFileIO(AZ::IO::FileIOBase::GetDirectInstance()); // Wrap LocalFileIO the direct instance

            // SetDirectInstance will assert if this has already been set and we don't clear first
            AZ::IO::FileIOBase::SetDirectInstance(nullptr);
            // Wrap AZ:IO::LocalFileIO the direct instance
            AZ::IO::FileIOBase::SetDirectInstance(remoteFileIo);

            // Set file paths to uses aliases, they will be resolved by the remote file system.
            // Prefixing alias with / so they are treated as absolute paths by Path class,
            // otherwise odd concatenations of aliases happen leading to invalid paths when
            // resolved by the remote system.
            settingsRegistry->Set(FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/engine_path", "/@engroot@");
            settingsRegistry->Set(FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path", "/@projectroot@");
            settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder, "/@engroot@");
            settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath, "/@projectroot@");
            settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder, "/@products@");
            settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath, "/@user@");
            settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectLogPath, "/@log@");
            settingsRegistry->Set(AZ::SettingsRegistryMergeUtils::FilePathKey_DevWriteStorage, "/@usercache@");
        }
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
        constexpr size_t MaxCommandArgsCount = 128;
        using ArgumentContainer = AZStd::fixed_vector<char*, MaxCommandArgsCount>;
        ArgumentContainer argContainer(gameArgV, gameArgV + gameArgC);

        using FixedValueString = AZ::SettingsRegistryInterface::FixedValueString;

        // Initialize the Settings Registry with the Engine Path, Project Path and Project Name settings
        // Add the initial JSON object for "/O3DE/Runtime/Manifest/Project" and "/Amazon/AzCore/Bootstrap"
        FixedValueString launcherJsonPatch = R"(
            [
                { "op": "add", "path": "/O3DE", "value": { "Runtime": { "Manifest": { "Project": {} } } } },
                { "op": "add", "path": "/Amazon", "value": { "AzCore": { "Bootstrap": {} } } })";

        // Query the project_name baked into the launcher executable
        const AZStd::string_view launcherProjectName = GetProjectName();
        if (!launcherProjectName.empty())
        {
            // Append the project name setting to the JSON Patch
            launcherJsonPatch += FixedValueString::format(R"(,
                { "op": "add", "path": "/O3DE/Runtime/Manifest/Project/project_name", "value": "%.*s" })", AZ_STRING_ARG(launcherProjectName));
        }

        // Non-host platforms cannot use the project path that is #defined within the launcher.
        // In this case the the result of AZ::Utils::GetDefaultAppRoot is used instead
#if !AZ_TRAIT_OS_IS_HOST_OS_PLATFORM
        AZStd::string_view projectPath;
        // Make sure the defaultAppRootPath variable is in scope long enough until the projectPath string_view is used below
        AZStd::optional<AZ::IO::FixedMaxPathString> defaultAppRootPath = AZ::Utils::GetDefaultAppRootPath();
        if (defaultAppRootPath.has_value())
        {
            projectPath = *defaultAppRootPath;
        }
        if (!projectPath.empty())
        {
            launcherJsonPatch += FixedValueString::format(R"(,
                { "op": "add", "path": "/Amazon/AzCore/Bootstrap/project_path", "value": "%.*s" })", AZ_STRING_ARG(projectPath));


            // For non-host platforms set the engine root to be the project root
            // Since the directories available during execution are limited on those platforms
            AZStd::string_view enginePath = projectPath;
            launcherJsonPatch += FixedValueString::format(R"(,
                { "op": "add", "path": "/Amazon/AzCore/Bootstrap/engine_path", "value": "%.*s" })", AZ_STRING_ARG(enginePath));
        }
#endif

        // Now terminate the JSON Patch array with a trailing ']'
        launcherJsonPatch += R"(
            ])";

        AZ::ComponentApplicationSettings componentAppSettings;
        componentAppSettings.m_setregBootstrapJson = launcherJsonPatch;
        // Treat the bootstrap JSON as being in JSON Patch format
        componentAppSettings.m_setregFormat = AZ::SettingsRegistryInterface::Format::JsonPatch;
        AzGameFramework::GameApplication gameApplication(aznumeric_cast<int>(argContainer.size()), argContainer.data(), AZStd::move(componentAppSettings));
        // The settings registry has been created by the AZ::ComponentApplication constructor at this point
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        if (settingsRegistry == nullptr)
        {
            // Settings registry must be available at this point in order to continue
            return ReturnCode::ErrValidation;
        }

        // Save the build target name (usually myprojectname_gamelauncher, or myprojectname_serverlauncher, etc)
        // into the specialization list, so that the regset files for xxxxx.myprojectname_gamelauncher are included in the loaded set.
        // in generic mode, this needs to be updated to a name based on the project name, so it is not a string view, here.
        AZ::SettingsRegistryInterface::FixedValueString buildTargetName(GetBuildTargetName());

        // retrieve the project name as specified by the actual project.json (or updated from command line)
        AZ::SettingsRegistryInterface::FixedValueString updatedProjectName = AZ::Utils::GetProjectName();
        if (IsGenericLauncher())
        {
            constexpr AZStd::string_view O3DEPrefix = "O3DE_";
            // this will always be the value O3DE_xxxxx where xxxxx is the type of target ("GameLauncher/ServerLauncher/UnifiedLauncher/etc")
            // and O3DE is a placeholder for the project name.  Replace the "O3DE_" part with "{ProjectName}_" (keeping the underscore).
            if (buildTargetName.starts_with(O3DEPrefix))
            {
                auto replacementName = AZ::SettingsRegistryInterface::FixedValueString::format(
                    "%.*s_", aznumeric_cast<int>(updatedProjectName.size()), updatedProjectName.data());
                buildTargetName.replace(0, O3DEPrefix.size(), replacementName);
            }
        }
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(*settingsRegistry, buildTargetName);

        //Store the launcher type to the Settings Registry
        AZStd::string_view launcherType = GetLauncherTypeSpecialization();
        settingsRegistry->Set(LauncherTypeTag, launcherType);
        // Also add the launcher type as a specialization as well
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddSpecialization(*settingsRegistry, launcherType);
#if O3DE_HEADLESS_SERVER
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddSpecialization(*settingsRegistry, "headless");
        gameApplication.SetHeadless(true);
#else
        gameApplication.SetHeadless(false);
#endif // O3DE_HEADLESS_SERVER

#if AZ_TRAIT_CONSOLE_MODE_SUPPORT
        gameApplication.SetConsoleModeSupported(true);
#else
        gameApplication.SetConsoleModeSupported(false);
#endif // AZ_TRAIT_CONSOLE_MODE_SUPPORT

        // Finally add the "launcher" specialization tag into the Settings Registry
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddSpecialization(*settingsRegistry, LauncherFilenameTag);

        AZ_TracePrintf("Launcher", R"(Running project "%.*s")" "\n"
            R"(The project name has been successfully set in the Settings Registry at key "%s/project_name")"
            R"( for Launcher target "%.*s")" "\n",
            aznumeric_cast<int>(updatedProjectName.size()), updatedProjectName.data(),
            AZ::SettingsRegistryMergeUtils::ProjectSettingsRootKey,
            aznumeric_cast<int>(buildTargetName.size()), buildTargetName.data());

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

        // System Init Params ("Legacy" Open 3D Engine)
        SSystemInitParams systemInitParams;
        memset(&systemInitParams, 0, sizeof(SSystemInitParams));

        {
            AzGameFramework::GameApplication::StartupParameters gameApplicationStartupParams;

        #if defined(AZ_MONOLITHIC_BUILD)
            gameApplicationStartupParams.m_createStaticModulesCallback = CreateStaticModules;
            gameApplicationStartupParams.m_loadDynamicModules = false;
        #endif // defined(AZ_MONOLITHIC_BUILD)

            const char* isDedicatedServerCommand = IsDedicatedServer() ? "sv_isDedicated true" : "sv_isDedicated false";
            AZ::Interface<AZ::IConsole>::Get()->PerformCommand(isDedicatedServerCommand);

            gameApplication.Start({}, gameApplicationStartupParams);


            //connect to the asset processor using the bootstrap values
            const bool allowedEngineConnection = !systemInitParams.bToolMode && !systemInitParams.bTestMode && bg_ConnectToAssetProcessor;
            if (!ConnectToAssetProcessor(allowedEngineConnection))
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

            //Initialize the Debug trace instance to create necessary environment variables
            AZ::Debug::Trace::Instance().Init();

            if (!IsDedicatedServer() && !systemInitParams.bToolMode && !systemInitParams.bTestMode)
            {
                if (auto nativeUI = AZ::Interface<AZ::NativeUI::NativeUIRequests>::Get(); nativeUI != nullptr)
                {
                    nativeUI->SetMode(AZ::NativeUI::Mode::ENABLED);
                }
            }
        }

        if (mainInfo.m_onPostAppStart)
        {
            mainInfo.m_onPostAppStart();
        }

        azstrncpy(systemInitParams.szSystemCmdLine, sizeof(systemInitParams.szSystemCmdLine),
            mainInfo.m_commandLine, mainInfo.m_commandLineLen);

        systemInitParams.sLogFileName = GetLogFilename();
        systemInitParams.hInstance = mainInfo.m_instance;
        systemInitParams.hWnd = mainInfo.m_window;
        systemInitParams.pPrintSync = mainInfo.m_printSink;
        systemInitParams.bDedicatedServer = IsDedicatedServer();

        AZ::s64 remoteFileSystemEnabled{};
        AZ::SettingsRegistryMergeUtils::PlatformGet(*settingsRegistry, remoteFileSystemEnabled,
            AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey, "remote_filesystem");
        if (remoteFileSystemEnabled != 0)
        {
            // Reset local variable pathToAssets now that it's using remote file system
            pathToAssets = "";
            settingsRegistry->Get(pathToAssets, AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);

            AZ_TracePrintf("Launcher", "Application is configured for VFS");
            AZ_TracePrintf("Launcher", "Log and cache files will be written to the Cache directory on your host PC");

#if defined(AZ_ENABLE_TRACING)
            constexpr const char* message = "If your game does not run, check any of the following:\n"
                                            "\t- Verify the remote_ip address is correct in bootstrap.cfg";
#endif
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
            AZ_TracePrintf("Launcher", "Application is configured to use device local files at %s\n", pathToAssets.c_str());
            AZ_TracePrintf("Launcher", "Log and cache files will be written to device storage\n");
        }

        // Create CrySystem.
    #if !defined(AZ_MONOLITHIC_BUILD)
        constexpr const char* crySystemLibraryName = AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX  "CrySystem" AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION;
        AZStd::unique_ptr<AZ::DynamicModuleHandle> crySystemLibrary = AZ::DynamicModuleHandle::Create(crySystemLibraryName);
        if (crySystemLibrary->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired))
        {
            PFNCREATESYSTEMINTERFACE CreateSystemInterface =
                crySystemLibrary->GetFunction<PFNCREATESYSTEMINTERFACE>("CreateSystemInterface");

            if (CreateSystemInterface)
            {
                systemInitParams.pSystem = CreateSystemInterface(systemInitParams);
            }
        }
    #else
        systemInitParams.pSystem = CreateSystemInterface(systemInitParams);
    #endif // !defined(AZ_MONOLITHIC_BUILD)

        AZ::ComponentApplicationLifecycle::SignalEvent(*settingsRegistry, "LegacySystemInterfaceCreated", R"({})");

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
                auto autoExecFile = AZ::IO::FixedMaxPath{pathToAssets} / "autoexec.cfg";
                AZ::Interface<AZ::IConsole>::Get()->ExecuteConfigFile(autoExecFile.Native());

                // Find out if console command file was passed
                // via --console-command-file=%filename% and execute it
                ExecuteConsoleCommandFile(gameApplication);

                gEnv->pSystem->ExecuteCommandLine(false);

                AZ::ComponentApplicationLifecycle::SignalEvent(*settingsRegistry, "LegacyCommandLineProcessed", R"({})");

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

    #if !defined(_RELEASE)
        // until CrySystem can be removed (or made to be managed by the component application),
        // we need to manually clear the BudgetTracker before CrySystem is unloaded so the Budget
        // pointer(s) it has references to are cleared properly
        if (auto budgetTracker = AZ::Interface<AZ::Debug::BudgetTracker>::Get(); budgetTracker)
        {
            budgetTracker->Reset();
        }
    #endif // !defined(_RELEASE)

        // The order of operations here is to delete CrySystem, stop the game application, then unload the CrySystem dll.
        // If we unloaded the CrySystem dll before stopping the game application, we can potentially have crashes
        // if the CrySystem dll created any EBus contexts, since those contexts would get destroyed before subsystems could
        // disconnect from the buses.
        SAFE_DELETE(systemInitParams.pSystem);
        gameApplication.Stop();
        crySystemLibrary.reset(nullptr);
    #else
        SAFE_DELETE(systemInitParams.pSystem);
        gameApplication.Stop();
#endif // !defined(AZ_MONOLITHIC_BUILD)

        AZ::Debug::Trace::Instance().Destroy();

        return status;
    }
}
