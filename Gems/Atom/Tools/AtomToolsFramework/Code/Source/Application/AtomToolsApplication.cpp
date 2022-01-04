/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <AtomToolsFramework/Application/AtomToolsApplication.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowFactoryRequestBus.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzQtComponents/Components/GlobalEventFilter.h>

#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AzToolsFrameworkModule.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>

#include "AtomToolsFramework_Traits_Platform.h"

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QMessageBox>
#include <QObject>
AZ_POP_DISABLE_WARNING

namespace AtomToolsFramework
{
    AZStd::string AtomToolsApplication::GetBuildTargetName() const
    {
        return AZStd::string("AtomTools");
    }

    const char* AtomToolsApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseAtomTools";
#elif defined(_DEBUG)
        return "DebugAtomTools";
#else
        return "ProfileAtomTools";
#endif
    }

    AtomToolsApplication::AtomToolsApplication(int* argc, char*** argv)
        : Application(argc, argv)
        , AzQtApplication(*argc, *argv)
    {
        // Suppress spam from the Source Control system
        m_traceLogger.AddWindowFilter(AzToolsFramework::SCC_WINDOW);

        installEventFilter(new AzQtComponents::GlobalEventFilter(this));

        AZ::IO::FixedMaxPath engineRootPath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        }

        m_styleManager.reset(new AzQtComponents::StyleManager(this));
        m_styleManager->initialize(this, engineRootPath);

        m_timer.setInterval(1);
        connect(&m_timer, &QTimer::timeout, this, [this]()
        {
            this->PumpSystemEventLoopUntilEmpty();
            this->Tick();
        });

        connect(this, &QGuiApplication::applicationStateChanged, this, [this]()
        {
            // Limit the update interval when not in focus to reduce power consumption and interference with other applications
            this->m_timer.setInterval((applicationState() & Qt::ApplicationActive) ? 1 : 32);
        });
    }

    AtomToolsApplication ::~AtomToolsApplication()
    {
        m_styleManager.reset();
        AtomToolsMainWindowNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
    }

    void AtomToolsApplication::CreateReflectionManager()
    {
        Base::CreateReflectionManager();
        GetSerializeContext()->CreateEditContext();
    }

    void AtomToolsApplication::Reflect(AZ::ReflectContext* context)
    {
        Base::Reflect(context);

        AzToolsFramework::AssetBrowser::AssetBrowserEntry::Reflect(context);
        AzToolsFramework::AssetBrowser::RootAssetBrowserEntry::Reflect(context);
        AzToolsFramework::AssetBrowser::FolderAssetBrowserEntry::Reflect(context);
        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::Reflect(context);
        AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry::Reflect(context);

        AzToolsFramework::QTreeViewWithStateSaving::Reflect(context);
        AzToolsFramework::QWidgetSavedState::Reflect(context);

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.AtomTools.general' module
            auto addGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "atomtools.general");
            };
            // The reflection here is based on patterns in CryEditPythonHandler::Reflect
            addGeneral(behaviorContext->Method(
                "idle_wait_frames", &AtomToolsApplication::PyIdleWaitFrames, nullptr,
                "Waits idling for a frames. Primarily used for auto-testing."));
        }
    }

    void AtomToolsApplication::RegisterCoreComponents()
    {
        Base::RegisterCoreComponents();
        RegisterComponentDescriptor(AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Thumbnailer::ThumbnailerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Components::PropertyManagerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::AssetSystem::AssetSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::PerforceComponent::CreateDescriptor());
    }

    AZ::ComponentTypeList AtomToolsApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = Base::GetRequiredSystemComponents();

        components.insert(
            components.end(),
            {
                azrtti_typeid<AzToolsFramework::AssetBrowser::AssetBrowserComponent>(),
                azrtti_typeid<AzToolsFramework::Thumbnailer::ThumbnailerComponent>(),
                azrtti_typeid<AzToolsFramework::Components::PropertyManagerComponent>(),
                azrtti_typeid<AzToolsFramework::PerforceComponent>(),
            });

        return components;
    }

    void AtomToolsApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Base::CreateStaticModules(outModules);
        outModules.push_back(aznew AzToolsFramework::AzToolsFrameworkModule);
    }

    void AtomToolsApplication::StartCommon(AZ::Entity* systemEntity)
    {
        AzFramework::AssetSystemStatusBus::Handler::BusConnect();
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusConnect();

        Base::StartCommon(systemEntity);

        const bool clearLogFile = GetSettingOrDefault("/O3DE/AtomToolsFramework/Application/ClearLogOnStart", false);
        m_traceLogger.OpenLogFile(GetBuildTargetName() + ".log", clearLogFile);

        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotificationBus::Broadcast(
            &AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications::OnDatabaseInitialized);

        if (!AZ::RPI::RPISystemInterface::Get()->IsInitialized())
        {
            AZ::RPI::RPISystemInterface::Get()->InitializeSystemAssets();
        }

        LoadSettings();

        AtomToolsMainWindowNotificationBus::Handler::BusConnect();
        AtomToolsMainWindowFactoryRequestBus::Broadcast(&AtomToolsMainWindowFactoryRequestBus::Handler::CreateMainWindow);

        auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
        if (editorPythonEventsInterface)
        {
            // The PythonSystemComponent does not call StartPython to allow for lazy python initialization, so start it here
            // The PythonSystemComponent will call StopPython when it deactivates, so we do not need our own corresponding call to
            // StopPython
            editorPythonEventsInterface->StartPython();
        }

        // Delay execution of commands and scripts post initialization
        QTimer::singleShot(0, [this]() { ProcessCommandLine(m_commandLine); });

        m_timer.start();
    }

    void AtomToolsApplication::OnMainWindowClosing()
    {
        ExitMainLoop();
    }

    void AtomToolsApplication::Destroy()
    {
        // before modules are unloaded, destroy UI to free up any assets it cached
        AtomToolsMainWindowFactoryRequestBus::Broadcast(&AtomToolsMainWindowFactoryRequestBus::Handler::DestroyMainWindow);
        m_styleManager.reset();

        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();
        AtomToolsMainWindowNotificationBus::Handler::BusDisconnect();
        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::StartDisconnectingAssetProcessor);

