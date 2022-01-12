/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Communication/LocalServer.h>
#include <AtomToolsFramework/Communication/LocalSocket.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowNotificationBus.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/UserSettings/UserSettingsProvider.h>

#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetSystemBus.h>

#include <AzQtComponents/Application/AzQtApplication.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <AzToolsFramework/API/AssetDatabaseBus.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/Logger/TraceLogger.h>

#include <QTimer>

namespace AtomToolsFramework
{
    //! Base class for Atom tools to inherit from
    class AtomToolsApplication
        : public AzFramework::Application
        , public AzQtComponents::AzQtApplication
        , protected AzToolsFramework::AssetDatabase::AssetDatabaseRequestsBus::Handler
        , protected AzFramework::AssetSystemStatusBus::Handler
        , protected AzToolsFramework::EditorPythonConsoleNotificationBus::Handler
        , protected AZ::UserSettingsOwnerRequestBus::Handler
        , protected AtomToolsMainWindowNotificationBus::Handler
    {
    public:
        AZ_TYPE_INFO(AtomTools::AtomToolsApplication, "{A0DF25BA-6F74-4F11-9F85-0F99278D5986}");

        using Base = AzFramework::Application;

        AtomToolsApplication(int* argc, char*** argv);
        ~AtomToolsApplication();

        virtual bool LaunchLocalServer();

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application
        void CreateReflectionManager() override;
        void Reflect(AZ::ReflectContext* context) override;
        void RegisterCoreComponents() override;
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override;
        const char* GetCurrentConfigurationName() const override;
        void StartCommon(AZ::Entity* systemEntity) override;
        void Tick() override;
        void Stop() override;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // AtomsToolMainWindowNotificationBus::Handler overrides...
        void OnMainWindowClosing() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AssetDatabaseRequestsBus::Handler overrides...
        bool GetAssetDatabaseLocation(AZStd::string& result) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::Application overrides...
        void Destroy() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetSystemStatusBus::Handler overrides...
        void AssetSystemAvailable() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::ComponentApplication overrides...
        void QueryApplicationType(AZ::ApplicationTypeQuery& appType) const override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::UserSettingsOwnerRequestBus::Handler overrides...
        void SaveSettings() override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorPythonConsoleNotificationBus::Handler overrides...
        void OnTraceMessage(AZStd::string_view message) override;
        void OnErrorMessage(AZStd::string_view message) override;
        void OnExceptionMessage(AZStd::string_view message) override;
        ////////////////////////////////////////////////////////////////////////

        //! Executable target name generally used as a prefix for logging and other saved files
        virtual AZStd::string GetBuildTargetName() const;

        //! List of filters for assets that need to be pre-built to run the application
        virtual AZStd::vector<AZStd::string> GetCriticalAssetFilters() const;

        virtual void LoadSettings();
        virtual void UnloadSettings();
        virtual void CompileCriticalAssets();
        virtual void ProcessCommandLine(const AZ::CommandLine& commandLine);

        static void PyIdleWaitFrames(uint32_t frames);

        AzToolsFramework::TraceLogger m_traceLogger;

        AZStd::unique_ptr<AzQtComponents::StyleManager> m_styleManager;

        //! Local user settings are used to store material browser tree expansion state
        AZ::UserSettingsProvider m_localUserSettings;

        //! Are local settings loaded
        bool m_activatedLocalUserSettings = false;

        QTimer m_timer;

        AtomToolsFramework::LocalSocket m_socket;
        AtomToolsFramework::LocalServer m_server;
    };
} // namespace AtomToolsFramework
