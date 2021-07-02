/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/Asset/AssetSystemComponent.h>

#include <AzToolsFramework/AzToolsFrameworkModule.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/API/EditorPythonRunnerRequestsBus.h>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/SourceControl/PerforceComponent.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserComponent.h>
#include <AzToolsFramework/Thumbnails/ThumbnailerComponent.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <AtomToolsFramework/Util/Util.h>

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

#include <Source/MaterialEditorApplication.h>
#include <MaterialEditor_Traits_Platform.h>

#include <Atom/Document/MaterialDocumentModule.h>
#include <Atom/Document/MaterialDocumentSystemRequestBus.h>

#include <Atom/Viewport/MaterialViewportModule.h>

#include <Atom/Window/MaterialEditorWindowModule.h>
#include <Atom/Window/MaterialEditorWindowFactoryRequestBus.h>
#include <Atom/Window/MaterialEditorWindowRequestBus.h>
#include <AzCore/Utils/Utils.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QObject>
#include <QMessageBox>
AZ_POP_DISABLE_WARNING

namespace MaterialEditor
{
    //! This function returns the build system target name of "MaterialEditor
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }

    const char* MaterialEditorApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseMaterialEditor";
#elif defined(_DEBUG)
        return "DebugMaterialEditor";
#else
        return "ProfileMaterialEditor";
