#pragma once

#include <AtomToolsFramework/Communication/LocalServer.h>
#include <AtomToolsFramework/Communication/LocalSocket.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Logging/LogFile.h>
#include <AzQtComponents/Application/AzQtApplication.h>
#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/Logger/TraceLogger.h>

#include <QTimer>

namespace AtomToolsFramework
{
    //!Base class for Atom tools to inherit from
    class AtomToolsApplication
        : public AzFramework::Application
        , public AzQtComponents::AzQtApplication
        , protected AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
        , protected AzFramework::AssetSystemStatusBus::Handler
        , protected AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
        , protected AZ::Debug::TraceMessageBus::Handler
        , protected AZ::UserSettingsOwnerRequestBus::Handler
    {
    public:
        AZ_TYPE_INFO(AtomTools::AtomToolsApplication, "{30F90CA5-1253-49B5-8143-19CEE37E22BB}");

        using Base = AzFramework::Application;

        AtomToolsApplication(int* argc, char*** argv);

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void CreateReflectionManager() override;
        void Reflect(AZ::ReflectContext* context) override;
        void RegisterCoreComponents() override;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Tick(float deltaOverride = -1.f) override;
        void Stop() override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AssetDatabaseRequestsBus::Handler overrides...
        bool GetAssetDatabaseLocation(AZStd::string& result) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application overrides...
        void Destroy() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::ComponentApplication overrides...
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::UserSettingsOwnerRequestBus::Handler overrides...
        void SaveSettings() override;
        //////////////////////////////////////////////////////////////////////////

        virtual void LoadSettings();
        virtual void UnloadSettings();
        virtual void CompileCriticalAssets();
        virtual void ProcessCommandLine(const AZ::CommandLine& commandLine);
        virtual bool LaunchDiscoveryService();
        virtual void StartInternal();

        static void PyIdleWaitFrames(uint32_t frames);

        AzToolsFramework::TraceLogger m_traceLogger;

        //! Local user settings are used to store material browser tree expansion state
        AZ::UserSettingsProvider m_localUserSettings;

        //! Are local settings loaded
        bool m_activatedLocalUserSettings = false;

        QTimer m_timer;

        AtomToolsFramework::LocalSocket m_socket;
        AtomToolsFramework::LocalServer m_server;
    };
} // namespace AtomToolsFramework