#if AZ_TRAIT_ATOMTOOLSFRAMEWORK_SKIP_APP_DESTROY
        ::_exit(0);
#else
        Base::Destroy();
#endif
    }

    AZStd::vector<AZStd::string> AtomToolsApplication::GetCriticalAssetFilters() const
    {
        return AZStd::vector<AZStd::string>({});
    }

    void AtomToolsApplication::AssetSystemAvailable()
    {
        bool connectedToAssetProcessor = false;

        // When the AssetProcessor is already launched it should take less than a second to perform a connection
        // but when the AssetProcessor needs to be launch it could take up to 15 seconds to have the AssetProcessor initialize
        // and able to negotiate a connection when running a debug build
        // and to negotiate a connection

        auto targetName = GetBuildTargetName();

        AzFramework::AssetSystem::ConnectionSettings connectionSettings;
        AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
        connectionSettings.m_connectionDirection =
            AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
        connectionSettings.m_connectionIdentifier = GetBuildTargetName();
        connectionSettings.m_loggingCallback = [targetName]([[maybe_unused]] AZStd::string_view logData)
        {
            AZ_UNUSED(targetName);  // Prevent unused warning in release builds
            AZ_TracePrintf(targetName.c_str(), "%.*s", aznumeric_cast<int>(logData.size()), logData.data());
        };
        AzFramework::AssetSystemRequestBus::BroadcastResult(
            connectedToAssetProcessor, &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);

        if (connectedToAssetProcessor)
        {
            CompileCriticalAssets();
        }

        AzFramework::AssetSystemStatusBus::Handler::BusDisconnect();
    }

    void AtomToolsApplication::CompileCriticalAssets()
    {
        AZ_TracePrintf(GetBuildTargetName().c_str(), "Compiling critical assets.\n");

        QStringList failedAssets;

        // Forced asset processor to synchronously process all critical assets
        // Note: with AssetManager's current implementation, a compiled asset won't be added in asset registry until next system tick.
        // So the asset id won't be found right after CompileAssetSync call.
        for (const AZStd::string& assetFilters : GetCriticalAssetFilters())
        {
            AZ_TracePrintf(GetBuildTargetName().c_str(), "Compiling critical asset matching: %s.\n", assetFilters.c_str());

            // Wait for the asset be compiled
            AzFramework::AssetSystem::AssetStatus status = AzFramework::AssetSystem::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                status, &AzFramework::AssetSystemRequestBus::Events::CompileAssetSync, assetFilters);
            if (status != AzFramework::AssetSystem::AssetStatus_Compiled)
            {
                failedAssets.append(assetFilters.c_str());
            }
        }

        if (!failedAssets.empty())
        {
            QMessageBox::critical(
                activeWindow(), QString("Failed to compile critical assets"),
                QString("Failed to compile the following critical assets:\n%1\n%2")
                    .arg(failedAssets.join(",\n"))
                    .arg("Make sure this is an Atom project."));
            ExitMainLoop();
        }
    }

    void AtomToolsApplication::SaveSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(context, "No serialize context");

            char resolvedPath[AZ_MAX_PATH_LEN] = "";
            AZStd::string fileName = "@user@/" + GetBuildTargetName() + "UserSettings.xml";

            AZ::IO::FileIOBase::GetInstance()->ResolvePath(
                fileName.c_str(), resolvedPath, AZ_ARRAY_SIZE(resolvedPath));
            m_localUserSettings.Save(resolvedPath, context);
        }
    }

    void AtomToolsApplication::LoadSettings()
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(context, "No serialize context");

        char resolvedPath[AZ_MAX_PATH_LEN] = "";
        AZStd::string fileName = "@user@/" + GetBuildTargetName() + "UserSettings.xml";

        AZ::IO::FileIOBase::GetInstance()->ResolvePath(fileName.c_str(), resolvedPath, AZ_MAX_PATH_LEN);

        m_localUserSettings.Load(resolvedPath, context);
        m_localUserSettings.Activate(AZ::UserSettings::CT_LOCAL);
        AZ::UserSettingsOwnerRequestBus::Handler::BusConnect(AZ::UserSettings::CT_LOCAL);
        m_activatedLocalUserSettings = true;
    }

    void AtomToolsApplication::UnloadSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            SaveSettings();
            m_localUserSettings.Deactivate();
            AZ::UserSettingsOwnerRequestBus::Handler::BusDisconnect();
            m_activatedLocalUserSettings = false;
        }
    }

    void AtomToolsApplication::ProcessCommandLine(const AZ::CommandLine& commandLine)
    {
        const AZStd::string activateWindowSwitchName = "activatewindow";
        if (commandLine.HasSwitch(activateWindowSwitchName))
        {
            AtomToolsFramework::AtomToolsMainWindowRequestBus::Broadcast(
                &AtomToolsFramework::AtomToolsMainWindowRequestBus::Handler::ActivateWindow);
        }

        const AZStd::string timeoputSwitchName = "timeout";
        if (commandLine.HasSwitch(timeoputSwitchName))
        {
            const AZStd::string& timeoutValue = commandLine.GetSwitchValue(timeoputSwitchName, 0);
            const uint32_t timeoutInMs = atoi(timeoutValue.c_str());
            AZ_Printf(GetBuildTargetName().c_str(), "Timeout scheduled, shutting down in %u ms", timeoutInMs);
            QTimer::singleShot(
                timeoutInMs,
                [this]
                {
                    AZ_Printf(GetBuildTargetName().c_str(), "Timeout reached, shutting down");
                    ExitMainLoop();
                });
        }

        // Process command line options for running one or more python scripts on startup
        const AZStd::string runPythonScriptSwitchName = "runpython";
        size_t runPythonScriptCount = commandLine.GetNumSwitchValues(runPythonScriptSwitchName);
        for (size_t runPythonScriptIndex = 0; runPythonScriptIndex < runPythonScriptCount; ++runPythonScriptIndex)
        {
            const AZStd::string runPythonScriptPath = commandLine.GetSwitchValue(runPythonScriptSwitchName, runPythonScriptIndex);
            AZStd::vector<AZStd::string_view> runPythonArgs;

            AZ_Printf(GetBuildTargetName().c_str(), "Launching script: %s", runPythonScriptPath.c_str());
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs, runPythonScriptPath, runPythonArgs);
        }

        const AZStd::string exitAfterCommandsSwitchName = "exitaftercommands";
        if (commandLine.HasSwitch(exitAfterCommandsSwitchName))
        {
            ExitMainLoop();
        }
    }

    bool AtomToolsApplication::LaunchLocalServer()
    {
        // Determine if this is the first launch of the tool by attempting to connect to a running server
        if (m_socket.Connect(QApplication::applicationName()))
        {
            // If the server was located, the application is already running.
            // Forward commandline options to other application instance.
            QByteArray buffer;
            buffer.append("ProcessCommandLine:");

            // Add the command line options from this process to the message, skipping the executable path
            for (int argi = 1; argi < m_argC; ++argi)
            {
                buffer.append(QString(m_argV[argi]).append("\n").toUtf8());
            }

            // Inject command line option to always bring the main window to the foreground
            buffer.append("--activatewindow\n");

            m_socket.Send(buffer);
            m_socket.Disconnect();
            return false;
        }

        // Setup server to handle basic commands
        m_server.SetReadHandler(
            [this](const QByteArray& buffer)
            {
                // Handle commmand line params from connected socket
                if (buffer.startsWith("ProcessCommandLine:"))
                {
                    // Remove header and parse commands
                    AZStd::string params(buffer.data(), buffer.size());
                    params = params.substr(strlen("ProcessCommandLine:"));

                    AZStd::vector<AZStd::string> tokens;
                    AZ::StringFunc::Tokenize(params, tokens, "\n");

                    if (!tokens.empty())
                    {
                        AZ::CommandLine commandLine;
                        commandLine.Parse(tokens);
                        QTimer::singleShot(0, [this, commandLine]() { ProcessCommandLine(commandLine); });
                    }
                }
            });

        // Launch local server
        if (!m_server.Connect(QApplication::applicationName()))
        {
            return false;
        }

        return true;
    }

    bool AtomToolsApplication::GetAssetDatabaseLocation(AZStd::string& result)
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath assetDatabaseSqlitePath;
        if (settingsRegistry &&
            settingsRegistry->Get(assetDatabaseSqlitePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder))
        {
            assetDatabaseSqlitePath /= "assetdb.sqlite";
            result = AZStd::string_view(assetDatabaseSqlitePath.Native());
            return true;
        }

        return false;
    }

    void AtomToolsApplication::Tick()
    {
        TickSystem();
        Base::Tick();

        if (WasExitMainLoopRequested())
        {
            m_timer.disconnect();
            quit();
        }
    }

    void AtomToolsApplication::Stop()
    {
        AtomToolsMainWindowFactoryRequestBus::Broadcast(&AtomToolsMainWindowFactoryRequestBus::Handler::DestroyMainWindow);
        m_styleManager.reset();

        UnloadSettings();
        Base::Stop();
    }

    void AtomToolsApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    {
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Tool;
    }

    void AtomToolsApplication::OnTraceMessage([[maybe_unused]] AZStd::string_view message)
    {
#if defined(AZ_ENABLE_TRACING)
        AZStd::vector<AZStd::string> lines;
        AzFramework::StringFunc::Tokenize(
            message, lines, "\n",
            false, // Keep empty strings
            false // Keep space strings
        );

        for (auto& line : lines)
        {
            AZ_TracePrintf(GetBuildTargetName().c_str(), "Python: %s\n", line.c_str());
        }
#endif
    }

    void AtomToolsApplication::OnErrorMessage(AZStd::string_view message)
    {
        // Use AZ_TracePrintf instead of AZ_Error or AZ_Warning to avoid all the metadata noise
        OnTraceMessage(message);
    }

    void AtomToolsApplication::OnExceptionMessage([[maybe_unused]] AZStd::string_view message)
    {
        AZ_Error(GetBuildTargetName().c_str(), false, "Python: " AZ_STRING_FORMAT, AZ_STRING_ARG(message));
    }

    // Copied from PyIdleWaitFrames in CryEdit.cpp
    void AtomToolsApplication::PyIdleWaitFrames(uint32_t frames)
    {
        struct Ticker : public AZ::TickBus::Handler
        {
            Ticker(QEventLoop* loop, uint32_t targetFrames)
                : m_loop(loop)
                , m_targetFrames(targetFrames)
            {
                AZ::TickBus::Handler::BusConnect();
            }
            ~Ticker()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }

            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override
            {
                AZ_UNUSED(deltaTime);
                AZ_UNUSED(time);
                if (++m_elapsedFrames == m_targetFrames)
                {
                    m_loop->quit();
                }
            }
            QEventLoop* m_loop = nullptr;
            uint32_t m_elapsedFrames = 0;
            uint32_t m_targetFrames = 0;
        };

        QEventLoop loop;
        Ticker ticker(&loop, frames);
        loop.exec();
    }
} // namespace AtomToolsFramework