#endif
    }

    MaterialEditorApplication::MaterialEditorApplication(int* argc, char*** argv)
        : Application(argc, argv)
        , QApplication(*argc, *argv)
    {
        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), GetBuildTargetName());

        connect(&m_timer, &QTimer::timeout, this, [&]()
        {
            this->PumpSystemEventLoopUntilEmpty();
            this->Tick();
        });
    }

    MaterialEditorApplication::~MaterialEditorApplication()
    {
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();
        MaterialEditorWindowNotificationBus::Handler::BusDisconnect();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
    }

    void MaterialEditorApplication::CreateReflectionManager()
    {
        Application::CreateReflectionManager();
        GetSerializeContext()->CreateEditContext();
    }

    void MaterialEditorApplication::Reflect(AZ::ReflectContext* context)
    {
        Application::Reflect(context);

        AzToolsFramework::AssetBrowser::AssetBrowserEntry::Reflect(context);
        AzToolsFramework::AssetBrowser::RootAssetBrowserEntry::Reflect(context);
        AzToolsFramework::AssetBrowser::FolderAssetBrowserEntry::Reflect(context);
        AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry::Reflect(context);
        AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry::Reflect(context);

        AzToolsFramework::QTreeViewWithStateSaving::Reflect(context);
        AzToolsFramework::QWidgetSavedState::Reflect(context);

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // this will put these methods into the 'azlmbr.materialeditor.general' module
            auto addGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "materialeditor.general");
            };
            // The reflection here is based on patterns in CryEditPythonHandler::Reflect
            addGeneral(behaviorContext->Method("idle_wait_frames", &MaterialEditorApplication::PyIdleWaitFrames, nullptr, "Waits idling for a frames. Primarily used for auto-testing."));
        }
    }

    void MaterialEditorApplication::RegisterCoreComponents()
    {
        Application::RegisterCoreComponents();
        RegisterComponentDescriptor(AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Thumbnailer::ThumbnailerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Components::PropertyManagerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::AssetSystem::AssetSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::PerforceComponent::CreateDescriptor());
    }

    AZ::ComponentTypeList MaterialEditorApplication::GetRequiredSystemComponents() const
    {
        AZ::ComponentTypeList components = Application::GetRequiredSystemComponents();

        components.insert(components.end(), {
                azrtti_typeid<AzToolsFramework::AssetBrowser::AssetBrowserComponent>(),
                azrtti_typeid<AzToolsFramework::Thumbnailer::ThumbnailerComponent>(),
                azrtti_typeid<AzToolsFramework::Components::PropertyManagerComponent>(),
                azrtti_typeid<AzToolsFramework::PerforceComponent>(),
            });

        return components;
    }

    void MaterialEditorApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Application::CreateStaticModules(outModules);
        outModules.push_back(aznew AzToolsFramework::AzToolsFrameworkModule);
        outModules.push_back(aznew MaterialDocumentModule);
        outModules.push_back(aznew MaterialViewportModule);
        outModules.push_back(aznew MaterialEditorWindowModule);
    }

    void MaterialEditorApplication::StartCommon(AZ::Entity* systemEntity)
    {
        {
            //[GFX TODO][ATOM-408] This needs to be updated in some way to support the MaterialViewport render widget
        }

        AzFramework::AssetSystemStatusBus::Handler::BusConnect();
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusConnect();

        AzFramework::Application::StartCommon(systemEntity);

        StartInternal();

        m_timer.start();
    }

    void MaterialEditorApplication::OnMaterialEditorWindowClosing()
    {
        ExitMainLoop();
    }

    void MaterialEditorApplication::Destroy()
    {
        // before modules are unloaded, destroy UI to free up any assets it cached
        MaterialEditor::MaterialEditorWindowFactoryRequestBus::Broadcast(
            &MaterialEditor::MaterialEditorWindowFactoryRequestBus::Handler::DestroyMaterialEditorWindow);

        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();
        MaterialEditorWindowNotificationBus::Handler::BusDisconnect();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

        m_logFile = {};
        m_startupLogSink = {};

        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::StartDisconnectingAssetProcessor);
        Application::Destroy();
    }

    void MaterialEditorApplication::AssetSystemAvailable()
    {
        bool connectedToAssetProcessor = false;

        // When the AssetProcessor is already launched it should take less than a second to perform a connection
        // but when the AssetProcessor needs to be launch it could take up to 15 seconds to have the AssetProcessor initialize
        // and able to negotiate a connection when running a debug build
        // and to negotiate a connection

        AzFramework::AssetSystem::ConnectionSettings connectionSettings;
        AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
        connectionSettings.m_connectionDirection = AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
        connectionSettings.m_connectionIdentifier = "MaterialEditor";
        connectionSettings.m_loggingCallback = []([[maybe_unused]] AZStd::string_view logData)
        {
            AZ_TracePrintf("Material Editor", "%.*s", aznumeric_cast<int>(logData.size()), logData.data());
        };
        AzFramework::AssetSystemRequestBus::BroadcastResult(connectedToAssetProcessor,
            &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);
        if (connectedToAssetProcessor)
        {
            CompileCriticalAssets();
        }

        AzFramework::AssetSystemStatusBus::Handler::BusDisconnect();
    }


    void MaterialEditorApplication::CompileCriticalAssets()
    {
        AZ_TracePrintf("MaterialEditor", "Compiling critical assets.\n");

        // List of common asset filters for things that need to be compiled to run the material editor
        // Some of these things will not be necessary once we have proper support for queued asset loading and reloading
        const AZStd::string assetFiltersArray[] =
        {
            "passes/",
            "config/",
            "MaterialEditor/",
        };

        QStringList failedAssets;

        // Forced asset processor to synchronously process all critical assets
        // Note: with AssetManager's current implementation, a compiled asset won't be added in asset registry until next system tick. 
        // So the asset id won't be found right after CompileAssetSync call. 
        for (const AZStd::string& assetFilters : assetFiltersArray)
        {
            AZ_TracePrintf("MaterialEditor", "Compiling critical asset matching: %s.\n", assetFilters.c_str());

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
            QMessageBox::critical(activeWindow(),
                QString("Failed to compile critical assets"),
                QString("Failed to compile the following critical assets:\n%1\n%2")
                .arg(failedAssets.join(",\n"))
                .arg("Make sure this is an Atom project."));
            ExitMainLoop();
        }
    }

    void MaterialEditorApplication::SaveSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(context, "No serialize context");

            char resolvedPath[AZ_MAX_PATH_LEN] = "";
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@user@/MaterialEditorUserSettings.xml", resolvedPath, AZ_ARRAY_SIZE(resolvedPath));
            m_localUserSettings.Save(resolvedPath, context);
        }
    }

    bool MaterialEditorApplication::OnOutput(const char* window, const char* message)
    {
        // Suppress spam from the Source Control system
        if (0 == strncmp(window, AzToolsFramework::SCC_WINDOW, AZ_ARRAY_SIZE(AzToolsFramework::SCC_WINDOW)))
        {
            return true;
        }

        if (m_logFile)
        {
            m_logFile->AppendLog(AzFramework::LogFile::SEV_NORMAL, window, message);
        }
        else
        {
            m_startupLogSink.push_back({ window, message });
        }
        return false;
    }

    void MaterialEditorApplication::ProcessCommandLine(const AZ::CommandLine& commandLine)
    {
        const AZStd::string activateWindowSwitchName = "activatewindow";
        if (commandLine.HasSwitch(activateWindowSwitchName))
        {
            MaterialEditor::MaterialEditorWindowRequestBus::Broadcast(
                &MaterialEditor::MaterialEditorWindowRequestBus::Handler::ActivateWindow);
        }

        const AZStd::string timeoputSwitchName = "timeout";
        if (commandLine.HasSwitch(timeoputSwitchName))
        {
            const AZStd::string& timeoutValue = commandLine.GetSwitchValue(timeoputSwitchName, 0);
            const uint32_t timeoutInMs = atoi(timeoutValue.c_str());
            AZ_Printf("MaterialEditor", "Timeout scheduled, shutting down in %u ms", timeoutInMs);
            QTimer::singleShot(timeoutInMs, [this] {
                AZ_Printf("MaterialEditor", "Timeout reached, shutting down");
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

            AZ_Printf("MaterialEditor", "Launching script: %s", runPythonScriptPath.c_str());
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
                runPythonScriptPath,
                runPythonArgs);
        }

        // Process command line options for opening one or more material documents on startup
        size_t openDocumentCount = commandLine.GetNumMiscValues();
        for (size_t openDocumentIndex = 0; openDocumentIndex < openDocumentCount; ++openDocumentIndex)
        {
            const AZStd::string openDocumentPath = commandLine.GetMiscValue(openDocumentIndex);

            AZ_Printf("MaterialEditor", "Opening document: %s", openDocumentPath.c_str());
            MaterialDocumentSystemRequestBus::Broadcast(&MaterialDocumentSystemRequestBus::Events::OpenDocument, openDocumentPath);
        }

        const AZStd::string exitAfterCommandsSwitchName = "exitaftercommands";
        if (commandLine.HasSwitch(exitAfterCommandsSwitchName))
        {
            ExitMainLoop();
        }
    }

    void MaterialEditorApplication::WriteStartupLog()
    {
        using namespace AzFramework;

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO != nullptr, "FileIO should be running at this point");

        // There is no log system online so we have to create your own log file.
        char resolveBuffer[AZ_MAX_PATH_LEN] = { 0 };
        fileIO->ResolvePath("@user@", resolveBuffer, AZ_MAX_PATH_LEN);

        // Note: @log@ hasn't been set at this point
        AZStd::string logDirectory;
        StringFunc::Path::Join(resolveBuffer, "log", logDirectory);
        fileIO->SetAlias("@log@", logDirectory.c_str());

        fileIO->CreatePath("@root@");
        fileIO->CreatePath("@user@");
        fileIO->CreatePath("@log@");

        AZStd::string logPath;
        StringFunc::Path::Join(logDirectory.c_str(), "MaterialEditor.log", logPath);

        m_logFile.reset(aznew LogFile(logPath.c_str()));
        if (m_logFile)
        {
            m_logFile->SetMachineReadable(false);
            for (const LogMessage& message : m_startupLogSink)
            {
                m_logFile->AppendLog(LogFile::SEV_NORMAL, message.window.c_str(), message.message.c_str());
            }
            m_startupLogSink = {};
            m_logFile->FlushLog();
        }
    }

    void MaterialEditorApplication::LoadSettings()
    {
        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(context, "No serialize context");

        char resolvedPath[AZ_MAX_PATH_LEN] = "";
        AZ::IO::FileIOBase::GetInstance()->ResolvePath("@user@/EditorUserSettings.xml", resolvedPath, AZ_MAX_PATH_LEN);

        m_localUserSettings.Load(resolvedPath, context);
        m_localUserSettings.Activate(AZ::UserSettings::CT_LOCAL);
        AZ::UserSettingsOwnerRequestBus::Handler::BusConnect(AZ::UserSettings::CT_LOCAL);
        m_activatedLocalUserSettings = true;
    }

    void MaterialEditorApplication::UnloadSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            SaveSettings();
            m_localUserSettings.Deactivate();
            AZ::UserSettingsOwnerRequestBus::Handler::BusDisconnect();
            m_activatedLocalUserSettings = false;
        }
    }

    bool MaterialEditorApplication::LaunchDiscoveryService()
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
        m_server.SetReadHandler([this](const QByteArray& buffer) {
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
                    ProcessCommandLine(commandLine);
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

    void MaterialEditorApplication::StartInternal()
    {
        if (WasExitMainLoopRequested())
        {
            return;
        }

        WriteStartupLog();

        if (!LaunchDiscoveryService())
        {
            ExitMainLoop();
            return;
        }

        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotificationBus::Broadcast(&AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications::OnDatabaseInitialized);

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, "@assets@/assetcatalog.xml");

        AZ::RPI::RPISystemInterface::Get()->InitializeSystemAssets();

        LoadSettings();

        MaterialEditorWindowNotificationBus::Handler::BusConnect();

        MaterialEditor::MaterialEditorWindowFactoryRequestBus::Broadcast(
            &MaterialEditor::MaterialEditorWindowFactoryRequestBus::Handler::CreateMaterialEditorWindow);

        auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
        if (editorPythonEventsInterface)
        {
            // The PythonSystemComponent does not call StartPython to allow for lazy python initialization, so start it here
            // The PythonSystemComponent will call StopPython when it deactivates, so we do not need our own corresponding call to StopPython
            editorPythonEventsInterface->StartPython();
        }

        // Delay execution of commands and scripts post initialization
        QTimer::singleShot(0, [this]() { ProcessCommandLine(m_commandLine); });
    }

    bool MaterialEditorApplication::GetAssetDatabaseLocation(AZStd::string& result)
    {
        AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        AZ::IO::FixedMaxPath assetDatabaseSqlitePath;
        if (settingsRegistry && settingsRegistry->Get(assetDatabaseSqlitePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheProjectRootFolder))
        {
            assetDatabaseSqlitePath /= "assetdb.sqlite";
            result = AZStd::string_view(assetDatabaseSqlitePath.Native());
            return true;
        }

        return false;
    }

    void MaterialEditorApplication::Tick(float deltaOverride)
    {
        TickSystem();
        Application::Tick(deltaOverride);

        if (WasExitMainLoopRequested())
        {
            m_timer.disconnect();
            quit();
        }
    }

    void MaterialEditorApplication::Stop()
    {
        MaterialEditor::MaterialEditorWindowFactoryRequestBus::Broadcast(
            &MaterialEditor::MaterialEditorWindowFactoryRequestBus::Handler::DestroyMaterialEditorWindow);

        UnloadSettings();
        AzFramework::Application::Stop();
    }

    void MaterialEditorApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    {
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Game;
    }

    void MaterialEditorApplication::OnTraceMessage([[maybe_unused]] AZStd::string_view message)
    {
#if defined(AZ_ENABLE_TRACING)
        AZStd::vector<AZStd::string> lines;
        AzFramework::StringFunc::Tokenize(
            message,
            lines,
            "\n",
            false, // Keep empty strings
            false // Keep space strings
        );

        for (auto& line : lines)
        {
            AZ_TracePrintf("MaterialEditor", "Python: %s\n", line.c_str());
        }
#endif
    }

    void MaterialEditorApplication::OnErrorMessage(AZStd::string_view message)
    {
        // Use AZ_TracePrintf instead of AZ_Error or AZ_Warning to avoid all the metadata noise
        OnTraceMessage(message);
    }

    void MaterialEditorApplication::OnExceptionMessage([[maybe_unused]] AZStd::string_view message)
    {
        AZ_Error("MaterialEditor", false, "Python: " AZ_STRING_FORMAT, AZ_STRING_ARG(message));
    }

    // Copied from PyIdleWaitFrames in CryEdit.cpp
    void MaterialEditorApplication::PyIdleWaitFrames(uint32_t frames)
    {
        struct Ticker : public AZ::TickBus::Handler
        {
            Ticker(QEventLoop* loop, uint32_t targetFrames) : m_loop(loop), m_targetFrames(targetFrames)
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

} // namespace MaterialEditor
