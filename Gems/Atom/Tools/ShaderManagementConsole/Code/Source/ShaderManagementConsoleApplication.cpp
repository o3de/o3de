/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Network/AssetProcessorConnection.h>

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

#include <Source/ShaderManagementConsoleApplication.h>
#include <ShaderManagementConsole_Traits_Platform.h>

#include <Atom/Document/ShaderManagementConsoleDocumentModule.h>
#include <Atom/Document/ShaderManagementConsoleDocumentSystemRequestBus.h>

#include <Atom/Window/ShaderManagementConsoleWindowModule.h>
#include <Atom/Window/ShaderManagementConsoleWindowRequestBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QFileInfo>
#include <QObject>
#include <QMessageBox>
AZ_POP_DISABLE_WARNING

namespace ShaderManagementConsole
{
    AZStd::string_view GetBuildTargetName()
    {
#if !defined (LY_CMAKE_TARGET)
#error "LY_CMAKE_TARGET must be defined in order to add this source file to a CMake executable target"
#endif
        return AZStd::string_view{ LY_CMAKE_TARGET };
    }

    const char* ShaderManagementConsoleApplication::GetCurrentConfigurationName() const
    {
#if defined(_RELEASE)
        return "ReleaseShaderManagementConsole";
#elif defined(_DEBUG)
        return "DebugShaderManagementConsole";
#else
        return "ProfileShaderManagementConsole";
#endif
    }

    ShaderManagementConsoleApplication::ShaderManagementConsoleApplication(int* argc, char*** argv)
        : Application(argc, argv)
        , QApplication(*argc, *argv)
    {
        // The settings registry has been created at this point, so add the CMake target
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddBuildSystemTargetSpecialization(
            *AZ::SettingsRegistry::Get(), GetBuildTargetName());

        connect(&m_timer, &QTimer::timeout, this, [&]()
        {
            this->PumpSystemEventLoopUntilEmpty();
            this->Tick();
        });
    }

    void ShaderManagementConsoleApplication::CreateReflectionManager()
    {
        Application::CreateReflectionManager();
        GetSerializeContext()->CreateEditContext();
    }

    void ShaderManagementConsoleApplication::Reflect(AZ::ReflectContext* context)
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
            // this will put these methods into the 'azlmbr.shadermanagementconsole.general' module
            auto addGeneral = [](AZ::BehaviorContext::GlobalMethodBuilder methodBuilder)
            {
                methodBuilder->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Editor")
                    ->Attribute(AZ::Script::Attributes::Module, "shadermanagementconsole.general");
            };
            // The reflection here is based on patterns in CryEditPythonHandler::Reflect
            addGeneral(behaviorContext->Method("idle_wait_frames", &ShaderManagementConsoleApplication::PyIdleWaitFrames, nullptr, "Waits idling for a frames. Primarily used for auto-testing."));
        }
    }

    void ShaderManagementConsoleApplication::RegisterCoreComponents()
    {
        Application::RegisterCoreComponents();
        RegisterComponentDescriptor(AzToolsFramework::AssetBrowser::AssetBrowserComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Thumbnailer::ThumbnailerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Components::PropertyManagerComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::AssetSystem::AssetSystemComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::PerforceComponent::CreateDescriptor());
    }

    AZ::ComponentTypeList ShaderManagementConsoleApplication::GetRequiredSystemComponents() const
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

    void ShaderManagementConsoleApplication::CreateStaticModules(AZStd::vector<AZ::Module*>& outModules)
    {
        Application::CreateStaticModules(outModules);
        outModules.push_back(aznew AzToolsFramework::AzToolsFrameworkModule);
        outModules.push_back(aznew ShaderManagementConsoleDocumentModule);
        outModules.push_back(aznew ShaderManagementConsoleWindowModule);
    }

    void ShaderManagementConsoleApplication::StartCommon(AZ::Entity* systemEntity)
    {
        AzFramework::AssetSystemStatusBus::Handler::BusConnect();
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusConnect();
        AZ::Debug::TraceMessageBus::Handler::BusConnect();

        AzFramework::Application::StartCommon(systemEntity);

        StartInternal();

        m_timer.start();
    }

    void ShaderManagementConsoleApplication::OnShaderManagementConsoleWindowClosing()
    {
        ExitMainLoop();
        AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
        ShaderManagementConsoleWindowNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorPythonConsoleNotificationBus::Handler::BusDisconnect();
    }

    void ShaderManagementConsoleApplication::Destroy()
    {
        // before modules are unloaded, destroy UI to free up any assets it cached
        ShaderManagementConsole::ShaderManagementConsoleWindowRequestBus::Broadcast(&ShaderManagementConsole::ShaderManagementConsoleWindowRequestBus::Handler::DestroyShaderManagementConsoleWindow);

        ShaderManagementConsoleWindowNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusDisconnect();

        AzFramework::AssetSystemRequestBus::Broadcast(&AzFramework::AssetSystem::AssetSystemRequests::StartDisconnectingAssetProcessor);

        Application::Destroy();
    }

    void ShaderManagementConsoleApplication::AssetSystemAvailable()
    {
        // Try connect to AP first before try to launch it manually.
        bool connected = false;
        auto ConnectToAssetProcessorWithIdentifier = [&connected](AzFramework::AssetSystem::AssetSystemRequests* assetSystemRequests)
        {
            // When the AssetProcessor is already launched it should take less than a second to perform a connection
            // but when the AssetProcessor needs to be launch it could take up to 15 seconds to have the AssetProcessor initialize
            // and able to negotiate a connection when running a debug build
            // and to negotiate a connection

            AzFramework::AssetSystem::ConnectionSettings connectionSettings;
            AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
            connectionSettings.m_connectionDirection = AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
            connectionSettings.m_connectionIdentifier = "Shader Management Console";
            connectionSettings.m_loggingCallback = []([[maybe_unused]] AZStd::string_view logData)
            {
                AZ_TracePrintf("Shader Management Console", "%.*s", aznumeric_cast<int>(logData.size()), logData.data());
            };

            connected = assetSystemRequests->EstablishAssetProcessorConnection(connectionSettings);
        };
        AzFramework::AssetSystemRequestBus::Broadcast(ConnectToAssetProcessorWithIdentifier);

        if (connected)
        {
            CompileCriticalAssets();
        }

        AzFramework::AssetSystemStatusBus::Handler::BusDisconnect();
    }

    void ShaderManagementConsoleApplication::CompileCriticalAssets()
    {
        AZ_TracePrintf("Shader Management Console", "Compiling critical assets.\n");

        // List of common asset filters for things that need to be compiled to run
        // Some of these things will not be necessary once we have proper support for queued asset loading and reloading
        const AZStd::string assetFilterss[] =
        {
            "passes/",
            "config/",
        };

        QStringList failedAssets;

        // Forced asset processor to synchronously process all critical assets
        // Note: with AssetManager's current implementation, a compiled asset won't be added in asset registry until next system tick. 
        // So the asset id won't be found right after CompileAssetSync call. 
        for (const AZStd::string& assetFilters : assetFilterss)
        {
            AZ_TracePrintf("Shader Management Console", "Compiling critical asset matching: %s.\n", assetFilters.c_str());

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
            m_closing = true;
        }
    }

    void ShaderManagementConsoleApplication::SaveSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(context, "No serialize context");

            char resolvedPath[AZ_MAX_PATH_LEN] = "";
            AZ::IO::FileIOBase::GetInstance()->ResolvePath("@user@/EditorUserSettings.xml", resolvedPath, AZ_ARRAY_SIZE(resolvedPath));
            m_localUserSettings.Save(resolvedPath, context);
        }
    }

    bool ShaderManagementConsoleApplication::OnPrintf(const char* window, const char* /*message*/)
    {
        // Suppress spam from the Source Control system
        if (0 == strncmp(window, AzToolsFramework::SCC_WINDOW, AZ_ARRAY_SIZE(AzToolsFramework::SCC_WINDOW)))
        {
            return true;
        }

        return false;
    }

    void ShaderManagementConsoleApplication::ProcessCommandLine()
    {
        // Process command line options for running one or more python scripts on startup
        const AZStd::string runPythonScriptSwitchName = "runpython";
        size_t runPythonScriptCount = m_commandLine.GetNumSwitchValues(runPythonScriptSwitchName);
        for (size_t runPythonScriptIndex = 0; runPythonScriptIndex < runPythonScriptCount; ++runPythonScriptIndex)
        {
            const AZStd::string runPythonScriptPath = m_commandLine.GetSwitchValue(runPythonScriptSwitchName, runPythonScriptIndex);
            AZStd::vector<AZStd::string_view> runPythonArgs;
            AzToolsFramework::EditorPythonRunnerRequestBus::Broadcast(
                &AzToolsFramework::EditorPythonRunnerRequestBus::Events::ExecuteByFilenameWithArgs,
                runPythonScriptPath,
                runPythonArgs);
        }

        // Process command line options for opening one or more documents on startup
        size_t openDocumentCount = m_commandLine.GetNumMiscValues();
        for (size_t openDocumentIndex = 0; openDocumentIndex < openDocumentCount; ++openDocumentIndex)
        {
            const AZStd::string openDocumentPath = m_commandLine.GetMiscValue(openDocumentIndex);
            ShaderManagementConsoleDocumentSystemRequestBus::Broadcast(&ShaderManagementConsoleDocumentSystemRequestBus::Events::OpenDocument, openDocumentPath);
        }
    }

    void ShaderManagementConsoleApplication::LoadSettings()
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

    void ShaderManagementConsoleApplication::UnloadSettings()
    {
        if (m_activatedLocalUserSettings)
        {
            SaveSettings();
            m_localUserSettings.Deactivate();
            AZ::UserSettingsOwnerRequestBus::Handler::BusDisconnect();
            m_activatedLocalUserSettings = false;
        }
    }

    bool ShaderManagementConsoleApplication::LaunchDiscoveryService()
    {
        const QStringList arguments = { "-fail_silently" };

        return AtomToolsFramework::LaunchTool("GridHub", AZ_TRAIT_SHADER_MANAGEMENT_CONSOLE_EXT, arguments);
    }

    void ShaderManagementConsoleApplication::StartInternal()
    {
        if (m_closing)
        {
            return;
        }

        //[GFX TODO][ATOM-415] Try to factor out some of this stuff with AtomSampleViewerApplication
        AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotificationBus::Broadcast(&AzToolsFramework::AssetBrowser::AssetDatabaseLocationNotifications::OnDatabaseInitialized);

        AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequestBus::Events::LoadCatalog, "@assets@/assetcatalog.xml");

        AZ::RPI::RPISystemInterface::Get()->InitializeSystemAssets();

        LoadSettings();

        LaunchDiscoveryService();

        ShaderManagementConsoleWindowNotificationBus::Handler::BusConnect();

        ShaderManagementConsole::ShaderManagementConsoleWindowRequestBus::Broadcast(&ShaderManagementConsole::ShaderManagementConsoleWindowRequestBus::Handler::CreateShaderManagementConsoleWindow);

        auto editorPythonEventsInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get();
        if (editorPythonEventsInterface)
        {
            // The PythonSystemComponent does not call StartPython to allow for lazy python initialization, so start it here
            // The PythonSystemComponent will call StopPython when it deactivates, so we do not need our own corresponding call to StopPython
            editorPythonEventsInterface->StartPython();
        }

        ProcessCommandLine();
    }

    bool ShaderManagementConsoleApplication::GetAssetDatabaseLocation(AZStd::string& result)
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

    void ShaderManagementConsoleApplication::Tick(float deltaOverride)
    {
        TickSystem();
        Application::Tick(deltaOverride);

        if (m_closing)
        {
            m_timer.disconnect();
            quit();
        }
    }

    void ShaderManagementConsoleApplication::Stop()
    {
        UnloadSettings();
        AzFramework::Application::Stop();
    }

    void ShaderManagementConsoleApplication::QueryApplicationType(AZ::ApplicationTypeQuery& appType) const
    {
        appType.m_maskValue = AZ::ApplicationTypeQuery::Masks::Game;
    }

    void ShaderManagementConsoleApplication::OnTraceMessage([[maybe_unused]] AZStd::string_view message)
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
            AZ_TracePrintf("Shader Management Console", "Python: %s\n", line.c_str());
        }
#endif
    }

    void ShaderManagementConsoleApplication::OnErrorMessage(AZStd::string_view message)
    {
        // Use AZ_TracePrintf instead of AZ_Error or AZ_Warning to avoid all the metadata noise
        OnTraceMessage(message);
    }

    void ShaderManagementConsoleApplication::OnExceptionMessage([[maybe_unused]] AZStd::string_view message)
    {
        AZ_Error("Shader Management Console", false, "Python: " AZ_STRING_FORMAT, AZ_STRING_ARG(message));
    }

    // Copied from PyIdleWaitFrames in CryEdit.cpp
    void ShaderManagementConsoleApplication::PyIdleWaitFrames(uint32_t frames)
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

} // namespace ShaderManagementConsole
